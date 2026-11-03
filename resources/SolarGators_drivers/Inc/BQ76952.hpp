//
// Created by ahmed on 10/28/2026.
//

#ifndef BLAZEFIRMWARE_BQ76952_H
#define BLAZEFIRMWARE_BQ76952_H

#include <cstddef>
#include <cstdint>

#if defined(STM32L433xx)
#include "stm32l4xx_hal.h"
#elif defined(STM32G483xx) || defined(STM32G473xx)
#include "stm32g4xx_hal.h"
#elif defined(STM32U5G9xx)
#include "stm32u5xx_hal.h"
#endif

// HAL_I2C_MODULE_ENABLED comes from the board's *_hal_conf.h (CubeMX I2C enabled)
#if defined(HAL_I2C_MODULE_ENABLED)
#define BQ76952_AVAILABLE 1
#else
#define BQ76952_AVAILABLE 0
#endif

#include "BQ769X2_Registers.h"

#if BQ76952_AVAILABLE

#define BQ_I2C_ADDR_WRITE 0x10
#define BQ_I2C_ADDR_READ 0x11

class BQ76952
{
   public:
    BQ76952();
    HAL_StatusTypeDef Init(I2C_HandleTypeDef *hi2c);
    HAL_StatusTypeDef Init(I2C_HandleTypeDef *hi2c, uint8_t i2cAddress);
    uint8_t i2cAddressWrite{};
    uint8_t i2cAddressRead{};

    [[nodiscard]] HAL_StatusTypeDef Reset() const;
    HAL_StatusTypeDef ConfigUpdate(bool config_update);  // TODO
    static HAL_StatusTypeDef ConfigureVoltageRegs();     // TODO
    HAL_StatusTypeDef ReadVoltages();                    // TODO: Add support for connected cells
    HAL_StatusTypeDef ReadSafetyFaults();
    HAL_StatusTypeDef ReadCurrent();

    //Sleep and Deep Sleep / Mode related functions
    HAL_StatusTypeDef EnterDeepSleep();
    HAL_StatusTypeDef ExitDeepSleep();

    [[nodiscard]] HAL_StatusTypeDef Shutdown() const;

    [[nodiscard]] int16_t GetCellVoltage(uint32_t cell_num) const;
    [[nodiscard]] int16_t GetPackVoltage() const;
    [[nodiscard]] int16_t GetStackVoltage() const;
    [[nodiscard]] int16_t GetAvgCellVoltage() const;
    [[nodiscard]] int16_t GetHighCellVoltage() const;
    [[nodiscard]] int16_t GetLowCellVoltage() const;
    [[nodiscard]] bool GetConfigUpdateStatus() const;

    static constexpr uint8_t CELL_NO_TO_ADDR(uint8_t cell_no)
    {
        return BQ769X2_CMD_VOLTAGE_CELL_1 + ((cell_no - 1) * 2);
    }
    HAL_StatusTypeDef WriteBytes(uint8_t reg_addr, const uint8_t *data, size_t num_bytes) const;
    HAL_StatusTypeDef ReadBytes(uint8_t reg_addr, uint8_t *data, size_t num_bytes) const;
    HAL_StatusTypeDef DirectReadU2(uint8_t reg_addr, uint16_t *value) const;  // TODO
    HAL_StatusTypeDef DirectReadI2(uint8_t reg_addr, int16_t *value) const;   // TODO
    HAL_StatusTypeDef SubcmdRead(uint16_t subcmd, uint32_t *value, size_t num_bytes) const;
    HAL_StatusTypeDef SubcmdReadU1(uint16_t subcmd, uint8_t *value) const;
    HAL_StatusTypeDef SubcmdReadU2(uint16_t subcmd, uint16_t *value) const;
    HAL_StatusTypeDef SubcmdReadU4(uint16_t subcmd, uint32_t *value) const;
    HAL_StatusTypeDef SubcmdReadI1(uint16_t subcmd, int8_t *value) const;   // TODO: Remove
    HAL_StatusTypeDef SubcmdReadI2(uint16_t subcmd, int16_t *value) const;  // TODO: Remove
    HAL_StatusTypeDef SubcmdReadI4(uint16_t subcmd, int32_t *value) const;  // TODO: Remove
    HAL_StatusTypeDef SubcmdReadF4(uint16_t subcmd, float *value) const;
    [[nodiscard]] HAL_StatusTypeDef SubcmdWrite(uint16_t subcmd,
                                                uint32_t value,
                                                size_t num_bytes) const;
    [[nodiscard]] HAL_StatusTypeDef SubcmdCmdOnly(uint16_t subcmd) const;
    [[nodiscard]] HAL_StatusTypeDef SubcmdCmdWriteU1(uint16_t subcmd, uint8_t value) const;
    [[nodiscard]] HAL_StatusTypeDef SubcmdCmdWriteU2(uint16_t subcmd, uint16_t value) const;
    [[nodiscard]] HAL_StatusTypeDef SubcmdCmdWriteU4(uint16_t subcmd, uint32_t value) const;
    [[nodiscard]] HAL_StatusTypeDef SubcmdCmdWriteI1(uint16_t subcmd, int8_t value) const;
    [[nodiscard]] HAL_StatusTypeDef SubcmdCmdWriteI2(uint16_t subcmd, int16_t value) const;
    [[nodiscard]] HAL_StatusTypeDef SubcmdCmdWriteI4(uint16_t subcmd, int32_t value) const;
    [[nodiscard]] HAL_StatusTypeDef SubcmdCmdWriteF4(uint16_t subcmd, float value) const;
    HAL_StatusTypeDef UpdateMode();

    I2C_HandleTypeDef *hi2c_{};

    int16_t cell_voltages_[12] = {0};  // Voltage of each cell in mV
    int16_t avg_cell_voltage_ = 0;     // Average cell voltage in mV
    int16_t high_cell_voltage_ = 0;    // Highest cell voltage in mV
    int16_t low_cell_voltage_ = 0;     // Lowest cell voltage in mV
    int16_t pack_current_ = 0;         // Brief battery pack current, mA

    int16_t pack_voltage_ = 0;   // Pack voltage in mV
    int16_t stack_voltage_ = 0;  // Stack voltage in mV

    float chip_temperature_ = 0;  // temperature of chip in C

    enum BQMode
    {
        BQ_MODE_NORMAL,
        BQ_MODE_SLEEP,
        BQ_MODE_DEEPSLEEP,
        BQ_MODE_SHUTDOWN,
        BQ_MODE_CONFIGUPDATE
    };
    BQMode current_mode_ = BQ_MODE_NORMAL;  // current mode BMS chip is in

    uint8_t stat_a_byte = 0x00;
    uint8_t stat_b_byte = 0x00;
    uint8_t stat_c_byte = 0x00;

    bool manual_bal_disabled_ = false;
    bool auto_bal_charging_enabled_ = false;
    bool auto_bal_relax_enabled_ = false;
    bool auto_bal_sleep_enabled_ = false;
};

#endif

#endif  //BLAZEFIRMWARE_BQ76952_H
