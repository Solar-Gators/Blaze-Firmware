//
// Created by r3car on 10/25/2026.
//

#include "user_threads.h"

#include "cmsis_os.h"

#include "Daughterboard.h"

void userStartVoltagesCheck(void* argument)
{
    (void) argument;

    // Bring up the AFE. Keep retrying; updateVoltages() reports VF_AFE_INIT meanwhile.
    while (!bmsdaughter::initAfe())
    {
        bmsdaughter::updateVoltages();  // publishes the init fault
        osDelay(500);
    }

    const uint32_t period_ticks = (bmsdaughter::VoltagePeriodMs * osKernelGetTickFreq()) / 1000u;
    uint32_t next_wake = osKernelGetTickCount();

    for (;;)
    {
        bmsdaughter::updateVoltages();

        // Fixed-rate loop: period doesn't drift with I2C read time
        next_wake += period_ticks;
        osDelayUntil(next_wake);
    }
}

void userStartTempsCheck(void* argument)
{
    for (;;)
    {
        osDelay(1);
    }
}

void userStartCANMessages(void* argument)
{
    for (;;)
    {
        osDelay(1);
    }
}

void userStartDefaultTask(void* argument)

{
    for (;;)
    {
        osDelay(1);
    }
}