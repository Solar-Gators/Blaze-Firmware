//
// Created by r3car on 10/25/2026.
//

#include "user_threads.h"

#include "cmsis_os.h"

void userStartVoltagesCheck(void* argument)
{
    for (;;)
    {
        osDelay(1);
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