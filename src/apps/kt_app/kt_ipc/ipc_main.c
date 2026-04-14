/******************************************************************************
 * Copyright (c) 2026 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#include <signal.h>
#include <poll.h>

#include "kt_app.h"
#include "kt_app_logger.h"
#include "kt_app_face_db.h"
#include "kt_app_cmds.h"

/* Kt App IPC Main Functions */

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
	void *exit_status_main = NULL;
	void *exit_status_ipc  = NULL;

	/* Register the CTRL-C handler */
	signal(SIGINT, handle_ctrl_c);

	/**
	 * Create a shared memory buffer for both kt_app_main and kt_app_ipc
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

	/* Initialize the socket paths */
	os_snprintf(app_ctx.req_resp_socket_path,
				sizeof(app_ctx.req_resp_socket_path),
				KT_APP_IPC_REQ_RESP_SOCKET);
	os_snprintf(app_ctx.stream_socket_path, sizeof(app_ctx.stream_socket_path),
				KT_APP_IPC_STREAM_SOCKET);

	/* Announce the IPC mode */
	os_pr_info("******************************************************\n");
	os_pr_info("Katana App - IPC mode:\n"
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

	/* Spawn the kt_app_ipc thread */
	os_thread_t      kt_app_ipc_thread;
	os_thread_attr_t kt_app_ipc_thread_attr = {0};
	os_thread_create(&kt_app_ipc_thread, &kt_app_ipc_thread_attr, kt_app_ipc,
					 &app_ctx);

	/* Ask the user to press 'q' to quit */
	os_pr_info("Press 'q' to quit\n");
	while (!app_ctx.should_exit) {
		struct pollfd pfd = {
			.fd     = STDIN_FILENO,
			.events = POLLIN,
		};
		int pr =
			poll(&pfd, 1, 250); /* wake periodically to check should_exit */
		if (pr > 0 && (pfd.revents & POLLIN)) {
			int c = getchar();
			if (c == 'q') {
				app_ctx.should_exit = true;
				kt_app_face_db_flush_to_disk((App_Handle)&app_ctx);
				break;
			}
		}
		/* If pr == 0 (timeout) or EINTR, just loop and recheck should_exit */
	}

	/* Wait for the threads to finish */
	os_thread_join(kt_app_main_thread, &exit_status_main);
	if (exit_status_main != (void *)0) {
		os_pr_err("kt_app_main thread exited with non-zero status\n");
		goto err_ipc_after_main_fail;
	}
	os_thread_join(kt_app_ipc_thread, &exit_status_ipc);
	if (exit_status_ipc != (void *)0) {
		os_pr_err("kt_app_ipc thread exited with non-zero status\n");
		goto err_ipc_cleanup;
	}

	/* Flush face DB on normal shutdown initiated via IPC as well */
	kt_app_face_db_flush_to_disk((App_Handle)&app_ctx);

	/* Close log streams and face DB file */
	kt_app_log_fini((App_Handle)&app_ctx);
	kt_app_face_db_fini((App_Handle)&app_ctx);

	/* Free the kt_app_shmem */
	os_free(p_kt_app_shmem);

	return 0;

err_ipc_after_main_fail:
	(void)os_thread_cancel(kt_app_ipc_thread);
	(void)os_thread_join(kt_app_ipc_thread, &exit_status_ipc);
err_ipc_cleanup:
	kt_app_log_fini((App_Handle)&app_ctx);
	kt_app_face_db_fini((App_Handle)&app_ctx);
	os_mutex_destroy(&p_kt_app_shmem->mutex);
	os_free(p_kt_app_shmem);
	return -1;
}