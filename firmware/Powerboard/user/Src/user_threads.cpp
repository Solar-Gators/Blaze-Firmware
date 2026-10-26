//
// Created by r3car on 10/25/2026.
//

#include "user_threads.h"

#include "cmsis_os.h"

void userDefaultTask(void* argument)
{
    for (;;)
    {
        osDelay(1);
    }
}