/*
 *
 * Copyright 2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the GPIO pin for output.
 * This can be used to control the power for SE05x.
 *
 * @return 0 on success, -1 on failure.
 */
int se05x_host_gpio_init();

/**
 * @brief De-initialize the GPIO pin.
 *
 * @return 0 on success, -1 on failure.
 */
int se05x_host_gpio_deinit();

/**
 * @brief Set the GPIO pin value.
 *
 * @param value 0 for low, 1 for high.
 * @return 0 on success, -1 on failure.
 */
int se05x_host_gpio_set_value(bool value);

#ifdef __cplusplus
}
#endif
