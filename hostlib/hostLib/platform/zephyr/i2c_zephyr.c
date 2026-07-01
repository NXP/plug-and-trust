/*
 *
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * I2C implmentation for Zephyr
 */

/* ********************** Include files ********************** */
#include <stdlib.h>
#include "i2c_a7.h"
#include "se05x_enums.h"
#include <zephyr/drivers/i2c.h>
#define SIMW_LOG_REGISTER
#include "sm_port.h"

/* ********************** Defines ********************** */
#define I2C_DEV_NODE DT_ALIAS(se05x_i2c)
#define SE05X_I2C_DEV_ADDR 0x48

/* ********************** Global variables ********************** */
const struct device * i2c_dev = NULL;

/* ********************** Functions ********************** */

/* Backoff Delay */
static int gBackoffDelay;

void resetBackoffDelay()
{
    gBackoffDelay = 0;
}

static void BackOffDelay_Wait()
{
    if (gBackoffDelay < 200)
    {
        gBackoffDelay += 1;
    }
    k_msleep(gBackoffDelay);
}

/**
 * Opens the communication channel to I2C device
 */
i2c_error_t axI2CInit(void ** conn_ctx, const char * pDevName)
{
    uint32_t i2c_cfg = I2C_SPEED_SET(I2C_SPEED_FAST_PLUS) | I2C_MODE_CONTROLLER;

    i2c_dev = DEVICE_DT_GET(I2C_DEV_NODE);
    if (!i2c_dev)
    {
        SMLOG_E("Error in i2c device_get_binding \n");
        return I2C_FAILED;
    }

    /* i2c configuration() */
    if (i2c_configure(i2c_dev, i2c_cfg))
    {
        SMLOG_E("Error in i2c_configure \n");
        return I2C_FAILED;
    }

    return I2C_OK;
}

void axI2CTerm(void * conn_ctx, int mode)
{
    return;
}

unsigned int axI2CWrite(void * conn_ctx, unsigned char bus_unused_param, unsigned char addr, unsigned char * pTx,
                        unsigned short txLen)
{
    unsigned int rv;
    if (i2c_write(i2c_dev, pTx, txLen, SE05X_I2C_DEV_ADDR))
    {
        // SMLOG_E("i2c write failed\n");
        rv = I2C_FAILED;
    } else {
        resetBackoffDelay();
        rv = I2C_OK;
    }
    return rv;
}

unsigned int axI2CRead(void * conn_ctx, unsigned char bus, unsigned char addr, unsigned char * pRx, unsigned short rxLen)
{
    unsigned int rv;

    if (i2c_read(i2c_dev, pRx, rxLen, SE05X_I2C_DEV_ADDR))
    {
        // SMLOG_E("i2c read failed");
        // SMLOG_E("Attempting I2C bus recovery");
        i2c_recover_bus(i2c_dev);
        BackOffDelay_Wait();
        rv = I2C_FAILED;
    } else {
        resetBackoffDelay();
        rv = I2C_OK;
    }
    return rv;
}