//
// Created by ahmed on 10/29/2026.
//
#include "Daughterboard.h"

#include <cstdint>

#include "cmsis_os.h"

#include "BQ76952.hpp"
#include "BQ769X2_Registers.h"
#include "main.h"

extern "C" I2C_HandleTypeDef hi2c2;

namespace bmsdaughter
{

namespace
{

BQ76952 bq;

VoltageSnapshot snapshot{};
bool snapshot_valid = false;

bool afe_ready = false;
uint8_t comm_fail_count = 0;
uint8_t ov_count = 0;
uint8_t uv_count = 0;
uint8_t latched_faults = VF_NONE;  // debounced OV/UV state

void publish(const VoltageSnapshot &s)
{
    taskENTER_CRITICAL();
    snapshot = s;
    snapshot_valid = true;
    taskEXIT_CRITICAL();
}

// keeps the last readings but updates the fault bits
void publishFaultsOnly(uint8_t faults)
{
    taskENTER_CRITICAL();
    snapshot.faults = faults;
    taskEXIT_CRITICAL();
}

// fault bit only flips after FaultDebounceSamples samples in a row disagree with it
void debounce(bool condition, uint8_t &count, uint8_t bit)
{
    const bool latched = (latched_faults & bit) != 0;
    if (condition != latched)
    {
        if (++count >= FaultDebounceSamples)
        {
            latched_faults ^= bit;
            count = 0;
        }
    }
    else
    {
        count = 0;
    }
}

}  // namespace

bool initAfe()
{
    afe_ready = false;

    // reset wipes the BQ's RAM config so vcell mode has to be set every init
    if (bq.Init(&hi2c2) != HAL_OK)
        return false;

    if (bq.SetVcellMode(VcellMode) != HAL_OK)
        return false;

    if (bq.GetNumCells() != NumCells)
        return false;

    afe_ready = true;
    comm_fail_count = 0;
    return true;
}

void updateVoltages()
{
    if (!afe_ready)
    {
        publishFaultsOnly(VF_AFE_INIT | VF_COMM);
        return;
    }

    if (bq.ReadVoltages() != HAL_OK)
    {
        if (comm_fail_count < 255)
            comm_fail_count++;

        if (comm_fail_count >= CommReinitLimit)
        {
            initAfe();
            comm_fail_count = CommFailLimit;  // stay faulted until a good read
        }

        if (comm_fail_count >= CommFailLimit)
            publishFaultsOnly(latched_faults | VF_COMM);
        return;  // don't publish a failed read
    }
    comm_fail_count = 0;

    VoltageSnapshot s{};
    for (uint8_t i = 0; i < NumCells; i++)
        s.cell_mv[i] = bq.GetCellVoltage(i);
    s.stack_mv = bq.GetStackVoltage();

    // invalid cells keep their raw value, get flagged, and are left out of min/max/avg
    s.min_mv = INT16_MAX;
    s.max_mv = INT16_MIN;
    uint8_t n_valid = 0;
    bool any_ov = false;
    bool any_uv = false;

    for (uint8_t i = 0; i < NumCells; i++)
    {
        const int16_t v = s.cell_mv[i];
        if (v < CellPlausibleMinMv || v > CellPlausibleMaxMv)
            continue;

        s.valid_mask |= static_cast<uint16_t>(1u << i);
        n_valid++;
        s.sum_mv += v;

        if (v > s.max_mv)
        {
            s.max_mv = v;
            s.max_idx = i;
        }
        if (v < s.min_mv)
        {
            s.min_mv = v;
            s.min_idx = i;
        }

        if (v > CellOvMv)
            any_ov = true;
        if (v < CellUvMv)
            any_uv = true;
    }

    uint8_t faults = VF_NONE;

    if (n_valid < NumCells)
        faults |= VF_CELL_INVALID;

    if (n_valid > 0)
    {
        s.avg_mv = static_cast<int16_t>(s.sum_mv / n_valid);
    }
    else
    {
        s.min_mv = 0;
        s.max_mv = 0;
        s.avg_mv = 0;
    }

    // only compare stack vs cell sum when every cell is valid
    if (n_valid == NumCells)
    {
        int32_t diff = s.stack_mv - s.sum_mv;
        if (diff < 0)
            diff = -diff;
        if (diff > StackMismatchMv)
            faults |= VF_STACK_MISMATCH;
    }

    debounce(any_ov, ov_count, VF_CELL_OV);
    debounce(any_uv, uv_count, VF_CELL_UV);

    s.faults = faults | latched_faults;
    s.timestamp_ms = osKernelGetTickCount() * 1000u / osKernelGetTickFreq();

    publish(s);
}

bool getVoltageSnapshot(VoltageSnapshot &out)
{
    taskENTER_CRITICAL();
    out = snapshot;
    const bool valid = snapshot_valid;
    taskEXIT_CRITICAL();
    return valid;
}

void getCellTemp() {}

void sendBalancingCommand() {}

void sendStatusMessage() {}

}  // namespace bmsdaughter
