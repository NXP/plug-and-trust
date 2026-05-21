/*
 *
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <nxLog_App.h>
#include <stdbool.h>
#include "se05x_host_gpio.h"

#include "fsl_common.h"
#include "fsl_port.h"
#include "fsl_gpio.h"
#include "pin_mux.h"
#include "FreeRTOS.h"
#include "task.h"
#include "fsl_adapter_gpio.h"

#ifdef gAppLowpowerEnabled_d
extern void PWR_DisallowDeviceToSleep();
extern void PWR_AllowDeviceToSleep();
#endif

/***********************************************************************************************************************
 * Definitions
 **********************************************************************************************************************/

#if defined(CONFIG_SE05X_HOST_GPIO)
/*Use PTC4 for SE notification control with OM-SE051ARD-H on MCXW72 FRDM*/
/*Connect SEI_O2 on  OM-SE051ARD-H to PTC4 on MCXW72 FRDM*/
#define SE05X_NOTIFY_PORT          PORTC
#define SE05X_NOTIFY_GPIO          GPIOC
#define SE05X_NOTIFY_PIN           4U
/***********************************************************************************************************************
 * Variables
 **********************************************************************************************************************/
GPIO_HANDLE_DEFINE(gpioHandleNotify);
hal_gpio_pin_config_t gpioNotifyPinConfig = {
    .direction = kHAL_GpioDirectionIn,
    .level = 0,
    .port = 2, /*PORTC*/
    .pin = SE05X_NOTIFY_PIN,
};
/***********************************************************************************************************************
 * Prototypes
 **********************************************************************************************************************/
static void SE05X_HandleFallingEdge(void *param);

/***********************************************************************************************************************
 * Code
 **********************************************************************************************************************/

/*!
 * @brief Handle falling edge detection in HAL gpio callback
 */
static void SE05X_HandleFallingEdge(void *param)
{
    /* Add delay before reset to allow any pending operations to complete can be configurable to platforms*/
    SDK_DelayAtLeastUs(90000, CLOCK_GetFreq(kCLOCK_CoreSysClk));
    /* Perform system reset - this is a safe operation in task context */
    NVIC_SystemReset();

    /* Should never reach here after reset */
    while(1);
}
#endif

/*!
 * @brief Initialize SE05X GPIO notification system for MCXW72
 * @return NULL on success, non-NULL on failure (to match void* signature)
 */
void * se05x_host_gpio_notification_monitor_init(void * arg)
{
    (void)arg;  // Unused parameter
#if defined(CONFIG_SE05X_HOST_GPIO)
    LOG_I("MCXW72: se05x_host_gpio_notification_monitor_init");
    /* Clock Configuration: Peripheral clocks are enabled; module does not stall low power mode entry */
    CLOCK_EnableClock(kCLOCK_PortA);

    const port_pin_config_t NOTIFY = {/* Internal pull-up resistor is enabled */
                                   (uint16_t)kPORT_PullUp,
                                   /* Low internal pull resistor value is selected. */
                                   (uint16_t)kPORT_LowPullResistor,
                                   /* Slow slew rate is configured */
                                   (uint16_t)kPORT_SlowSlewRate,
                                   /* Passive input filter is disabled */
                                   (uint16_t)kPORT_PassiveFilterDisable,
                                   /* Open drain output is enabled */
                                   (uint16_t)kPORT_OpenDrainEnable,
                                   /* Low drive strength is configured */
                                   (uint16_t)kPORT_LowDriveStrength,
                                   /* Normal drive strength is configured */
                                   (uint16_t)kPORT_NormalDriveStrength,
                                   /* Pin is configured as PTC4 */
                                   (uint16_t)kPORT_MuxAsGpio,
                                   /* Pin Control Register fields [15:0] are not locked */
                                   (uint16_t)kPORT_UnlockRegister};
    PORT_SetPinConfig(SE05X_NOTIFY_PORT, SE05X_NOTIFY_PIN, &NOTIFY);
    
    HAL_GpioInit(gpioHandleNotify, &gpioNotifyPinConfig);
    HAL_GpioSetTriggerMode(gpioHandleNotify, kHAL_GpioInterruptFallingEdge);
    (void)HAL_GpioInstallCallback(gpioHandleNotify, SE05X_HandleFallingEdge, NULL);
#else
    LOG_I("MCXW72: CONFIG_SE05X_HOST_GPIO not defined, GPIO notification disabled");
    return (void *)(intptr_t)-1;
#endif
    return NULL;  // Success
}

/*!
 * @brief Deinitialize SE05X GPIO notification system for MCXW72
 */
void se05x_host_gpio_notification_monitor_deinit(void)
{
#if defined(CONFIG_SE05X_HOST_GPIO)
    HAL_GpioDeinit(gpioHandleNotify);
    LOG_I("MCXW72: SE05X GPIO notification deinitialized");
#else
    LOG_I("MCXW72: CONFIG_SE05X_HOST_GPIO not defined, GPIO notification deinit skipped");
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
#if defined gAppLowpowerEnabled_d && gAppLowpowerEnabled_d == 1
    if(is_high)
        PWR_DisallowDeviceToSleep();
#endif  
    bool value = false;
#if CONFIG_SE05X_BOARD_H2
    value = (is_high == true) ? false : true;
#else
    value = is_high;
#endif

    /*Use PTC0 for SE ENA control with OM-SE051ARD-H on MCXW72 FRDM*/
    GPIO_PinWrite(GPIOC, 0U, (uint8_t) value);
    /* Add delay to allow SE05x to power up and initialize before I2C communication */
    if (is_high)
    {
        vTaskDelay(pdMS_TO_TICKS(1));  /* 1ms delay for SE05x power-up */
    }
#if defined gAppLowpowerEnabled_d && gAppLowpowerEnabled_d == 1
    if(!is_high)
        PWR_AllowDeviceToSleep();
#endif
    return 0;
}