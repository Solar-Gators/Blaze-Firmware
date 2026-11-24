/*
 * contactor_bringup.cpp
 *
 * Minimal bring-up / "hello chip" test for the 2026 Central BMS contactor controller.
 *
 * GOAL of this file: prove you can TALK to the BTS71040 and read it back over SPI
 * BEFORE you ever command a contactor closed. Do not skip to switching contactors
 * until this reports the chip is alive and the checksum verifies.
 *
 * What it does:
 *   1. Constructs the low-level driver with the verified pin map.
 *   2. Runs ContactorController::begin() (safe init + checksum verify).
 *   3. Reads diagnostics and prints them over UART so you can see the result.
 *   4. ONLY if everything is clean, optionally closes the main contactors and
 *      prints the measured coil current. (Commented out by default - enable when
 *      you are ready and the pack side is safe to energize.)
 *
 * Assumes CubeMX has already generated and called:
 *   MX_GPIO_Init(), MX_SPI1_Init(), MX_ADC2_Init(), MX_USARTx_UART_Init()
 * and that `hspi1`, `hadc2`, and your UART handle exist.
 *
 * printf-over-UART: this uses a simple HAL_UART_Transmit helper. If you already
 * have printf retargeted to UART, use that instead.
 */

#include "contactor_control.hpp"
#include <cstdio>
#include <cstring>
#include <cstdarg>

// --- CubeMX-generated handles (declared in main.c/.h) -----------------------
extern SPI_HandleTypeDef  hspi1;
extern ADC_HandleTypeDef  hadc2;
extern UART_HandleTypeDef huart2;   // <-- change to whatever UART you wired for the debug console

// --- tiny UART print helper -------------------------------------------------
static void uprint(const char* s) {
    HAL_UART_Transmit(&huart2, reinterpret_cast<uint8_t*>(const_cast<char*>(s)),
                      static_cast<uint16_t>(strlen(s)), 100);
}
static void uprintf(const char* fmt, ...) {
    char buf[128];
    va_list ap; va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    uprint(buf);
}

// --- verified pin map (2026 MotherBoard) ------------------------------------
static Bts71040::Pins contactorPins = {
    .ncs_port = GPIOA, .ncs_pin = GPIO_PIN_4,                        // NCS_L
    .in_port  = { GPIOB,      GPIOB,      GPIOB,      GPIOC },       // IN0, IN1, IN2, IN3
    .in_pin   = { GPIO_PIN_2, GPIO_PIN_1, GPIO_PIN_0, GPIO_PIN_5 }  // PB2,  PB1,  PB0,  PC5
};

static Bts71040           g_dev(&hspi1, contactorPins);
static ContactorController g_contactors(g_dev, &hadc2);

// Call this ONCE from main(), after all the MX_*_Init() calls, before the main loop.
void contactor_bringup_run(void)
{
    uprint("\r\n=== BTS71040 contactor bring-up ===\r\n");

    // Step 1+2: safe init + checksum verify.
    HAL_StatusTypeDef st = g_contactors.begin();
    if (st != HAL_OK) {
        uprint("begin() FAILED - SPI error or checksum mismatch.\r\n");
        uprint("Check: SPI1 mode (CPOL=0/CPHA=1, 8-bit, <=5MHz), NCS_L=PA4 wiring,\r\n"
               "       and that nothing else is driving the shared SPI bus right now.\r\n");
        return;   // DO NOT touch contactors if the chip isn't verified.
    }
    uprint("begin() OK - chip is talking and config checksum verified.\r\n");

    // Step 3: read and print diagnostics.
    ContactorController::Faults f;
    if (g_contactors.poll(f) != HAL_OK) {
        uprint("poll() SPI read failed.\r\n");
        return;
    }
    uprintf("STDDIAG: raw=0x%02X  TER=%d CSV=%d LHI=%d SLP=%d SBM=%d VSMON=%d\r\n",
            f.std.raw, f.std.TER, f.std.CSV, f.std.LHI, f.std.SLP, f.std.SBM, f.std.VSMON);
    uprintf("WRNDIAG: raw=0x%01X  ch[%d %d %d %d]\r\n",
            f.wrn.raw, f.wrn.WRN[0], f.wrn.WRN[1], f.wrn.WRN[2], f.wrn.WRN[3]);
    uprintf("ERRDIAG: raw=0x%01X  ch[%d %d %d %d]\r\n",
            f.err.raw, f.err.ERR[0], f.err.ERR[1], f.err.ERR[2], f.err.ERR[3]);
    uprintf("Any fault flagged: %s\r\n", f.any ? "YES" : "no");

    // Expected healthy result with 12V present and nothing energized:
    //   TER=0, CSV=0, VSMON=0, all WRN/ERR = 0.  VSMON=1 would mean the chip sees
    //   undervoltage on VS (check the 12V supply / fuse F801).

    // ------------------------------------------------------------------------
    // Step 4 (OPTIONAL - leave commented until you are ready to energize):
    //
    // if (!f.any) {
    //     uprint("Closing MAIN contactors...\r\n");
    //     g_contactors.closeMain();
    //     HAL_Delay(50);                       // let coils pull in
    //     float i1 = g_contactors.readCurrentAmps(ContactorController::MAIN_1);
    //     float i2 = g_contactors.readCurrentAmps(ContactorController::MAIN_2);
    //     uprintf("MAIN_1 coil ~%.2f A, MAIN_2 coil ~%.2f A\r\n", i1, i2);
    //
    //     // re-check diagnostics under load
    //     g_contactors.poll(f);
    //     uprintf("Under load - any fault: %s\r\n", f.any ? "YES" : "no");
    //
    //     HAL_Delay(2000);
    //     g_contactors.openMain();
    //     uprint("MAIN contactors opened.\r\n");
    // }
    // ------------------------------------------------------------------------

    uprint("=== bring-up done ===\r\n");
}

/*
 * In your main.c, after the init calls:
 *
 *     extern "C" void contactor_bringup_run(void);   // if main.c is C
 *     ...
 *     contactor_bringup_run();
 *     while (1) {
 *         // later: periodic g_contactors.poll() + fault handling here
 *     }
 *
 * (If main.c is plain C, wrap contactor_bringup_run in extern "C" so the linker
 *  finds it. Or rename main.c to main.cpp.)
 */
