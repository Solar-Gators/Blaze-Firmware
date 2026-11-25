#include "user_threads.h"

#include "cmsis_os2.h"
#include "frontvcu.h"
#include "main.h"

#include <cstdint>

void userDefaultTask(void* argument)
{
    (void) argument;

    // run init function
    if (frontvcu::init() != HAL_OK)
    {
        Error_Handler();
        return;
    }

    // Convert 100 ms to kernel ticks, rounding up.
    const uint32_t period_ticks = static_cast<uint32_t>(
        (100ULL * osKernelGetTickFreq() + 999ULL) / 1000ULL);

    for (;;)
    {
        // send can
        frontvcu::sendCANMessagesTX();

        // delay 100ms
        osDelay(period_ticks);
    }
}