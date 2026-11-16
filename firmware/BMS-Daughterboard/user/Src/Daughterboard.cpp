//
// Created by ahmed on 10/29/2026.
//
#include "Daughterboard.h"

#include <cstdint>
#include <cstring>

#include "cmsis_os.h"

#include "BQ76952.hpp"
#include "BQ769X2_Registers.h"
#include "main.h"

extern "C" I2C_HandleTypeDef hi2c2;

namespace bmsdaughter
{

namespace
{

BQ76952 bq;  // static storage, not on the 512-byte task stack

VoltageSnapshot g_snapshot{};  // published copy, guarded by a critical section
bool g_snapshot_valid = false;

bool g_afe_ready = false;
uint8_t g_comm_fail_count = 0;
uint8_t g_ov_count = 0;
uint8_t g_uv_count = 0;
uint8_t g_latched_faults = VF_NONE;  // debounced OV/UV state carried between cycles

void publish(const VoltageSnapshot &s)
{
    taskENTER_CRITICAL();
    g_snapshot = s;
    g_snapshot_valid = true;
    taskEXIT_CRITICAL();
}

void publishFaultsOnly(uint8_t faults)
{
    // Keep the last readings but mark them stale/faulted
    taskENTER_CRITICAL();
    g_snapshot.faults = faults;
    taskEXIT_CRITICAL();
}

// Set/clear a latched fault bit only after kFaultDebounceSamples consecutive
// samples disagree with its current state.
void debounce(bool condition, uint8_t &count, uint8_t bit)
{
    const bool latched = (g_latched_faults & bit) != 0;
    if (condition != latched)
    {
        if (++count >= FaultDebounceSamples)
        {
            g_latched_faults ^= bit;
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
    g_afe_ready = false;

    // Init() sends RESET, waits, then checks the device ACKs.
    // RESET wipes RAM config, so SetVcellMode must be re-applied every init.
    if (bq.Init(&hi2c2) != HAL_OK)
        return false;

    // Writes Vcell Mode, reads it back, and limits ReadVoltages() to those inputs
    if (bq.SetVcellMode(VcellMode) != HAL_OK)
        return false;

    if (bq.GetNumCells() != NumCells)
        return false;  // kVcellMode doesn't have exactly kNumCells bits set

    g_afe_ready = true;
    g_comm_fail_count = 0;
    return true;
}

void updateVoltages()
{
    if (!g_afe_ready)
    {
        publishFaultsOnly(VF_AFE_INIT | VF_COMM);
        return;
    }

    // ---- Read (driver retries each register once) --------------------------
    if (bq.ReadVoltages() != HAL_OK)
    {
        if (g_comm_fail_count < 255)
            g_comm_fail_count++;

        if (g_comm_fail_count >= CommReinitLimit)
        {
            initAfe();  // try to recover the AFE; sets g_afe_ready on success
            g_comm_fail_count = CommFailLimit;  // stay faulted until a good read
        }

        if (g_comm_fail_count >= CommFailLimit)
            publishFaultsOnly(g_latched_faults | VF_COMM);
        return;  // never publish a partial/failed read as good data
    }
    g_comm_fail_count = 0;

    VoltageSnapshot s{};
    for (uint8_t i = 0; i < NumCells; i++)
        s.cell_mv[i] = bq.GetCellVoltage(i);
    s.stack_mv = bq.GetStackVoltage();

    // ---- Validate + stats ---------------------------------------------------
    // No substitution: invalid cells keep their raw value, are flagged,
    // and are excluded from min/max/avg.
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

        if (v > s.max_mv)  // two independent ifs: a cell can be both min and max
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

    // Stack cross-check only makes sense when every cell read is valid
    if (n_valid == NumCells)
    {
        int32_t diff = s.stack_mv - s.sum_mv;
        if (diff < 0)
            diff = -diff;
        if (diff > StackMismatchMv)
            faults |= VF_STACK_MISMATCH;
    }

    // ---- Debounce OV/UV (set and clear both need N consecutive samples) -----
    debounce(any_ov, g_ov_count, VF_CELL_OV);
    debounce(any_uv, g_uv_count, VF_CELL_UV);

    s.faults = faults | g_latched_faults;
    s.timestamp_ms = osKernelGetTickCount() * 1000u / osKernelGetTickFreq();

    publish(s);
}

bool getVoltageSnapshot(VoltageSnapshot &out)
{
    taskENTER_CRITICAL();
    out = g_snapshot;
    const bool valid = g_snapshot_valid;
    taskEXIT_CRITICAL();
    return valid;
}

void getCellTemp() {}

void sendBalancingCommand() {}

void sendStatusMessage() {}

}  // namespace bmsdaughter