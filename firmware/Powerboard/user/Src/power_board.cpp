#include "power_board.h"

#include "cmsis_os.h"
#include "main.h"
#include <stdint.h>
#include <stdbool.h>

/* Peripheral handles are defined in main.c (CubeMX generated) */
extern TIM_HandleTypeDef htim1;     /* Fan 2 PWM, PA7 (TIM1_CH1N, complementary output) */
extern TIM_HandleTypeDef htim16;    /* Fan 1 PWM, PA6 (TIM16_CH1) */
extern ADC_HandleTypeDef hadc1;     /* PA0 = ADC1_IN5 (MC_TEMP), PA1 = ADC1_IN6 (BAT_TEMP) */
extern I2C_HandleTypeDef hi2c2;     /* INA226 fan current monitor, PB10/PB11 */

/* =====================================================================
 *  TUNABLE SETTINGS
 * ===================================================================== */

/* ---- Fan curve ---- */
#define T_ON_C          30.0f   /* fan turns on at or above this temperature          */
#define T_OFF_C         28.0f   /* fan turns off below this (hysteresis, avoids chatter) */
#define T_MAX_C         60.0f   /* full speed at or above this temperature            */
#define MIN_DUTY_PCT    20U     /* lowest duty once running (avoids stalling)         */
#define LOOP_PERIOD_MS  100U    /* control loop period                                */

/* ---- Bench testing without sensors (no debugger needed) ----
 * Set USE_FAKE_TEMP to 1, rebuild, flash. Fans follow FAKE_TEMP_C instead of the ADC. */
#define USE_FAKE_TEMP   0
#define FAKE_TEMP_C     45.0f

/* ---- Which temperature sensors are fitted/used ---- */
#define USE_MC_TEMP     1       /* PA0 / ADC1_IN5 */
#define USE_BAT_TEMP    1      /* PA1 / ADC1_IN6 */

/* ---- ADC ---- */
#define VDDA_V          3.3f
#define ADC_FULL_SCALE  4095.0f
#define ADC_OVERSAMPLE  8U
#define ADC_RAW_MIN_OK  20U     /* readings outside this window = shorted/open sensor */
#define ADC_RAW_MAX_OK  4075U

/* ---- Temperature sensor transfer function (CONFIRM WITH YOUR SENSOR!) ----
 * Schematic: MC_TEMP_A --R205 10k--+--R207 4.7k-- GND, ADC reads the middle node,
 * so ADC voltage = sensor voltage * 4.7/(10+4.7).
 * Assumed sensor: linear, 10 mV/degC, 0 mV at 0 degC (LM35 style).
 * For a TMP36 use TEMP_OFFSET_MV 500.0f. */
#define TEMP_DIV_RATIO  (4.7f / (10.0f + 4.7f))
#define TEMP_MV_PER_C   10.0f
#define TEMP_OFFSET_MV  0.0f

/* ---- INA226 (A1 = 3V3, A0 = GND -> 7-bit address 0x44), 5 mOhm shunt ---- */
#define INA226_ADDR         (0x44U << 1)
#define INA226_REG_SHUNT    0x01U
#define INA226_SHUNT_LSB_V  2.5e-6f
#define INA226_SHUNT_OHM    0.005f

/* ---- Fan fault detection: duty high but almost no current ---- */
#define FAN_FAULT_DUTY_PCT  50U
#define FAN_FAULT_MIN_A     0.05f
#define FAN_FAULT_TIME_MS   2000U

/* =====================================================================
 *  GLOBALS (add to a debugger watch window if you get one working)
 * ===================================================================== */
volatile float    fake_temp_C   = FAKE_TEMP_C;
volatile float    mc_temp_C     = 0.0f;
volatile float    bat_temp_C    = 0.0f;
volatile uint16_t mc_adc_raw    = 0;
volatile uint16_t bat_adc_raw   = 0;
volatile uint8_t  fan_duty_pct  = 0;
volatile float    fan_current_A = 0.0f;
volatile uint8_t  fan_fault     = 0;
volatile uint8_t  sensor_fault  = 0;

/* =====================================================================
 *  PWM
 * ===================================================================== */
static void setFanDuty(TIM_HandleTypeDef* tim, uint32_t ch, uint8_t pct)
{
    if (pct > 100U) pct = 100U;
    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(tim);
    __HAL_TIM_SET_COMPARE(tim, ch, ((arr + 1U) * pct) / 100U);
}

/* =====================================================================
 *  ADC / TEMPERATURE
 * ===================================================================== */
/* Average ADC_OVERSAMPLE conversions of one channel. Returns 0 on success. */
static int adcReadAvg(uint32_t channel, uint16_t* raw)
{
    ADC_ChannelConfTypeDef cfg{};
    cfg.Channel      = channel;
    cfg.Rank         = ADC_REGULAR_RANK_1;
    cfg.SamplingTime = ADC_SAMPLETIME_92CYCLES_5;  /* long sampling: ~3 kOhm source impedance */
    cfg.SingleDiff   = ADC_SINGLE_ENDED;
    cfg.OffsetNumber = ADC_OFFSET_NONE;
    cfg.Offset       = 0;

    if (HAL_ADC_ConfigChannel(&hadc1, &cfg) != HAL_OK) return -1;

    uint32_t sum = 0;
    for (uint32_t i = 0; i < ADC_OVERSAMPLE; i++) {
        if (HAL_ADC_Start(&hadc1) != HAL_OK) return -1;
        if (HAL_ADC_PollForConversion(&hadc1, 10) != HAL_OK) {
            HAL_ADC_Stop(&hadc1);
            return -1;
        }
        sum += HAL_ADC_GetValue(&hadc1);
        HAL_ADC_Stop(&hadc1);
    }
    *raw = (uint16_t)(sum / ADC_OVERSAMPLE);
    return 0;
}

/* Read one temperature channel in degC. Returns 0 on success, -1 if invalid. */
static int readTempC(uint32_t channel, volatile uint16_t* raw_out, float* temp_C)
{
    uint16_t raw = 0;
    if (adcReadAvg(channel, &raw) != 0) return -1;
    *raw_out = raw;
    if (raw < ADC_RAW_MIN_OK || raw > ADC_RAW_MAX_OK) return -1;

    float v_adc       = ((float)raw * VDDA_V) / ADC_FULL_SCALE;
    float v_sensor_mV = (v_adc / TEMP_DIV_RATIO) * 1000.0f;
    *temp_C = (v_sensor_mV - TEMP_OFFSET_MV) / TEMP_MV_PER_C;
    return 0;
}

/* =====================================================================
 *  INA226 FAN CURRENT
 * ===================================================================== */
static int ina226ReadCurrentA(float* amps)
{
    uint8_t buf[2];
    if (HAL_I2C_Mem_Read(&hi2c2, INA226_ADDR, INA226_REG_SHUNT,
                         I2C_MEMADD_SIZE_8BIT, buf, 2, 20) != HAL_OK) {
        return -1;
    }
    int16_t raw = (int16_t)(((uint16_t)buf[0] << 8) | buf[1]);
    *amps = ((float)raw * INA226_SHUNT_LSB_V) / INA226_SHUNT_OHM;   /* I = Vshunt / R */
    return 0;
}

/* =====================================================================
 *  FAN CURVE
 * ===================================================================== */
static uint8_t computeDuty(float t, bool sensors_ok)
{
    static bool fan_on = false;

    if (!sensors_ok) {          /* fail-safe: bad sensor -> cool at full speed */
        fan_on = true;
        return 100U;
    }

    if (fan_on) {
        if (t < T_OFF_C) fan_on = false;
    } else {
        if (t >= T_ON_C) fan_on = true;
    }

    if (!fan_on)       return 0U;
    if (t >= T_MAX_C)  return 100U;
    if (t <= T_ON_C)   return (uint8_t)MIN_DUTY_PCT;

    return (uint8_t)((float)MIN_DUTY_PCT +
                     ((t - T_ON_C) / (T_MAX_C - T_ON_C)) * (100.0f - (float)MIN_DUTY_PCT));
}

/* =====================================================================
 *  ONE PASS OF THE FAN CONTROL LOOP (called repeatedly from userDefaultTask)
 * ===================================================================== */
void fanControlStep(void)
{
    static uint8_t  started        = 0;
    static uint32_t low_current_ms = 0;

    if (!started) {
        HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
        HAL_TIM_PWM_Start(&htim16, TIM_CHANNEL_1);
        HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);
        setFanDuty(&htim16, TIM_CHANNEL_1, 100U);   /* safe default until first reading */
        setFanDuty(&htim1,  TIM_CHANNEL_1, 100U);
        started = 1;
    }

    /* ---- 1. Get the control temperature ---- */
    float control_temp = 0.0f;
    bool  sensors_ok   = true;

#if USE_FAKE_TEMP
    control_temp = fake_temp_C;
#else
    bool have_temp = false;

  #if USE_MC_TEMP
    float t_mc = 0.0f;
    if (readTempC(ADC_CHANNEL_5, &mc_adc_raw, &t_mc) == 0) {
        mc_temp_C = t_mc;
        control_temp = t_mc;
        have_temp = true;
    } else {
        sensors_ok = false;
    }
  #endif

  #if USE_BAT_TEMP
    float t_bat = 0.0f;
    if (readTempC(ADC_CHANNEL_6, &bat_adc_raw, &t_bat) == 0) {
        bat_temp_C = t_bat;
        if (!have_temp || t_bat > control_temp) control_temp = t_bat;   /* hottest wins */
        have_temp = true;
    } else {
        sensors_ok = false;
    }
  #endif

    if (!have_temp) sensors_ok = false;
#endif

    sensor_fault = sensors_ok ? 0U : 1U;

    /* ---- 2. Temperature -> duty -> both fans ---- */
    uint8_t duty = computeDuty(control_temp, sensors_ok);
    setFanDuty(&htim16, TIM_CHANNEL_1, duty);
    setFanDuty(&htim1,  TIM_CHANNEL_1, duty);
    fan_duty_pct = duty;

    /* ---- 3. Fan current sanity check ---- */
    float amps = 0.0f;
    if (ina226ReadCurrentA(&amps) == 0) {
        fan_current_A = amps;
        if (duty >= FAN_FAULT_DUTY_PCT && amps < FAN_FAULT_MIN_A) {
            if (low_current_ms < FAN_FAULT_TIME_MS) low_current_ms += LOOP_PERIOD_MS;
            if (low_current_ms >= FAN_FAULT_TIME_MS) fan_fault = 1U;
        } else {
            low_current_ms = 0;
            fan_fault = 0U;
        }
    }

    osDelay(LOOP_PERIOD_MS);
}