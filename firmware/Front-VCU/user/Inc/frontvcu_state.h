#ifndef BLAZEFIRMWARE_FRONTVCU_STATE_H
#define BLAZEFIRMWARE_FRONTVCU_STATE_H

#include <atomic>
#include <cstdint>

namespace frontvcu
{

struct FrontVCUState
{
    // combined receive statistics across all three CAN peripherals.
    std::atomic<uint32_t> received_frames{0};
    std::atomic<uint32_t> rejected_frames{0};

    std::atomic<uint32_t> last_received_counter{0};
    std::atomic<uint32_t> last_received_ms{0};

    // Failures to enqueue an outgoing frame.
    // Does not include failures after hardware submission.
    std::atomic<uint32_t> enqueue_failures{0};
};

// Individual fields are atomic, but reading several fields does not
// produce a synchronized snapshot.
extern FrontVCUState state;

}  // namespace frontvcu

#endif  // BLAZEFIRMWARE_FRONTVCU_STATE_H