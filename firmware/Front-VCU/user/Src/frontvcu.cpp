#include "frontvcu.h"

#include "frontvcu_can.h"
#include "frontvcu_state.h"

#include <cstdint>

namespace frontvcu
{

// The single definition of the shared application state.
FrontVCUState state{};

HAL_StatusTypeDef init()
{
    return can_init();
}

void sendCANMessagesTX()
{
    static uint16_t counter = 0;

    if (sendTestCounter(counter) != HAL_OK)
    {
        state.enqueue_failures.fetch_add(1);
    }

    ++counter;
}

}  // namespace frontvcu