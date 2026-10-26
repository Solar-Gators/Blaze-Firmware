//
// Created by rich on 10/22/26.
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