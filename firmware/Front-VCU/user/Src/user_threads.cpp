//
// Created by rich on 10/22/26.
//

#include "user_threads.h"

#include "cmsis_os.h"

#include "stm32g4xx_hal_iwdg.h"

extern IWDG_HandleTypeDef hiwdg;

void userDefaultTask(void* argument)
{
    for (;;)
    {
        osDelay(1000);
        HAL_IWDG_Refresh(&hiwdg);
    }
}