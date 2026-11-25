#include "frontvcu_can.h"

#include "CanDriver.hpp"

#include <atomic>
#include <cstdint>
#include <initializer_list>

extern "C"
{
    // CubeMX defines these handles in main.c.
    extern FDCAN_HandleTypeDef hfdcan1;
    extern FDCAN_HandleTypeDef hfdcan2;
    extern FDCAN_HandleTypeDef hfdcan3;
}

namespace frontvcu
{
namespace
{

// Example message IDs, not established Blaze message definitions.
// temporary examples
constexpr uint32_t ButtonsId = 0x101;
constexpr uint32_t DisplayStatusId = 0x102;
constexpr uint32_t BatteryStatusId = 0x201;
constexpr uint32_t SensorReadingId = 0x301;

// Retained for the existing periodic transmit example.
constexpr uint32_t TestMessageId = 0x321;

// Example received values for inspection in the debugger.
// Move application-owned values into FrontVCUState when needed.
std::atomic<uint8_t> button_bits{0};
std::atomic<uint8_t> display_status{0};
std::atomic<uint16_t> battery_mv{0};
std::atomic<uint16_t> sensor_reading{0};

// FDCAN1, ID 0x101:
// One byte containing example button flags.
// bascially this is run on can recieve, we need to extract the frame data into a state variable
// to use
HAL_StatusTypeDef onButtons(
    const sg::CANFrame& frame,
    void* context)
{
    (void) context;

    if (frame.format != sg::CANFrameFormat::CLASSIC ||
        !frame.isData(sg::CANFrameLen::BYTES_1))
    {
        return HAL_ERROR;
    }

    button_bits.store(frame.data[0]);
    return HAL_OK;
}

// FDCAN1, ID 0x102:
// One byte containing an example display status.
HAL_StatusTypeDef onDisplayStatus(
    const sg::CANFrame& frame,
    void* context)
{
    (void) context;

    if (frame.format != sg::CANFrameFormat::CLASSIC ||
        !frame.isData(sg::CANFrameLen::BYTES_1))
    {
        return HAL_ERROR;
    }

    display_status.store(frame.data[0]);
    return HAL_OK;
}

// FDCAN2, ID 0x201:
// Two bytes containing an example voltage in millivolts.
// Least significant byte first.
HAL_StatusTypeDef onBatteryStatus(
    const sg::CANFrame& frame,
    void* context)
{
    (void) context;

    if (frame.format != sg::CANFrameFormat::CLASSIC ||
        !frame.isData(sg::CANFrameLen::BYTES_2))
    {
        return HAL_ERROR;
    }

    const uint16_t value = static_cast<uint16_t>(
        static_cast<uint16_t>(frame.data[0]) |
        (static_cast<uint16_t>(frame.data[1]) << 8));

    battery_mv.store(value);
    return HAL_OK;
}

// FDCAN3, ID 0x301:
// Two bytes containing an example unsigned sensor reading.
// Least significant byte first.
HAL_StatusTypeDef onSensorReading(
    const sg::CANFrame& frame,
    void* context)
{
    (void) context;

    if (frame.format != sg::CANFrameFormat::CLASSIC ||
        !frame.isData(sg::CANFrameLen::BYTES_2))
    {
        return HAL_ERROR;
    }

    const uint16_t value = static_cast<uint16_t>(
        static_cast<uint16_t>(frame.data[0]) |
        (static_cast<uint16_t>(frame.data[1]) << 8));

    sensor_reading.store(value);
    return HAL_OK;
}

// Each device owns independent queues, tasks, and filters.
// These objects remain alive for the lifetime of the firmware.

// creating driver objects for each hardware perifieral, includes defining can IDs to recieve and callbacks

// FDCAN1: buttons and display status.
sg::CANDevice can1(
    &hfdcan1,
    {
        {ButtonsId, {onButtons}},
        {DisplayStatusId, {onDisplayStatus}}
    },
    {});

// FDCAN2: battery status.
sg::CANDevice can2(
    &hfdcan2,
    {
        {BatteryStatusId, {onBatteryStatus}}
    },
    {});

// FDCAN3: sensor readings.
sg::CANDevice can3(
    &hfdcan3,
    {
        {SensorReadingId, {onSensorReading}}
    },
    {});

// Configure standard-ID data-frame filters before starting.
// adds receive filters and starts device
HAL_StatusTypeDef startDevice(
    sg::CANDevice& device,
    std::initializer_list<uint32_t> ids)
{
    for (uint32_t id : ids)
    {
        const HAL_StatusTypeDef status = device.addFilterId(
            id,
            sg::CANFrameIDType::STANDARD,
            sg::CANFrameRTRMode::DATA,
            sg::CANFramePriority::HIGH);

        if (status != HAL_OK)
        {
            return status;
        }
    }

    return device.startCANDevice();
}

}  // namespace

// add ids filters here as well
HAL_StatusTypeDef can_init()
{
    // Call once from a running RTOS task.
    // Start sequentially; do not retry after startup failure.
    HAL_StatusTypeDef status =
        startDevice(can1, {ButtonsId, DisplayStatusId});

    if (status != HAL_OK)
    {
        return status;
    }

    status = startDevice(can2, {BatteryStatusId});

    if (status != HAL_OK)
    {
        return status;
    }

    return startDevice(can3, {SensorReadingId});
}

HAL_StatusTypeDef sendTestCounter(uint16_t counter)
{
    sg::CANFrame frame{};

    frame.can_id = TestMessageId;
    frame.id_type = sg::CANFrameIDType::STANDARD;
    frame.rtr_mode = sg::CANFrameRTRMode::DATA;
    frame.format = sg::CANFrameFormat::CLASSIC;

    const uint8_t payload[2] = {
        static_cast<uint8_t>(counter & 0xFFu),
        static_cast<uint8_t>(counter >> 8)
    };

    if (!frame.LoadData(payload, sg::CANFrameLen::BYTES_2))
    {
        return HAL_ERROR;
    }

    // Transmit ID 0x321 on FDCAN1.
    // Receive filters do not restrict outgoing messages.
    return can1.send(frame);
}

}  // namespace frontvcu