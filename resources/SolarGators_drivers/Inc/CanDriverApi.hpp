#pragma once

#include <cstdint>

#include "cmsis_os2.h"

#if defined(STM32L433xx) || defined(STM32L476xx)
#include "stm32l4xx_hal.h"
#elif defined(STM32G473xx) || defined(STM32G483xx)
#include "stm32g4xx_hal.h"
#elif defined(STM32U575xx) || defined(STM32U5G9xx)
#include "stm32u5xx_hal.h"
#else
#error "Unsupported STM32: add its family HAL header here."
#endif

#if defined(HAL_FDCAN_MODULE_ENABLED)
using CanHandle_t = FDCAN_HandleTypeDef;
using CanFilter_t = FDCAN_FilterTypeDef;
#elif defined(HAL_CAN_MODULE_ENABLED)
using CanHandle_t = CAN_HandleTypeDef;
using CanFilter_t = CAN_FilterTypeDef;
#else
#error "Enable CAN or FDCAN in this board's CubeMX configuration."
#endif

namespace sg
{

// Actual payload lengths in bytes, not HAL-encoded DLC values.
enum class CANFrameLen : uint8_t
{
    BYTES_0 = 0,
    BYTES_1 = 1,
    BYTES_2 = 2,
    BYTES_3 = 3,
    BYTES_4 = 4,
    BYTES_5 = 5,
    BYTES_6 = 6,
    BYTES_7 = 7,
    BYTES_8 = 8,
    BYTES_12 = 12,
    BYTES_16 = 16,
    BYTES_20 = 20,
    BYTES_24 = 24,
    BYTES_32 = 32,
    BYTES_48 = 48,
    BYTES_64 = 64
};

enum class CANFrameIDType : uint8_t
{
    STANDARD,
    EXTENDED
};

enum class CANFrameRTRMode : uint8_t
{
    DATA,
    REMOTE
};

// These select receive FIFOs, not arbitration priority.
// This implementation supports HIGH/FIFO0 only.
enum class CANFramePriority : uint8_t
{
    HIGH,
    LOW
};

enum class CANFrameFormat : uint8_t
{
    CLASSIC,
    FD,
    FD_BRS
};

}  // namespace sg