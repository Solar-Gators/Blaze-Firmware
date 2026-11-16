#ifndef CURRENT_SENSE_DRIVERS_H
#define CURRENT_SENSE_DRIVERS_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    SSA2_OK = 0,
    SSA2_ERR_ARG,
    SSA2_ERR_SPI,
    SSA2_ERR_NO_DEVICE,
    SSA2_ERR_CONFIG,
    SSA2_ERR_CRC,
    SSA2_ERR_TIMEOUT,
    SSA2_ERR_NOT_READY,
    SSA2_ERR_SATURATED,
    SSA2_ERR_DEVICE_RESET,
    SSA2_ERR_CAL_RANGE
} ssa2_status_t;

typedef enum {
    SSA2_OSR_64    = 64,
    SSA2_OSR_128   = 128,
    SSA2_OSR_256   = 256,
    SSA2_OSR_512   = 512,
    SSA2_OSR_1024  = 1024,
    SSA2_OSR_2048  = 2048,
    SSA2_OSR_4096  = 4096,
    SSA2_OSR_8192  = 8192,
    SSA2_OSR_16384 = 16384
} ssa2_osr_t;
typedef enum {
    SSA2_PGA_GAIN_1 = 0,
    SSA2_PGA_GAIN_2 = 1,
    SSA2_PGA_GAIN_4 = 2
} ssa2_pga_gain_t;
typedef struct {
    int (*spi_transfer)(void *ctx, const uint8_t *tx, uint8_t *rx, size_t len);
    bool (*drdy_is_low)(void *ctx);
    void (*delay_us)(void *ctx, uint32_t us);
    void *ctx;
} ssa2_hal_t;
typedef struct {
    uint8_t channel;
    ssa2_osr_t osr;
    ssa2_pga_gain_t pga_gain;
    uint32_t fclkin_hz;
    float  rs_ohms;
    float rp_ohms;
    bool invert;
} ssa2_config_t;
typedef struct {
    float offset_lsb;
    float gain_trim;
} ssa2_cal_t;
typedef struct {
    ssa2_hal_t hal;
    ssa2_cal_t cal;
    float amps_per_lsb;
    float sensor_clip_lsb;
    float volts_per_lsb;
    float full_scale_a;
    float sample_rate_hz;
    uint32_t poll_us;
    uint8_t channel;
} ssa2_t;
ssa2_status_t ssa2_init(ssa2_t *dev, const ssa2_hal_t *hal,const ssa2_config_t *cfg);
ssa2_status_t ssa2_read_current(ssa2_t *dev, float *amps);
bool ssa2_data_ready(const ssa2_t *dev);
ssa2_status_t ssa2_read_sample(ssa2_t *dev, float *amps);
ssa2_status_t ssa2_flush(ssa2_t *dev);
ssa2_status_t ssa2_calibrate_zero(ssa2_t *dev, uint32_t samples);
ssa2_status_t ssa2_calibrate_span(ssa2_t *dev, float known_amps,uint32_t samples);
void ssa2_get_calibration(const ssa2_t *dev, ssa2_cal_t *cal);
void ssa2_set_calibration(ssa2_t *dev, const ssa2_cal_t *cal);
float ssa2_full_scale_a(const ssa2_t *dev);
float ssa2_sample_rate_hz(const ssa2_t *dev);
#ifdef __cplusplus
}
#endif
#endif