/*
 *
 * Copyright 2025-2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the host GPIO pin for controlling SE05x power.
 *
 * Configures the GPIO pin as an output to enable power control for the SE05x
 * secure element. This must be called before any GPIO operations.
 *
 * @return 0 on success, -1 on failure.
 */
int se05x_host_gpio_power_init(void);

/**
 * @brief De-initialize the host GPIO pin for SE05x power control.
 *
 * Releases GPIO resources and resets the pin configuration. Should be called
 * during cleanup or shutdown.
 *
 * @return 0 on success, -1 on failure.
 */
int se05x_host_gpio_power_deinit(void);

/**
 * @brief Set the SE05x power control GPIO pin state.
 *
 * Controls the power state of the SE05x secure element by setting the GPIO
 * pin to high (power on) or low (power off).
 *
 * @param[in] is_high true to set pin high (power on), false to set pin low (power off).
 * @return 0 on success, -1 on failure.
 */
int se05x_host_gpio_power_set(bool is_high);

/**
 * @brief Initialize the SE05x GPIO notification monitoring thread.
 *
 * Starts a background thread or task that monitors GPIO interrupt notifications
 * from the SE05x secure element. These notifications can signal events such as
 * command completion or status changes.
 *
 * @param[in] arg Optional argument passed to the monitoring thread (implementation-specific).
 * @return NULL on success, non-NULL error pointer on failure.
 */
void * se05x_host_gpio_notification_monitor_init(void * arg);

/**
 * @brief De-initialize the SE05x GPIO notification monitoring thread.
 *
 * Stops the notification monitoring thread and releases associated resources.
 * Should be called during cleanup to ensure proper shutdown.
 */
void se05x_host_gpio_notification_monitor_deinit(void);

#ifdef __cplusplus
}
#endif
