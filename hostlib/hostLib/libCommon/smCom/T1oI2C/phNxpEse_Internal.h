/*
 * Copyright 2024 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PHNXPESE_INTERNAL_H_
#define _PHNXPESE_INTERNAL_H_

#include <phNxpEse_Api.h>
#include <i2c_a7.h>

#ifdef T1oI2C_UM1225_SE050
/* MW version 02.13.00 onwards */
#   error Do not define T1oI2C_UM1225_SE050, define T1oI2C_UM11225 instead.
#endif

/********************* Definitions and structures *****************************/

typedef enum
{
   ESE_STATUS_CLOSE = 0x00,
   ESE_STATUS_BUSY,
   ESE_STATUS_RECOVERY,
   ESE_STATUS_IDLE,
   ESE_STATUS_OPEN,
} phNxpEse_LibStatus;

/* I2C Control structure */
typedef struct phNxpEse_Context
{
    phNxpEse_LibStatus   EseLibStatus;      /* Indicate if Ese Lib is open or closed */
    void *pDevHandle;

    uint8_t p_read_buff[MAX_DATA_LEN];
    uint16_t cmd_len;
    uint8_t p_cmd_data[MAX_DATA_LEN];
    phNxpEse_initParams initParams;
} phNxpEse_Context_t;


ESESTATUS phNxpEse_WriteFrame(void* conn_ctx, uint32_t data_len, const uint8_t *p_data);
ESESTATUS phNxpEse_read(void* conn_ctx, uint32_t *data_len, uint8_t **pp_data);
void phNxpEse_clearReadBuffer(void* conn_ctx);
void phNxpEse_waitForWTX(void* conn_ctx);

#endif /* _PHNXPESE_INTERNAL_H_ */
