#pragma once

#include "CanDriverApi.hpp"
#include "FreeRTOS.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstring>
#include <initializer_list>
#include <type_traits>
#include <unordered_map>
#include <vector>

#ifndef CANDEVICE_MAX_BUSES
#define CANDEVICE_MAX_BUSES 3
#endif

#if configSUPPORT_STATIC_ALLOCATION != 1
#error "CANDevice requires FreeRTOS static allocation."
#endif

namespace sg
{

struct CANFrame
{
    uint32_t can_id = 0;
    CANFrameIDType id_type = CANFrameIDType::STANDARD;
    CANFrameRTRMode rtr_mode = CANFrameRTRMode::DATA;
    CANFrameLen len = CANFrameLen::BYTES_0;
    uint32_t timestamp = 0;  // Reception time in HAL milliseconds.

#if defined(HAL_FDCAN_MODULE_ENABLED)
    static constexpr uint8_t max_len = 64;
#else
    static constexpr uint8_t max_len = 8;
#endif

    std::array<uint8_t, max_len> data{};

    // Appended so existing six-field aggregate initializers still work.
    CANFrameFormat format = CANFrameFormat::CLASSIC;

    // source must contain at least length bytes.
    // send() separately validates legal CAN/CAN FD payload lengths.
    bool LoadData(const uint8_t* source, CANFrameLen length)
    {
        const auto count = static_cast<uint8_t>(length);

        if (count > max_len || (count != 0 && source == nullptr))
        {
            return false;
        }

        if (count != 0)
        {
            std::memmove(data.data(), source, count);
        }

        for (std::size_t i = count; i < data.size(); ++i)
        {
            data[i] = 0;
        }

        len = length;
        return true;
    }

    bool isData(CANFrameLen expected_length) const
    {
        return rtr_mode == CANFrameRTRMode::DATA &&
               len == expected_length;
    }
};

// FreeRTOS queues copy frames as raw bytes.
static_assert(
    std::is_trivially_copyable_v<CANFrame>,
    "CANFrame must remain trivially copyable.");

using CanCallback = HAL_StatusTypeDef (*)(const CANFrame&, void*);

using CallbackMap =
    std::unordered_map<uint32_t, const std::vector<CanCallback>>;

class CANDevice
{
   public:
    CANDevice(
        CanHandle_t* handle,
        std::initializer_list<CallbackMap::value_type> standard_callbacks,
        std::initializer_list<CallbackMap::value_type> extended_callbacks)
        : hcan_(handle),
          standard_callbacks_(standard_callbacks),
          extended_callbacks_(extended_callbacks)
    {
    }

    // Keep each CANDevice alive for the lifetime of the firmware.
    // Prefer global/static storage: each instance contains task stacks
    // and queue buffers.
    //
    // Configure filters/callbacks before starting.
    // Start once from a running RTOS task, after CubeMX initialization.
    // Serialize startup calls for different CANDevice instances.
    //
    // A failure after startup begins is fatal for this instance.
    // Do not retry or reuse its static RTOS storage.
    HAL_StatusTypeDef startCANDevice();

    HAL_StatusTypeDef addFilterId(
        uint32_t id,
        CANFrameIDType type,
        CANFrameRTRMode rtr,
        CANFramePriority priority);

    // Accepts the inclusive interval [id, id + range].
    // range == 0 accepts exactly one identifier.
    //
    // bxCAN may need multiple filter banks to represent the interval.
    // If software filter capacity is insufficient, no filters are added.
    //
    // Hardware filter capacity is checked during startup.
    HAL_StatusTypeDef addFilterRange(
        uint32_t id,
        uint32_t range,
        CANFrameIDType type,
        CANFrameRTRMode rtr,
        CANFramePriority priority);

    bool addDefaultCallback(CanCallback callback)
    {
        if (startup_attempted_)
        {
            return false;
        }

        default_callback_ = callback;
        return true;
    }

    // Task context only.
    // HAL_OK means copied into the software queue, not delivered on the bus.
    HAL_StatusTypeDef send(const CANFrame& frame);

    // HAL interrupt entry point; application code should not call this.
    static HAL_StatusTypeDef RxCallback(CanHandle_t* handle);

    uint32_t getProcessedMessagesCount() const
    {
        return processed_count_.load();
    }

    // Successful submissions to hardware, not acknowledged transmissions.
    uint32_t getSentMessagesCount() const
    {
        return submitted_count_.load();
    }

    // Frames dropped because of receive queue pressure or invalid decoding.
    // Does not include hardware FIFO overruns.
    uint32_t getDroppedFrameCount() const
    {
        return dropped_rx_count_.load();
    }

    // Failures after a frame was accepted into the software transmit queue.
    // Does not include errors occurring after submission to hardware.
    uint32_t getFailedTxCount() const
    {
        return failed_tx_count_.load();
    }

    uint32_t getCallbackErrorCount() const
    {
        return callback_error_count_.load();
    }

    HAL_StatusTypeDef getLastTxStatus() const
    {
        return last_tx_status_.load();
    }

    CANDevice(const CANDevice&) = delete;
    CANDevice& operator=(const CANDevice&) = delete;
    CANDevice(CANDevice&&) = delete;
    CANDevice& operator=(CANDevice&&) = delete;

   private:
    static constexpr std::size_t FILTER_CAPACITY = 14;
    static constexpr uint32_t QUEUE_SIZE = 8;
    static constexpr uint32_t TX_WAIT_MS = 10;

    CanHandle_t* hcan_;

    // Callback tables still use the C++ allocator during construction.
    const CallbackMap standard_callbacks_;
    const CallbackMap extended_callbacks_;

    // Callback context is always a pointer to this CANDevice.
    CanCallback default_callback_ = nullptr;

    std::array<CanFilter_t, FILTER_CAPACITY> filters_{};
    std::size_t filter_count_ = 0;

    bool startup_attempted_ = false;
    std::atomic<bool> started_{false};

    osMessageQueueId_t tx_queue_ = nullptr;
    osMessageQueueId_t rx_queue_ = nullptr;

    StaticQueue_t tx_queue_cb_{};
    StaticQueue_t rx_queue_cb_{};

    alignas(8) uint8_t tx_queue_storage_[QUEUE_SIZE * sizeof(CANFrame)]{};
    alignas(8) uint8_t rx_queue_storage_[QUEUE_SIZE * sizeof(CANFrame)]{};

    osThreadId_t tx_task_ = nullptr;
    osThreadId_t rx_task_ = nullptr;

    StaticTask_t tx_tcb_{};
    StaticTask_t rx_tcb_{};

    alignas(8) uint32_t tx_stack_[512]{};
    alignas(8) uint32_t rx_stack_[512]{};

    std::atomic<uint32_t> processed_count_{0};
    std::atomic<uint32_t> submitted_count_{0};
    std::atomic<uint32_t> dropped_rx_count_{0};
    std::atomic<uint32_t> failed_tx_count_{0};
    std::atomic<uint32_t> callback_error_count_{0};

    std::atomic<HAL_StatusTypeDef> last_tx_status_{HAL_OK};

    inline static CANDevice* registry_[CANDEVICE_MAX_BUSES]{};

    const std::vector<CanCallback>* findCallbacks(
        const CANFrame& frame) const;

    bool validTx(const CANFrame& frame) const;

    HAL_StatusTypeDef configureFilters();
    HAL_StatusTypeDef submit(CANFrame& frame);

    void cleanupStartFailure();

    static void HandleRx(void* argument);
    static void HandleTx(void* argument);
};

}  // namespace sg