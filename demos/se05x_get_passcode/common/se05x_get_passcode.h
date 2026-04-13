/*
 *
 * Copyright 2020,2025-2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once
/**
 * @brief Read the pass-code value from the SE051H trust provisioned pass-code
 * binary file.
 * @param[in] pCtx - Boot context. (Passing NULL will ensure the application
 * opens the session to SE).
 * @param[in] passcode_set_no - Pass-code set number in SE051H.
 * @return None
 */
#ifdef __cplusplus
extern "C" {
#endif

#include <ex_sss_boot.h>

void se05x_get_passcode(ex_sss_boot_ctx_t *pCtx, uint8_t passcode_set_no);

#ifdef __cplusplus
}
#endif