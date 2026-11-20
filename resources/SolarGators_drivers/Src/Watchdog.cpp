#include "Watchdog.h"

osEventFlagsId_t Watchdog::ev_group = NULL;
void (*Watchdog::pet_fn)(void) = NULL;
WD_Task Watchdog::tasks[31] = {0};
uint8_t Watchdog::task_cnt = 0;
uint32_t Watchdog::all_task_mask = 0;

void Watchdog::init(void (*pet_fn)(void))
{
    Watchdog::pet_fn = pet_fn;
    task_cnt = 0;
    all_task_mask = 0;
    ev_group = osEventFlagsNew(NULL);
}

void Watchdog::new_task(uint32_t timeout_ms)
{
    taskENTER_CRITICAL(); //just in case

    if (task_cnt >= 31)
    {
        taskEXIT_CRITICAL();
        return;
    }

    uint32_t mask = (uint32_t)((uint32_t)1 << task_cnt);
    tasks[task_cnt] = {
        mask,
        timeout_ms,
        (int32_t)timeout_ms,
    };
    all_task_mask |= mask;
    this->bit = mask;
    task_cnt++;

    taskEXIT_CRITICAL();

    if (ev_group != NULL) osEventFlagsSet(ev_group, this->bit);
}

void Watchdog::check_in(void)
{
    if (ev_group == NULL || bit == 0) return; // in case dumb
    osEventFlagsSet(ev_group, bit);
}

void Watchdog::start(void)
{
    if (ev_group == NULL || pet_fn == NULL) return; // restart board if start fails?

    for (;;)
    {
        if (task_cnt == 0)
        {
            // wait for any flag
            osEventFlagsWait(ev_group, 0x7FFFFFFF, osFlagsWaitAny | osFlagsNoClear, osWaitForever);
            continue;
        }
        int32_t low_t = tasks[0].time_left_ms;
        for (int i = 0; i < task_cnt; ++i)
            if (tasks[i].time_left_ms < low_t)
                low_t = tasks[i].time_left_ms;
        low_t = (low_t < 0) ? 0 : low_t;

        uint32_t start = osKernelGetTickCount();
        uint32_t flags = osEventFlagsWait(ev_group, all_task_mask, osFlagsWaitAny | osFlagsNoClear, low_t*osKernelGetTickFreq()/1000);
        uint32_t passed = (osKernelGetTickCount()-start) * 1000 / osKernelGetTickFreq();

        bool all_good = true;
        bool checked_in = !(flags & 0x80000000);
        for (int i = 0; i < task_cnt; ++i)
        {
            if (!(checked_in && (flags & tasks[i].mask)))
            {
                tasks[i].time_left_ms -= (int32_t)passed;
                if (tasks[i].time_left_ms <= 0) all_good = false;
            }
            else
            {
                tasks[i].time_left_ms = (int32_t)tasks[i].timeout_ms;
                osEventFlagsClear(ev_group, tasks[i].mask);
            }
        }
        if (all_good) pet_fn();
        else while (true);
    }
}