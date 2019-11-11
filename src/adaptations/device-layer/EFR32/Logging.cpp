/*
 *
 *    Copyright (c) 2019 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 *    @file
 *          Provides implementations for the OpenWeave and LwIP logging
 *          functions on Silicon Labs EFR32 platforms.
 *
 *          Logging should be initialized by a call to efr32LogInit().  A
 *          spooler task is created that sends the logs to the UART.  Log
 *          entries are queued. If the queue is full then by default error
 *          logs wait indefinitely until a slot is available whereas
 *          non-error logs are dropped to avoid delays.
 */            

#include <stdio.h>
#include <retargetserial.h>
#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/Support/logging/WeaveLogging.h>
#include <task.h>
#include <queue.h>

// FreeRTOS includes
#include "SEGGER_RTT.h"
#include "SEGGER_RTT_Conf.h"


// RTT Buffer size and name
#ifndef LOG_RTT_BUFFER_INDEX
#define LOG_RTT_BUFFER_INDEX 0
#endif

#ifndef LOG_RTT_BUFFER_NAME
#define LOG_RTT_BUFFER_NAME "Terminal"
#endif

#ifndef LOG_RTT_BUFFER_SIZE
#define LOG_RTT_BUFFER_SIZE 256
#endif


#ifdef COLOR_LOGS
#define LOG_ERROR  "\e[1;31m<error >\e[0m "
#define LOG_WARN   "\e[1;33m<warn  >\e[0m "
#define LOG_INFO   "\e[0m<info  > "
#define LOG_DETAIL "\e[1;34m[detail]\e[0m "
#define LOG_LWIP   "\e[0m<lwip  > "
#define LOG_EFR32  "\e[0m<efr32 > "
#else
#define LOG_ERROR  "<error > "
#define LOG_WARN   "<warn  > "
#define LOG_INFO   "<info  > "
#define LOG_DETAIL "<detail> "
#define LOG_LWIP   "<lwip  > "
#define LOG_EFR32  "<efr32 > "
#endif

// How long to wait to drop error log messages (Default to not drop error messages)
#ifndef LOG_ERROR_TIMEOUT
#define LOG_ERROR_TIMEOUT   ( portMAX_DELAY )
#endif

// How long to wait to drop efr32Log messages (default to not drop efr32Log messages)
#ifndef LOG_EFR32_TIMEOUT
#define LOG_EFR32_TIMEOUT ( portMAX_DELAY )
#endif

// How long to wait for non-error log messages to be dropped if spool queue is full. (Default to drop immediately)
#ifndef LOG_TIMEOUT
#define LOG_TIMEOUT         ( 0 )  
#endif

// Maximum number of messages to queue to spooler
#ifndef LOG_QUEUE_LEN
#define LOG_QUEUE_LEN       ( 50 )
#endif

// Maximum size of each log message
#ifndef LOG_ITEM_SZ
#define LOG_ITEM_SZ        WEAVE_DEVICE_CONFIG_LOG_MESSAGE_MAX_SIZE
#endif

#if WEAVE_DEVICE_CONFIG_ENABLE_THREAD
#include <openthread/platform/logging.h>
#endif // WEAVE_DEVICE_CONFIG_ENABLE_THREAD

using namespace ::nl::Weave;
using namespace ::nl::Weave::DeviceLayer;
using namespace ::nl::Weave::DeviceLayer::Internal;

static bool sLogInitialized = false;
static char sMsgBuffer[LOG_ITEM_SZ];
static QueueHandle_t sLogQueueHandle; 
static TaskHandle_t sLogSpoolerTaskHandle;
static uint8_t sLogBuffer[LOG_RTT_BUFFER_SIZE];
static bool sHardFault = false;


/**
 * Enqueue a log message to the Logging Spooler task.  If
 * this is called before tasks are started it prints directly to
 * the RTT or UART.
 */
static void PrintLog(const char *msg, TickType_t timeout)
{
#if EFR32_LOG_ENABLED
    if (sLogInitialized)
    {
        if (sHardFault == false && xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
        {
            xQueueSend(sLogQueueHandle, msg, timeout);
        }
        else
        {
#if RTT_LOGGING_ENABLED
            size_t sz;
            sz = strlen(msg);  
            SEGGER_RTT_WriteNoLock(0, msg, sz);
            const char *newline = "\r\n";
            sz = strlen(newline);
            SEGGER_RTT_WriteNoLock(0, newline, sz);
#endif
        }
    }
#endif // EFR32_LOG_ENABLED
}

/**
 * Logger task receives log messages from a queue and writes them to RTT or serial port via printf
 */
static void LogSpoolerTaskMain(void * pvParameter)
{
#if EFR32_LOG_ENABLED
  while (1)
  {
      if (xQueueReceive(sLogQueueHandle, sMsgBuffer, portMAX_DELAY) == pdTRUE)
      {
#if RTT_LOGGING_ENABLED
          size_t sz;
          sz = strlen(sMsgBuffer);  
          SEGGER_RTT_WriteNoLock(0, sMsgBuffer, sz);
          const char *newline = "\r\n";
          sz = strlen(newline);
          SEGGER_RTT_WriteNoLock(0, newline, sz);
#endif
      }      
  }
#endif // EFR32_LOG_ENABLED
}


/**
 * Initialize the serial port for logging
 */
extern "C" int efr32LogInit(void)
{
#if EFR32_LOG_ENABLED

#if RTT_LOGGING_ENABLED
    SEGGER_RTT_ConfigUpBuffer(LOG_RTT_BUFFER_INDEX, LOG_RTT_BUFFER_NAME, sLogBuffer, LOG_RTT_BUFFER_SIZE,
                                          SEGGER_RTT_MODE_NO_BLOCK_TRIM);    
#endif

    sLogQueueHandle = xQueueCreate(LOG_QUEUE_LEN, LOG_ITEM_SZ);  

    if (sLogQueueHandle == NULL) {
        return -1;
    }

    if (xTaskCreate(LogSpoolerTaskMain, "logs", WEAVE_DEVICE_CONFIG_LOG_TASK_STACK_SIZE / sizeof(StackType_t), NULL,
                    WEAVE_DEVICE_CONFIG_LOG_TASK_PRIORITY, &sLogSpoolerTaskHandle) != pdPASS)
    {
        return -1;
    }
    
    sLogInitialized = true;  

#endif // EFR32_LOG_ENABLED
    return 0;
}

/**
 * General-purpose logging function
 */
extern "C" void efr32Log(const char *aFormat, ...)
{
    va_list v;

    va_start(v, aFormat);

#if EFR32_LOG_ENABLED

    char formattedMsg[LOG_ITEM_SZ];

    strcpy (formattedMsg, LOG_EFR32);      
    size_t prefixLen = strlen(formattedMsg);
    size_t len = vsnprintf(formattedMsg + prefixLen, sizeof formattedMsg - prefixLen, aFormat, v);

    if (len >= sizeof formattedMsg - prefixLen)
    {
        formattedMsg[sizeof formattedMsg - 1] = '\0';
    }

    PrintLog(formattedMsg, LOG_EFR32_TIMEOUT);

#endif // EFR32_LOG_ENABLED

    va_end(v);
}



namespace {

void GetModuleName(char *buf, uint8_t module)
{
    if (module == ::nl::Weave::Logging::kLogModule_DeviceLayer)
    {
        memcpy(buf, "DL", 3);
    }
    else
    {
        ::nl::Weave::Logging::GetModuleName(buf, module);
    }
}

} // unnamed namespace

namespace nl {
namespace Weave {
namespace DeviceLayer {

/**
 * Called whenever a log message is emitted by Weave or LwIP.
 *
 * This function is intended be overridden by the application to, e.g.,
 * schedule output of queued log entries.
 */
void __attribute__((weak)) OnLogOutput(void)
{
}

} // namespace DeviceLayer
} // namespace Weave
} // namespace nl


namespace nl {
namespace Weave {
namespace Logging {

/**
 * OpenWeave log output function.
 */
void Log(uint8_t module, uint8_t category, const char *aFormat, ...)
{
    va_list v;
    TickType_t timeout;
    
    va_start(v, aFormat);

#if EFR32_LOG_ENABLED && _WEAVE_USE_LOGGING

    if (IsCategoryEnabled(category))
    {
        char formattedMsg[LOG_ITEM_SZ];
        size_t formattedMsgLen;

        constexpr size_t maxPrefixLen = nlWeaveLoggingModuleNameLen + 3;
        static_assert(sizeof(formattedMsg) > maxPrefixLen);

        switch (category) {
        case kLogCategory_Error:
            strcpy (formattedMsg, LOG_ERROR);
            timeout = LOG_ERROR_TIMEOUT;
            break;
        case kLogCategory_Progress:
        case kLogCategory_Retain:
        default:
            strcpy(formattedMsg, LOG_INFO);
            timeout = LOG_TIMEOUT;
            break;
        case kLogCategory_Detail:
            strcpy(formattedMsg, LOG_DETAIL);
            timeout = LOG_TIMEOUT;
            break;
        }

        formattedMsgLen = strlen(formattedMsg);

        // Form the log prefix, e.g. "[DL] "
        formattedMsg[formattedMsgLen++] = '[';
        ::GetModuleName(formattedMsg + formattedMsgLen, module);
        formattedMsgLen = strlen(formattedMsg);
        formattedMsg[formattedMsgLen++] = ']';
        formattedMsg[formattedMsgLen++] = ' ';

        size_t len = vsnprintf(formattedMsg + formattedMsgLen, sizeof formattedMsg - formattedMsgLen, aFormat, v);

        if (len >= sizeof formattedMsg - formattedMsgLen) {
          formattedMsg[sizeof formattedMsg - 1] = '\0';
        }

        PrintLog(formattedMsg, timeout);            
    }

    // Let the application know that a log message has been emitted.
    DeviceLayer::OnLogOutput();

#endif // EFR32_LOG_ENABLED

    va_end(v);
}

} // namespace Logging
} // namespace Weave
} // namespace nl


/**
 * LwIP log output function.
 */
extern "C" void LwIPLog(const char *aFormat, ...)
{
    va_list v;
    
    va_start(v, aFormat);

#if EFR32_LOG_ENABLED

    char formattedMsg[LOG_ITEM_SZ];
    
    strcpy (formattedMsg, LOG_LWIP);
    size_t prefixLen = strlen(formattedMsg);
    size_t len = vsnprintf(formattedMsg + prefixLen, sizeof formattedMsg - prefixLen, aFormat, v);

    if (len >= sizeof formattedMsg - prefixLen) 
    {
        formattedMsg[sizeof formattedMsg - 1] = '\0';
    }

    PrintLog(formattedMsg, LOG_TIMEOUT);
    // Let the application know that a log message has been emitted.
    DeviceLayer::OnLogOutput();

#endif // EFR32_LOG_ENABLED

    va_end(v);
}

/** 
 * Platform logging function for OpenThread
 */
#if WEAVE_DEVICE_CONFIG_ENABLE_THREAD
extern "C" void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, ...)
{
    va_list v;    
    TickType_t timeout;
    (void)aLogRegion;

    va_start(v, aFormat);

#if EFR32_LOG_ENABLED

    char formattedMsg[LOG_ITEM_SZ];

    if (sLogInitialized) {
      switch (aLogLevel) {
      case OT_LOG_LEVEL_CRIT:
          strcpy(formattedMsg, LOG_ERROR "[ot] ");
          timeout = LOG_ERROR_TIMEOUT;
          break;
      case OT_LOG_LEVEL_WARN:
          strcpy(formattedMsg, LOG_WARN "[ot] ");
          timeout = LOG_TIMEOUT;
          break;
      case OT_LOG_LEVEL_NOTE:
          strcpy(formattedMsg, LOG_INFO "[ot] ");
          timeout = LOG_TIMEOUT;
          break;
      case OT_LOG_LEVEL_INFO:
          strcpy(formattedMsg, LOG_INFO "[ot] ");
          timeout = LOG_TIMEOUT;
          break;
      case OT_LOG_LEVEL_DEBG:
          strcpy(formattedMsg, LOG_DETAIL "[ot] ");
          timeout = LOG_TIMEOUT;
          break;
      default:
          strcpy(formattedMsg, LOG_DETAIL "[ot] ");
          timeout = LOG_TIMEOUT;
          break;
      }
      
      size_t prefixLen = strlen(formattedMsg);
      size_t len = vsnprintf(formattedMsg + prefixLen, sizeof(formattedMsg) - prefixLen, aFormat, v);

      if (len >= sizeof formattedMsg - prefixLen) {
        formattedMsg[sizeof formattedMsg - 1] = '\0';
      }

      PrintLog(formattedMsg, timeout);
    }
    
    // Let the application know that a log message has been emitted.
    DeviceLayer::OnLogOutput();

#endif // EFR32_LOG_ENABLED

    va_end(v);
}
#endif // WEAVE_DEVICE_CONFIG_ENABLE_THREAD


#if HARD_FAULT_LOG_ENABLE && EFR32_LOG_ENABLED
extern "C" void otSysInit(int argc, char **argv);

/**
 * Log register contents to UART when a hard fault occurs.
 */
extern "C" void debugHardfault(uint32_t *sp)
{
    uint32_t cfsr  = SCB->CFSR;
    uint32_t hfsr  = SCB->HFSR;
    uint32_t mmfar = SCB->MMFAR;
    uint32_t bfar  = SCB->BFAR;

    uint32_t r0  = sp[0];
    uint32_t r1  = sp[1];
    uint32_t r2  = sp[2];
    uint32_t r3  = sp[3];
    uint32_t r12 = sp[4];
    uint32_t lr  = sp[5];
    uint32_t pc  = sp[6];
    uint32_t psr = sp[7];
    char buffer[40];

    sLogInitialized = true;
    sHardFault = true;
    
    snprintf(buffer, sizeof buffer, LOG_ERROR "HardFault:\n");
    PrintLog(buffer, 0);
    snprintf(buffer, sizeof buffer, "SCB->CFSR   0x%08lx", cfsr);
    PrintLog(buffer, 0);
    snprintf(buffer, sizeof buffer, "SCB->HFSR   0x%08lx", hfsr);
    PrintLog(buffer, 0);
    snprintf(buffer, sizeof buffer, "SCB->MMFAR  0x%08lx", mmfar);
    PrintLog(buffer, 0);
    snprintf(buffer, sizeof buffer, "SCB->BFAR   0x%08lx", bfar);
    PrintLog(buffer, 0);
    snprintf(buffer, sizeof buffer, "SP          0x%08lx", (uint32_t)sp);
    PrintLog(buffer, 0);
    snprintf(buffer, sizeof buffer, "R0          0x%08lx\n", r0);
    PrintLog(buffer, 0);
    snprintf(buffer, sizeof buffer, "R1          0x%08lx\n", r1);
    PrintLog(buffer, 0);
    snprintf(buffer, sizeof buffer, "R2          0x%08lx\n", r2);
    PrintLog(buffer, 0);
    snprintf(buffer, sizeof buffer, "R3          0x%08lx\n", r3);
    PrintLog(buffer, 0);
    snprintf(buffer, sizeof buffer, "R12         0x%08lx\n", r12);
    PrintLog(buffer, 0);
    snprintf(buffer, sizeof buffer, "LR          0x%08lx\n", lr);
    PrintLog(buffer, 0);
    snprintf(buffer, sizeof buffer, "PC          0x%08lx\n", pc);
    PrintLog(buffer, 0);
    snprintf(buffer, sizeof buffer, "PSR         0x%08lx\n", psr);
    PrintLog(buffer, 0);

    while(1);
}

/**
 * Override default hard-fault handler
 */
extern "C" __attribute__( (naked) ) void HardFault_Handler(void)
{
    __asm volatile
    (
        "tst lr, #4                                    \n"
        "ite eq                                        \n"
        "mrseq r0, msp                                 \n"
        "mrsne r0, psp                                 \n"
        "ldr r1, debugHardfault_address                \n"
        "bx r1                                         \n"
        "debugHardfault_address: .word debugHardfault  \n"
    );
}

#endif // HARD_FAULT_LOG_ENABLE

