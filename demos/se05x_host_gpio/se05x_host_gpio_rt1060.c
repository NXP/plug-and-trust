/*
 *
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <nxLog_App.h>
#include <stdbool.h>
#include "se05x_host_gpio.h"
#include "board.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "fsl_adapter_gpio.h"
#include "fsl_iomuxc.h"

/***********************************************************************************************************************
 * Definitions
 **********************************************************************************************************************/
#if defined(CONFIG_SE05X_HOST_GPIO)
#define SE05X_NOTIFY_GPIO_PORT          1U
#define SE05X_NOTIFY_GPIO_PIN           2U

#define GPIO_HANDLER_TASK_PRIORITY      (configMAX_PRIORITIES - 2)
#define GPIO_HANDLER_TASK_STACK_SIZE    512

/***********************************************************************************************************************
 * Variables
 **********************************************************************************************************************/
static TaskHandle_t g_GpioHandlerTaskHandle = NULL;
static SemaphoreHandle_t g_GpioInterruptSemaphore = NULL;
static volatile bool g_TaskRunning = false;
static GPIO_HANDLE_DEFINE(s_GpioHandle);  // HAL GPIO handle
static TaskHandle_t g_DeinitWaitingTask = NULL;

/***********************************************************************************************************************
 * Prototypes
 **********************************************************************************************************************/
static void SE05X_NotifyTask(void *pvParameters);
static void SE05X_HandleFallingEdge(void);
static void SE05X_GpioCallback(void *param);

/***********************************************************************************************************************
 * Code
 **********************************************************************************************************************/

/*!
 * @brief GPIO callback function (called from HAL GPIO adapter ISR context)
 */
static void SE05X_GpioCallback(void *param)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* Signal the handler task that an interrupt occurred */
    if (g_GpioInterruptSemaphore != NULL)
    {
        xSemaphoreGiveFromISR(g_GpioInterruptSemaphore, &xHigherPriorityTaskWoken);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/*!
 * @brief Handle falling edge detection in task context
 */
static void SE05X_HandleFallingEdge(void)
{
    /* Restart Matter application to handle the SE05x notification */
    LOG_I("RT1060: SE05X notification received, restarting application...");

    /* Small delay to ensure log messages are flushed to output */
    vTaskDelay(pdMS_TO_TICKS(100));

    /* Perform system reset - this is a safe operation in task context */
    NVIC_SystemReset();

    /* Should never reach here after reset */
    while(1);
}

/*!
 * @brief SE05X GPIO interrupt handler task
 */
static void SE05X_NotifyTask(void *pvParameters)
{
    (void)pvParameters;

    while (g_TaskRunning)
    {
        /* Block waiting for the semaphore from ISR */
        if (xSemaphoreTake(g_GpioInterruptSemaphore, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            /* Semaphore received - an interrupt occurred */
            if (!g_TaskRunning)
            {
                LOG_I("RT1060: SE05X GPIO deinit called");
                break;
            }
            /* Process the interrupt event in task context */
            SE05X_HandleFallingEdge();
        }
    }
    if (g_DeinitWaitingTask != NULL)
    {
        xTaskNotifyGive(g_DeinitWaitingTask);
    }
    g_GpioHandlerTaskHandle = NULL;
    vTaskDelete(NULL);
}
#endif

/*!
 * @brief Initialize SE05X GPIO notification system for RT1060
 * @return NULL on success, non-NULL on failure (to match void* signature)
 */
void * se05x_host_gpio_notification_monitor_init(void * arg)
{
    (void)arg;  // Unused parameter
#if defined(CONFIG_SE05X_HOST_GPIO)

    hal_gpio_pin_config_t gpio_config = {
        .port = SE05X_NOTIFY_GPIO_PORT,
        .pin = SE05X_NOTIFY_GPIO_PIN,
        .direction = kHAL_GpioDirectionIn,
        .level = 0U
    };

    /* Create binary semaphore for interrupt signaling */
    g_GpioInterruptSemaphore = xSemaphoreCreateBinary();
    if (g_GpioInterruptSemaphore == NULL)
    {
        LOG_E("RT1060: Failed to create GPIO interrupt semaphore");
        return (void *)(intptr_t)-1;
    }

    /* Enable IOMUXC clock */
    CLOCK_EnableClock(kCLOCK_Iomuxc);

    /* RT1060: Configure IO MUX for GPIO1_IO02 (notification pin) */
    IOMUXC_SetPinMux(IOMUXC_GPIO_AD_B0_02_GPIO1_IO02, 0U);

    /* Initialize GPIO using HAL adapter */
    if (HAL_GpioInit((hal_gpio_handle_t)s_GpioHandle, &gpio_config) != kStatus_HAL_GpioSuccess)
    {
        LOG_E("RT1060: Failed to initialize GPIO");
        vSemaphoreDelete(g_GpioInterruptSemaphore);
        g_GpioInterruptSemaphore = NULL;
        return (void *)(intptr_t)-1;
    }

    /* Install callback for GPIO interrupt */
    if (HAL_GpioInstallCallback((hal_gpio_handle_t)s_GpioHandle,
                                SE05X_GpioCallback,
                                NULL) != kStatus_HAL_GpioSuccess)
    {
        LOG_E("RT1060: Failed to install GPIO callback");
        HAL_GpioDeinit((hal_gpio_handle_t)s_GpioHandle);
        vSemaphoreDelete(g_GpioInterruptSemaphore);
        g_GpioInterruptSemaphore = NULL;
        return (void *)(intptr_t)-1;
    }

    /* Set trigger mode to falling edge */
    if (HAL_GpioSetTriggerMode((hal_gpio_handle_t)s_GpioHandle,
                               kHAL_GpioInterruptFallingEdge) != kStatus_HAL_GpioSuccess)
    {
        LOG_E("RT1060: Failed to set GPIO trigger mode");
        HAL_GpioDeinit((hal_gpio_handle_t)s_GpioHandle);
        vSemaphoreDelete(g_GpioInterruptSemaphore);
        g_GpioInterruptSemaphore = NULL;
        return (void *)(intptr_t)-1;
    }

    /* Set task running flag before creating the task */
    g_TaskRunning = true;

    /* Create the interrupt handler task */
    if (xTaskCreate(SE05X_NotifyTask,
                    "SE05X_IntHandler",
                    GPIO_HANDLER_TASK_STACK_SIZE,
                    NULL,
                    GPIO_HANDLER_TASK_PRIORITY,
                    &g_GpioHandlerTaskHandle) != pdPASS)
    {
        LOG_E("RT1060: Failed to create SE05X GPIO interrupt handler task");

        /* Cleanup on failure */
        HAL_GpioDeinit((hal_gpio_handle_t)s_GpioHandle);
        vSemaphoreDelete(g_GpioInterruptSemaphore);
        g_GpioInterruptSemaphore = NULL;
        g_TaskRunning = false;
        return (void *)(intptr_t)-1;
    }
#else
    LOG_I("RT1060: CONFIG_SE05X_HOST_GPIO not defined,skipping GPIO notification init");
    return (void *)(intptr_t)-1;
#endif
    return NULL;  // Success
}

/*!
 * @brief Deinitialize SE05X GPIO notification system for RT1060
 */
void se05x_host_gpio_notification_monitor_deinit(void)
{
#if defined(CONFIG_SE05X_HOST_GPIO)
    if (!g_TaskRunning)
    {
        return;
    }

    /* Stop the task first */
    g_TaskRunning = false;

    g_DeinitWaitingTask = xTaskGetCurrentTaskHandle();

    /* Disable GPIO interrupt through HAL adapter (this also disables NVIC) */
    HAL_GpioSetTriggerMode((hal_gpio_handle_t)s_GpioHandle, kHAL_GpioInterruptDisable);

    /* Give semaphore to unblock task if waiting */
    if (g_GpioInterruptSemaphore != NULL)
    {
        xSemaphoreGive(g_GpioInterruptSemaphore);

        /* Wait for task to exit gracefully */
        if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(500)) == 0)
        {
            if (g_GpioHandlerTaskHandle != NULL)
            {
                vTaskDelete(g_GpioHandlerTaskHandle);
                g_GpioHandlerTaskHandle = NULL;
            }
        }

        /* Delete semaphore */
        vSemaphoreDelete(g_GpioInterruptSemaphore);
        g_GpioInterruptSemaphore = NULL;
    }

    g_DeinitWaitingTask = NULL;

    /* Deinitialize GPIO using HAL adapter */
    HAL_GpioDeinit((hal_gpio_handle_t)s_GpioHandle);

    LOG_I("RT1060: SE05X GPIO notification deinitialized");
#else
    LOG_I("RT1060: CONFIG_SE05X_HOST_GPIO not defined");
#endif
}

int se05x_host_gpio_power_init()
{
    LOG_I("se05x_host_gpio_power_init - do nothing");
    return 0;
}

int se05x_host_gpio_power_deinit()
{
    LOG_I("se05x_host_gpio_power_deinit - do nothing");
    return 0;
}

int se05x_host_gpio_power_set(bool is_high)
{
#if defined(CONFIG_SE05X_HOST_GPIO)
    GPIO_PinWrite(GPIO1, 3U, (uint8_t) is_high);
#else
    LOG_I("RT1060: CONFIG_SE05X_HOST_GPIO not defined");
#endif
    return 0;
}
