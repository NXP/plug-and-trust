/*
 *
 * Copyright 2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <nxLog_App.h>

#if defined (SE05X_HOST_GPIO_FRDM_IMX93) && SE05X_HOST_GPIO_FRDM_IMX93 == 1
#include <gpiod.h>
struct gpiod_line_request *request = NULL;   // Pin number 29 on P11 connector
#endif

#if defined(SE05X_HOST_GPIO_RPI) && SE05X_HOST_GPIO_RPI == 1
// RPI GPIO configuration
#define GPIO_NUMBER "529"   // Pin number 11 / GPIO17
#define GPIO_DIR "/sys/class/gpio"
#endif

int se05x_host_gpio_init()
{
#if defined(SE05X_HOST_GPIO_RPI) && SE05X_HOST_GPIO_RPI == 1
    int fd = 0;
    int bytes_written = 0;
    char path[64]    = {0};

    // Export the GPIO
    fd = open(GPIO_DIR "/export", O_WRONLY);
    if (fd < 0) {
        if (errno == EBUSY) {
            // Already exported, not a fatal error
            LOG_W("GPIO %s already exported.\n", GPIO_NUMBER);
        }
        else {
            LOG_E("Error opening export");
            return -1;
        }
    }
    else {
        bytes_written = write(fd, GPIO_NUMBER, strlen(GPIO_NUMBER));
        if (bytes_written < 0) {
            LOG_E("Error writing to export");
            close(fd);
            return -1;
        }
        close(fd);
    }

    // Set direction
    snprintf(path, sizeof(path), GPIO_DIR "/gpio%s/direction", GPIO_NUMBER);
    fd = open(path, O_WRONLY);
    if (fd < 0) {
        LOG_E("Error opening direction");
        return -1;
    }

    bytes_written = write(fd, "out", 3);
    if (bytes_written < 0) {
        LOG_E("Error writing direction");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
#elif defined (SE05X_HOST_GPIO_FRDM_IMX93) && SE05X_HOST_GPIO_FRDM_IMX93 == 1
    struct gpiod_line_settings *settings = NULL;
    struct gpiod_line_config *line_cfg = NULL;
    struct gpiod_chip *chip = NULL;
    const char *const chip_path = "/dev/gpiochip0";
    const unsigned int line_offset = 5;
    int ret = -1;

    chip = gpiod_chip_open(chip_path);
    if (!chip) {
        LOG_E("Failed to open GPIO chip at path %s\n", chip_path);
        return ret;
    }

    settings = gpiod_line_settings_new();
    if (!settings) {
        LOG_E("Failed to allocate line settings\n");
        goto cleanup_chip;
    }

    ret = gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
    if (ret != 0) {
        LOG_E("Failed to set line direction to output\n");
        goto cleanup_settings;
    }

    line_cfg = gpiod_line_config_new();
    if (!line_cfg) {
        LOG_E("Failed to allocate line config\n");
        goto cleanup_settings;
    }

    ret = gpiod_line_config_add_line_settings(line_cfg, &line_offset, 1, settings);
    if (ret != 0) {
        LOG_E("Failed to add line settings to config\n");
        goto cleanup_line_cfg;
    }

    request = gpiod_chip_request_lines(chip, NULL, line_cfg);
    if (!request) {
        LOG_E("Failed to request GPIO lines\n");
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
    LOG_I("se05x_host_gpio_init not implemented for this platform. \n");
    return 0;
#endif
}

int se05x_host_gpio_deinit()
{
#if defined(SE05X_HOST_GPIO_RPI) && SE05X_HOST_GPIO_RPI == 1
    int fd = open(GPIO_DIR "/unexport", O_WRONLY);
    int bytes_written = 0;

    if (fd < 0) {
        LOG_E("Error opening unexport");
        return -1;
    }

    bytes_written = write(fd, GPIO_NUMBER, strlen(GPIO_NUMBER));
    if (bytes_written < 0) {
        LOG_E("Error writing to unexport");
        close(fd);
        return -1;
    }

    close(fd);
#elif defined (SE05X_HOST_GPIO_FRDM_IMX93) && SE05X_HOST_GPIO_FRDM_IMX93 == 1
    gpiod_line_request_release(request);
#else
    LOG_I("se05x_host_gpio_deinit not implemented for this platform. \n");
#endif
    return 0;
}

int se05x_host_gpio_set_value(bool value)
{
#if defined(SE05X_HOST_GPIO_RPI) && SE05X_HOST_GPIO_RPI == 1

    char path[64] = {0};
    const char *val_str = NULL;
    int bytes_written = 0;
    int fd = 0;

    snprintf(path, sizeof(path), GPIO_DIR "/gpio%s/value", GPIO_NUMBER);

    fd = open(path, O_WRONLY);
    if (fd < 0) {
        LOG_E("Error opening value");
        return -1;
    }

    val_str   = value ? "1" : "0";
    bytes_written = write(fd, val_str, 1);
    if (bytes_written < 0) {
        LOG_E("Error writing value");
        close(fd);
        return -1;
    }

    close(fd);
#elif defined (SE05X_HOST_GPIO_FRDM_IMX93) && SE05X_HOST_GPIO_FRDM_IMX93 == 1
    int ret = 0;
    const unsigned int line_offset = 5;
    ret = gpiod_line_request_set_value(request, line_offset, value);
    if (ret != 0) {
        LOG_E("Failed to set value\n");
        gpiod_line_request_release(request);
        return -1;
    }
#else
    LOG_I("se05x_host_gpio_set_value not implemented for this platform. \n");
#endif
    return 0;
}
