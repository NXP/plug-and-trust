/*
 *
 * Copyright 2019,2024 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file
 *
 * ex_sss_ports.h:  Default ports being used in Examples and test cases
 *
 * $Date: Mar 10, 2019 $
 * $Author: ing05193 $
 * $Revision$
 */

#ifndef SSS_EX_INC_EX_SSS_PORTS_H_
#define SSS_EX_INC_EX_SSS_PORTS_H_

/* *****************************************************************************************************************
 *   Includes
 * ***************************************************************************************************************** */

/* *****************************************************************************************************************
 * MACROS/Defines
 * ***************************************************************************************************************** */

#define EX_SSS_BOOT_SSS_PORT "EX_SSS_BOOT_SSS_PORT"
#ifdef __linux__
#define EX_SSS_BOOT_SSS_COMPORT_DEFAULT "/dev/ttyACM0"
#else
#define EX_SSS_BOOT_SSS_COMPORT_DEFAULT "\\\\.\\COM7"
#endif
#define EX_SSS_BOOT_SSS_SOCKET_HOSTNAME_DEFAULT "127.0.0.1"
#ifdef SMCOM_JRCP_V1_AM
#define EX_SSS_BOOT_SSS_SOCKET_PORTSZ_DEFAULT "8040"
#define EX_SSS_BOOT_SSS_SOCKET_PORTNUMBER_DEFAULT 8040
#else
#define EX_SSS_BOOT_SSS_SOCKET_PORTSZ_DEFAULT "8050"
#define EX_SSS_BOOT_SSS_SOCKET_PORTNUMBER_DEFAULT 8050
#endif
#define EX_SSS_BOOT_SSS_PCSC_READER_DEFAULT "NXP SE050C v03.01.00 0"
#ifdef ACCESS_MGR_UNIX_SOCKETS
#define EX_SSS_BOOT_SSS_SOCKETPORT_DEFAULT "/var/run/am"
#else
#define EX_SSS_BOOT_SSS_SOCKETPORT_DEFAULT  \
    EX_SSS_BOOT_SSS_SOCKET_HOSTNAME_DEFAULT \
    ":" EX_SSS_BOOT_SSS_SOCKET_PORTSZ_DEFAULT
#endif

/* *****************************************************************************************************************
 * Types/Structure Declarations
 * ***************************************************************************************************************** */

/* *****************************************************************************************************************
 *   Extern Variables
 * ***************************************************************************************************************** */

/* *****************************************************************************************************************
 *   Function Prototypes
 * ***************************************************************************************************************** */

#endif /* SSS_EX_INC_EX_SSS_PORTS_H_ */
