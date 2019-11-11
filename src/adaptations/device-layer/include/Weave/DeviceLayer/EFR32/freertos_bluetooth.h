/***************************************************************************//**
 * @brief Adaptation for running Bluetooth in RTOS
 *******************************************************************************
 * # License
 * <b>Copyright 2019 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

#ifndef FREERTOS_BLUETOOTH_H
#define FREERTOS_BLUETOOTH_H

#if __cplusplus
    extern "C" {
#endif

#include "rtos_gecko.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"
#include "event_groups.h"

//Bluetooth event flag group
extern EventGroupHandle_t bluetooth_event_flags;
//Bluetooth event flag definitions
#define BLUETOOTH_EVENT_FLAG_STACK       (0x01)    //Bluetooth task needs an update
#define BLUETOOTH_EVENT_FLAG_LL          (0x02)    //Linklayer task needs an update
#define BLUETOOTH_EVENT_FLAG_CMD_WAITING (0x04)    //BGAPI command is waiting to be processed
#define BLUETOOTH_EVENT_FLAG_RSP_WAITING (0x08)    //BGAPI response is waiting to be processed
#define BLUETOOTH_EVENT_FLAG_EVT_WAITING (0x10)   //BGAPI event is waiting to be processed
#define BLUETOOTH_EVENT_FLAG_EVT_HANDLED (0x20)   //BGAPI event is handled

//Bluetooth event data pointer
extern volatile struct gecko_cmd_packet*  bluetooth_evt;

// Function prototype for initializing Bluetooth stack.
typedef errorcode_t(*bluetooth_stack_init_func)();

/**
 * Start Bluetooth tasks. The given Bluetooth stack initialization function
 * will be called at a proper time. Application should not initialize
 * Bluetooth stack anywhere else.
 *
 * @param ll_priority link layer task priority
 * @param stack_priority Bluetooth stack task priority
 * @param initialize_bluetooth_stack The function for initializing Bluetooth stack
 */
errorcode_t bluetooth_start(UBaseType_t ll_priority,
		            UBaseType_t stack_priority,
		            bluetooth_stack_init_func initialize_bluetooth_stack);

// Set the callback for wakeup, Bluetooth task will call this when it has a new event
// It must only used to wake up application task, for example by posting task semaphore
typedef void (*wakeupCallback)(void);
void BluetoothSetWakeupCallback(wakeupCallback cb);
//Bluetooth stack needs an update
extern void BluetoothUpdate(void);
//Linklayer is updated
extern void BluetoothLLCallback(void);

//Mutex functions for using Bluetooth from multiple tasks
void BluetoothPend(void);
void BluetoothPost(void);

EventBits_t vRaiseEventFlagBasedOnContext(EventGroupHandle_t xEventGroup, EventBits_t uxBitsToWaitFor, BaseType_t *pxHigherPriorityTaskWoken);
EventBits_t vSendToQueueBasedOnContext(QueueHandle_t xQueue, void *xItemToQueue, TickType_t xTicksToWait, BaseType_t *pxHigherPriorityTaskWoken);

#if __cplusplus
}
#endif

#endif //FREERTOS_BLUETOOTH_H
