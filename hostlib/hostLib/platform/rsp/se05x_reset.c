/*
 *
 * Copyright 2019,2024-2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "sm_timer.h"
#include "ax_reset.h"
#include "se05x_reset_apis.h"

#ifndef SE05X_EN_PIN
#define SE05X_EN_PIN 22
#endif

void axReset_HostConfigure()
{
    int fd;
    char buf[50];
    /* Open export file to export GPIO */
    fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd < 0) {
        perror("Failed to open GPIO export file ");
        return;
    }
    /* Export GPIO pin to toggle */
    if (snprintf(buf, sizeof(buf), "%d", SE05X_EN_PIN) < 0) {
        perror("snprintf failed");
        return;
    }
    if (write(fd, buf, strlen(buf)) < 1) {
        perror("Failed to export Enable pin ");
        goto exit;
    }
    close(fd);

    /* Open direction file to configure GPIO direction */
    if (snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d/direction", SE05X_EN_PIN) < 0) {
        perror("snprintf failed");
        return;
    }
    printf("**** buf **** = %s\n", buf);
    fd = open(buf, O_WRONLY);
    if (fd < 0) {
        sm_usleep(1000 * 1000);
        fd = open(buf, O_WRONLY);
        if (fd < 0) {
            axReset_HostUnconfigure();
            perror("Failed to open GPIO direction file ");
            return;
        }
    }
    /* Configure direction of exported GPIO */
    if (write(fd, "out", 3) < 1) {
        perror("Failed to Configure Enable pin ");
        axReset_HostUnconfigure();
        goto exit;
    }

exit:
    close(fd);
    return;
}

void axReset_HostUnconfigure()
{
    int fd;
    char buf[50];
    fd = open("/sys/class/gpio/unexport", O_WRONLY);
    if (fd < 0) {
        perror("Failed to open unexport file ");
        return;
    }

    if (snprintf(buf, sizeof(buf), "%d", SE05X_EN_PIN) < 0) {
        perror("snprintf error");
        return;
    }
    if (write(fd, buf, strlen(buf)) < 1) {
        perror("Failed to unexport GPIO ");
    }

    close(fd);
    return;
}

/*
 * Where applicable, PowerCycle the SE
 *
 * Pre-Requisite: @ref axReset_Configure has been called
 */
void axReset_ResetPulseDUT(int reset_logic)
{
    axReset_PowerDown(reset_logic);
    sm_usleep(2000);
    axReset_PowerUp(reset_logic);
    return;
}

/*
 * Where applicable, put SE in low power/standby mode
 *
 * Pre-Requisite: @ref axReset_Configure has been called
 */
void axReset_PowerDown(int reset_logic)
{
    int fd;
    char buf[50];
    char logic[10];
    if (snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d/value", SE05X_EN_PIN) < 0) {
        perror("snprintf failed");
        return;
    }
    fd = open(buf, O_WRONLY);
    if (fd < 0) {
        perror("Failed to open GPIO value file ");
        axReset_HostUnconfigure();
        return;
    }

    if (snprintf(logic, sizeof(logic), "%d", !reset_logic) < 0) {
        perror("snprintf failed");
        return;
    }
    if (write(fd, logic, 1) < 1) {
        perror("Failed to toggle GPIO high ");
        axReset_HostUnconfigure();
    }

    close(fd);
}

/*
 * Where applicable, put SE in powered/active mode
 *
 * Pre-Requisite: @ref axReset_Configure has been called
 */
void axReset_PowerUp(int reset_logic)
{
    int fd;
    char buf[50];
    char logic[10];
    if (snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d/value", SE05X_EN_PIN) < 0) {
        perror("snprintf failed");
        return;
    }
    fd = open(buf, O_WRONLY);
    if (fd < 0) {
        perror("Failed to open GPIO value file ");
        axReset_HostUnconfigure();
        return;
    }

    if (snprintf(logic, sizeof(logic), "%d", reset_logic) < 0) {
        perror("snprintf failed");
        return;
    }
    if (write(fd, logic, 1) < 1) {
        perror("Failed to toggle GPIO high ");
        axReset_HostUnconfigure();
    }

    close(fd);
}

#if SSS_HAVE_APPLET_SE05X_IOT || SSS_HAVE_APPLET_LOOPBACK

#define SE05X_RESET_CHECK_52F_VERSION(app_ver) ((((app_ver >> 8) & 0xFF) >= 0x10) && (((app_ver >> 8) & 0xFF) <= 0x1F))

void se05x_ic_reset(uint32_t applet_version)
{
    if (SE05X_RESET_CHECK_52F_VERSION(applet_version)){
        axReset_ResetPulseDUT(0);
    }
    else {
        axReset_ResetPulseDUT(SE_RESET_LOGIC);
    }

    smComT1oI2C_ComReset(NULL);
    sm_usleep(3000);
    return;
}

#endif
