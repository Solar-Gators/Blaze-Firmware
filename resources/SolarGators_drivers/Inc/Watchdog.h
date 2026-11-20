#ifndef BLAZEFIRMWARE_WATCHDOG_H
#define BLAZEFIRMWARE_WATCHDOG_H

#include "cmsis_os.h"
#include <cstdint>
#include <cstdbool>
#include <FreeRTOS.h>
#include <task.h>

typedef struct {
    uint32_t mask;
    uint32_t timeout_ms;
    int32_t time_left_ms;
} WD_Task;

class Watchdog
{
public:
    static void init(void (*pet_fn)(void));
    void new_task(uint32_t timeout_ms);
    void check_in();
    static void start(void);

private:
    static osEventFlagsId_t ev_group;
    static void (*pet_fn)(void);
    static WD_Task tasks[31]; // err bit 32
    static uint8_t task_cnt;
    static uint32_t all_task_mask;

    uint32_t bit = 0;
};




#endif  //BLAZEFIRMWARE_WATCHDOG_H