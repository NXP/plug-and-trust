/*
 *
 * Copyright 2025-2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <fcntl.h>
#include <nxLog_App.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(CONFIG_SE05X_HOST_GPIO_FRDM_IMX93) && CONFIG_SE05X_HOST_GPIO_FRDM_IMX93 == 1
#include <unistd.h>
#endif
#include "se05x_host_gpio.h"

#if defined(CONFIG_SE05X_HOST_GPIO_FRDM_IMX93) && CONFIG_SE05X_HOST_GPIO_FRDM_IMX93 == 1
#include <gpiod.h>
struct gpiod_line_request * request = NULL;
#define GPIO_NAME "/dev/gpiochip0"
#define GPIO_NOTIF_PIN 6  // Pin number 31 on P11 connector
#define GPIO_ENABLE_PIN 5 // Pin number 29 on P11 connector
static volatile bool g_exit_gpio_thread = false;
#endif

#if defined(CONFIG_SE05X_HOST_GPIO_RPI) && CONFIG_SE05X_HOST_GPIO_RPI == 1
// RPI GPIO configuration
#define GPIO_NUMBER "529" // Pin number 11 / GPIO17
#define GPIO_DIR "/sys/class/gpio"
#endif

int se05x_host_gpio_power_init(void)
{
#if defined(CONFIG_SE05X_HOST_GPIO_RPI) && CONFIG_SE05X_HOST_GPIO_RPI == 1
    int fd            = -1;
    int bytes_written = 0;
    char path[64]     = { 0 };
    int ret           = -1;

    // Export the GPIO
    fd = open(GPIO_DIR "/export", O_WRONLY);
    if (fd < 0)
    {
        if (errno == EBUSY)
        {
            // Already exported, not a fatal error
            LOG_W("GPIO %s already exported.\n", GPIO_NUMBER);
        }
        else
        {
            LOG_E("Failed to open GPIO export: %s", strerror(errno));
            return -1;
        }
    }
    else
    {
        bytes_written = write(fd, GPIO_NUMBER, strlen(GPIO_NUMBER));
        if (bytes_written < 0)
        {
            LOG_E("Failed to write to GPIO export: %s", strerror(errno));
            close(fd);
            return -1;
        }
        close(fd);
        fd = -1;

        // Small delay to allow sysfs to create the GPIO files
        usleep(100000); // 100ms
    }

    // Set direction to output
    ret = snprintf(path, sizeof(path), GPIO_DIR "/gpio%s/direction", GPIO_NUMBER);
    if (ret < 0 || ret >= (int) sizeof(path))
    {
        LOG_E("Failed to construct GPIO direction path");
        return -1;
    }
    fd = open(path, O_WRONLY);
    if (fd < 0)
    {
        LOG_E("Failed to open GPIO direction file: %s", strerror(errno));
        return -1;
    }

    bytes_written = write(fd, "out", 3);
    if (bytes_written < 0)
    {
        LOG_E("Failed to set GPIO direction: %s", strerror(errno));
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
#elif defined(CONFIG_SE05X_HOST_GPIO_FRDM_IMX93) && CONFIG_SE05X_HOST_GPIO_FRDM_IMX93 == 1
    struct gpiod_line_settings * settings = NULL;
    struct gpiod_line_config * line_cfg   = NULL;
    struct gpiod_chip * chip              = NULL;
    const unsigned int line_offset        = GPIO_ENABLE_PIN;
    int ret                               = -1;

    // Validate that request is not already initialized
    if (request != NULL)
    {
        LOG_W("GPIO already initialized, releasing previous request");
        gpiod_line_request_release(request);
        request = NULL;
    }

    chip = gpiod_chip_open(GPIO_NAME);
    if (!chip)
    {
        LOG_E("Failed to open GPIO chip at path %s", GPIO_NAME);
        return ret;
    }

    settings = gpiod_line_settings_new();
    if (!settings)
    {
        LOG_E("Failed to allocate line settings");
        goto cleanup_chip;
    }

    ret = gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
    if (ret != 0)
    {
        LOG_E("Failed to set line direction to output");
        goto cleanup_settings;
    }

    line_cfg = gpiod_line_config_new();
    if (!line_cfg)
    {
        LOG_E("Failed to allocate line config");
        ret = -1;
        goto cleanup_settings;
    }

    ret = gpiod_line_config_add_line_settings(line_cfg, &line_offset, 1, settings);
    if (ret != 0)
    {
        LOG_E("Failed to add line settings to config");
        goto cleanup_line_cfg;
    }

    request = gpiod_chip_request_lines(chip, NULL, line_cfg);
    if (!request)
    {
        LOG_E("Failed to request GPIO lines");
        ret = -1;
        goto cleanup_line_cfg;
    }

    ret = 0;

cleanup_line_cfg:
    gpiod_line_config_free(line_cfg);
cleanup_settings:
    gpiod_line_settings_free(settings);
cleanup_chip:
    gpiod_chip_close(chip);

    return ret;
#else
    LOG_I("se05x_host_gpio_power_init not implemented for this platform.");
    return 0;
#endif
}

int se05x_host_gpio_power_deinit(void)
{
#if defined(CONFIG_SE05X_HOST_GPIO_RPI) && CONFIG_SE05X_HOST_GPIO_RPI == 1
    int fd            = -1;
    int bytes_written = 0;

    fd = open(GPIO_DIR "/unexport", O_WRONLY);
    if (fd < 0)
    {
        LOG_E("Failed to open GPIO unexport: %s", strerror(errno));
        return -1;
    }

    bytes_written = write(fd, GPIO_NUMBER, strlen(GPIO_NUMBER));
    if (bytes_written < 0)
    {
        LOG_E("Failed to write to GPIO unexport: %s", strerror(errno));
        close(fd);
        return -1;
    }

    close(fd);
#elif defined(CONFIG_SE05X_HOST_GPIO_FRDM_IMX93) && CONFIG_SE05X_HOST_GPIO_FRDM_IMX93 == 1
    if (request == NULL)
    {
        LOG_W("GPIO request is already NULL, nothing to deinitialize");
        return 0;
    }
    gpiod_line_request_release(request);
    request = NULL;
#else
    LOG_I("se05x_host_gpio_power_deinit not implemented for this platform.");
#endif
    return 0;
}

int se05x_host_gpio_power_set(bool is_high)
{
#if defined(CONFIG_SE05X_HOST_GPIO_RPI) && CONFIG_SE05X_HOST_GPIO_RPI == 1

    char path[64]        = { 0 };
    const char * val_str = NULL;
    int bytes_written    = 0;
    int fd               = -1;
    int ret              = -1;

    ret = snprintf(path, sizeof(path), GPIO_DIR "/gpio%s/value", GPIO_NUMBER);
    if (ret < 0 || ret >= (int) sizeof(path))
    {
        LOG_E("Failed to construct GPIO value path");
        return -1;
    }

    fd = open(path, O_WRONLY);
    if (fd < 0)
    {
        LOG_E("Failed to open GPIO value file: %s", strerror(errno));
        return -1;
    }

    val_str       = is_high ? "1" : "0";
    bytes_written = write(fd, val_str, 1);
    if (bytes_written < 0)
    {
        LOG_E("Failed to write GPIO value: %s", strerror(errno));
        close(fd);
        return -1;
    }

    close(fd);
#elif defined(CONFIG_SE05X_HOST_GPIO_FRDM_IMX93) && CONFIG_SE05X_HOST_GPIO_FRDM_IMX93 == 1
    int ret                        = 0;
    const unsigned int line_offset = GPIO_ENABLE_PIN;

    if (request == NULL)
    {
        LOG_E("GPIO request is NULL");
        return -1;
    }
#if defined(CONFIG_SE05X_BOARD_H2) && CONFIG_SE05X_BOARD_H2 == 1
    ret = gpiod_line_request_set_value(request, line_offset, is_high ? GPIOD_LINE_VALUE_INACTIVE : GPIOD_LINE_VALUE_ACTIVE);
#else
    ret = gpiod_line_request_set_value(request, line_offset, is_high ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE);
#endif
    if (ret != 0)
    {
        LOG_E("Failed to set value");
        gpiod_line_request_release(request);
        return -1;
    }
#else
    LOG_I("se05x_host_gpio_power_set not implemented for this platform.");
#endif
    return 0;
}

void * se05x_host_gpio_notification_monitor_init(void * arg)
{
#if defined(CONFIG_SE05X_HOST_GPIO_FRDM_IMX93) && CONFIG_SE05X_HOST_GPIO_FRDM_IMX93 == 1
    struct gpiod_chip * chip                     = NULL;
    struct gpiod_line_settings * settings        = NULL;
    struct gpiod_line_config * line_cfg          = NULL;
    struct gpiod_line_request * callback_request = NULL;
    struct gpiod_edge_event_buffer * event_buf   = NULL;
    struct gpiod_edge_event * event              = NULL;
    int ret;
    const unsigned int line_offset = GPIO_NOTIF_PIN;
    (void) arg;

    /* Open GPIO chip */
    chip = gpiod_chip_open(GPIO_NAME);
    if (!chip)
    {
        LOG_E("Failed to open GPIO chip %s", GPIO_NAME);
        return (void *) (intptr_t) -1;
    }

    /* Line settings */
    settings = gpiod_line_settings_new();
    if (!settings)
    {
        LOG_E("Failed to create line settings");
        goto cleanup;
    }

    ret = gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
    if (ret != 0)
    {
        LOG_E("Failed to set line direction");
        goto cleanup;
    }

    ret = gpiod_line_settings_set_edge_detection(settings, GPIOD_LINE_EDGE_FALLING);
    if (ret != 0)
    {
        LOG_E("Failed to set edge detection");
        goto cleanup;
    }

    /* Line config */
    line_cfg = gpiod_line_config_new();
    if (!line_cfg)
    {
        LOG_E("Failed to create line config");
        goto cleanup;
    }

    ret = gpiod_line_config_add_line_settings(line_cfg, &line_offset, 1, settings);
    if (ret != 0)
    {
        LOG_E("Failed to add line settings to config");
        goto cleanup;
    }

    /* Request the line */
    callback_request = gpiod_chip_request_lines(chip, NULL, line_cfg);
    if (!callback_request)
    {
        LOG_E("Failed to request GPIO lines");
        goto cleanup;
    }

    /* Event buffer */
    event_buf = gpiod_edge_event_buffer_new(1);
    if (!event_buf)
    {
        LOG_E("Failed to create event buffer");
        goto cleanup;
    }

    LOG_I("Waiting for GPIO callback on line %u...", line_offset);

    while (!g_exit_gpio_thread)
    {
        ret = gpiod_line_request_wait_edge_events(callback_request, 1000);
        if (ret < 0)
        {
            LOG_E("wait_edge_events failed: %s", strerror(errno));
            break;
        }

        if (ret == 0)
        {
            // Timeout - no events available
            continue;
        }

        ret = gpiod_line_request_read_edge_events(callback_request, event_buf, 1);
        if (ret < 0)
        {
            LOG_E("read_edge_events failed");
            break;
        }

        event = gpiod_edge_event_buffer_get_event(event_buf, 0);
        if (!event)
        {
            LOG_E("Failed to get event from buffer");
            continue;
        }

        if (gpiod_edge_event_get_event_type(event) == GPIOD_EDGE_EVENT_FALLING_EDGE)
        {
            LOG_I("GPIO Callback notification detected, restarting application...");
            sleep(1);

            // Cleanup before restart
            gpiod_edge_event_buffer_free(event_buf);
            gpiod_line_request_release(callback_request);
            gpiod_line_config_free(line_cfg);
            gpiod_line_settings_free(settings);
            gpiod_chip_close(chip);

            char * new_argv[] = { "/proc/self/exe", "--wifi", "--thread", NULL };
            // Restart the current application
            if (execvp("/proc/self/exe", new_argv) == -1)
            {
                LOG_E("Failed to restart application: %s", strerror(errno));
                return (void *) (intptr_t) -1;
            }
        }
    }

    LOG_I("Exit from gpio monitor thread");

cleanup:
    /* Cleanup */
    if (event_buf)
    {
        gpiod_edge_event_buffer_free(event_buf);
    }
    if (callback_request)
    {
        gpiod_line_request_release(callback_request);
    }
    if (line_cfg)
    {
        gpiod_line_config_free(line_cfg);
    }
    if (settings)
    {
        gpiod_line_settings_free(settings);
    }
    if (chip)
    {
        gpiod_chip_close(chip);
    }
#else
    (void) arg;
    LOG_I("se05x_host_gpio_notification_monitor_init not implemented for this platform.");
#endif
    return (void *) (intptr_t) 0;
}

void se05x_host_gpio_notification_monitor_deinit(void)
{
#if defined(CONFIG_SE05X_HOST_GPIO_FRDM_IMX93) && CONFIG_SE05X_HOST_GPIO_FRDM_IMX93 == 1
    g_exit_gpio_thread = true;
#else
    LOG_I("se05x_host_gpio_notification_monitor_deinit not implemented for this platform.");
#endif
}
