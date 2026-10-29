//
// Created by ahmed on 10/28/2026.
//

#include "BQ76952.hpp"
#include <cstring>

#ifndef BQ76952_
#define BQ76952_

BQ76952::BQ76952()= default;

HAL_StatusTypeDef BQ76952::Init(I2C_HandleTypeDef *hi2c, const uint8_t i2cAddress){
    hi2c_ = hi2c;
    i2cAddressWrite = i2cAddress;
    i2cAddressRead = i2cAddress+1;

    Reset();
    volatile uint32_t delay = 100000;
    while(delay--);

    // DatamemWriteU1(BQ769X2_SET_PROT_ENABLED_A, 12);

    return HAL_I2C_IsDeviceReady(hi2c_, i2cAddressWrite, 100, 50);
}

HAL_StatusTypeDef BQ76952::Init(I2C_HandleTypeDef *hi2c){
	return Init(hi2c, BQ_I2C_ADDR_WRITE);
}

HAL_StatusTypeDef BQ76952::Reset() {
    return SubcmdCmdOnly(BQ769X2_SUBCMD_RESET);
}

HAL_StatusTypeDef BQ76952::ConfigUpdate(const bool config_update){
    HAL_StatusTypeDef status;

    if (config_update) {
        status = SubcmdCmdOnly(BQ769X2_SUBCMD_SET_CFGUPDATE);
    }
    else {
        status = SubcmdCmdOnly(BQ769X2_SUBCMD_EXIT_CFGUPDATE);
    }

    if(status != HAL_OK)
        return status;

    if (config_update) {
        current_mode_ = BQ_MODE_CONFIGUPDATE;
    }
    else {
        current_mode_ = BQ_MODE_NORMAL;
    }

    return HAL_OK;
}

HAL_StatusTypeDef BQ76952::ConfigureVoltageRegs() {
    return HAL_OK;
}

HAL_StatusTypeDef BQ76952::ReadVoltages() {
    HAL_StatusTypeDef status = HAL_OK;

    int32_t voltage_sum = 0;
    low_cell_voltage_ = INT16_MAX;
    high_cell_voltage_ = INT16_MIN;

    for (int i = 0; i < 12; i++) {

//    	if((i == 0) || (i == 1) || (i == 11)){
    		for (int retry = 0; retry < 5; retry++) {
				status = DirectReadI2(CELL_NO_TO_ADDR(i+1), &cell_voltages_[i]);
				if (status == HAL_OK && cell_voltages_[i] < 4200 && cell_voltages_[i] > 1000) break;  // 1V to 5V is reasonable
    		}
    		if(cell_voltages_[i] < 0 || cell_voltages_[i] > 10000){
    			cell_voltages_[i] = 3890;
    		}
//    	}else{
    		//status = DirectReadI2(CELL_NO_TO_ADDR(i+1), &cell_voltages_[i]);
//    	}
        if (status != HAL_OK) { return status; }

        if (cell_voltages_[i] > high_cell_voltage_) {
            high_cell_voltage_ = cell_voltages_[i];
        }
        else if (cell_voltages_[i] < low_cell_voltage_) {
            low_cell_voltage_ = cell_voltages_[i];
        }

        voltage_sum += cell_voltages_[i];
    }

    avg_cell_voltage_ = voltage_sum / 12;

    int16_t pack_voltage_temp; // this number will be in userV, by default 1 userV = 10 mV
    status = DirectReadI2(BQ769X2_CMD_VOLTAGE_PACK, &pack_voltage_temp);
    if (status != HAL_OK) { return status; }
    pack_voltage_ = pack_voltage_temp*10; // pack_voltage_ is in mV


    int16_t stack_voltage_temp; // this number will be in userV, by default 1 userV = 10 mV
    status = DirectReadI2(BQ769X2_CMD_VOLTAGE_STACK, &stack_voltage_temp);
    if (status != HAL_OK) { return status; }
    stack_voltage_ = stack_voltage_temp*10; //stack_voltage_ is in mV

    return HAL_OK;
}

HAL_StatusTypeDef BQ76952::ReadCurrent(){
    HAL_StatusTypeDef status;

    int16_t current = 0;// this value is in userA, where 1 userA = 1 mA by default
    status = DirectReadI2(BQ769X2_CMD_CURRENT_CC2, &current);

    if(status != HAL_OK)
        return status;

    pack_current_ = current; // in mA

    return status;
}

HAL_StatusTypeDef BQ76952::ReadSafetyFaults() {
    HAL_StatusTypeDef status = ReadBytes(BQ769X2_CMD_SAFETY_ALERT_A, &stat_a_byte, 1);
    if (status != HAL_OK)
        return status;
    status = ReadBytes(BQ769X2_CMD_SAFETY_ALERT_B, &stat_b_byte, 1);
    if (status != HAL_OK)
        return status;
    status = ReadBytes(BQ769X2_CMD_SAFETY_ALERT_C, &stat_c_byte, 1);
    if (status != HAL_OK)
        return status;

    return HAL_OK;
}

HAL_StatusTypeDef BQ76952::Shutdown() const
{
    return SubcmdCmdOnly(BQ769X2_SUBCMD_SHUTDOWN);
}

HAL_StatusTypeDef BQ76952::EnterDeepSleep(){
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

int16_t BQ76952::GetCellVoltage(uint32_t cell_num) const
{
    if (cell_num > 16) {
        return INT16_MIN;
    }

    return cell_voltages_[cell_num];
}

int16_t BQ76952::GetPackVoltage() const {
    return pack_voltage_;
}

int16_t BQ76952::GetStackVoltage() const {
    return stack_voltage_;
}

int16_t BQ76952::GetAvgCellVoltage() const {
    return avg_cell_voltage_;
}

int16_t BQ76952::GetHighCellVoltage() const {
    return high_cell_voltage_;
}

int16_t BQ76952::GetLowCellVoltage() const {
    return low_cell_voltage_;
}

bool BQ76952::GetConfigUpdateStatus() const{
    return current_mode_ == BQ_MODE_CONFIGUPDATE;
}

HAL_StatusTypeDef BQ76952::WriteBytes(const uint8_t reg_addr, const uint8_t *data, const size_t num_bytes) const {
    uint8_t buf[5];

    if (num_bytes > 4){
        return HAL_ERROR;
    }

    buf[0] = reg_addr;
    memcpy(buf + 1, data, num_bytes);

    return HAL_I2C_Master_Transmit(hi2c_, i2cAddressWrite, buf, num_bytes + 1, 1000);
}

HAL_StatusTypeDef BQ76952::ReadBytes(uint8_t reg_addr, uint8_t *data, const size_t num_bytes) const {
    HAL_I2C_Master_Transmit(hi2c_, i2cAddressRead, &reg_addr, 1, 1000);

    return HAL_I2C_Master_Receive(hi2c_, i2cAddressRead, data, num_bytes, 1000);
}

HAL_StatusTypeDef BQ76952::DirectReadU2(const uint8_t reg_addr, uint16_t *value) const
{
    uint8_t buf[2];

    HAL_StatusTypeDef status = ReadBytes(reg_addr, buf, 2);

    if (status != HAL_OK)
        return status;

    *value = (buf[1] << 8) | buf[0];

    return HAL_OK;
}

HAL_StatusTypeDef BQ76952::DirectReadI2(const uint8_t reg_addr, int16_t *value) const {
    uint8_t buf[2];

    HAL_StatusTypeDef status = ReadBytes(reg_addr, buf, 2);

    if (status != HAL_OK)
        return status;

    *value = (int16_t)((buf[1] << 8) | buf[0]);

    return HAL_OK;
}

HAL_StatusTypeDef BQ76952::SubcmdRead(const uint16_t subcmd, uint32_t *value, const size_t num_bytes) const {
    static uint8_t buf_data[0x20];

    uint8_t buf_subcmd[2] = { static_cast<uint8_t>(subcmd), static_cast<uint8_t>(subcmd >> 8)}; // put subcmd into a buffer

    HAL_StatusTypeDef status = WriteBytes(BQ769X2_CMD_SUBCMD_LOWER, buf_subcmd, 2);
    if (status != HAL_OK)
        return status;

    HAL_Delay(1);

    int num_tries = 0;
    while(1){
        status = ReadBytes(BQ769X2_CMD_SUBCMD_LOWER, buf_data, 2);

        if (status != HAL_OK){
            return status;
        } else if(num_tries > 10){
            return HAL_ERROR;
        }
        else {
            if(buf_subcmd[0] != buf_data[0] || buf_subcmd[1] != buf_data[1]){
                HAL_Delay(1);
                num_tries++;
            }else{
                break;
            }
        }
    }

    uint8_t data_length;

    status = ReadBytes(BQ769X2_SUBCMD_DATA_LENGTH, &data_length, 1);
    if (status != HAL_OK)
        return status;

    data_length -= 4; // subtract subcmd + checksum + length bytes

    if(data_length > 0x20 || num_bytes > 4){
        return HAL_ERROR; // error
    }

    *value = 0;
    status = ReadBytes(BQ769X2_SUBCMD_DATA_START, buf_data, data_length);
    if (status != HAL_OK)
        return status;

    for(uint8_t i = 0; i < num_bytes; i++){
        *value += buf_data[i] << (i * 8);
    }

    //    just for testing:
//    uint8_t checksum;
//    status = ReadBytes(BQ769X2_SUBCMD_DATA_CHECKSUM, &checksum, 1);

    return status;
}

HAL_StatusTypeDef BQ76952::SubcmdReadU1(const uint16_t subcmd, uint8_t *value) const {
    uint32_t temp;

    HAL_StatusTypeDef status = SubcmdRead(subcmd, &temp, 1);

    if (status != HAL_OK)
        return status;

    *value = (uint8_t)temp;

    return HAL_OK;
}

HAL_StatusTypeDef BQ76952::SubcmdReadU2(const uint16_t subcmd, uint16_t *value) const {
    uint32_t temp;

    HAL_StatusTypeDef status = SubcmdRead(subcmd, &temp, 2);

    if (status != HAL_OK)
        return status;

    *value = (uint16_t)temp;

    return HAL_OK;
}

HAL_StatusTypeDef BQ76952::SubcmdReadU4(const uint16_t subcmd, uint32_t *value) const {
    uint32_t temp;

    HAL_StatusTypeDef status = SubcmdRead(subcmd, &temp, 4);

    if (status != HAL_OK)
        return status;

    *value = temp;

    return HAL_OK;
}

HAL_StatusTypeDef BQ76952::SubcmdReadI1(const uint16_t subcmd, int8_t *value) const {
    uint32_t temp;

    HAL_StatusTypeDef status = SubcmdRead(subcmd, &temp, 1);

    if (status != HAL_OK)
        return status;

    *value = (int8_t)temp;

    return HAL_OK;
}

HAL_StatusTypeDef BQ76952::SubcmdReadI2(const uint16_t subcmd, int16_t *value) const {
    uint32_t temp;

    HAL_StatusTypeDef status = SubcmdRead(subcmd, &temp, 2);

    if (status != HAL_OK)
        return status;

    *value = (int16_t)temp;

    return HAL_OK;
}

HAL_StatusTypeDef BQ76952::SubcmdReadI4(const uint16_t subcmd, int32_t *value) const {
    uint32_t temp;

    HAL_StatusTypeDef status = SubcmdRead(subcmd, &temp, 4);

    if (status != HAL_OK)
        return status;

    *value = (int32_t)temp;

    return HAL_OK;
}

HAL_StatusTypeDef BQ76952::SubcmdReadF4(const uint16_t subcmd, float *value) const{
    float f32;
    HAL_StatusTypeDef status = SubcmdRead(subcmd, (uint32_t *)&f32, 4);

    if (status != HAL_OK)
        return status;

    *value = f32;
    return HAL_OK;
}

HAL_StatusTypeDef BQ76952::SubcmdWrite(const uint16_t subcmd, const uint32_t value, const size_t num_bytes) const {
    uint8_t buf_data[4];
    uint8_t buf_subcmd[2] = { static_cast<uint8_t>(subcmd & 0x00FF), static_cast<uint8_t>(subcmd >> 8) };
    uint8_t buf_len_and_check[2];

    HAL_StatusTypeDef status = WriteBytes(BQ769X2_CMD_SUBCMD_LOWER, buf_subcmd, 2);
    if(status != HAL_OK)
        return status;

    if (num_bytes > 4){
        return HAL_ERROR;
    }

    auto checksum = static_cast<uint8_t>(buf_subcmd[0] + buf_subcmd[1]);
    if (num_bytes > 0){
        for(int i = 0; i < static_cast<int>(num_bytes); i++){
            buf_data[i] = (value >> (i * 8) & 0x000000FF);
            checksum = static_cast<uint8_t>(checksum + buf_data[i]);
        }
        status = WriteBytes(BQ769X2_SUBCMD_DATA_START, buf_data, num_bytes);
    }
    checksum = ~checksum;

    if (status != HAL_OK)
    	return status;

    buf_len_and_check[1] = num_bytes + 4;
    buf_len_and_check[0] = checksum;

    status = WriteBytes(BQ769X2_SUBCMD_DATA_CHECKSUM, buf_len_and_check, 2);

    return status;
}

HAL_StatusTypeDef BQ76952::SubcmdCmdOnly(const uint16_t subcmd) const {
    return SubcmdWrite(subcmd, 0, 0);
}

HAL_StatusTypeDef BQ76952::SubcmdCmdWriteU1(const uint16_t subcmd, uint8_t value) const {
    return SubcmdWrite(subcmd, value, 1);
}

HAL_StatusTypeDef BQ76952::SubcmdCmdWriteU2(const uint16_t subcmd, uint16_t value) const {
    return SubcmdWrite(subcmd, value, 2);
}

HAL_StatusTypeDef BQ76952::SubcmdCmdWriteU4(const uint16_t subcmd, uint32_t value) const {
    return SubcmdWrite(subcmd, value, 4);
}

HAL_StatusTypeDef BQ76952::SubcmdCmdWriteI1(const uint16_t subcmd, int8_t value) const {
    return SubcmdWrite(subcmd, value, 1);
}

HAL_StatusTypeDef BQ76952::SubcmdCmdWriteI2(const uint16_t subcmd, int16_t value) const {
    return SubcmdWrite(subcmd, value, 2);
}

HAL_StatusTypeDef BQ76952::SubcmdCmdWriteI4(const uint16_t subcmd, int32_t value) const
{
    return SubcmdWrite(subcmd, value, 4);
}

HAL_StatusTypeDef BQ76952::SubcmdCmdWriteF4(const uint16_t subcmd, float value) const {
    const auto *u32 = reinterpret_cast<uint32_t *>(&value);
    return SubcmdWrite(subcmd, *u32, 4);
}

HAL_StatusTypeDef BQ76952::UpdateMode(){
    uint16_t buf;
    HAL_StatusTypeDef status = DirectReadU2(BQ769X2_CMD_BATTERY_STATUS, &buf);
    if(status != HAL_OK)
        return HAL_ERROR;

    if(buf & (1 << 15)){
        current_mode_ = BQ_MODE_SLEEP;
        return status;
    }else if(buf & (1 << 0)){
        current_mode_ = BQ_MODE_CONFIGUPDATE;
        return status;
    }

    status = DirectReadU2(BQ769X2_CMD_CONTROL_STATUS, &buf);
    if(status != HAL_OK)
        return HAL_ERROR;

    if(buf & (1 << 2)){
        current_mode_ = BQ_MODE_DEEPSLEEP;
        return status;
    }

    current_mode_ = BQ_MODE_NORMAL;
    return status;
}

#endif