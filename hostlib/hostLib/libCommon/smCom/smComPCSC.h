/* Copyright 2018 NXP
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _SMCOMPCSC_H_
#define _SMCOMPCSC_H_

#include "smCom.h"

#define ESTABLISH_SCI2C 0x00
#define RESUME_SCI2C 0x01

/* ------------------------------------------------------------------------- */

U16 smComPCSC_Open(const char *reader_name_in);

U16 smComPCSC_Close(U8 mode);

#endif /* _SMCOMPCSC_H_ */
