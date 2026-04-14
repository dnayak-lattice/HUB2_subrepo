/******************************************************************************
 * Copyright (c) 2026 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#include "kt_app_logger.h"

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
					 const char *log_type)
{
	struct kt_app_ctx *p_app_ctx = (struct kt_app_ctx *)app_handle;
	time_t             now       = time(NULL);
	struct tm          tm_info;
	localtime_r(&now, &tm_info);
	char time_str[32];

	strftime(time_str, sizeof(time_str), "%Y-%m-%d_%H-%M-%S", &tm_info);

	/* Check which log file to initialize */
	if (os_strncmp(log_type, "cmd", strlen("cmd")) == 0) {
		os_snprintf(p_app_ctx->cmd_log_file_path,
					sizeof(p_app_ctx->cmd_log_file_path), "%s/%s",
					KT_APP_LOG_DIR,
					KT_APP_CMD_LOG_FILE_NAME);

		*log_file = fopen(p_app_ctx->cmd_log_file_path, "a");
		if (*log_file == NULL) {
			os_pr_err("%s: Failed to open log file: %s: %s\n", __func__,
					  p_app_ctx->cmd_log_file_path, strerror(errno));
			return;
		}

	} else if (os_strncmp(log_type, "stream", strlen("stream")) == 0) {
		os_snprintf(p_app_ctx->stream_log_file_path,
					sizeof(p_app_ctx->stream_log_file_path), "%s/%s",
					KT_APP_LOG_DIR,
					KT_APP_STREAM_LOG_FILE_NAME);

		*log_file = fopen(p_app_ctx->stream_log_file_path, "a");
		if (*log_file == NULL) {
			os_pr_err("%s: Failed to open log file: %s: %s\n", __func__,
					  p_app_ctx->stream_log_file_path, strerror(errno));
			return;
		}
	} else {
		os_pr_err("%s: Invalid log type: %s\n", __func__, log_type);
		return;
	}

	/* Write a header entry with timestamp */
	strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm_info);
	os_fprintf(*log_file, "===== Log started at %s =====\n", time_str);
	fflush(*log_file);

	return;
}

/**
 * Finalize the logger
 *
 * @param app_handle The App handle
 *
 * @return void
 */
void kt_app_log_fini(App_Handle app_handle)
{
	/* Finalize all the log files*/
	struct kt_app_ctx *p_app_ctx = (struct kt_app_ctx *)app_handle;
	if (p_app_ctx->cmd_log_file != NULL) {
		fclose(p_app_ctx->cmd_log_file);
		p_app_ctx->cmd_log_file = NULL;
	}
	if (p_app_ctx->stream_log_file != NULL) {
		fclose(p_app_ctx->stream_log_file);
		p_app_ctx->stream_log_file = NULL;
	}

	return;
}

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
					  ...)
{
	struct kt_app_ctx *p_app_ctx = (struct kt_app_ctx *)app_handle;

	FILE              *log_file  = NULL;
	if (os_strncmp(log_type, "cmd", strlen("cmd")) == 0) {
		log_file = p_app_ctx->cmd_log_file;
	} else if (os_strncmp(log_type, "stream", strlen("stream")) == 0) {
		log_file = p_app_ctx->stream_log_file;
	} else {
		os_pr_err("%s: Invalid log type: %s\n", __func__, log_type);
		return;
	}

	va_list args;
	va_start(args, log_entry);

	time_t    now = time(NULL);
	struct tm tm_info;
	localtime_r(&now, &tm_info);
	char time_str[32];

	strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm_info);

	fprintf(log_file, "%s: ", time_str);

	vfprintf(log_file, log_entry, args);

	fprintf(log_file, "\n");

	va_end(args);

	fflush(log_file);

	return;
}