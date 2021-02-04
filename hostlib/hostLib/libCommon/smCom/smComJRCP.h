/**
 * @file smComSCI2c.h
 * @author NXP Semiconductors
 * @version 1.0
 * @par License
 *
 * Copyright 2016,2020 NXP
 * SPDX-License-Identifier: Apache-2.0
 *
 * @par Description
 * This file provides the API of the SmCom JRCP-V2 communication layer.
 *
 *****************************************************************************/

#ifndef _SMCOMJRCP_H_
#define _SMCOMJRCP_H_

#include "smCom.h"

U16 smComJRCP_Close(void* conn_ctx, U8 mode);

#ifndef TGT_A71CH
/**
 * Initializes or resumes the JRCP communication layer. Deprecated, use smComSCI2C_Open instead
 * @param mode      Either ::ESTABLISH_JRCP to open or re-open communication with a SM, or ::RESUME_JRCP to resume communication (typically handover from boot loader to main OS)
 * @param seqCnt    Ignored in case mode==::ESTABLISH_JRCP; SCI2C protocol seqCnt to set in case communication is resumed.
 * @param SCI2Catr     IN: Pointer to buffer to contain JRCP_ATR value
 * @param SCI2CatrLen  IN: Size of buffer provided; OUT: Actual length of atr retrieved
 * @return
 */
void smComJRCP_Init(void); // Deprecated
#endif

/**
 * Initializes or resumes the JRCP communication layer.
 * @param mode      Either ::ESTABLISH_JRCP to open or re-open communication with a SM, or ::RESUME_JRCP to resume communication (typically handover from boot loader to main OS)
 * @param seqCnt    Ignored in case mode==::ESTABLISH_JRCP; JRCP protocol seqCnt to set in case communication is resumed.
 * @param SCI2Catr     IN: Pointer to buffer to contain JRCP value
 * @param SCI2CatrLen  IN: Size of buffer provided; OUT: Actual length of atr retrieved
 * @return
 */
U16 smComJRCP_Open(void** conn_ctx, const char *hostName, unsigned int portNum);

/**
 * @brief      Send string to JRCP V2 Simulator
 *
 * @param[in]  conn_ctx Connection context
 * @param[in]  comp     The component
 * @param[in]  level    The log level
 * @param[in]  buffer   Pointer to buffer which contain JRCP value
 */
void smComJRCP_Echo(void* conn_ctx, const char *comp, const char *level, const char *buffer);

/**
 * @brief      Reset JRCP communication
 *
 * @param[in]  conn_ctx           Connection context
 * @param[in]  instruction_bytes  The instruction bytes
 *
 * @return      SMCOM Error codes.
 */
U16 smComJRCP_Reset(void* conn_ctx, uint32_t instruction_bytes);

/**
 * @brief      Retrieve NVM count by sending request to JRCP V2 Server.
 *
 * @param      IN:  pCount    Pointer to variable where NVM count to be stored.
 *             IN:  conn_ctx  Connection context
 *             OUT: pCount    NVM count
 *
 * @return     SMCOM Error codes.
 */
U32 smComJRCP_NvmCount(void* conn_ctx, U32 *pCount);

#endif /* _SMCOMJRCP_H_ */
