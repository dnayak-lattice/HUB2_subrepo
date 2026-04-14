/******************************************************************************
 * Copyright (c) 2026 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#ifndef __KT_APP_LOGGER_H__
#define __KT_APP_LOGGER_H__

#include "kt_app.h"
#include "os_funcs.h"

/* Kt App Logger Functions */

/**
 * Initialize the logger
 *
 * @param app_handle The App handle
 * @param log_file The log file
 * @param log_type The log type
 *
 * @return void
 */
void kt_app_log_init(App_Handle  app_handle,
					 FILE      **log_file,
					 const char *log_type);

/**
 * Finalize the logger
 *
 * @param app_handle The App handle
 *
 * @return void
 */
void kt_app_log_fini(App_Handle app_handle);

/**
 * Write a log entry
 *
 * @param app_handle The App handle
 * @param log_type The log type
 * @param log_entry The log entry
 *
 * @return void
 */
void kt_app_log_write(App_Handle  app_handle,
					  const char *log_type,
					  const char *log_entry,
					  ...);

/* Short cuts for logging commands and stream data */
#define cmd_log_write(app_handle, log_entry, ...)                              \
	kt_app_log_write(app_handle, "cmd", log_entry, ##__VA_ARGS__)
#define stream_log_write(app_handle, log_entry, ...)                           \
	kt_app_log_write(app_handle, "stream", log_entry, ##__VA_ARGS__)

#endif /* __KT_APP_LOGGER_H__ */