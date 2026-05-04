/** @file sm_port.h
 *  @brief Platform specific content.
 *
 * Copyright 2021,2022,2024,2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SM_PORT_H_INC
#define SM_PORT_H_INC

/* ********************** Include files ********************** */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

/* ********************** Defines ********************** */

#define CONFIG_PLUGANDTRUST_LOG_LEVEL 4

#ifdef SIMW_LOG_REGISTER
LOG_MODULE_REGISTER(simw_lib, CONFIG_PLUGANDTRUST_LOG_LEVEL);
#else
LOG_MODULE_DECLARE(simw_lib, CONFIG_PLUGANDTRUST_LOG_LEVEL);
#endif

#define SMLOG_I(...) LOG_INF(__VA_ARGS__);
#define SMLOG_E(...) LOG_ERR(__VA_ARGS__);
#define SMLOG_W(...) LOG_WRN(__VA_ARGS__);
#define SMLOG_D(...) LOG_DBG(__VA_ARGS__);

#define SMLOG_AU8_D(BUF, LEN) LOG_HEXDUMP_DBG(BUF, LEN, "");
#define SMLOG_MAU8_D(MSG, BUF, LEN) LOG_HEXDUMP_DBG(BUF, LEN, MSG)

#define sm_malloc k_malloc
#define sm_free k_free

#define SM_MUTEX_DEFINE(x) K_MUTEX_DEFINE(x)
#define SM_MUTEX_EXTERN_DEFINE(x) extern struct k_mutex x
#define SM_MUTEX_INIT(x) k_mutex_init(&x)
#define SM_MUTEX_DEINIT(x)
#define SM_MUTEX_LOCK(x) k_mutex_lock(&x, K_FOREVER)
#define SM_MUTEX_UNLOCK(x) k_mutex_unlock(&x)

#ifndef FALSE
#define FALSE false
#endif

#ifndef TRUE
#define TRUE true
#endif

#endif // #ifndef SM_PORT_H_INC