//
// Created by r3car on 10/25/2026.
//

#include "cmsis_os.h"
#include "user_threads.h"

void userDefaultTask(void* argument)
{
    for (;;)
    {
        osDelay(1);
    }
}