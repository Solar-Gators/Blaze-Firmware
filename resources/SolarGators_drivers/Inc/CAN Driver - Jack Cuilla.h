//
// Created by jc170 on 11/9/2026.
//
#ifndef BLAZEFIRMWARE_CAN_DRIVER_JACK_CUILLA_H
#define BLAZEFIRMWARE_CAN_DRIVER_JACK_CUILLA_H
#endif  //BLAZEFIRMWARE_CAN_DRIVER_JACK_CUILLA_H

#pragma once
#include <array>
#include <cstdint>
#include <cstring>

#include "main.h"
#include "stm32l4xx_hal_can.h"

namespace BMS
{
struct Results
{
    // I added this line of code to initialize all of the values to be zero.
    uint8_t num_cells = 12; // This line allows us to define 12 cells which we are using.
    float avg_C = 0;
    uint16_t avg_cell_mV = 0;
    float high_C = 0;
    uint8_t high_temp_idx = 0;
    uint16_t high_cell_mV = 0;
    uint16_t low_cell_mV = 0;
    uint8_t low_cell_phys_idx = 0;
    uint8_t high_cell_phys_idx = 0;
    uint8_t faults = 0;
    std::array<uint16_t, 12> cell_voltages_mV{};
    std::array<float, 12> ntc_C{};
};
}

namespace CanFrames
{

struct Frame8
{
    std::array<uint8_t, 8> bytes{};
    uint8_t dlc = 8;
};

// This will eventually be used to tranmit the CAN.
bool transmit(const Frame8& frame, CAN_HandleTypeDef* hcan);

inline Frame8 make_high_temp(const BMS::Results& r)
{
    Frame8 f{};
    f.bytes[0] = 0;
    std::memcpy(&f.bytes[1], &r.high_C, 4);
    f.bytes[5] = r.high_temp_idx;
    return f;
}

inline Frame8 make_voltage_extremes(const BMS::Results& r)
{
    Frame8 f{};
    f.bytes[0] = 1; // Cell ID 1
    f.bytes[1] = r.high_cell_mV & 0xFF;
    f.bytes[2] = r.high_cell_mV >> 8;
    f.bytes[3] = r.low_cell_mV & 0xFF;
    f.bytes[4] = r.low_cell_mV >> 8;
    f.bytes[5] = r.low_cell_phys_idx;
    f.bytes[6] = r.high_cell_phys_idx;
    f.bytes[7] = r.faults;
    return f;
}

inline Frame8 make_average_stats(const BMS::Results& r)
{
    Frame8 f{};
    f.bytes[0] = 2; // Cell ID 2
    std::memcpy(&f.bytes[1], &r.avg_C, 4);
    f.bytes[5] = r.avg_cell_mV & 0xFF;
    f.bytes[6] = r.avg_cell_mV >> 8;
    f.bytes[7] = r.num_cells;
    return f;
}

// ****************** START OF CODE BY JACK ******************
// I used this as my model but the rest are referenced based on the same format.
inline Frame8 make_voltage_messages(const BMS::Results&r)
{
	Frame8 f{}; // Create empty 8-byte CAN frame
	    f.bytes[0] = 3; // Tells that cell ID is 3 (Cell 0).
	    f.bytes[1] = r.cell_voltages_mV[0] & 0xFF; // Defines cell 0 voltage low byte.
	    f.bytes[2] = r.cell_voltages_mV[0] >> 8; // Defines cell 0 voltage high byte.
	    std::memcpy(&f.bytes[3], &r.ntc_C[0], 4); // Defines Cell 0 temperature (float, bytes 3-6)
	    return f; // Returns the frame.
}

inline Frame8 make_voltage_message2 (const BMS::Results&r)
{
	Frame8 f{};
	    f.bytes[0] = 4;
	    f.bytes[1] = r.cell_voltages_mV[1] & 0xFF;
	    f.bytes[2] = r.cell_voltages_mV[1] >> 8;
	    std::memcpy(&f.bytes[3], &r.ntc_C[1], 4);
	    return f;
}

inline Frame8 make_voltage_message3 (const BMS::Results&r)
{
	Frame8 f{};
	    f.bytes[0] = 5;
	    f.bytes[1] = r.cell_voltages_mV[2] & 0xFF;
	    f.bytes[2] = r.cell_voltages_mV[2] >> 8;
	    std::memcpy(&f.bytes[3], &r.ntc_C[2], 4);
	    return f;
}

inline Frame8 make_voltage_message4 (const BMS::Results&r)
{
	Frame8 f{};
	    f.bytes[0] = 6;
	    f.bytes[1] = r.cell_voltages_mV[3] & 0xFF;
	    f.bytes[2] = r.cell_voltages_mV[3] >> 8;
	    std::memcpy(&f.bytes[3], &r.ntc_C[3], 4);
	    return f;
}

inline Frame8 make_voltage_message5 (const BMS::Results&r)
{
	Frame8 f{};
	    f.bytes[0] = 7;
	    f.bytes[1] = r.cell_voltages_mV[4] & 0xFF;
	    f.bytes[2] = r.cell_voltages_mV[4] >> 8;
	    std::memcpy(&f.bytes[3], &r.ntc_C[4], 4);
	    return f;
}

inline Frame8 make_voltage_message6 (const BMS::Results&r)
{
	Frame8 f{};
	    f.bytes[0] = 8;
	    f.bytes[1] = r.cell_voltages_mV[5] & 0xFF;
	    f.bytes[2] = r.cell_voltages_mV[5] >> 8;
	    std::memcpy(&f.bytes[3], &r.ntc_C[5], 4);
	    return f;

}
//  ****************** END OF CODE BY JACK ******************

// ****************** START OF CODE BY JACK (10/22/2026) ******************
inline Frame8 make_voltage_messages7(const BMS::Results&r)
{
	Frame8 f{}; // Create empty 8-byte CAN frame
	    f.bytes[0] = 9; // Tells that cell ID is 3 (Cell 0).
	    f.bytes[1] = r.cell_voltages_mV[6] & 0xFF; // Defines cell 0 voltage low byte.
	    f.bytes[2] = r.cell_voltages_mV[6] >> 8; // Defines cell 0 voltage high byte.
	    std::memcpy(&f.bytes[3], &r.ntc_C[6], 4); // Defines Cell 0 temperature (float, bytes 3-6)
	    return f; // Returns the frame.
}

inline Frame8 make_voltage_message8 (const BMS::Results&r)
{
	Frame8 f{};
	    f.bytes[0] = 10;
	    f.bytes[1] = r.cell_voltages_mV[7] & 0xFF;
	    f.bytes[2] = r.cell_voltages_mV[7] >> 8;
	    std::memcpy(&f.bytes[3], &r.ntc_C[7], 4);
	    return f;
}
inline Frame8 make_voltage_message9 (const BMS::Results&r)
{
	Frame8 f{};
	    f.bytes[0] = 11;
	    f.bytes[1] = r.cell_voltages_mV[8] & 0xFF;
	    f.bytes[2] = r.cell_voltages_mV[8] >> 8;
	    std::memcpy(&f.bytes[3], &r.ntc_C[8], 4);
	    return f;
}
inline Frame8 make_voltage_message10 (const BMS::Results&r)
{
	Frame8 f{};
	    f.bytes[0] = 12;
	    f.bytes[1] = r.cell_voltages_mV[9] & 0xFF;
	    f.bytes[2] = r.cell_voltages_mV[9] >> 8;
	    std::memcpy(&f.bytes[3], &r.ntc_C[9], 4);
	    return f;
}
inline Frame8 make_voltage_message11 (const BMS::Results&r)
{
	Frame8 f{};
	    f.bytes[0] = 13;
	    f.bytes[1] = r.cell_voltages_mV[10] & 0xFF;
	    f.bytes[2] = r.cell_voltages_mV[10] >> 8;
	    std::memcpy(&f.bytes[3], &r.ntc_C[10], 4);
	    return f;
}
inline Frame8 make_voltage_message12 (const BMS::Results&r)
{
	Frame8 f{};
	    f.bytes[0] = 14;
	    f.bytes[1] = r.cell_voltages_mV[11] & 0xFF;
	    f.bytes[2] = r.cell_voltages_mV[11] >> 8;
	    std::memcpy(&f.bytes[3], &r.ntc_C[11], 4);
	    return f;

}
//  ****************** END OF CODE BY JACK (Updated 11/9/26) ******************
}


