//
// Created by rich on 10/22/26.
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