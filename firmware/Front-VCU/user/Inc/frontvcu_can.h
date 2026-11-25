#ifndef BLAZEFIRMWARE_FRONTVCU_CAN_H
#define BLAZEFIRMWARE_FRONTVCU_CAN_H

#include "main.h"

#include <cstdint>

namespace frontvcu
{

// TODO: Call once from a running RTOS task maybe or an init task that dies
// Configures filters and starts all three peripherals sequentially.
// Do not retry after a startup failure.
HAL_StatusTypeDef can_init();

// Sends the bench-test counter on FDCAN1.
// HAL_OK means accepted into the driver's software TX queue.
HAL_StatusTypeDef sendTestCounter(uint16_t counter);

}  // namespace frontvcu

#endif  // BLAZEFIRMWARE_FRONTVCU_CAN_H