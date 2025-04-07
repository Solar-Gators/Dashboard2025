/*
 * sg_can.h
 *
 *  Created on: Mar 12, 2025
 *      Author: Matthew Shen, Jonathon Brown
 */

#ifndef SG_CAN_H_
#define SG_CAN_H_

#include "main.h"
#include "stdint.h"

#include <map>
#include <vector>
#include <deque>
#include <atomic>
#include <algorithm>
#include <cstring>

#include "cmsis_os.h"


#define THREAD_PRIORITY osPriorityAboveNormal   /* Priority of Rx and Tx threads */

#define TX_QUEUE_SIZE 3                         /* Size of Tx message queue */
#define TX_TIMEOUT    10                        /* Timeout for tx thread in ms */

#define NUM_FILTER_BANKS 14                     /* 14 filters on single CAN, 28 on dual CAN */
#define MAX_RX_MSGS      28                     /* Size of Rx message map */

#define CAN_PRIORITY_NORMAL 0                   /* Interrupt waits for FIFO full */
#define CAN_PRIORITY_HIGH   1                   /* Interrupt waits for FIFO pending */
#define CAN_FILTER_ALL      0                   /* Accept all messages */
#define CAN_FILTER_CUSTOM   2                   /* Accept messages in rx_messages_ */


class CANFrame {
public:
    CANFrame(uint32_t can_id,
                uint32_t id_type,
                uint32_t rtr_mode,
                uint32_t len,
                void (*rxCallback)(uint8_t data[]) = NULL):
        can_id(can_id), id_type(id_type), rtr_mode(rtr_mode), len(len), rxCallback(rxCallback)
    {
        mutex_id_ = osMutexNew(&mutex_attributes_);
    };

    inline osStatus_t Lock(uint32_t timeout = osWaitForever){
        return osMutexAcquire(mutex_id_, timeout);
    }

    inline osStatus_t Unlock(){
        return osMutexRelease(mutex_id_);
    }

    void LoadData(uint8_t data[], uint32_t len) {
        uint32_t load_len = std::min(len, this->len);
        std::memcpy(&this->data[0], &data[0], load_len);
    }

    uint32_t            can_id;             /* CAN ID, can be standard or extended */
    uint32_t            id_type;            /* CAN ID type, 0 if standard ID, 4 if extended ID */
    uint32_t            rtr_mode;           /* RTR mode, 0 if not RTR message, 2 if RTR */
    uint32_t            len;                /* payload data length */
    uint8_t             data[8];            /* payload data array, maximum of 8 bytes */
    void (*rxCallback)(uint8_t data[]);     /* pointer to rx callback function */
    osMutexId_t         mutex_id_;          /* mutex id for message */
    uint32_t            count_;             /* number of times message has been received */
    uint32_t            timestamp_;         /* timestamp of last message received */

private:
    StaticSemaphore_t   mutex_control_block_;
    const osMutexAttr_t mutex_attributes_ = {
        .name = "CANFrame Mutex",
        .attr_bits = osMutexRecursive,
        .cb_mem = &mutex_control_block_,
        .cb_size = sizeof(mutex_control_block_),
    };
};


class CANDevice {
    friend class CANController;
public:
    CANDevice(CAN_HandleTypeDef *hcan) { hcan_ = hcan; }
    HAL_StatusTypeDef Start();
    HAL_StatusTypeDef Send(CANFrame *msg);
    uint32_t GetStatus();
    void SetRxFlag();
    osThreadId_t tx_task_id_;
    osThreadId_t rx_task_id_;

protected:
    CAN_HandleTypeDef *hcan_;
    std::map<uint32_t, CANFrame*>* rx_messages_; // size: MAX_RX_MSGS
    osMessageQueueId_t tx_queue_ = osMessageQueueNew(TX_QUEUE_SIZE, sizeof(CANFrame*), NULL);
    std::atomic<uint32_t> status_;
    osEventFlagsId_t rx_event_flag_ = osEventFlagsNew(NULL);

    /* Tx thread definitions */
    uint32_t tx_task_buffer[128];   // Size can be much smaller w/o logging
    StaticTask_t tx_task_control_block_;
    const osThreadAttr_t tx_task_attributes_ = {
        .name = "CAN Tx Task",
        .attr_bits = osThreadDetached,
        .cb_mem = &tx_task_control_block_,
        .cb_size = sizeof(tx_task_control_block_),
        .stack_mem = &tx_task_buffer[0],
        .stack_size = sizeof(tx_task_buffer),
        .priority = (osPriority_t) THREAD_PRIORITY,
        .tz_module = 0,
        .reserved = 0,
    };

    /* Rx thread definitions */
    uint32_t rx_task_buffer[128];   // Size can be much smaller w/o logging
    StaticTask_t rx_task_control_block_;
    const osThreadAttr_t rx_task_attributes_ = {
        .name = "CAN Rx Task",
        .attr_bits = osThreadDetached,
        .cb_mem = &rx_task_control_block_,
        .cb_size = sizeof(rx_task_control_block_),
        .stack_mem = &rx_task_buffer[0],
        .stack_size = sizeof(rx_task_buffer),
        .priority = (osPriority_t) THREAD_PRIORITY,
        .tz_module = 0,
        .reserved = 0,
    };

    /* Tx thread function */
    void HandleTx(void* argument);

    /* Rx thread function */
    void HandleRx(void* argument);

    /* Tx timeout handler thread */
    void HandleTxTimeout();
};

/**
 * Class for controlling CAN devices and messages.
*/
class CANController {
public:
    static HAL_StatusTypeDef AddDevice(CANDevice *device);
    static HAL_StatusTypeDef AddRxMessage(CANFrame *msg);
    static HAL_StatusTypeDef AddRxMessage(CANFrame *msg, void (*rxCallback)(uint8_t*));
    static HAL_StatusTypeDef AddRxMessages(CANFrame *msg[], uint32_t num_msgs);
    static HAL_StatusTypeDef AddFilterAll();
    static HAL_StatusTypeDef AddFilterId(uint32_t can_id, uint32_t id_type,
                                         uint32_t rtr_mode, uint32_t priority);
    static HAL_StatusTypeDef AddFilterIdRange(uint32_t can_id, uint32_t range,
                                              uint32_t id_type, uint32_t rtr_mode,
                                              uint32_t priority);
    static HAL_StatusTypeDef Start();
    static HAL_StatusTypeDef Send(CANFrame *msg);
    static HAL_StatusTypeDef SendOnDevice(CANDevice *device, CANFrame *msg);
    static HAL_StatusTypeDef GetMessage(uint32_t can_id, CANFrame *msg);
    static HAL_StatusTypeDef GetDeviceStatus(CANDevice *device);
    static void RxCallback(CAN_HandleTypeDef *hcan);
protected:
    //static inline std::vector<CANDevice*> devices_; // size: 3
    static inline CANDevice* devices_t[3];
    static inline int device_count = 0;
    static inline std::map<uint32_t, CANFrame*> rx_messages_; // size: MAX_RX_MSGS
    static inline uint32_t num_msgs_ = 0;
    static inline std::deque<CAN_FilterTypeDef> filters_; // size: NUM_FILTER_BANKS*2
};

#endif /* SG_CAN_H_ */
