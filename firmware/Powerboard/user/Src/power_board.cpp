#include "power_board.h"

#include "main.h"
#include "cmsis_os.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

extern "C" {
extern TIM_HandleTypeDef htim1;     /* Fan 2 PWM, PA7 (TIM1_CH1N)  */
extern TIM_HandleTypeDef htim16;    /* Fan 1 PWM, PA6 (TIM16_CH1)  */
extern ADC_HandleTypeDef hadc1;     /* PA0 = IN5 (MC), PA1 = IN6 (BAT) */
extern I2C_HandleTypeDef hi2c2;     /* INA226, PB10/PB11           */
}

/* ---- Fan curve ---- */
#define T_ON_C          30.0f   /* fan on at or above            */
#define T_OFF_C         28.0f   /* fan off below (hysteresis)    */
#define T_MAX_C         60.0f   /* full speed at or above        */
#define MIN_DUTY_PCT    20U     /* lowest running duty           */
#define LOOP_PERIOD_MS  100U

/* ---- ADC / sensor (CONFIRM WITH YOUR SENSOR) ----
 * Divider: 10k top, 4.7k bottom. Assumed sensor: 10 mV/degC, 0 mV at 0 degC
 * (LM35 style). For a TMP36 set TEMP_OFFSET_MV to 500.0f. */
#define VDDA_V          3.3f
#define ADC_OVERSAMPLE  8U
#define ADC_RAW_MIN     5U      /* outside [MIN, MAX] = shorted/open sensor */
#define ADC_RAW_MAX     4075U
#define TEMP_DIV_RATIO  (4.7f / (10.0f + 4.7f))
#define TEMP_MV_PER_C   10.0f
#define TEMP_OFFSET_MV  0.0f

/* ---- INA226 (7-bit addr 0x44), 5 mOhm shunt ---- */
#define INA226_ADDR         (0x44U << 1)
#define INA226_REG_SHUNT    0x01U
#define INA226_SHUNT_LSB_V  2.5e-6f
#define INA226_SHUNT_OHM    0.005f

/* ---- Fan fault: duty high but almost no current ---- */
#define FAN_FAULT_DUTY_PCT  50U
#define FAN_FAULT_MIN_A     0.05f
#define FAN_FAULT_TIME_MS   2000U

volatile float   mc_temp_C     = VALUE_INVALID;
volatile float   bat_temp_C    = VALUE_INVALID;
volatile uint8_t fan_duty_pct  = 0;
volatile float   fan_current_A = VALUE_INVALID;
volatile uint8_t fan_fault     = 0;
volatile uint8_t sensor_fault  = 0;

static void setFanDuty(TIM_HandleTypeDef* tim, uint8_t pct)
{
    if (pct > 100U) pct = 100U;
    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(tim);
    __HAL_TIM_SET_COMPARE(tim, TIM_CHANNEL_1, ((arr + 1U) * pct) / 100U);
}

/* Oversampled read of one ADC channel, converted to degC. false = invalid. */
static bool readTempC(uint32_t channel, float* temp_C)
{
    ADC_ChannelConfTypeDef cfg{};
    cfg.Channel      = channel;
    cfg.Rank         = ADC_REGULAR_RANK_1;
    cfg.SamplingTime = ADC_SAMPLETIME_92CYCLES_5;
    cfg.SingleDiff   = ADC_SINGLE_ENDED;
    cfg.OffsetNumber = ADC_OFFSET_NONE;

    if (HAL_ADC_ConfigChannel(&hadc1, &cfg) != HAL_OK) return false;

    uint32_t sum = 0;
    for (uint32_t i = 0; i < ADC_OVERSAMPLE; i++) {
        if (HAL_ADC_Start(&hadc1) != HAL_OK) return false;
        if (HAL_ADC_PollForConversion(&hadc1, 10) != HAL_OK) {
            HAL_ADC_Stop(&hadc1);
            return false;
        }
        sum += HAL_ADC_GetValue(&hadc1);
        HAL_ADC_Stop(&hadc1);
    }

    uint32_t raw = sum / ADC_OVERSAMPLE;
    if (raw < ADC_RAW_MIN || raw > ADC_RAW_MAX) return false;

    float v_sensor_mV = ((float)raw * VDDA_V / 4095.0f) / TEMP_DIV_RATIO * 1000.0f;
    *temp_C = (v_sensor_mV - TEMP_OFFSET_MV) / TEMP_MV_PER_C;
    return true;
}

static bool readFanCurrentA(float* amps)
{
    uint8_t buf[2];
    if (HAL_I2C_Mem_Read(&hi2c2, INA226_ADDR, INA226_REG_SHUNT,
                         I2C_MEMADD_SIZE_8BIT, buf, 2, 20) != HAL_OK) return false;
    int16_t raw = (int16_t)(((uint16_t)buf[0] << 8) | buf[1]);
    *amps = ((float)raw * INA226_SHUNT_LSB_V) / INA226_SHUNT_OHM;
    return true;
}

static uint8_t computeDuty(float t, bool sensors_ok)
{
    static bool fan_on = false;

    if (!sensors_ok) {              /* fail-safe: full speed */
        fan_on = true;
        return 100U;
    }

    if (fan_on) { if (t < T_OFF_C) fan_on = false; }
    else        { if (t >= T_ON_C) fan_on = true;  }

    if (!fan_on)      return 0U;
    if (t >= T_MAX_C) return 100U;
    if (t <= T_ON_C)  return MIN_DUTY_PCT;

    return (uint8_t)((float)MIN_DUTY_PCT +
           ((t - T_ON_C) / (T_MAX_C - T_ON_C)) * (100.0f - (float)MIN_DUTY_PCT));
}

void fanControlStep(void)
{
    static bool     started        = false;
    static uint8_t  cal_fault      = 0;
    static uint32_t low_current_ms = 0;

    if (!started) {
        if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED) != HAL_OK)
            cal_fault = FAULT_ADC_CAL;
        setFanDuty(&htim16, 100U);          /* safe default until first reading */
        setFanDuty(&htim1,  100U);
        HAL_TIM_PWM_Start(&htim16, TIM_CHANNEL_1);
        HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);
        started = true;
    }

    uint8_t faults = cal_fault;
    float t_mc = 0.0f, t_bat = 0.0f;

    /* 1. Temperatures (hottest valid sensor controls; any fault -> full speed) */
    if (readTempC(ADC_CHANNEL_5, &t_mc)) mc_temp_C = t_mc;
    else { mc_temp_C = VALUE_INVALID; faults |= FAULT_MC_TEMP; }

    if (readTempC(ADC_CHANNEL_6, &t_bat)) bat_temp_C = t_bat;
    else { bat_temp_C = VALUE_INVALID; faults |= FAULT_BAT_TEMP; }

    float control_temp = (t_bat > t_mc) ? t_bat : t_mc;
    bool  sensors_ok   = !(faults & (FAULT_MC_TEMP | FAULT_BAT_TEMP));

    /* 2. Duty -> both fans */
    uint8_t duty = computeDuty(control_temp, sensors_ok);
    setFanDuty(&htim16, duty);
    setFanDuty(&htim1,  duty);
    fan_duty_pct = duty;

    /* 3. Fan current check */
    float amps = 0.0f;
    if (readFanCurrentA(&amps)) {
        fan_current_A = amps;
        if (duty >= FAN_FAULT_DUTY_PCT && fabsf(amps) < FAN_FAULT_MIN_A) {
            if (low_current_ms < FAN_FAULT_TIME_MS) low_current_ms += LOOP_PERIOD_MS;
            if (low_current_ms >= FAN_FAULT_TIME_MS) fan_fault = 1U;
        } else {
            low_current_ms = 0;
            fan_fault = 0U;
        }
    } else {
        fan_current_A  = VALUE_INVALID;
        low_current_ms = 0;
        faults |= FAULT_INA226;
    }

    sensor_fault = faults;
    osDelay(LOOP_PERIOD_MS);
}