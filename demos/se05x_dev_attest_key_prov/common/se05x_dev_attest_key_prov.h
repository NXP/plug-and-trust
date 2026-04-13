/*
 *
 * Copyright 2020,2025-2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <ex_sss_boot.h>

/**
 * @brief provision SE05x secure element with device attestation key pair and certificate.
 * @param[in] pCtx - Boot context. (Passing NULL will ensure the application
 * opens the session to SE).
 * @return None
 */
void se05x_dev_attest_key_prov(ex_sss_boot_ctx_t *pCtx);

#ifdef __cplusplus
}
#endif