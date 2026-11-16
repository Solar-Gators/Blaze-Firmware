//
// Created by aelug on 10/29/2026.
//

#pragma once

#include "../../../firmware/BMS-Motherboard/Drivers/STM32G4xx_HAL_Driver/Inc/stm32g4xx_hal_def.h" // fix later to just be "stm32g4xx_hal.h"
#include "../../../firmware/BMS-Motherboard/Drivers/STM32G4xx_HAL_Driver/Inc/stm32g4xx_hal_i2c.h"
#include <array>
#include <cstdint>

// #include "i2c_api.hpp" // future option to swap to logic using the I2CDevice base class (not currently made)

// Address 0x0E: Reset and DRDY/INT Config Register
#define HDC_SOFT_RES_MASK 0b1000'0000

// Address 0x0F: Measurement Config Register
#define HDC_MEAS_TRIG_MASK 0b0000'0001

// Address 0xFF (high byte) and 0xFE (low byte): Device ID
#define HDC_DEVICE_ID 0x07D0

#define HDC_RAW_BYTES 4 // result bytes read at once: temp low/high, humidity low/high

namespace sg
{
class HDC2080DMBR
{
public:

    /**
     * @brief Constructs the HDC2080 driver and binds it to the given I2C handle.
     *
     * @param h Pointer to the HAL I2C handle.
     */
    HDC2080DMBR(I2C_HandleTypeDef* h) : hi2c_(h) {}

    /**
     * @brief Verifies device ID, soft resets, and configures measurement settings.
     *
     * @return HAL_OK on success, HAL_ERROR otherwise.
     */
    HAL_StatusTypeDef init();

    /**
     * @brief Triggers a measurement and returns temperature in degrees Celsius.
     *
     * @param temp_celsius Pointer to store the result.
     *
     * @return HAL_OK on success, HAL_ERROR otherwise.
     */
    HAL_StatusTypeDef getTemperature(float* temp_celsius);

    /**
     * @brief Triggers a measurement and returns relative humidity in %RH.
     *
     * @param humidity_rh Pointer to store the result.
     *
     * @return HAL_OK on success, HAL_ERROR otherwise.
     */
    HAL_StatusTypeDef getHumidity(float* humidity_rh);

private:
    static constexpr uint8_t i2c_address = (0x40 << 1); // sensor's name on the bus (ADDR pin to GND), shifted left 1 for STM32 HAL

    I2C_HandleTypeDef* hi2c_;

    std::array<uint8_t, HDC_RAW_BYTES> raw_{}; // holds the 4 bytes we read

    enum class registers : uint8_t
    {

        TEMP_LOW = 0x00, // temp result, lower 8 bits
        TEMP_HIGH = 0x01, // temp result, upper 8 bits
        HUMIDITY_LOW = 0x02, // humidity result, lower 8 bits
        HUMIDITY_HIGH = 0x03, // humidity result, upper 8 bits
        CONFIG = 0x0E, // reset, auto-measure rate, heater, and interrupt
        MEASUREMENT_CONFIG = 0x0F, // resolution, what to measure, and the measurement trigger bit
        DEVICE_ID_LOW = 0xFE, // device ID, lower 8 bits
        DEVICE_ID_HIGH = 0xFF, // device ID, upper 8 bits

        // Optional registers to add later:

        /* INTERRUPT_DRDY = 0x04,
        TEMPERATURE_MAX = 0x05,
        HUMIDITY_MAX = 0x06,
        INTERRUPT_ENABLE = 0x07,
        TEMP_OFFSET_ADJUST = 0x08,
        HUM_OFFSET_ADJUST = 0x09,
        TEMP_THR_L = 0x0A,
        TEMP_THR_H = 0x0B,
        RH_THR_L = 0x0C,
        RH_THR_H = 0x0D,
        MANUFACTURER_ID_LOW = 0xFC,
        MANUFACTURER_ID_HIGH = 0xFD,

        */
    };


    /* FUNCTIONS */

    /**
     * @brief Reads length bytes from reg into data.
     *
     * @param reg Register address to read from.
     * @param data Buffer to store the result.
     * @param length Number of bytes to read.
     *
     * @return HAL_OK on success, otherwise the HAL error status.
     */
    HAL_StatusTypeDef readRegister(uint8_t reg, uint8_t *data, uint16_t length);

    /**
     * @brief Writes data to reg.
     *
     * @param reg Register address to write to.
     * @param data Byte value to write.
     *
     * @return HAL_OK on success, otherwise the HAL error status.
     */
    HAL_StatusTypeDef writeRegister(uint8_t reg, uint8_t data);

    /**
     * @brief Reads registers 0xFE-0xFF and checks against HDC_DEVICE_ID (0x07D0).
     *
     * @return HAL_OK on success, HAL_ERROR otherwise.
     */
    HAL_StatusTypeDef checkDeviceID();

    /**
     * @brief Sets SOFT_RES bit in register 0x0E. Self-clearing.
     *
     * @return HAL_OK on success, HAL_ERROR otherwise.
     */
    HAL_StatusTypeDef softReset();

    /**
     * @brief Sets MEAS_TRIG bit in register 0x0F to start a measurement. Self-clearing.
     *
     * @return HAL_OK on success, HAL_ERROR otherwise.
     */
    HAL_StatusTypeDef triggerMeasurement();

    /**
     * @brief Reads 4 raw result bytes from registers 0x00-0x03 into raw_.
     *
     * @return HAL_OK on success, otherwise the HAL error status.
     */
    HAL_StatusTypeDef readRaw();

    /**
     * @brief Converts raw bytes to degrees Celsius: (raw / 65536.0) * 165.0 - 40.5.
     *
     * @param low Register 0x00.
     * @param high Register 0x01.
     *
     * @return Temperature in degrees Celsius.
     */
    static float rawToCelsius(uint8_t low, uint8_t high);

    /**
     * @brief Converts raw bytes to %RH: (raw / 65536.0) * 100.0.
     *
     * @param low Register 0x02.
     * @param high Register 0x03.
     *
     * @return Relative humidity in %RH.
     */
    static float rawToHumidity(uint8_t low, uint8_t high);

};
}// namespace sg