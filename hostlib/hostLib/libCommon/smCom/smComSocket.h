/**
 * @file smComSocket.h
 * @author NXP Semiconductors
 * @version 1.1
 * @par License
 * Copyright 2016,2017,2020 NXP
 * SPDX-License-Identifier: Apache-2.0
 *
 * @par Description
 *
 *****************************************************************************/

#ifndef _SCCOMSOCKET_H_
#define _SCCOMSOCKET_H_

#include "smCom.h"

#ifdef __cplusplus
extern "C" {
#endif

U16 smComSocket_Close(void);
U16 smComSocket_Open(void** conn_ctx, U8 *pIpAddrString, U16 portNo, U8 *pAtr, U16 *atrLen);
#if defined(_WIN32) && defined(TGT_A70CU)
U16 smComSocket_Init(U8 *pIpAddrString, U16 portNo, U8 *pAtr, U16 *pAtrLength, U16 maxAtrLength);
#endif
U32 smComSocket_Transceive(void* conn_ctx, apdu_t *pApdu);
U32 smComSocket_TransceiveFD(int fd, apdu_t *pApdu);
U32 smComSocket_TransceiveRaw(void* conn_ctx, U8 *pTx, U16 txLen, U8 *pRx, U32 *pRxLen);
U32 smComSocket_TransceiveRawFD(int fd, U8 *pTx, U16 txLen, U8 *pRx, U32 *pRxLen);
U32 smComSocket_CloseFD(int fd);
U32 smComSocket_GetATRFD(int fd, U8* pAtr, U16* atrLen);
U32 smComSocket_LockChannelFD(int fd);
U32 smComSocket_UnlockChannelFD(int fd);

U32 smComSocket_LockChannel(void);
U32 smComSocket_UnlockChannel(void);

#ifdef __cplusplus
}
#endif
#endif //_SCCOMSOCKET_H_
