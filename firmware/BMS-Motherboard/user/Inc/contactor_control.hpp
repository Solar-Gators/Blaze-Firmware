/*
 * contactor_control.hpp
 *
 * Application-level contactor control for the 2026 Central BMS MotherBoard.
 * Sits on top of last year's Bts71040 low-level driver (bts71040.hpp/.cpp),
 * which handles the raw SPI protocol, register map, and checksum.
 *
 * Chip:  Infineon BTS71040-4ESA (4-channel high-side switch) = U600
 * Board: 2026 Central BMS MotherBoard, Contactor Controller block
 *
 * Verified wiring (STM32G473RCT6, from Core.SchDoc):
 *   SPI1  : SCK=PA5, MISO=PA6, MOSI=PA7   (shared bus - also serves the shunt ADC, CS=PA3)
 *   NCS_L : PA4   (this chip's chip-select, GPIO, idle HIGH)
 *   IN0   : PB2   -> OUT0  (MAIN contactor)
 *   IN1   : PB1   -> OUT1  (MAIN contactor)
 *   IN2   : PB0   -> OUT2
 *   IN3   : PC5   -> OUT3
 *   IS    : PC4   -> ADC2_IN5   (current sense output, 1.2k sense resistor + ~1us RC filter)
 */

#pragma once
#include "bts71040.hpp"
#include "stm32g4xx_hal.h"

class ContactorController {
public:
    // Channel indices as wired on the 2026 board. 0-based, matching the OUTn / INn
    // silkscreen labels. NOTE: last year's Bts71040::setChannel() is 1-BASED, so we
    // translate internally (board channel 0 -> driver channel 1, etc.).
    enum Channel : uint8_t {
        MAIN_1 = 0,   // OUT0 / IN0 / PB2  -- main contactor
        MAIN_2 = 1,   // OUT1 / IN1 / PB1  -- main contactor
        AUX_0  = 2,   // OUT2 / IN2 / PB0
        AUX_1  = 3,   // OUT3 / IN3 / PC5
    };

    // Snapshot of all three diagnostic registers plus a convenience "any fault" flag.
    struct Faults {
        bool              any;   // true if any warning/error/std-diag fault is flagged
        Bts71040::StdDiag std;
        Bts71040::WrnDiag wrn;
        Bts71040::ErrDiag err;
    };

    // dev  : the low-level BTS71040 driver, already constructed with the correct Pins + SPI handle
    // hadc : ADC handle configured for ADC2_IN5 (PC4), 12-bit, single conversion
    ContactorController(Bts71040& dev, ADC_HandleTypeDef* hadc)
        : dev_(dev), hadc_(hadc) {}

    // One-time safe initialization. Puts all channels OPEN, configures protection,
    // writes + verifies the config checksum, and confirms the chip is talking.
    // Returns HAL_OK only if SPI worked AND the checksum verified (chip config is good).
    // Call this once at startup BEFORE commanding any contactor closed.
    HAL_StatusTypeDef begin();

    // --- Control ----------------------------------------------------------------
    // The two main contactors (OUT0 + OUT1) are driven together.
    void closeMain();
    void openMain();

    // Individual channel control.
    void close(Channel ch);
    void open(Channel ch);

    // Emergency: force every channel OPEN immediately. Safe to call from a fault handler.
    void allOpen();

    // --- Diagnostics ------------------------------------------------------------
    // Read all three diagnostic registers. Call this periodically in your main loop.
    // Returns HAL_OK if the SPI reads succeeded (f.any tells you if a fault is present).
    HAL_StatusTypeDef poll(Faults& f);

    // --- Current sense ----------------------------------------------------------
    // Read the load current of one channel (in Amps) via the IS pin on ADC2_IN5.
    // Only meaningful while that channel is CLOSED (IS is high-impedance when the
    // channel is off). Accuracy is best near nominal current (+-8%) and coarse at
    // low currents - treat it as "is the coil roughly pulling the current I expect",
    // not a precision measurement.
    float readCurrentAmps(Channel ch);

private:
    Bts71040&          dev_;
    ADC_HandleTypeDef* hadc_;

    // --- Current-sense constants (from BTS71040-4ESA datasheet + this board) -----
    static constexpr float KILIS       = 2000.0f; // default (high range), KRC.KRCn = 0
    static constexpr float R_SENSE_OHM = 1200.0f; // R612 on the board (IS -> GND)
    static constexpr float ADC_VREF    = 3.3f;    // ADC reference voltage
    static constexpr float ADC_FULL    = 4095.0f; // 12-bit ADC full scale
    // IL = V_IS * (KILIS / R_SENSE_OHM) = V_IS * 1.667 A/V

    // Translate a board 0-based channel to the driver's 1-based channel.
    static inline uint8_t toDriverCh(Channel ch) { return static_cast<uint8_t>(ch) + 1; }
};
