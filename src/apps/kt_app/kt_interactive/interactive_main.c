/******************************************************************************
 * Copyright (c) 2026 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/
#include <signal.h>

#include "kt_app.h"
#include "kt_app_logger.h"
#include "kt_app_face_db.h"
#include "kt_app_cmds.h"

/* Kt App Interactive Main Functions */

/* Kt App globals */

/* App context */
struct kt_app_ctx app_ctx = {0};

/**
 * Add a handler for CTRL-C
 *
 * @param signal The signal
 *
 * @return void
 */
static void handle_ctrl_c(int signal)
{
	os_pr_info("CTRL+C pressed, exiting program\n");

	/* Flush the face database to disk */
	kt_app_face_db_flush_to_disk((App_Handle)&app_ctx);

	/* Set the should_exit flag to true */
	app_ctx.should_exit = true;

	/* Exit the program */
	exit(0);
}

/**
 * Main function
 *
 * @param argc The number of arguments
 * @param argv The arguments
 *
 * @return int 0 on success, -1 on failure
 */
int main(int argc, char *argv[])
{
	void *exit_status_main        = NULL;
	void *exit_status_interactive = NULL;

	/* Register the CTRL-C handler */
	signal(SIGINT, handle_ctrl_c);

	/**
	 * Create a shared memory buffer for both kt_app_main and kt_app_interactive
	 * threads to share data and flags.
	 */
	struct kt_app_shmem *p_kt_app_shmem =
		(struct kt_app_shmem *)os_malloc(sizeof(struct kt_app_shmem));
	if (NULL == p_kt_app_shmem) {
		os_pr_err("Failed to allocate memory for kt_app_shmem\n");
		return -1;
	}

	/* Initialize the loggers */
	kt_app_log_init((App_Handle)&app_ctx, &app_ctx.cmd_log_file, "cmd");
	kt_app_log_init((App_Handle)&app_ctx, &app_ctx.stream_log_file, "stream");

	/* Initialize the face database */
	kt_app_face_db_init((App_Handle)&app_ctx,
						KT_APP_DB_DIR "/" KT_APP_FACE_DB_FILE_NAME);

	/* Announce the interactive mode */
	os_pr_info("******************************************************\n");
	os_pr_info("Katana App - Interactive mode:\n"
			   "HUB Version: %s\n"
			   "KtLib Version: %s\n",
			   hub_get_version_string(), Kt_GetVersionString());
#if defined(KT_APP_SIMULATOR_MODE)
	os_pr_info("WARNING: Simulator mode enabled\n");
#endif
	os_pr_info("******************************************************\n");

	/* Print the name of the log file */
	os_pr_info("Cmd log file: %s\n", app_ctx.cmd_log_file_path);
	os_pr_info("Stream log file: %s\n", app_ctx.stream_log_file_path);
	os_pr_info("Face database file: %s\n", app_ctx.face_db_file_path);
	os_pr_info("******************************************************\n");

	os_mutex_init(&p_kt_app_shmem->mutex);

	p_kt_app_shmem->cmd_pending = false;

	/* Initialize the kt_app_ctx */
	app_ctx.kt_app_shmem        = p_kt_app_shmem;

	/* Spawn the kt_app_main thread */
	os_thread_t      kt_app_main_thread;
	os_thread_attr_t kt_app_main_thread_attr = {0};
	os_thread_create(&kt_app_main_thread, &kt_app_main_thread_attr, kt_app_main,
					 &app_ctx);

	/* Spawn the kt_app_interactive thread */
	os_thread_t      kt_app_interactive_thread;
	os_thread_attr_t kt_app_interactive_thread_attr = {0};
	os_thread_create(&kt_app_interactive_thread,
					 &kt_app_interactive_thread_attr, kt_app_interactive,
					 &app_ctx);

	/* Wait for the threads to finish */
	os_thread_join(kt_app_main_thread, &exit_status_main);
	if (exit_status_main != (void *)0) {
		os_pr_err("kt_app_main thread exited with non-zero status\n");
		goto err_kt_app_interactive_2;
	}
	os_thread_join(kt_app_interactive_thread, &exit_status_interactive);
	if (exit_status_interactive != (void *)0) {
		os_pr_err("kt_app_interactive thread exited with non-zero status\n");
		goto err_kt_app_interactive_1;
	}

	/* Close log streams and face DB file */
	kt_app_log_fini((App_Handle)&app_ctx);
	kt_app_face_db_fini((App_Handle)&app_ctx);

	/* Free the kt_app_shmem */
	os_free(p_kt_app_shmem);

	return 0;

err_kt_app_interactive_2:
	(void)os_thread_cancel(kt_app_interactive_thread);
	(void)os_thread_join(kt_app_interactive_thread, &exit_status_interactive);
err_kt_app_interactive_1:
	kt_app_log_fini((App_Handle)&app_ctx);
	kt_app_face_db_fini((App_Handle)&app_ctx);
	os_mutex_destroy(&p_kt_app_shmem->mutex);
	os_free(p_kt_app_shmem);
	return -1;
}