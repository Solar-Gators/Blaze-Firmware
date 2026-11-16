//
// Created by aelug on 10/29/2026.
//

#include "../../../firmware/BMS-Motherboard/Drivers/STM32G4xx_HAL_Driver/Inc/stm32g4xx_hal.h" // fix later to just be "stm32g4xx_hal.h"
#include "../../../firmware/BMS-Motherboard/Drivers/STM32G4xx_HAL_Driver/Inc/stm32g4xx_hal_def.h"
#include "../../../firmware/BMS-Motherboard/Drivers/STM32G4xx_HAL_Driver/Inc/stm32g4xx_hal_i2c.h"
#include "HDC2080DMBR.hpp"

#define I2C_TIMEOUT_MS 100 // time HAL waits on the bus before returning an error

namespace sg
{

HAL_StatusTypeDef HDC2080DMBR::init()
{
    if (checkDeviceID() != HAL_OK) return HAL_ERROR; // wrong chip, or sensor not responding
    if (softReset() != HAL_OK) return HAL_ERROR; // resets registers to default
    HAL_Delay(5); // time for sensor to finish resetting

    // config for humidity + temperature, 14-bit resolution
    if (writeRegister(static_cast<uint8_t>(registers::MEASUREMENT_CONFIG), 0x00) != HAL_OK) return HAL_ERROR;

    return HAL_OK;
}

HAL_StatusTypeDef HDC2080DMBR::getTemperature(float* temp_celsius)
{
    if (triggerMeasurement() != HAL_OK) return HAL_ERROR; // starts measurement
    HAL_Delay(10); // wait for conversion
    if (readRaw() != HAL_OK) return HAL_ERROR; // fetches the 4 result bytes into raw_

    *temp_celsius = rawToCelsius(raw_[0], raw_[1]); // raw_[0] = register 0x00 (low), raw_[1] = 0x01 (high)

    return HAL_OK;
}

HAL_StatusTypeDef HDC2080DMBR::getHumidity(float* humidity_rh)
{
    if (triggerMeasurement() != HAL_OK) return HAL_ERROR; // starts measurement
    HAL_Delay(10); // wait for conversion
    if (readRaw() != HAL_OK) return HAL_ERROR; // fetches the 4 result bytes into raw_

    *humidity_rh = rawToHumidity(raw_[2], raw_[3]); // raw_[2] = register 0x02 (low), raw_[3] = 0x03 (high)

    return HAL_OK;
}

HAL_StatusTypeDef HDC2080DMBR::readRegister(uint8_t reg, uint8_t* data, uint16_t length)
{
    // HAL sends the register number, then reads 'length' bytes; the register number auto-increments
    return HAL_I2C_Mem_Read(hi2c_, i2c_address, reg, I2C_MEMADD_SIZE_8BIT, data, length, I2C_TIMEOUT_MS);
}

HAL_StatusTypeDef HDC2080DMBR::writeRegister(uint8_t reg, uint8_t data)
{
    // HAL sends the register number, then the one byte to store in it
    return HAL_I2C_Mem_Write(hi2c_, i2c_address, reg, I2C_MEMADD_SIZE_8BIT, &data, 1, I2C_TIMEOUT_MS);
}

HAL_StatusTypeDef HDC2080DMBR::checkDeviceID()
{
    uint8_t buf[2] = {0};

    // reads 0xFE and 0xFF in one go: buf[0] = low byte and buf[1] = high byte
    if (readRegister(static_cast<uint8_t>(registers::DEVICE_ID_LOW), buf, 2) != HAL_OK) return HAL_ERROR;

    uint16_t id = (static_cast<uint16_t>(buf[1]) << 8) | buf[0]; // combines both halves (high byte first)
    return (id == HDC_DEVICE_ID) ? HAL_OK : HAL_ERROR; // should read 0x07D0
}

HAL_StatusTypeDef HDC2080DMBR::softReset()
{
    uint8_t reg = 0x00;
    if (readRegister(static_cast<uint8_t>(registers::CONFIG), &reg, 1) != HAL_OK) return HAL_ERROR; // reads first so bits are kept

    reg |= HDC_SOFT_RES_MASK; // set bit 7; the sensor clears itself when the reset finishes
    return writeRegister(static_cast<uint8_t>(registers::CONFIG), reg);
}

HAL_StatusTypeDef HDC2080DMBR::triggerMeasurement()
{
    uint8_t reg = 0x00;
    if (readRegister(static_cast<uint8_t>(registers::MEASUREMENT_CONFIG), &reg, 1) != HAL_OK) return HAL_ERROR; // reads first so bits are kept

    reg |= HDC_MEAS_TRIG_MASK; // set bit 0 to start a measurement; the sensor clears it when done
    return writeRegister(static_cast<uint8_t>(registers::MEASUREMENT_CONFIG), reg);
}

HAL_StatusTypeDef HDC2080DMBR::readRaw()
{
    // read all 4 result bytes starting at 0x00: temp low, temp high, humidity low, humidity high
    return readRegister(static_cast<uint8_t>(registers::TEMP_LOW), raw_.data(), HDC_RAW_BYTES);
}

float HDC2080DMBR::rawToCelsius(uint8_t low, uint8_t high)
{
    uint16_t raw = (static_cast<uint16_t>(high) << 8) | low; // combines both halves
    return (raw / 65536.0f) * 165.0f - 40.5f; // datasheet equation to get recorded temperature
}

float HDC2080DMBR::rawToHumidity(uint8_t low, uint8_t high)
{
    uint16_t raw = (static_cast<uint16_t>(high) << 8) | low; // combines both halves
    return (raw / 65536.0f) * 100.0f; // datasheet equation to get recorded humidity
}

} // namespace sg