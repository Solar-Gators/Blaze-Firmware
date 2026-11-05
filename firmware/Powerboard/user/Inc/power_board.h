#ifndef POWER_BOARD_H
#define POWER_BOARD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* One pass of the fan-control loop: reads sensors, sets fan PWM, checks fan
 * current, then sleeps 100 ms with osDelay(). Call repeatedly from a task. */
void fanControlStep(void);

/* Fan controller status (defined in power_board.cpp).
 * Declared here so other files, such as a CAN task, can read them. */
extern volatile float    mc_temp_C;
extern volatile float    bat_temp_C;
extern volatile uint8_t  fan_duty_pct;
extern volatile float    fan_current_A;
extern volatile uint8_t  fan_fault;
extern volatile uint8_t  sensor_fault;

#ifdef __cplusplus
}
#endif

#endif /* POWER_BOARD_H */