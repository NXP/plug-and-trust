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
    return 0;
#else
    LOG_I("se05x_host_gpio_deinit not implemented for this platform. \n");
    return 0;
#endif
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
    return 0;
#else
    LOG_I("se05x_host_gpio_set_value not implemented for this platform. \n");
    return 0;
#endif
}
