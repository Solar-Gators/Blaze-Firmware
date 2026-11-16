#ifndef POWER_BOARD_H
#define POWER_BOARD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* sensor_fault bits */
#define FAULT_MC_TEMP   0x01U
#define FAULT_BAT_TEMP  0x02U
#define FAULT_INA226    0x04U
#define FAULT_ADC_CAL   0x08U

/* Published in place of a temperature/current that could not be read */
#define VALUE_INVALID   (-999.0f)

void fanControlStep(void);

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