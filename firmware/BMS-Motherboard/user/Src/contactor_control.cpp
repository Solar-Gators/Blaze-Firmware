/*
 * contactor_control.cpp
 *
 * Implementation of ContactorController. See contactor_control.hpp for wiring.
 *
 * All raw SPI / register access goes through last year's Bts71040 driver.
 * This file only adds the application logic: safe init, open/close, diagnostics,
 * and current-sense conversion.
 */

#include "contactor_control.hpp"

/* ===========================================================================
 *  begin() - safe one-time initialization
 *
 *  Order matters here. The two things people get wrong on this chip:
 *
 *   (1) DCR.MUX. On power-up DCR.MUX = 111B, which is SLEEP mode and BLANKS all
 *       outputs - the channels physically cannot turn on while MUX = 111B.
 *       You MUST write DCR to move MUX away from 111B (we use 000B) or every
 *       close() will silently do nothing. This is the #1 "my contactor won't
 *       switch" bug. (Datasheet 10.x DCR.MUX table.)
 *
 *   (2) Checksum. After configuring the protection registers you write a 4-bit
 *       checksum to ICS, then read STDDIAG.CSV: 0 = pass, 1 = fail. If CSV = 1,
 *       the chip did not accept your config the way you think it did.
 * ===========================================================================*/
HAL_StatusTypeDef ContactorController::begin()
{
    // 0) Start from a known-safe state: every channel OPEN.
    //    The GPIO init (CubeMX) already set INx low, but be explicit.
    dev_.writeInputsMask(0x00);

    // 1) Configure protection registers.
    //    RCD = Latch mode on ALL channels (RCD.RCDn = 1): after a fault the channel
    //    stays OFF until explicitly cleared, instead of auto-retrying. For a safety
    //    contactor this is the conservative choice - a fault keeps the pack isolated.
    //    (If you later want auto-restart, set these bits to 0.)
    if (dev_.writeRCD(0x0F, /*setSWRBefore=*/true) != HAL_OK) return HAL_ERROR;

    //    OCR = overcurrent threshold. 0x00 = high threshold IL(OVL0) (datasheet default).
    //    TUNE THIS once you know your contactor coil's inrush/hold current - set bits to
    //    1 for the lower threshold if you want faster trip on a given channel.
    if (dev_.writeOCR(0x00, /*setSWRBefore=*/false) != HAL_OK) return HAL_ERROR;

    //    KRC = current sense range. 0x00 = high range -> kILIS = 2000 (matches the
    //    constant in the header). Leave at default unless you change the sense math.
    if (dev_.writeKRC(0x00, /*setSWRBefore=*/false) != HAL_OK) return HAL_ERROR;

    // 2) CRITICAL: write DCR to take MUX out of 111B so outputs can actually switch.
    //    0x00 -> SWR=0, MUX=000 (also routes channel-0 current to IS by default).
    if (dev_.writeDCR(0x00) != HAL_OK) return HAL_ERROR;

    // 3) Compute and write the config checksum, then verify it.
    //    Pass the SAME 4-bit fields we just programmed above.
    const uint8_t ocr = 0x00, rcd = 0x0F, krc = 0x00, src = 0x00;
    const bool    col = false; const uint8_t pcc = 0x00;
    uint8_t csum = Bts71040::computeChecksum4(ocr, rcd, krc, src, col, pcc);
    if (dev_.writeICS(csum, /*setSWRBefore=*/true) != HAL_OK) return HAL_ERROR;

    // 4) Read STDDIAG and confirm the checksum verified (CSV must be 0).
    Bts71040::StdDiag d;
    if (dev_.readSTDDIAG(d) != HAL_OK) return HAL_ERROR;
    if (d.CSV) {
        // Checksum mismatch: the chip's stored config does not match what we sent.
        // Do NOT proceed to close contactors.
        return HAL_ERROR;
    }

    return HAL_OK;
}

/* ===========================================================================
 *  Control
 * ===========================================================================*/
void ContactorController::closeMain()
{
    dev_.setChannel(toDriverCh(MAIN_1), true);
    dev_.setChannel(toDriverCh(MAIN_2), true);
}

void ContactorController::openMain()
{
    dev_.setChannel(toDriverCh(MAIN_1), false);
    dev_.setChannel(toDriverCh(MAIN_2), false);
}

void ContactorController::close(Channel ch)
{
    dev_.setChannel(toDriverCh(ch), true);
}

void ContactorController::open(Channel ch)
{
    dev_.setChannel(toDriverCh(ch), false);
}

void ContactorController::allOpen()
{
    // Single write clears all four IN pins at once - fastest path to safe.
    dev_.writeInputsMask(0x00);
}

/* ===========================================================================
 *  Diagnostics
 * ===========================================================================*/
HAL_StatusTypeDef ContactorController::poll(Faults& f)
{
    if (dev_.readSTDDIAG(f.std) != HAL_OK) return HAL_ERROR;
    if (dev_.readWRNDIAG(f.wrn) != HAL_OK) return HAL_ERROR;
    if (dev_.readERRDIAG(f.err) != HAL_OK) return HAL_ERROR;

    // "any fault" = any per-channel warning or latched error, or a std-diag flag
    // that indicates a real problem (supply monitor / transmission error / checksum).
    bool wrn_any = f.wrn.WRN[0] || f.wrn.WRN[1] || f.wrn.WRN[2] || f.wrn.WRN[3];
    bool err_any = f.err.ERR[0] || f.err.ERR[1] || f.err.ERR[2] || f.err.ERR[3];
    bool std_any = f.std.TER || f.std.CSV || f.std.VSMON;
    f.any = wrn_any || err_any || std_any;

    return HAL_OK;
}

/* ===========================================================================
 *  Current sense
 *
 *  The chip puts a small current on the IS pin proportional to the load current
 *  of the channel selected by DCR.MUX:   I_IS = I_load / kILIS
 *  The board turns that into a voltage across R612 (1.2k):  V_IS = I_IS * 1200
 *  So:   I_load = V_IS * kILIS / 1200  =  V_IS * 1.667 A/V
 * ===========================================================================*/
float ContactorController::readCurrentAmps(Channel ch)
{
    // Route this channel's sense current to the IS pin: DCR.MUX = channel index (0..3).
    // (Writing DCR keeps MUX != 111B, so outputs stay enabled.)
    dev_.writeDCR(static_cast<uint8_t>(ch) & 0x07);

    // Let the ~1us RC filter settle (a few time constants). 1ms is plenty / simple.
    HAL_Delay(1);

    // Single ADC2_IN5 conversion.
    HAL_ADC_Start(hadc_);
    if (HAL_ADC_PollForConversion(hadc_, 10) != HAL_OK) {
        HAL_ADC_Stop(hadc_);
        return -1.0f;   // sentinel: conversion failed
    }
    uint32_t raw = HAL_ADC_GetValue(hadc_);
    HAL_ADC_Stop(hadc_);

    float v_is = (static_cast<float>(raw) / ADC_FULL) * ADC_VREF;
    return v_is * (KILIS / R_SENSE_OHM);
}
