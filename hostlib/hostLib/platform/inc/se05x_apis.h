/*
 *
 * Copyright 2018-2019,2024 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _SE05X_API_H
#define _SE05X_API_H

/*
 * Define Reset logic for reset pin on SE
 * Active high for SE050
 * Change SE_RESET_LOGIC to 0 for SE052
 */
#define SE_RESET_LOGIC 1

void se05x_ic_reset(void);

#endif // _SE05X_API_H
