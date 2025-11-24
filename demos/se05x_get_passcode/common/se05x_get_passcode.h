/*
 *
 * Copyright 2020,2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>

/**
 * @brief Read the pass-code value from the SE051H trust provisioned pass-code
 * binary file.
 * @param[in] passcode_set_no - Pass-code set number in SE051H.
 * @return None
 */
void se05x_get_passcode(uint8_t passcode_set_no);
