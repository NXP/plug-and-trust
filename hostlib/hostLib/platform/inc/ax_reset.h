/*
 *
 * Copyright 2018-2019,2024-2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _AX_RESET_H
#define _AX_RESET_H

#include "sm_types.h"

/*
 * Where applicable, Configure the PINs on the Host
 *
 */
void axReset_HostConfigure(void);

/*
 * Where applicable, PowerCycle the SE
 * @input: reset logic - 0=Active Low. 1= Active High
 *
 * Pre-Requistie: @ref axReset_Configure has been called
 */
void axReset_ResetPulseDUT(int reset_logic);

/*
 * Where applicable, put SE in low power/standby mode
 * @input: reset logic - 0=Active Low. 1= Active High
 *
 * Pre-Requistie: @ref axReset_Configure has been called
 */
void axReset_PowerDown(int reset_logic);

/*
 * Where applicable, put SE in powered/active mode
 * @input: reset logic - 0=Active Low. 1= Active High
 *
 * Pre-Requistie: @ref axReset_Configure has been called
 */
void axReset_PowerUp(int reset_logic);

/*
 * Where applicable, Unconfigure the PINs on the Host
 *
 */
void axReset_HostUnconfigure(void);

#endif // _AX_RESET_H
