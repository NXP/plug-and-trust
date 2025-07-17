/*
 *
 * Copyright 2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef INC_FREERTOS_H /* Header guard of FreeRTOS */
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#endif /* INC_FREERTOS_H */
#include "task.h"
#include <assert.h>
#include "se05x_dev_attest_key_prov.h"

static TaskHandle_t gSSSExRtosTaskHandle = NULL;

#if configAPPLICATION_ALLOCATED_HEAP
uint8_t __attribute__((section(".heap"))) ucHeap[configTOTAL_HEAP_SIZE];
#endif

void se05x_dev_attest_task (void *pvParam);

extern "C" void BOARD_InitHardware(void);

int main (int argc, char * argv[])
{
    BOARD_InitHardware();
    if (xTaskCreate(&se05x_dev_attest_task, "se05x_dev_attest_key_prov_task", 8000, NULL, 2, &gSSSExRtosTaskHandle) != pdPASS) {
        while (1)
        ;
    }
    vTaskStartScheduler();
}

void se05x_dev_attest_task (void *pvParam){
    se05x_dev_attest_key_prov();
}

#if (defined(configCHECK_FOR_STACK_OVERFLOW) && (configCHECK_FOR_STACK_OVERFLOW > 0))
void vApplicationStackOverflowHook(TaskHandle_t xTask, char * pcTaskName)
{
    assert(0);
}
#endif

extern "C" void __wrap_exit(int __status)
{
    assert(0);
}