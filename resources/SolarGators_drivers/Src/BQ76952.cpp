//
// Created by ahmed on 10/28/2026.
//

#include "BQ76952.hpp"

#if BQ76952_AVAILABLE

#include <cstring>

BQ76952::BQ76952() = default;

HAL_StatusTypeDef BQ76952::Init(I2C_HandleTypeDef *hi2c, const uint8_t i2cAddress)
{
    hi2c_ = hi2c;
    i2cAddressWrite = i2cAddress;
    i2cAddressRead = i2cAddress + 1;

    HAL_StatusTypeDef status = Reset();
    if (status != HAL_OK)
    {
        return status;
    }

    // RESET reboots the device and clears all RAM config back to OTP defaults
    vcell_mode_ = 0xFFFF;
    num_cells_ = MaxCells;
    current_mode_ = BQ_MODE_NORMAL;

    HAL_Delay(100);

    return HAL_I2C_IsDeviceReady(hi2c_, i2cAddressWrite, 100, 50);
}

HAL_StatusTypeDef BQ76952::Init(I2C_HandleTypeDef *hi2c)
{
    return Init(hi2c, BQ_I2C_ADDR_WRITE);
}

HAL_StatusTypeDef BQ76952::Reset() const
{
    return SubcmdCmdOnly(BQ769X2_SUBCMD_RESET);
}

HAL_StatusTypeDef BQ76952::ConfigUpdate(const bool config_update)
{
    HAL_StatusTypeDef status =
        SubcmdCmdOnly(config_update ? BQ769X2_SUBCMD_SET_CFGUPDATE : BQ769X2_SUBCMD_EXIT_CFGUPDATE);
    if (status != HAL_OK)
        return status;

    // Wait for Battery Status bit 0 (CFGUPDATE) to reflect the new mode
    for (int i = 0; i < 20; i++)
    {
        uint16_t batt_status = 0;
        if (DirectReadU2(BQ769X2_CMD_BATTERY_STATUS, &batt_status) == HAL_OK)
        {
            const bool in_cfg = (batt_status & 0x0001u) != 0;
            if (in_cfg == config_update)
            {
                current_mode_ = config_update ? BQ_MODE_CONFIGUPDATE : BQ_MODE_NORMAL;
                return HAL_OK;
            }
        }
        HAL_Delay(1);
    }

    return HAL_TIMEOUT;
}

HAL_StatusTypeDef BQ76952::SetVcellMode(const uint16_t mask)
{
    if (mask == 0)
        return HAL_ERROR;  // 0x0000 means "all 16" to the chip; be explicit instead

    HAL_StatusTypeDef status = ConfigUpdate(true);
    if (status != HAL_OK)
        return status;

    const HAL_StatusTypeDef wr = SubcmdCmdWriteU2(BQ769X2_SET_CONF_VCELL_MODE, mask);
    const HAL_StatusTypeDef ex = ConfigUpdate(false);  // always try to leave CONFIG_UPDATE
    if (wr != HAL_OK)
        return wr;
    if (ex != HAL_OK)
        return ex;

    uint16_t readback = 0;
    status = SubcmdReadU2(BQ769X2_SET_CONF_VCELL_MODE, &readback);
    if (status != HAL_OK)
        return status;
    if (readback != mask)
        return HAL_ERROR;

    vcell_mode_ = mask;
    num_cells_ = 0;
    for (uint8_t i = 0; i < MaxCells; i++)
    {
        if (mask & (1u << i))
            num_cells_++;
    }

    return HAL_OK;
}

HAL_StatusTypeDef BQ76952::ReadVoltages()
{
    HAL_StatusTypeDef status = HAL_OK;

    int32_t voltage_sum = 0;
    int16_t low = INT16_MAX;
    int16_t high = INT16_MIN;
    uint8_t n = 0;

    for (uint8_t input = 1; input <= MaxCells; input++)
    {
        if (!(vcell_mode_ & (1u << (input - 1))))
            continue;  // unused input

        int16_t v = 0;
        status = DirectReadI2(CELL_NO_TO_ADDR(input), &v);
        if (status != HAL_OK)
            status = DirectReadI2(CELL_NO_TO_ADDR(input), &v);  // one retry for bus glitches
        if (status != HAL_OK)
            return status;

        cell_voltages_[n] = v;  // raw value, never substituted
        voltage_sum += v;
        if (v > high)  // independent ifs: one cell can be both high and low
            high = v;
        if (v < low)
            low = v;
        n++;
    }

    if (n == 0)
        return HAL_ERROR;

    high_cell_voltage_ = high;
    low_cell_voltage_ = low;
    avg_cell_voltage_ = static_cast<int16_t>(voltage_sum / n);

    // Stack/pack are in userV; default DA Configuration gives 1 userV = 10 mV
    int16_t pack_voltage_temp;
    status = DirectReadI2(BQ769X2_CMD_VOLTAGE_PACK, &pack_voltage_temp);
    if (status != HAL_OK)
        return status;
    pack_voltage_ = static_cast<int32_t>(pack_voltage_temp) * 10;

    int16_t stack_voltage_temp;
    status = DirectReadI2(BQ769X2_CMD_VOLTAGE_STACK, &stack_voltage_temp);
    if (status != HAL_OK)
        return status;
    stack_voltage_ = static_cast<int32_t>(stack_voltage_temp) * 10;

    return HAL_OK;
}

HAL_StatusTypeDef BQ76952::ReadSafetyFaults()
{
    // STATUS = protections that have tripped. (ALERT = pre-trip warnings, 0x02/0x04/0x06.)
    HAL_StatusTypeDef status = ReadBytes(BQ769X2_CMD_SAFETY_STATUS_A, &stat_a_byte, 1);
    if (status != HAL_OK)
        return status;
    status = ReadBytes(BQ769X2_CMD_SAFETY_STATUS_B, &stat_b_byte, 1);
    if (status != HAL_OK)
        return status;
    status = ReadBytes(BQ769X2_CMD_SAFETY_STATUS_C, &stat_c_byte, 1);
    if (status != HAL_OK)
        return status;

    return HAL_OK;
}

HAL_StatusTypeDef BQ76952::ReadCurrent()
{
    // CC2 current in userA (default mA). int16 mA limits this to +/-32.7 A.
    return DirectReadI2(BQ769X2_CMD_CURRENT_CC2, &pack_current_);
}

HAL_StatusTypeDef BQ76952::Shutdown() const
{
    return SubcmdCmdOnly(BQ769X2_SUBCMD_SHUTDOWN);
}

HAL_StatusTypeDef BQ76952::EnterDeepSleep()
{
    // DEEPSLEEP must be sent twice in a row to take effect (TRM)
    HAL_StatusTypeDef status = SubcmdCmdOnly(BQ769X2_SUBCMD_DEEPSLEEP);
    if (status != HAL_OK)
        return status;
    status = SubcmdCmdOnly(BQ769X2_SUBCMD_DEEPSLEEP);
    if (status != HAL_OK)
        return status;

    current_mode_ = BQ_MODE_DEEPSLEEP;

    return status;
}

HAL_StatusTypeDef BQ76952::ExitDeepSleep()
{
    const HAL_StatusTypeDef status = SubcmdCmdOnly(BQ769X2_SUBCMD_EXIT_DEEPSLEEP);
    if (status != HAL_OK)
        return status;
    current_mode_ = BQ_MODE_NORMAL;

    return status;
}

int16_t BQ76952::GetCellVoltage(const uint32_t cell_idx) const
{
    if (cell_idx >= num_cells_)
    {
        return INT16_MIN;
    }

    return cell_voltages_[cell_idx];
}

int32_t BQ76952::GetPackVoltage() const
{
    return pack_voltage_;
}

int32_t BQ76952::GetStackVoltage() const
{
    return stack_voltage_;
}

int16_t BQ76952::GetAvgCellVoltage() const
{
    return avg_cell_voltage_;
}

int16_t BQ76952::GetHighCellVoltage() const
{
    return high_cell_voltage_;
}

int16_t BQ76952::GetLowCellVoltage() const
{
    return low_cell_voltage_;
}

uint8_t BQ76952::GetNumCells() const
{
    return num_cells_;
}

bool BQ76952::GetConfigUpdateStatus() const
{
    return current_mode_ == BQ_MODE_CONFIGUPDATE;
}

HAL_StatusTypeDef BQ76952::WriteBytes(const uint8_t reg_addr,
                                      const uint8_t *data,
                                      const size_t num_bytes) const
{
    uint8_t buf[5];

    if (num_bytes > 4)
    {
        return HAL_ERROR;
    }

    buf[0] = reg_addr;
    memcpy(buf + 1, data, num_bytes);

    return HAL_I2C_Master_Transmit(hi2c_, i2cAddressWrite, buf, num_bytes + 1, I2cTimeoutMs);
}

HAL_StatusTypeDef BQ76952::ReadBytes(const uint8_t reg_addr,
                                     uint8_t *data,
                                     const size_t num_bytes) const
{
    // writes the register address then reads with a repeated start
    return HAL_I2C_Mem_Read(hi2c_,
                            i2cAddressWrite,
                            reg_addr,
                            I2C_MEMADD_SIZE_8BIT,
                            data,
                            static_cast<uint16_t>(num_bytes),
                            I2cTimeoutMs);
}

HAL_StatusTypeDef BQ76952::DirectReadU2(const uint8_t reg_addr, uint16_t *value) const
{
    uint8_t buf[2];

    HAL_StatusTypeDef status = ReadBytes(reg_addr, buf, 2);

    if (status != HAL_OK)
        return status;

    *value = static_cast<uint16_t>((buf[1] << 8) | buf[0]);

    return HAL_OK;
}

HAL_StatusTypeDef BQ76952::DirectReadI2(const uint8_t reg_addr, int16_t *value) const
{
    uint8_t buf[2];

    HAL_StatusTypeDef status = ReadBytes(reg_addr, buf, 2);

    if (status != HAL_OK)
        return status;

    *value = static_cast<int16_t>((buf[1] << 8) | buf[0]);

    return HAL_OK;
}

HAL_StatusTypeDef BQ76952::SubcmdRead(const uint16_t subcmd,
                                      uint32_t *value,
                                      const size_t num_bytes) const
{
    static uint8_t buf_data[0x20];  // not reentrant: only one task may talk to the BQ

    if (num_bytes > 4)
    {
        return HAL_ERROR;
    }

    uint8_t buf_subcmd[2] = {static_cast<uint8_t>(subcmd),
                             static_cast<uint8_t>(subcmd >> 8)};  // put subcmd into a buffer

    HAL_StatusTypeDef status = WriteBytes(BQ769X2_CMD_SUBCMD_LOWER, buf_subcmd, 2);
    if (status != HAL_OK)
        return status;

    HAL_Delay(1);

    // Wait until 0x3E/0x3F echo the subcommand (data is ready)
    int num_tries = 0;
    while (true)
    {
        status = ReadBytes(BQ769X2_CMD_SUBCMD_LOWER, buf_data, 2);
        if (status != HAL_OK)
            return status;

        if (buf_subcmd[0] == buf_data[0] && buf_subcmd[1] == buf_data[1])
            break;

        if (++num_tries > 10)
            return HAL_TIMEOUT;

        HAL_Delay(1);
    }

    uint8_t data_length;

    status = ReadBytes(BQ769X2_SUBCMD_DATA_LENGTH, &data_length, 1);
    if (status != HAL_OK)
        return status;

    if (data_length < 4)
        return HAL_ERROR;  // would underflow below

    data_length -= 4;  // subtract subcmd + checksum + length bytes

    if (data_length > sizeof(buf_data) || data_length < num_bytes)
    {
        return HAL_ERROR;
    }

    *value = 0;
    if (data_length > 0)
    {
        status = ReadBytes(BQ769X2_SUBCMD_DATA_START, buf_data, data_length);
        if (status != HAL_OK)
            return status;
    }

    for (uint8_t i = 0; i < num_bytes; i++)
    {
        *value |= static_cast<uint32_t>(buf_data[i]) << (i * 8);
    }

    return HAL_OK;
}

HAL_StatusTypeDef BQ76952::SubcmdReadU1(const uint16_t subcmd, uint8_t *value) const
{
    uint32_t temp;

    HAL_StatusTypeDef status = SubcmdRead(subcmd, &temp, 1);

    if (status != HAL_OK)
        return status;

    *value = static_cast<uint8_t>(temp);

    return HAL_OK;
}

HAL_StatusTypeDef BQ76952::SubcmdReadU2(const uint16_t subcmd, uint16_t *value) const
{
    uint32_t temp;

    HAL_StatusTypeDef status = SubcmdRead(subcmd, &temp, 2);

    if (status != HAL_OK)
        return status;

    *value = static_cast<uint16_t>(temp);

    return HAL_OK;
}

HAL_StatusTypeDef BQ76952::SubcmdReadU4(const uint16_t subcmd, uint32_t *value) const
{
    uint32_t temp;

    HAL_StatusTypeDef status = SubcmdRead(subcmd, &temp, 4);

    if (status != HAL_OK)
        return status;

    *value = temp;

    return HAL_OK;
}

HAL_StatusTypeDef BQ76952::SubcmdReadI1(const uint16_t subcmd, int8_t *value) const
{
    uint32_t temp;

    HAL_StatusTypeDef status = SubcmdRead(subcmd, &temp, 1);

    if (status != HAL_OK)
        return status;

    *value = static_cast<int8_t>(temp);

    return HAL_OK;
}

HAL_StatusTypeDef BQ76952::SubcmdReadI2(const uint16_t subcmd, int16_t *value) const
{
    uint32_t temp;

    HAL_StatusTypeDef status = SubcmdRead(subcmd, &temp, 2);

    if (status != HAL_OK)
        return status;

    *value = static_cast<int16_t>(temp);

    return HAL_OK;
}

HAL_StatusTypeDef BQ76952::SubcmdReadI4(const uint16_t subcmd, int32_t *value) const
{
    uint32_t temp;

    HAL_StatusTypeDef status = SubcmdRead(subcmd, &temp, 4);

    if (status != HAL_OK)
        return status;

    *value = static_cast<int32_t>(temp);

    return HAL_OK;
}

HAL_StatusTypeDef BQ76952::SubcmdReadF4(const uint16_t subcmd, float *value) const
{
    uint32_t raw;
    HAL_StatusTypeDef status = SubcmdRead(subcmd, &raw, 4);

    if (status != HAL_OK)
        return status;

    memcpy(value, &raw, sizeof(float));
    return HAL_OK;
}

HAL_StatusTypeDef BQ76952::SubcmdWrite(const uint16_t subcmd,
                                       const uint32_t value,
                                       const size_t num_bytes) const
{
    if (num_bytes > 4)
    {
        return HAL_ERROR;
    }

    uint8_t buf_data[4];
    uint8_t buf_subcmd[2] = {static_cast<uint8_t>(subcmd & 0x00FF),
                             static_cast<uint8_t>(subcmd >> 8)};
    uint8_t buf_len_and_check[2];

    HAL_StatusTypeDef status = WriteBytes(BQ769X2_CMD_SUBCMD_LOWER, buf_subcmd, 2);
    if (status != HAL_OK)
        return status;

    // Command-only subcommands are complete once 0x3E/0x3F are written
    if (num_bytes == 0)
        return HAL_OK;

    auto checksum = static_cast<uint8_t>(buf_subcmd[0] + buf_subcmd[1]);
    for (size_t i = 0; i < num_bytes; i++)
    {
        buf_data[i] = static_cast<uint8_t>((value >> (i * 8)) & 0xFFu);
        checksum = static_cast<uint8_t>(checksum + buf_data[i]);
    }
    status = WriteBytes(BQ769X2_SUBCMD_DATA_START, buf_data, num_bytes);
    if (status != HAL_OK)
        return status;

    checksum = static_cast<uint8_t>(~checksum);

    buf_len_and_check[0] = checksum;
    buf_len_and_check[1] = static_cast<uint8_t>(num_bytes + 4);

    return WriteBytes(BQ769X2_SUBCMD_DATA_CHECKSUM, buf_len_and_check, 2);
}

HAL_StatusTypeDef BQ76952::SubcmdCmdOnly(const uint16_t subcmd) const
{
    return SubcmdWrite(subcmd, 0, 0);
}

HAL_StatusTypeDef BQ76952::SubcmdCmdWriteU1(const uint16_t subcmd, uint8_t value) const
{
    return SubcmdWrite(subcmd, value, 1);
}

HAL_StatusTypeDef BQ76952::SubcmdCmdWriteU2(const uint16_t subcmd, uint16_t value) const
{
    return SubcmdWrite(subcmd, value, 2);
}

HAL_StatusTypeDef BQ76952::SubcmdCmdWriteU4(const uint16_t subcmd, uint32_t value) const
{
    return SubcmdWrite(subcmd, value, 4);
}

HAL_StatusTypeDef BQ76952::SubcmdCmdWriteI1(const uint16_t subcmd, int8_t value) const
{
    return SubcmdWrite(subcmd, static_cast<uint8_t>(value), 1);
}

HAL_StatusTypeDef BQ76952::SubcmdCmdWriteI2(const uint16_t subcmd, int16_t value) const
{
    return SubcmdWrite(subcmd, static_cast<uint16_t>(value), 2);
}

HAL_StatusTypeDef BQ76952::SubcmdCmdWriteI4(const uint16_t subcmd, int32_t value) const
{
    return SubcmdWrite(subcmd, static_cast<uint32_t>(value), 4);
}

HAL_StatusTypeDef BQ76952::SubcmdCmdWriteF4(const uint16_t subcmd, float value) const
{
    uint32_t raw;
    memcpy(&raw, &value, sizeof(float));
    return SubcmdWrite(subcmd, raw, 4);
}

HAL_StatusTypeDef BQ76952::UpdateMode()
{
    uint16_t buf;
    HAL_StatusTypeDef status = DirectReadU2(BQ769X2_CMD_BATTERY_STATUS, &buf);
    if (status != HAL_OK)
        return status;

    if (buf & (1u << 15))  // Battery Status: SLEEP
    {
        current_mode_ = BQ_MODE_SLEEP;
        return status;
    }
    if (buf & (1u << 0))  // Battery Status: CFGUPDATE
    {
        current_mode_ = BQ_MODE_CONFIGUPDATE;
        return status;
    }

    status = DirectReadU2(BQ769X2_CMD_CONTROL_STATUS, &buf);
    if (status != HAL_OK)
        return status;

    if (buf & (1u << 2))  // Control Status: DEEPSLEEP
    {
        current_mode_ = BQ_MODE_DEEPSLEEP;
        return status;
    }

    current_mode_ = BQ_MODE_NORMAL;
    return status;
}

#endif