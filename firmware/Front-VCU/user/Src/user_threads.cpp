//
// Created by rich on 10/22/26.
//

#include "../Inc/user_threads.h"

void userDefaultTask(void* argument)
{
    for (;;)
    {
        osDelay(1);
    }
}