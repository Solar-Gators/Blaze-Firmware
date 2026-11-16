//
// Created by ahmed on 10/29/2026.
//

#ifndef BLAZEFIRMWARE_DAUGHTERBOARD_H
#define BLAZEFIRMWARE_DAUGHTERBOARD_H

#include "BQ76952.hpp"
#include "BQ769X2_Registers.h"

namespace bmsdaughter
{
constexpr uint8_t NumCells = 12;
constexpr uint16_t VcellMode = 0x87FF;
constexpr int16_t CellPlausibleMinMv = 500;
constexpr int16_t CellPlausibleMaxMv = 5000;
constexpr int16_t CellOvMv = 4210;
constexpr int16_t CellUvMv = 2490;

constexpr int32_t StackMismatchMv = 300;

// Task timing / filtering
constexpr uint32_t VoltagePeriodMs = 100;
constexpr uint8_t FaultDebounceSamples = 3;  // 300 ms at 100 ms period
constexpr uint8_t CommFailLimit = 3;         // consecutive failed cycles -> COMM fault
constexpr uint8_t CommReinitLimit = 10;      // consecutive failed cycles -> re-init AFE
enum VoltageFault : uint8_t
{
    VF_NONE = 0,
    VF_CELL_OV = 1u << 0,         // debounced over-voltage on any valid cell
    VF_CELL_UV = 1u << 1,         // debounced under-voltage on any valid cell
    VF_CELL_INVALID = 1u << 2,    // one or more cells outside plausible range
    VF_STACK_MISMATCH = 1u << 3,  // BQ stack voltage disagrees with cell sum
    VF_COMM = 1u << 4,            // I2C to BQ76952 failing; data is stale
    VF_AFE_INIT = 1u << 5,        // BQ76952 never initialised / config failed
};

// Everything the voltage task publishes. Other tasks get a copy.
struct VoltageSnapshot
{
    int16_t cell_mv[NumCells];  // raw readings, logical cell 0 = bottom cell
    uint16_t valid_mask;        // bit i set = cell i passed plausibility check
    int16_t min_mv;             // over valid cells only
    int16_t max_mv;
    uint8_t min_idx;
    uint8_t max_idx;
    int16_t avg_mv;
    int32_t sum_mv;         // sum of valid cells
    int32_t stack_mv;       // BQ76952 stack measurement
    uint8_t faults;         // VoltageFault bits
    uint32_t timestamp_ms;  // osKernelGetTickCount() of last good update
};

// Voltage task API
bool initAfe();                                 // reset + configure BQ76952
void updateVoltages();                          // one measurement cycle
bool getVoltageSnapshot(VoltageSnapshot &out);  // thread-safe copy; false if never valid

void getCellTemp();
void sendBalancingCommand();
void sendStatusMessage();

}  // namespace bmsdaughter
#endif  //BLAZEFIRMWARE_DAUGHTERBOARD_H
