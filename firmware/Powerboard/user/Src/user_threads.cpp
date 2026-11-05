//
// Created by r3car on 10/25/2026.
//

#include "user_threads.h"
#include "power_board.h"

#include "cmsis_os.h"

void userDefaultTask(void* argument)
{
    (void)argument;
    for (;;)
    {
        fanControlStep();   // one fan-control pass; ends with osDelay(100)
    }
}