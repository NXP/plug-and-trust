/* Copyright 2020,2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

/*TODO Review */
#include "smComSerial.h"
#include <stdlib.h>
#include <stdio.h>
#include "string.h"
#include <assert.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <paths.h>
#include <termios.h>
#include <sysexits.h>
#include <sys/param.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include "sm_printf.h"
#include "smComSocket.h"
#include "nxLog_smCom.h"
#include "inttypes.h"
#include "nxEnsure.h"
#include "sm_timer.h"

#define REMOTE_JC_SHELL_HEADER_LEN (4)
#define REMOTE_JC_SHELL_MSG_TYPE_APDU_DATA (0x01)
#include "sm_apdu.h"
#define MAX_BUF_SIZE (MAX_APDU_BUF_LENGTH)

static int gfileDescriptor = -1;

static int smUartSetInterfaceAttrib(int fd, int speed);


U32 smComVCom_Open(void** vcom_ctx, const char *portname)
{
    LOG_I("Opening %s", portname);

    (void)(vcom_ctx);

    gfileDescriptor = open(portname, O_RDWR | O_NOCTTY);
    LOG_I("gfileDescriptor = %d", gfileDescriptor);
    if (gfileDescriptor < 0)
    {
        LOG_E("error %d opening %s: %s\r\n", errno, portname, strerror (errno));
        goto error;
    }
    if (0 == smUartSetInterfaceAttrib(gfileDescriptor, B115200)) {
        smCom_Init(&smComVCom_Transceive, &smComVCom_TransceiveRaw);
    }
    else {
        LOG_W("smUartSetInterfaceAttrib Failed");
        goto error;
    }
    return 0;

error:
    if (gfileDescriptor != -1) {
        close(gfileDescriptor);
    }

    return 1;
}

U32 smComVCom_Close(void* conn_ctx)
{
    U32 status = 0;

    (void)(conn_ctx);

    smComSocket_CloseFD(gfileDescriptor);

    return status;
}

U32 smComVCom_GetATR(void* conn_ctx, U8 *pAtr, U16 *atrLen)
{
    (void)(conn_ctx);
    return smComSocket_GetATRFD(gfileDescriptor, pAtr, atrLen);
}

U32 smComVCom_Transceive(void* conn_ctx, apdu_t *pApdu)
{
    (void)(conn_ctx);
    return smComSocket_TransceiveFD(gfileDescriptor, pApdu);
}

U32 smComVCom_TransceiveRaw(void* conn_ctx, U8 *pTx, U16 txLen, U8 *pRx, U32 *pRxLen)
{
    (void)(conn_ctx);
    return smComSocket_TransceiveRawFD(gfileDescriptor, pTx, txLen, pRx, pRxLen);
}

static int smUartSetInterfaceAttrib(int fd, int speed)
{
    struct termios SerialPortSettings; /* Create the structure */

    bzero(&SerialPortSettings, sizeof(SerialPortSettings));

    SerialPortSettings.c_cflag = CRTSCTS | CS8 | CLOCAL | CREAD;
    if (cfsetspeed(&SerialPortSettings, speed)) // Set  baud
    {
        LOG_W("cfsetspeed Failed");
    }

    SerialPortSettings.c_iflag = IGNPAR;
    SerialPortSettings.c_oflag = 0;
    SerialPortSettings.c_cc[VMIN] = 1;
    SerialPortSettings.c_cc[VTIME] = 5;

    if (cfsetispeed(&SerialPortSettings, speed)) /* Set Read  Speed */
    {
        LOG_W("cfsetspeed Failed");
    }
    if (cfsetospeed(&SerialPortSettings, speed)) /* Set Write Speed */
    {
        LOG_W("cfsetspeed Failed");
    }
    //tcflush(fd, TCIFLUSH);
    if ((tcsetattr(fd, TCSANOW, &SerialPortSettings)) != 0) /* Set the attributes to the termios structure*/
    {
        LOG_W("Failed Setting attributes");
        return 1;
    }
    else {
        LOG_D("Attributes Set");
        return 0;
    }
}
