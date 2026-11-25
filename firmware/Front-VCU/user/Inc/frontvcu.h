#ifndef BLAZEFIRMWARE_FRONTVCU_H
#define BLAZEFIRMWARE_FRONTVCU_H

#include "main.h"

namespace frontvcu
{

// function called only once startup for initializing frontvcu
HAL_StatusTypeDef init();

// send can messages
void sendCANMessagesTX();

}  // namespace frontvcu

#endif  // BLAZEFIRMWARE_FRONTVCU_H