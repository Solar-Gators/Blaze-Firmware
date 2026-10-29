//
// Created by ahmed on 10/28/2026.
//

#ifndef BLAZEFIRMWARE_BQ76952_H
#define BLAZEFIRMWARE_BQ76952_H

#include <main.h>
#include "BQ76952_Registers.h"
#include <cmath>

#define BQ_I2C_ADDR_WRITE 0x10
#define BQ_I2C_ADDR_READ 0x11

class BQ76952 {
public:
	BQ76952();
    HAL_StatusTypeDef Init(I2C_HandleTypeDef *hi2c);
    HAL_StatusTypeDef Init(I2C_HandleTypeDef *hi2c, uint8_t i2cAddress);
    uint8_t i2cAddressWrite{};
    uint8_t i2cAddressRead{};

    HAL_StatusTypeDef Reset();
    HAL_StatusTypeDef ConfigUpdate(bool config_update); // TODO
    static HAL_StatusTypeDef ConfigureVoltageRegs(); // TODO
    HAL_StatusTypeDef ReadVoltages();       // TODO: Add support for connected cells
    HAL_StatusTypeDef ReadSafetyFaults();
    HAL_StatusTypeDef ReadCurrent();
    HAL_StatusTypeDef ReadTemperatures();

    // Balancing Related Functions
    HAL_StatusTypeDef ChangeBalancingStatus(bool disableManualBal, bool enableBalWhileSleep, bool enableBalWhileRelax, bool enableBalWhileCharging); // 1 for enable, 0 for disable
    HAL_StatusTypeDef DisableBalancing();

    //Sleep and Deep Sleep / Mode related functions
    HAL_StatusTypeDef EnterDeepSleep();
    HAL_StatusTypeDef ExitDeepSleep();
    HAL_StatusTypeDef ModifySleepCurrentBoundary(int16_t boundary); // in mA, signed integer but minimum value is zero, max is 32767, startup is 20 mA

    HAL_StatusTypeDef Shutdown() const;

    int16_t GetCellVoltage(uint32_t cell_num) const;
    int16_t GetPackVoltage() const;
    int16_t GetStackVoltage() const;
    int16_t GetAvgCellVoltage() const;
    int16_t GetHighCellVoltage() const;
    int16_t GetLowCellVoltage() const;
    int16_t GetPackCurrent() const;
    bool GetConfigUpdateStatus() const;

    static constexpr uint8_t CELL_NO_TO_ADDR(uint8_t cell_no) { return BQ769X2_CMD_VOLTAGE_CELL_1 + ((cell_no-1)*2); }
    HAL_StatusTypeDef WriteBytes(uint8_t reg_addr, const uint8_t *data, size_t num_bytes) const;
    HAL_StatusTypeDef ReadBytes(uint8_t reg_addr, uint8_t *data, size_t num_bytes) const;
    HAL_StatusTypeDef DirectReadU2(uint8_t reg_addr, uint16_t *value) const; // TODO
    HAL_StatusTypeDef DirectReadI2(uint8_t reg_addr, int16_t *value) const; // TODO
    HAL_StatusTypeDef SubcmdRead(uint16_t subcmd, uint32_t *value, size_t num_bytes) const;
    HAL_StatusTypeDef SubcmdReadU1(uint16_t subcmd, uint8_t *value) const;
    HAL_StatusTypeDef SubcmdReadU2(uint16_t subcmd, uint16_t *value) const;
    HAL_StatusTypeDef SubcmdReadU4(uint16_t subcmd, uint32_t *value) const;
    HAL_StatusTypeDef SubcmdReadI1(uint16_t subcmd, int8_t *value) const;       // TODO: Remove
    HAL_StatusTypeDef SubcmdReadI2(uint16_t subcmd, int16_t *value) const;     // TODO: Remove
    HAL_StatusTypeDef SubcmdReadI4(uint16_t subcmd, int32_t *value) const;     // TODO: Remove
    HAL_StatusTypeDef SubcmdReadF4(uint16_t subcmd, float *value) const;
    HAL_StatusTypeDef SubcmdWrite(uint16_t subcmd, uint32_t value, size_t num_bytes) const;
    HAL_StatusTypeDef SubcmdCmdOnly(uint16_t subcmd) const;
    HAL_StatusTypeDef SubcmdCmdWriteU1(uint16_t subcmd, uint8_t value) const;
    HAL_StatusTypeDef SubcmdCmdWriteU2(uint16_t subcmd, uint16_t value) const;
    HAL_StatusTypeDef SubcmdCmdWriteU4(uint16_t subcmd, uint32_t value) const;
    HAL_StatusTypeDef SubcmdCmdWriteI1(uint16_t subcmd, int8_t value) const;
    HAL_StatusTypeDef SubcmdCmdWriteI2(uint16_t subcmd, int16_t value) const;
    HAL_StatusTypeDef SubcmdCmdWriteI4(uint16_t subcmd, int32_t value) const;
    HAL_StatusTypeDef SubcmdCmdWriteF4(uint16_t subcmd, float value) const;
    HAL_StatusTypeDef UpdateMode();

    I2C_HandleTypeDef *hi2c_{};

    int16_t cell_voltages_[16] = {0};         // Voltage of each cell in mV
    int16_t avg_cell_voltage_ = 0;          // Average cell voltage in mV
    int16_t high_cell_voltage_ = 0;         // Highest cell voltage in mV
    int16_t low_cell_voltage_ = 0;          // Lowest cell voltage in mV
    int16_t pack_current_ = 0;              // Brief battery pack current, mA

    int16_t pack_voltage_ = 0;              // Pack voltage in mV
    int16_t stack_voltage_ = 0;             // Stack voltage in mV

    float chip_temperature_ = 0; // temperature of chip in C

    enum BQMode{BQ_MODE_NORMAL, BQ_MODE_SLEEP, BQ_MODE_DEEPSLEEP, BQ_MODE_SHUTDOWN, BQ_MODE_CONFIGUPDATE};
    BQMode current_mode_ = BQ_MODE_NORMAL; // current mode BMS chip is in

    uint8_t stat_a_byte = 0x00;
    uint8_t stat_b_byte = 0x00;
    uint8_t stat_c_byte = 0x00;

    bool manual_bal_disabled_		= false;
    bool auto_bal_charging_enabled_	= false;
    bool auto_bal_relax_enabled_	= false;
    bool auto_bal_sleep_enabled_	= false;

};

#endif  //BLAZEFIRMWARE_BQ76952_H
