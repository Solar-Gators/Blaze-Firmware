#include "main.h"

// Shared driver sources are compiled for every board.
// Boards without CAN enabled do not need this implementation.
#if defined(HAL_CAN_MODULE_ENABLED) || defined(HAL_FDCAN_MODULE_ENABLED)

#include "CanDriver.hpp"

#if defined(USE_FreeRTOS_HEAP_1)
#error "CANDevice startup cleanup requires RTOS object deletion; select FreeRTOS heap_4."
#endif

namespace sg
{
namespace
{

constexpr uint8_t payload_lengths[] = {
    0, 1, 2, 3, 4, 5, 6, 7,
    8, 12, 16, 20, 24, 32, 48, 64
};

#if defined(HAL_FDCAN_MODULE_ENABLED)

// Use HAL constants rather than assuming a particular DLC bit layout.
// Different STM32 HAL versions use different representations.
constexpr uint32_t hal_dlcs[] = {
    FDCAN_DLC_BYTES_0,
    FDCAN_DLC_BYTES_1,
    FDCAN_DLC_BYTES_2,
    FDCAN_DLC_BYTES_3,
    FDCAN_DLC_BYTES_4,
    FDCAN_DLC_BYTES_5,
    FDCAN_DLC_BYTES_6,
    FDCAN_DLC_BYTES_7,
    FDCAN_DLC_BYTES_8,
    FDCAN_DLC_BYTES_12,
    FDCAN_DLC_BYTES_16,
    FDCAN_DLC_BYTES_20,
    FDCAN_DLC_BYTES_24,
    FDCAN_DLC_BYTES_32,
    FDCAN_DLC_BYTES_48,
    FDCAN_DLC_BYTES_64
};

#endif

int lengthIndex(CANFrameLen length)
{
    for (int i = 0; i < 16; ++i)
    {
        if (payload_lengths[i] == static_cast<uint8_t>(length))
        {
            return i;
        }
    }

    return -1;
}

uint32_t millisecondsToTicks(uint32_t milliseconds)
{
    // CMSIS timeouts use kernel ticks. Round up.
    return static_cast<uint32_t>(
        (uint64_t{milliseconds} * osKernelGetTickFreq() + 999u) /
        1000u);
}

}  // namespace

HAL_StatusTypeDef CANDevice::addFilterId(
    uint32_t id,
    CANFrameIDType type,
    CANFrameRTRMode rtr,
    CANFramePriority priority)
{
    return addFilterRange(id, 0, type, rtr, priority);
}

HAL_StatusTypeDef CANDevice::addFilterRange(
    uint32_t id,
    uint32_t range,
    CANFrameIDType type,
    CANFrameRTRMode rtr,
    CANFramePriority priority)
{
    if (startup_attempted_ || priority != CANFramePriority::HIGH)
    {
        return HAL_ERROR;
    }

    if (type != CANFrameIDType::STANDARD &&
        type != CANFrameIDType::EXTENDED)
    {
        return HAL_ERROR;
    }

    if (rtr != CANFrameRTRMode::DATA &&
        rtr != CANFrameRTRMode::REMOTE)
    {
        return HAL_ERROR;
    }

    const uint32_t maximum =
        type == CANFrameIDType::STANDARD ? 0x7FFu : 0x1FFFFFFFu;

    // Validate before addition to prevent overflow.
    if (id > maximum || range > maximum - id)
    {
        return HAL_ERROR;
    }

    const uint32_t end = id + range;

#if defined(HAL_FDCAN_MODULE_ENABLED)

    // FDCAN cannot match RTR independently in each ID filter.
    // This implementation rejects received remote frames globally.
    // Transmitting classic remote requests is still supported.
    if (rtr != CANFrameRTRMode::DATA ||
        filter_count_ == filters_.size())
    {
        return HAL_ERROR;
    }

    CanFilter_t filter{};

    filter.IdType =
        type == CANFrameIDType::STANDARD
            ? FDCAN_STANDARD_ID
            : FDCAN_EXTENDED_ID;

    // Standard and extended filters have separate hardware index spaces.
    for (std::size_t i = 0; i < filter_count_; ++i)
    {
        if (filters_[i].IdType == filter.IdType)
        {
            ++filter.FilterIndex;
        }
    }

    filter.FilterType = FDCAN_FILTER_RANGE;
    filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    filter.FilterID1 = id;
    filter.FilterID2 = end;

    filters_[filter_count_++] = filter;

#else

    // bxCAN represents ranges using masks.
    // Split the interval into aligned power-of-two blocks.
    const std::size_t original_count = filter_count_;

    while (id <= end)
    {
        if (filter_count_ == filters_.size())
        {
            filter_count_ = original_count;
            return HAL_ERROR;
        }

        uint32_t block_size = 1;

        while (block_size <= (end - id + 1) / 2 &&
               (id & (block_size * 2 - 1)) == 0)
        {
            block_size *= 2;
        }

        const uint32_t shift =
            type == CANFrameIDType::STANDARD ? 21u : 3u;

        const uint32_t ide =
            type == CANFrameIDType::STANDARD
                ? CAN_ID_STD
                : CAN_ID_EXT;

        const uint32_t hal_rtr =
            rtr == CANFrameRTRMode::REMOTE
                ? CAN_RTR_REMOTE
                : CAN_RTR_DATA;

        const uint32_t packed_id =
            (id << shift) | ide | hal_rtr;

        // Match ID bits outside the block, plus IDE and RTR.
        const uint32_t packed_mask =
            ((maximum & ~(block_size - 1)) << shift) | 0x6u;

        CanFilter_t filter{};

        filter.FilterBank = static_cast<uint32_t>(filter_count_);
        filter.SlaveStartFilterBank = 14;
        filter.FilterMode = CAN_FILTERMODE_IDMASK;
        filter.FilterScale = CAN_FILTERSCALE_32BIT;
        filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
        filter.FilterActivation = ENABLE;

        filter.FilterIdHigh = packed_id >> 16;
        filter.FilterIdLow = packed_id & 0xFFFFu;
        filter.FilterMaskIdHigh = packed_mask >> 16;
        filter.FilterMaskIdLow = packed_mask & 0xFFFFu;

        filters_[filter_count_++] = filter;
        id += block_size;
    }

#endif

    return HAL_OK;
}

HAL_StatusTypeDef CANDevice::configureFilters()
{
#if defined(HAL_FDCAN_MODULE_ENABLED)

    // CubeMX must allocate enough standard and extended filter entries.
    for (std::size_t i = 0; i < filter_count_; ++i)
    {
        const auto& filter = filters_[i];

        const uint32_t capacity =
            filter.IdType == FDCAN_STANDARD_ID
                ? hcan_->Init.StdFiltersNbr
                : hcan_->Init.ExtFiltersNbr;

        if (filter.FilterIndex >= capacity)
        {
            return HAL_ERROR;
        }
    }

    // This driver owns the peripheral's filters.
    // Disable allocated entries before installing the requested filters.
    for (uint32_t type : {FDCAN_STANDARD_ID, FDCAN_EXTENDED_ID})
    {
        const uint32_t count =
            type == FDCAN_STANDARD_ID
                ? hcan_->Init.StdFiltersNbr
                : hcan_->Init.ExtFiltersNbr;

        for (uint32_t i = 0; i < count; ++i)
        {
            CanFilter_t filter{};

            filter.IdType = type;
            filter.FilterIndex = i;
            filter.FilterType = FDCAN_FILTER_RANGE;
            filter.FilterConfig = FDCAN_FILTER_DISABLE;

            if (HAL_FDCAN_ConfigFilter(hcan_, &filter) != HAL_OK)
            {
                return HAL_ERROR;
            }
        }
    }

    for (std::size_t i = 0; i < filter_count_; ++i)
    {
        if (HAL_FDCAN_ConfigFilter(hcan_, &filters_[i]) != HAL_OK)
        {
            return HAL_ERROR;
        }
    }

    // No explicit filters: accept all data frames through the global filter.
    const uint32_t unmatched =
        filter_count_ == 0
            ? FDCAN_ACCEPT_IN_RX_FIFO0
            : FDCAN_REJECT;

    return HAL_FDCAN_ConfigGlobalFilter(
        hcan_,
        unmatched,
        unmatched,
        FDCAN_REJECT_REMOTE,
        FDCAN_REJECT_REMOTE);

#else

    // The supported L433/L476 devices have one bxCAN instance.
    // Dual-bxCAN devices would need shared filter-bank allocation.
    for (uint32_t i = 0; i < FILTER_CAPACITY; ++i)
    {
        CanFilter_t filter{};

        filter.FilterBank = i;
        filter.SlaveStartFilterBank = 14;
        filter.FilterScale = CAN_FILTERSCALE_32BIT;
        filter.FilterMode = CAN_FILTERMODE_IDMASK;
        filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
        filter.FilterActivation = DISABLE;

        if (HAL_CAN_ConfigFilter(hcan_, &filter) != HAL_OK)
        {
            return HAL_ERROR;
        }
    }

    if (filter_count_ == 0)
    {
        CanFilter_t filter{};

        filter.FilterScale = CAN_FILTERSCALE_32BIT;
        filter.FilterMode = CAN_FILTERMODE_IDMASK;
        filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
        filter.FilterActivation = ENABLE;
        filter.SlaveStartFilterBank = 14;

        // Zero identifier and mask accept all identifiers/frame types.
        return HAL_CAN_ConfigFilter(hcan_, &filter);
    }

    for (std::size_t i = 0; i < filter_count_; ++i)
    {
        if (HAL_CAN_ConfigFilter(hcan_, &filters_[i]) != HAL_OK)
        {
            return HAL_ERROR;
        }
    }

    return HAL_OK;

#endif
}

HAL_StatusTypeDef CANDevice::startCANDevice()
{
    if (startup_attempted_ ||
        hcan_ == nullptr ||
        osKernelGetState() != osKernelRunning)
    {
        return HAL_ERROR;
    }

    for (auto* device : registry_)
    {
        if (device != nullptr && device->hcan_ == hcan_)
        {
            return HAL_ERROR;
        }
    }

    CANDevice** available_slot = nullptr;

    for (auto& device : registry_)
    {
        if (device == nullptr)
        {
            available_slot = &device;
            break;
        }
    }

    if (available_slot == nullptr)
    {
        return HAL_ERROR;
    }

    startup_attempted_ = true;
    *available_slot = this;

    osMessageQueueAttr_t tx_attributes{};
    tx_attributes.name = "CAN Tx Queue";
    tx_attributes.cb_mem = &tx_queue_cb_;
    tx_attributes.cb_size = sizeof(tx_queue_cb_);
    tx_attributes.mq_mem = tx_queue_storage_;
    tx_attributes.mq_size = sizeof(tx_queue_storage_);

    osMessageQueueAttr_t rx_attributes{};
    rx_attributes.name = "CAN Rx Queue";
    rx_attributes.cb_mem = &rx_queue_cb_;
    rx_attributes.cb_size = sizeof(rx_queue_cb_);
    rx_attributes.mq_mem = rx_queue_storage_;
    rx_attributes.mq_size = sizeof(rx_queue_storage_);

    tx_queue_ = osMessageQueueNew(
        QUEUE_SIZE, sizeof(CANFrame), &tx_attributes);

    rx_queue_ = osMessageQueueNew(
        QUEUE_SIZE, sizeof(CANFrame), &rx_attributes);

    if (tx_queue_ == nullptr ||
        rx_queue_ == nullptr ||
        configureFilters() != HAL_OK)
    {
        cleanupStartFailure();
        return HAL_ERROR;
    }

    osThreadAttr_t attributes{};

    attributes.name = "CAN Rx";
    attributes.cb_mem = &rx_tcb_;
    attributes.cb_size = sizeof(rx_tcb_);
    attributes.stack_mem = rx_stack_;
    attributes.stack_size = sizeof(rx_stack_);
    attributes.priority = osPriorityHigh;

    rx_task_ = osThreadNew(HandleRx, this, &attributes);

    attributes.name = "CAN Tx";
    attributes.cb_mem = &tx_tcb_;
    attributes.cb_size = sizeof(tx_tcb_);
    attributes.stack_mem = tx_stack_;
    attributes.stack_size = sizeof(tx_stack_);
    attributes.priority = osPriorityAboveNormal;

    tx_task_ = osThreadNew(HandleTx, this, &attributes);

    if (rx_task_ == nullptr || tx_task_ == nullptr)
    {
        cleanupStartFailure();
        return HAL_ERROR;
    }

#if defined(HAL_FDCAN_MODULE_ENABLED)

    const bool success =
        HAL_FDCAN_Start(hcan_) == HAL_OK &&
        HAL_FDCAN_ActivateNotification(
            hcan_, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) == HAL_OK;

#else

    const bool success =
        HAL_CAN_Start(hcan_) == HAL_OK &&
        HAL_CAN_ActivateNotification(
            hcan_, CAN_IT_RX_FIFO0_MSG_PENDING) == HAL_OK;

#endif

    if (!success)
    {
        cleanupStartFailure();
        return HAL_ERROR;
    }

    started_.store(true);
    return HAL_OK;
}

void CANDevice::cleanupStartFailure()
{
#if defined(HAL_FDCAN_MODULE_ENABLED)

    HAL_FDCAN_DeactivateNotification(
        hcan_, FDCAN_IT_RX_FIFO0_NEW_MESSAGE);
    HAL_FDCAN_Stop(hcan_);

#else

    HAL_CAN_DeactivateNotification(
        hcan_, CAN_IT_RX_FIFO0_MSG_PENDING);
    HAL_CAN_Stop(hcan_);

#endif

    for (auto& device : registry_)
    {
        if (device == this)
        {
            device = nullptr;
        }
    }

    if (rx_task_ != nullptr)
    {
        osThreadTerminate(rx_task_);
    }

    if (tx_task_ != nullptr)
    {
        osThreadTerminate(tx_task_);
    }

    if (rx_queue_ != nullptr)
    {
        osMessageQueueDelete(rx_queue_);
    }

    if (tx_queue_ != nullptr)
    {
        osMessageQueueDelete(tx_queue_);
    }

    // Do not retry startup or reuse this instance's static RTOS storage.
}

bool CANDevice::validTx(const CANFrame& frame) const
{
    if (frame.id_type != CANFrameIDType::STANDARD &&
        frame.id_type != CANFrameIDType::EXTENDED)
    {
        return false;
    }

    const uint32_t maximum =
        frame.id_type == CANFrameIDType::STANDARD
            ? 0x7FFu
            : 0x1FFFFFFFu;

    if (frame.can_id > maximum)
    {
        return false;
    }

    if (frame.rtr_mode != CANFrameRTRMode::DATA &&
        frame.rtr_mode != CANFrameRTRMode::REMOTE)
    {
        return false;
    }

    const int index = lengthIndex(frame.len);

    if (index < 0 ||
        static_cast<uint8_t>(frame.len) > CANFrame::max_len)
    {
        return false;
    }

    if (frame.format == CANFrameFormat::CLASSIC)
    {
        return index <= 8;
    }

#if defined(HAL_FDCAN_MODULE_ENABLED)

    // CAN FD does not support remote frames.
    if (frame.rtr_mode != CANFrameRTRMode::DATA)
    {
        return false;
    }

    if (frame.format == CANFrameFormat::FD)
    {
        return hcan_->Init.FrameFormat == FDCAN_FRAME_FD_NO_BRS ||
               hcan_->Init.FrameFormat == FDCAN_FRAME_FD_BRS;
    }

    if (frame.format == CANFrameFormat::FD_BRS)
    {
        return hcan_->Init.FrameFormat == FDCAN_FRAME_FD_BRS;
    }

#endif

    return false;
}

HAL_StatusTypeDef CANDevice::send(const CANFrame& frame)
{
    if (!started_.load() || !validTx(frame))
    {
        return HAL_ERROR;
    }

    const osStatus_t status = osMessageQueuePut(
        tx_queue_,
        &frame,
        0,
        millisecondsToTicks(TX_WAIT_MS));

    if (status == osOK)
    {
        return HAL_OK;
    }

    if (status == osErrorTimeout || status == osErrorResource)
    {
        return HAL_TIMEOUT;
    }

    return HAL_ERROR;
}

HAL_StatusTypeDef CANDevice::submit(CANFrame& frame)
{
    const uint32_t start = osKernelGetTickCount();
    const uint32_t timeout = millisecondsToTicks(TX_WAIT_MS);

    // ponytail: one-tick polling; use TX-complete notifications if lower
    // submission latency becomes necessary.
    for (;;)
    {
#if defined(HAL_FDCAN_MODULE_ENABLED)

        const uint32_t free_slots =
            HAL_FDCAN_GetTxFifoFreeLevel(hcan_);

#else

        const uint32_t free_slots =
            HAL_CAN_GetTxMailboxesFreeLevel(hcan_);

#endif

        if (free_slots != 0)
        {
            break;
        }

        // Unsigned subtraction handles kernel tick wraparound.
        if (osKernelGetTickCount() - start >= timeout)
        {
            return HAL_TIMEOUT;
        }

        osDelay(1);
    }

#if defined(HAL_FDCAN_MODULE_ENABLED)

    FDCAN_TxHeaderTypeDef header{};

    header.Identifier = frame.can_id;

    header.IdType =
        frame.id_type == CANFrameIDType::STANDARD
            ? FDCAN_STANDARD_ID
            : FDCAN_EXTENDED_ID;

    header.TxFrameType =
        frame.rtr_mode == CANFrameRTRMode::REMOTE
            ? FDCAN_REMOTE_FRAME
            : FDCAN_DATA_FRAME;

    header.DataLength = hal_dlcs[lengthIndex(frame.len)];

    header.FDFormat =
        frame.format == CANFrameFormat::CLASSIC
            ? FDCAN_CLASSIC_CAN
            : FDCAN_FD_CAN;

    header.BitRateSwitch =
        frame.format == CANFrameFormat::FD_BRS
            ? FDCAN_BRS_ON
            : FDCAN_BRS_OFF;

    header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    header.TxEventFifoControl = FDCAN_NO_TX_EVENTS;

    return HAL_FDCAN_AddMessageToTxFifoQ(
        hcan_, &header, frame.data.data());

#else

    CAN_TxHeaderTypeDef header{};

    header.StdId = frame.can_id;
    header.ExtId = frame.can_id;

    header.IDE =
        frame.id_type == CANFrameIDType::STANDARD
            ? CAN_ID_STD
            : CAN_ID_EXT;

    header.RTR =
        frame.rtr_mode == CANFrameRTRMode::REMOTE
            ? CAN_RTR_REMOTE
            : CAN_RTR_DATA;

    header.DLC = static_cast<uint8_t>(frame.len);

    uint32_t mailbox = 0;

    return HAL_CAN_AddTxMessage(
        hcan_, &header, frame.data.data(), &mailbox);

#endif
}

void CANDevice::HandleTx(void* argument)
{
    auto& self = *static_cast<CANDevice*>(argument);

    for (;;)
    {
        CANFrame frame{};

        if (osMessageQueueGet(
                self.tx_queue_,
                &frame,
                nullptr,
                osWaitForever) != osOK)
        {
            continue;
        }

        const HAL_StatusTypeDef status = self.submit(frame);
        self.last_tx_status_.store(status);

        if (status == HAL_OK)
        {
            ++self.submitted_count_;
        }
        else
        {
            ++self.failed_tx_count_;
        }

        // Failed submissions are dropped and counted.
        // This task does not retry indefinitely.
    }
}

const std::vector<CanCallback>* CANDevice::findCallbacks(
    const CANFrame& frame) const
{
    const auto& callbacks =
        frame.id_type == CANFrameIDType::STANDARD
            ? standard_callbacks_
            : extended_callbacks_;

    const auto iterator = callbacks.find(frame.can_id);

    return iterator == callbacks.end() ? nullptr : &iterator->second;
}

HAL_StatusTypeDef CANDevice::RxCallback(CanHandle_t* handle)
{
    CANDevice* self = nullptr;

    for (auto* device : registry_)
    {
        if (device != nullptr && device->hcan_ == handle)
        {
            self = device;
            break;
        }
    }

    if (self == nullptr)
    {
        return HAL_ERROR;
    }

    HAL_StatusTypeDef result = HAL_OK;

    // Limit this ISR invocation to the frames pending at entry.
#if defined(HAL_FDCAN_MODULE_ENABLED)

    uint32_t remaining =
        HAL_FDCAN_GetRxFifoFillLevel(handle, FDCAN_RX_FIFO0);

#else

    uint32_t remaining =
        HAL_CAN_GetRxFifoFillLevel(handle, CAN_RX_FIFO0);

#endif

    while (remaining != 0)
    {
        --remaining;
        CANFrame frame{};

#if defined(HAL_FDCAN_MODULE_ENABLED)

        FDCAN_RxHeaderTypeDef header{};

        if (HAL_FDCAN_GetRxMessage(
                handle,
                FDCAN_RX_FIFO0,
                &header,
                frame.data.data()) != HAL_OK)
        {
            return HAL_ERROR;
        }

        frame.can_id = header.Identifier;

        frame.id_type =
            header.IdType == FDCAN_STANDARD_ID
                ? CANFrameIDType::STANDARD
                : CANFrameIDType::EXTENDED;

        frame.rtr_mode =
            header.RxFrameType == FDCAN_DATA_FRAME
                ? CANFrameRTRMode::DATA
                : CANFrameRTRMode::REMOTE;

        int index = 0;

        while (index < 16 && hal_dlcs[index] != header.DataLength)
        {
            ++index;
        }

        if (index == 16)
        {
            ++self->dropped_rx_count_;
            continue;
        }

        frame.len = static_cast<CANFrameLen>(payload_lengths[index]);

        if (header.FDFormat == FDCAN_CLASSIC_CAN)
        {
            frame.format = CANFrameFormat::CLASSIC;

            // Classic DLC values above 8 still represent 8 payload bytes.
            if (index > 8)
            {
                frame.len = CANFrameLen::BYTES_8;
            }
        }
        else
        {
            frame.format =
                header.BitRateSwitch == FDCAN_BRS_ON
                    ? CANFrameFormat::FD_BRS
                    : CANFrameFormat::FD;
        }

#else

        CAN_RxHeaderTypeDef header{};

        if (HAL_CAN_GetRxMessage(
                handle,
                CAN_RX_FIFO0,
                &header,
                frame.data.data()) != HAL_OK)
        {
            return HAL_ERROR;
        }

        frame.can_id =
            header.IDE == CAN_ID_STD ? header.StdId : header.ExtId;

        frame.id_type =
            header.IDE == CAN_ID_STD
                ? CANFrameIDType::STANDARD
                : CANFrameIDType::EXTENDED;

        frame.rtr_mode =
            header.RTR == CAN_RTR_DATA
                ? CANFrameRTRMode::DATA
                : CANFrameRTRMode::REMOTE;

        frame.len = static_cast<CANFrameLen>(
            header.DLC <= 8 ? header.DLC : 8);

#endif

        frame.timestamp = HAL_GetTick();

        if (frame.rtr_mode == CANFrameRTRMode::REMOTE)
        {
            // A remote frame carries a requested length, not payload data.
            frame.data.fill(0);
        }

        if (self->findCallbacks(frame) == nullptr &&
            self->default_callback_ == nullptr)
        {
            continue;
        }

        if (osMessageQueuePut(
                self->rx_queue_,
                &frame,
                0,
                0) != osOK)
        {
            ++self->dropped_rx_count_;
            result = HAL_BUSY;
        }
    }

    return result;
}

void CANDevice::HandleRx(void* argument)
{
    auto& self = *static_cast<CANDevice*>(argument);

    for (;;)
    {
        CANFrame frame{};

        if (osMessageQueueGet(
                self.rx_queue_,
                &frame,
                nullptr,
                osWaitForever) != osOK)
        {
            continue;
        }

        if (const auto* callbacks = self.findCallbacks(frame))
        {
            for (CanCallback callback : *callbacks)
            {
                if (callback == nullptr ||
                    callback(frame, &self) != HAL_OK)
                {
                    ++self.callback_error_count_;
                }
            }
        }
        else if (self.default_callback_ != nullptr)
        {
            if (self.default_callback_(frame, &self) != HAL_OK)
            {
                ++self.callback_error_count_;
            }
        }
        else
        {
            continue;
        }

        ++self.processed_count_;
    }
}

}  // namespace sg

#if defined(HAL_FDCAN_MODULE_ENABLED)

extern "C" void HAL_FDCAN_RxFifo0Callback(
    FDCAN_HandleTypeDef* handle,
    uint32_t interrupts)
{
    if ((interrupts & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != 0)
    {
        sg::CANDevice::RxCallback(handle);
    }
}

#else

extern "C" void HAL_CAN_RxFifo0MsgPendingCallback(
    CAN_HandleTypeDef* handle)
{
    sg::CANDevice::RxCallback(handle);
}

#endif  // HAL_FDCAN_MODULE_ENABLED

#endif  // HAL_CAN_MODULE_ENABLED || HAL_FDCAN_MODULE_ENABLED