#include "CurrentSenseDrivers.h"
#define ADS_CMD_NULL 0x0000u
#define ADS_CMD_RESET  0x0011u
#define ADS_CMD_RREG   0xA000u
#define ADS_CMD_WREG   0x6000u
#define ADS_RSP_RESET  0xFF22u
#define ADS_ADDR_SHIFT 7u
#define ADS_REG_ID  0x00u
#define ADS_REG_MODE  0x02u
#define ADS_REG_CLOCK 0x03u
#define ADS_REG_GAIN  0x04u
#define ADS_ID_MASK 0xFF00u
#define ADS_ID_VALUE 0x2200u
#define ADS_STATUS_RESET (1u << 10)
#define ADS_MODE_VALUE 0x0110u
#define ADS_CLOCK_CH_EN_SHIFT 8u
#define ADS_CLOCK_TBM (1u << 5)
#define ADS_CLOCK_OSR_SHIFT 2u
#define ADS_CLOCK_PWR_VLP 0u
#define ADS_CLOCK_PWR_LP 1u
#define ADS_CLOCK_PWR_HR 2u

#define WORD_BYTES 3u
#define FRAME_WORDS  4u
#define FRAME_BYTES (WORD_BYTES * FRAME_WORDS)
#define FRAME_CRC_OFFSET (WORD_BYTES * 3u)
#define FRAME_CH_OFFSET   WORD_BYTES

#define POR_DELAY_US 1000u
#define RESET_DELAY_US 10u
#define POLLS_PER_SAMPLE 16u
#define WAIT_SAMPLE_PERIODS 8u
#define WAIT_POLLS (POLLS_PER_SAMPLE * WAIT_SAMPLE_PERIODS)
#define START_DELAY_TMOD 300u
#define DEFAULT_OSR 1024u
#define SINC3_SETTLE_PERIODS 3u

#define CODE_MAX 8388607L
#define CODE_MIN (-8388608L)
#define CODE_SPAN_F 8388608.0f
#define VREF_V 1.2f
#define FCLKIN_MIN_HZ   300000u
#define FCLKIN_MAX_HZ   8400000u
#define FCLKIN_VLP_MAX  2080000u
#define FCLKIN_LP_MAX  4150000u
#define ZIN_BASE_OHMS   330000.0f
#define ZIN_BASE_FMOD   4096000.0f

#define SENSOR_V_PER_A 0.0125f
#define SENSOR_MAX_A 200.0f
#define SENSOR_CLIP_V 2.6f
#define ZERO_LIMIT_V  0.005f
#define SPAN_TRIM_MIN 0.95f
#define SPAN_TRIM_MAX 1.05f

typedef struct {
    uint16_t response;
    int32_t channel[2];
    bool crc_ok;
} frame_t;

static float abs_f(float value){
    return (value<0.0f)?-value:value;
}
static uint16_t crc16_ccitt(const uint8_t *data, size_t len){
    uint16_t crc = 0xFFFFu;
    for (size_t i = 0u; i < len; i++) {
        crc = (uint16_t)(crc ^ (uint16_t)((uint16_t)data[i] << 8));
        for (uint8_t bit = 0u; bit < 8u; bit++) {
            if ((crc & 0x8000u) != 0u) {
                crc = (uint16_t)((uint16_t)(crc << 1) ^ 0x1021u);
            } else {
                crc = (uint16_t)(crc << 1);
            }
        }
    }
    return crc;
}

static int32_t decode_code(const uint8_t *word){
    uint32_t raw=((uint32_t)word[0] << 16) |((uint32_t)word[1] << 8) |(uint32_t)word[2];
    if ((raw & 0x800000u) != 0u) {
        return (int32_t)raw - 16777216L;
    }
    return (int32_t)raw;
}

static ssa2_status_t frame_transfer(const ssa2_t *dev, uint16_t command,uint16_t argument, frame_t *frame){
    uint8_t tx[FRAME_BYTES] = { 0u };
    uint8_t rx[FRAME_BYTES] = { 0u };
    tx[0] = (uint8_t)(command >> 8);
    tx[1] = (uint8_t)command;
    tx[3] = (uint8_t)(argument >> 8);
    tx[4] = (uint8_t)argument;
    if (dev->hal.spi_transfer(dev->hal.ctx, tx, rx, FRAME_BYTES) != 0) {
        return SSA2_ERR_SPI;
    }
    uint16_t received_crc = (uint16_t)(((uint16_t)rx[FRAME_CRC_OFFSET] << 8) |rx[FRAME_CRC_OFFSET + 1u]);
    frame->response = (uint16_t)(((uint16_t)rx[0] << 8) | rx[1]);
    frame->channel[0] = decode_code(&rx[FRAME_CH_OFFSET]);
    frame->channel[1] = decode_code(&rx[FRAME_CH_OFFSET + WORD_BYTES]);
    frame->crc_ok = (crc16_ccitt(rx, FRAME_CRC_OFFSET) == received_crc);
    return SSA2_OK;
}

static ssa2_status_t reg_read(const ssa2_t *dev, uint8_t address,uint16_t *value){
    frame_t frame;
    uint16_t command = (uint16_t)(ADS_CMD_RREG |((uint32_t)address << ADS_ADDR_SHIFT));
    ssa2_status_t status = frame_transfer(dev, command, 0u, &frame);
    if (status != SSA2_OK) {
        return status;
    }
    status = frame_transfer(dev, ADS_CMD_NULL, 0u, &frame);
    if (status != SSA2_OK) {
        return status;
    }
    if (!frame.crc_ok) {
        return SSA2_ERR_CRC;
    }
    *value = frame.response;
    return SSA2_OK;
}

static ssa2_status_t reg_write_verified(const ssa2_t *dev, uint8_t address,uint16_t value){
    frame_t frame;
    uint16_t readback = 0u;
    uint16_t command = (uint16_t)(ADS_CMD_WREG |((uint32_t)address << ADS_ADDR_SHIFT));
    ssa2_status_t status = frame_transfer(dev, command, value, &frame);
    if (status != SSA2_OK) {
        return status;
    }
    status = reg_read(dev, address, &readback);
    if (status != SSA2_OK) {
        return status;
    }
    return (readback == value) ? SSA2_OK : SSA2_ERR_CONFIG;
}

static ssa2_status_t wait_ready(const ssa2_t *dev, uint32_t polls){
    for (uint32_t i = 0u; i < polls; i++) {
        if (dev->hal.drdy_is_low(dev->hal.ctx)) {
            return SSA2_OK;
        }
        dev->hal.delay_us(dev->hal.ctx, dev->poll_us);
    }
    return SSA2_ERR_TIMEOUT;
}

static ssa2_status_t read_code(const ssa2_t *dev, int32_t *code){
    frame_t frame;
    ssa2_status_t status = frame_transfer(dev, ADS_CMD_NULL, 0u, &frame);
    if (status != SSA2_OK) {
        return status;
    }
    if (!frame.crc_ok) {
        return SSA2_ERR_CRC;
    }
    if ((frame.response & ADS_STATUS_RESET) != 0u) {
        return SSA2_ERR_DEVICE_RESET;
    }
    *code = frame.channel[dev->channel];
    return SSA2_OK;
}

static bool is_saturated(const ssa2_t *dev, int32_t code){
    if ((code >= CODE_MAX) || (code <= CODE_MIN)) {
        return true;
    }
    return abs_f((float)code) >= dev->sensor_clip_lsb;
}

static ssa2_status_t code_to_amps(const ssa2_t *dev, int32_t code, float *amps){
    *amps = ((float)code-dev->cal.offset_lsb) * dev->amps_per_lsb * dev->cal.gain_trim;
    return is_saturated(dev,code) ? SSA2_ERR_SATURATED : SSA2_OK;
}

static ssa2_status_t average_codes(ssa2_t *dev,uint32_t samples,float *mean){
    int64_t sum = 0;
    ssa2_status_t status = ssa2_flush(dev);
    if (status != SSA2_OK) {
        return status;
    }
    for (uint32_t i = 0u; i < samples; i++) {
        int32_t code = 0;
        status = wait_ready(dev, WAIT_POLLS);
        if (status == SSA2_OK) {
            status = read_code(dev, &code);
        }
        if (status != SSA2_OK) {
            return status;
        }
        if (is_saturated(dev, code)) {
            return SSA2_ERR_SATURATED;
        }
        sum += code;
    }
    *mean = (float)((double)sum / (double)samples);
    return SSA2_OK;
}

static bool osr_to_clock_bits(ssa2_osr_t osr, uint16_t *bits){
    uint16_t code;
    switch (osr) {
    case SSA2_OSR_64:
        *bits = ADS_CLOCK_TBM;
        return true;
    case SSA2_OSR_128:
            code = 0u; break;
    case SSA2_OSR_256:
            code = 1u; break;
    case SSA2_OSR_512:
            code = 2u; break;
    case SSA2_OSR_1024:
            code = 3u; break;
    case SSA2_OSR_2048:
            code = 4u; break;
    case SSA2_OSR_4096:  code = 5u; break;
    case SSA2_OSR_8192:  code = 6u; break;
    case SSA2_OSR_16384: code = 7u; break;
    default:
        return false;
    }
    *bits = (uint16_t)(code << ADS_CLOCK_OSR_SHIFT);
    return true;
}

static bool config_is_valid(const ssa2_config_t *cfg){
    return (cfg->channel <= 1u) &&((unsigned)cfg->pga_gain <= (unsigned)SSA2_PGA_GAIN_4) &&(cfg->fclkin_hz >= FCLKIN_MIN_HZ) &&(cfg->fclkin_hz <= FCLKIN_MAX_HZ) &&(cfg->rs_ohms >= 0.0f) && (cfg->rp_ohms >= 0.0f);
}
static uint16_t power_bits(uint32_t fclkin_hz){
    if (fclkin_hz <= FCLKIN_VLP_MAX) {
        return ADS_CLOCK_PWR_VLP;
    }
    if (fclkin_hz <= FCLKIN_LP_MAX) {
        return ADS_CLOCK_PWR_LP;
    }
    return ADS_CLOCK_PWR_HR;
}

static float frontend_ratio(const ssa2_config_t *cfg){
    float fmod_hz = (float)cfg->fclkin_hz / 2.0f;
    float zin = ZIN_BASE_OHMS * ZIN_BASE_FMOD / fmod_hz;
    float load = zin;
    if (cfg->rp_ohms > 0.0f) {
        load = (cfg->rp_ohms * zin) / (cfg->rp_ohms + zin);
    }
    return load/(load+(2.0f * cfg->rs_ohms));
}
static uint32_t settle_samples(uint32_t osr){
    return SINC3_SETTLE_PERIODS+((START_DELAY_TMOD + osr -1u) / osr) + 1u;
}
static uint32_t startup_wait_polls(const ssa2_t *dev, const ssa2_config_t *cfg){
    uint64_t tmod = (uint64_t)START_DELAY_TMOD + DEFAULT_OSR + (uint64_t)cfg->osr;
    uint64_t wait_us = (tmod * 4u * 1000000u) / cfg->fclkin_hz;
    return (uint32_t)(wait_us / dev->poll_us) + WAIT_POLLS;
}
static void derive_scaling(ssa2_t *dev, const ssa2_config_t *cfg){
    float pga_gain = (float)(1u << (unsigned)cfg->pga_gain);
    float ratio = frontend_ratio(cfg);
    float adc_full_scale_v = VREF_V / pga_gain;
    float adc_limit_a = adc_full_scale_v / (ratio * SENSOR_V_PER_A);
    float direction = cfg->invert ? -1.0f : 1.0f;
    uint32_t period_us = (uint32_t)(((uint64_t)2u * (uint64_t)cfg->osr *1000000u) / cfg->fclkin_hz);
    dev->channel = cfg->channel;
    dev->volts_per_lsb = adc_full_scale_v / (CODE_SPAN_F * ratio);
    dev->amps_per_lsb = direction * dev->volts_per_lsb / SENSOR_V_PER_A;
    dev->sensor_clip_lsb = SENSOR_CLIP_V / dev->volts_per_lsb;
    dev->full_scale_a = (adc_limit_a < SENSOR_MAX_A) ? adc_limit_a: SENSOR_MAX_A;
    dev->sample_rate_hz = (float)cfg->fclkin_hz / (2.0f * (float)cfg->osr);
    dev->poll_us = period_us / POLLS_PER_SAMPLE;
    if (dev->poll_us == 0u) {
        dev->poll_us = 1u;
    }
    dev->cal.offset_lsb = 0.0f;
    dev->cal.gain_trim = 1.0f;
}
static ssa2_status_t reset_device(const ssa2_t *dev){
    frame_t frame;
    uint16_t id = 0u;
    ssa2_status_t status;
    dev->hal.delay_us(dev->hal.ctx, POR_DELAY_US);
    status = frame_transfer(dev, ADS_CMD_RESET, 0u, &frame);
    if (status != SSA2_OK) {
        return status;
    }
    dev->hal.delay_us(dev->hal.ctx, RESET_DELAY_US);
    status = frame_transfer(dev, ADS_CMD_NULL, 0u, &frame);
    if (status != SSA2_OK) {
        return status;
    }
    if (frame.response != ADS_RSP_RESET) {
        return SSA2_ERR_NO_DEVICE;
    }
    status = reg_read(dev, ADS_REG_ID, &id);
    if (status != SSA2_OK) {
        return status;
    }
    return ((id & ADS_ID_MASK) == ADS_ID_VALUE) ? SSA2_OK : SSA2_ERR_NO_DEVICE;
}
ssa2_status_t ssa2_init(ssa2_t *dev, const ssa2_hal_t *hal,const ssa2_config_t *cfg){
    uint16_t osr_bits = 0u;
    if ((dev == NULL) || (hal == NULL) || (cfg == NULL) ||
        (hal->spi_transfer == NULL) || (hal->drdy_is_low == NULL) ||
        (hal->delay_us == NULL) || !config_is_valid(cfg) ||
        !osr_to_clock_bits(cfg->osr, &osr_bits)) {
        return SSA2_ERR_ARG;
    }
    dev->hal = *hal;
    derive_scaling(dev, cfg);
    uint16_t clock_value = (uint16_t)((1u << (ADS_CLOCK_CH_EN_SHIFT + cfg->channel)) |osr_bits | power_bits(cfg->fclkin_hz));
    uint16_t gain_value = (uint16_t)((unsigned)cfg->pga_gain <<(4u * (unsigned)cfg->channel));
    ssa2_status_t status = reset_device(dev);
    if (status == SSA2_OK) {
        status = reg_write_verified(dev, ADS_REG_MODE, ADS_MODE_VALUE);
    }
    if (status == SSA2_OK) {
        status = reg_write_verified(dev, ADS_REG_CLOCK, clock_value);
    }
    if (status == SSA2_OK) {
        status = reg_write_verified(dev, ADS_REG_GAIN, gain_value);
    }
    if (status == SSA2_OK) {
        status = ssa2_flush(dev);
    }
    uint32_t discard = settle_samples((uint32_t)cfg->osr);
    uint32_t polls = startup_wait_polls(dev, cfg);
    for (uint32_t i = 0u; (status == SSA2_OK) && (i < discard); i++) {
        int32_t discarded = 0;
        status = wait_ready(dev, polls);
        if (status == SSA2_OK) {
            status = read_code(dev, &discarded);
        }
    }
    return status;
}
ssa2_status_t ssa2_flush(ssa2_t *dev){
    if (dev == NULL) {
        return SSA2_ERR_ARG;
    }
    for (uint8_t i = 0u; i < 2u; i++) {
        frame_t frame;
        ssa2_status_t status = frame_transfer(dev, ADS_CMD_NULL, 0u, &frame);
        if (status != SSA2_OK) {
            return status;
        }
    }
    return SSA2_OK;
}
bool ssa2_data_ready(const ssa2_t *dev){
    return (dev != NULL) && dev->hal.drdy_is_low(dev->hal.ctx);
}
ssa2_status_t ssa2_read_sample(ssa2_t *dev, float *amps){
    int32_t code = 0;
    if ((dev == NULL) || (amps == NULL)) {
        return SSA2_ERR_ARG;
    }
    if (!dev->hal.drdy_is_low(dev->hal.ctx)) {
        return SSA2_ERR_NOT_READY;
    }
    ssa2_status_t status = read_code(dev, &code);
    if (status != SSA2_OK) {
        return status;
    }
    return code_to_amps(dev, code, amps);
}
ssa2_status_t ssa2_read_current(ssa2_t *dev, float *amps){
    if ((dev == NULL) || (amps == NULL)) {
        return SSA2_ERR_ARG;
    }
    ssa2_status_t status = ssa2_flush(dev);
    if (status == SSA2_OK) {
        status = wait_ready(dev, WAIT_POLLS);
    }
    if (status == SSA2_OK) {
        status = ssa2_read_sample(dev, amps);
    }
    return status;
}
ssa2_status_t ssa2_calibrate_zero(ssa2_t *dev, uint32_t samples){
    float mean = 0.0f;
    if ((dev == NULL) || (samples == 0u)) {
        return SSA2_ERR_ARG;
    }
    ssa2_status_t status = average_codes(dev, samples, &mean);
    if (status != SSA2_OK) {
        return status;
    }
    if (abs_f(mean * dev->volts_per_lsb) > ZERO_LIMIT_V) {
        return SSA2_ERR_CAL_RANGE;
    }
    dev->cal.offset_lsb = mean;
    return SSA2_OK;
}
ssa2_status_t ssa2_calibrate_span(ssa2_t *dev,float known_amps,uint32_t samples){
    float mean = 0.0f;
    if ((dev == NULL) || (samples == 0u) || (known_amps == 0.0f)) {
        return SSA2_ERR_ARG;
    }
    ssa2_status_t status = average_codes(dev, samples, &mean);
    if (status != SSA2_OK) {
        return status;
    }
    float measured = (mean - dev->cal.offset_lsb) * dev->amps_per_lsb;
    if (measured == 0.0f) {
        return SSA2_ERR_CAL_RANGE;
    }
    float trim = known_amps / measured;
    if ((trim < SPAN_TRIM_MIN) || (trim > SPAN_TRIM_MAX)) {
        return SSA2_ERR_CAL_RANGE;
    }
    dev->cal.gain_trim = trim;
    return SSA2_OK;
}
void ssa2_get_calibration(const ssa2_t *dev, ssa2_cal_t *cal){
    if ((dev != NULL) && (cal != NULL)) {
        *cal = dev->cal;
    }
}
void ssa2_set_calibration(ssa2_t *dev, const ssa2_cal_t *cal){
    if ((dev != NULL) && (cal != NULL)) {
        dev->cal = *cal;
    }
}
float ssa2_full_scale_a(const ssa2_t *dev){
    return (dev != NULL) ? dev->full_scale_a : 0.0f;
}
float ssa2_sample_rate_hz(const ssa2_t *dev){
    return (dev != NULL) ? dev->sample_rate_hz : 0.0f;
}