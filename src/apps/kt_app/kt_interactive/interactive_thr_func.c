/******************************************************************************
 * Copyright (c) 2026 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/
#include "kt_app.h"
#include "kt_app_req_resp.h"
#include "kt_app_logger.h"
#include "kt_app_face_db.h"

/* Kt App Interactive Thread Functions */

/**
 * Print the menu
 *
 * @return void
 */
static void print_menu(void)
{
	os_pr_info("Menu:\n");
	os_pr_info("1. Start Pipeline\n");
	os_pr_info("2. Start Face Identification\n");
	os_pr_info("3. Generate Face ID Async\n");
	os_pr_info("4. Update Face Profile\n");
	os_pr_info("5. List Face Profiles\n");
	os_pr_info("6. Start QR Monitor\n");
	os_pr_info("7. Stop Pipeline\n");
	os_pr_info("8. Stop Face Identification\n");
	os_pr_info("9. Delete Face ID Async\n");
	os_pr_info("10. Stop QR Monitor\n");
	os_pr_info("11. Shutdown App\n");
}

/**
 * kt_app_interactive_thread_func is the interactive thread function for the
 * kt_app. It is used to perform any app specific tasks that need to be done
 * periodically. In this example, we just print a message and sleep for 1
 * second.
 *
 * @param arg The argument
 *
 * @return void *
 */
void *kt_app_interactive(void *arg)
{
	struct kt_app_ctx     *p_app_ctx = (struct kt_app_ctx *)arg;
	struct kt_app_request  request;
	struct kt_app_response response;

	/**
	 * This thread presents a menu to the user and waits for the user to select
	 * an option.
	 */
	while (!p_app_ctx->should_exit) {
		print_menu();
		os_pr_info("Enter your choice: ");
		int choice;
		os_scanf("%d", &choice);

		enum kt_app_cmd cmd = (enum kt_app_cmd)choice;

		switch (cmd) {
		case KT_APP_CMD_START_PIPELINE:
		case KT_APP_CMD_START_FACE_IDENTIFICATION:
		case KT_APP_CMD_GENERATE_FACE_ID_ASYNC:
		case KT_APP_CMD_START_QR_MONITOR:
		case KT_APP_CMD_LIST_FACE_PROFILES:
			break;
		case KT_APP_CMD_UPDATE_FACE_PROFILE:
			uint32_t face_id;
			char     face_name[KT_APP_MAX_FACE_NAME_LENGTH] = {0};
			os_pr_info("Enter face ID: ");
			os_scanf("%d", &face_id);
			os_pr_info("Enter face name: ");
			os_scanf("%s", face_name);
			request.update_face_profile_request.face_id = face_id;
			os_memcpy(request.update_face_profile_request.face_name, face_name,
					  sizeof(request.update_face_profile_request.face_name));
			cmd_log_write(p_app_ctx, "Updating face profile: %d, %s", face_id,
						  face_name);
			bool updated = kt_app_face_db_update_face_profile(
				p_app_ctx, face_id, face_name);
			if (!updated) {
				os_pr_err(
					"Failed to update face profile in in-memory database\n");
				continue;
			}
			break;
		case KT_APP_CMD_EXIT:
			p_app_ctx->should_exit = true;
			kt_app_face_db_flush_to_disk(p_app_ctx);
			cmd_log_write(p_app_ctx, "%s, CMD[%d] exit", __func__, cmd);
			return NULL;
		default:
			cmd_log_write(p_app_ctx, "%s, CMD[%d] unknown", __func__, cmd);
			continue;
		}

		request.cmd = cmd;
		kt_app_send_request(p_app_ctx, &request);

		cmd_log_write(p_app_ctx, "%s, CMD[%d] sent", __func__, cmd);

		/* Wait for the command to be executed */
		os_sleep_ms(1000);

		int retry_count = 0;
		while (retry_count < KT_APP_CMD_RESP_RETRY_COUNT) {
			/* TBD-DPN: We assume that the last sent cmd is pending */
			/* Check if the command is pending */
			os_mutex_lock(&p_app_ctx->kt_app_shmem->mutex);
			bool cmd_pending = p_app_ctx->kt_app_shmem->cmd_pending;
			os_mutex_unlock(&p_app_ctx->kt_app_shmem->mutex);

			if (cmd_pending) {
				cmd_log_write(p_app_ctx, "%s, RESP[%d] pending, retry %d of %d",
							  __func__, cmd, retry_count,
							  KT_APP_CMD_RESP_RETRY_COUNT);
				retry_count++;
				os_sleep_ms(500);
				continue;
			}

			kt_app_get_response(p_app_ctx, &response);
			cmd = response.cmd;

			if (KT_APP_SUCCESS == response.ret_code) {
				cmd_log_write(p_app_ctx, "%s, RESP[%d] success", __func__, cmd);
				switch (cmd) {
				case KT_APP_CMD_GENERATE_FACE_ID_ASYNC:

					/**
					 * The Face ID entry has already been added to the in-memory
					 * database by the callback. We just update the name if the
					 * user wants to change it
					 */
					enum kt_app_ret_code op_status =
						response.generate_face_id_async_response.op_status;
					if (op_status != KT_APP_SUCCESS) {
						os_pr_err("Face ID generation failed, op status: %d\n",
								  op_status);
						break;
					}

					char yes_no[2]                              = {0};
					char face_name[KT_APP_MAX_FACE_NAME_LENGTH] = {0};
					os_pr_info(
						"Would you like to enter a name for Face ID[%d] ? "
						"(y/n): ",
						response.generate_face_id_async_response.face_id);
					os_scanf("%1s", yes_no);
					if (yes_no[0] == 'y') {
						os_pr_info("Enter face name: ");
						os_scanf("%s", face_name);
					} else {
						os_snprintf(face_name, sizeof(face_name), "Unknown");
					}

					bool updated = kt_app_face_db_update_face_profile(
						p_app_ctx,
						response.generate_face_id_async_response.face_id,
						face_name);
					if (!updated) {
						os_pr_err("Failed to update face profile in in-memory "
								  "database\n");
						break;
					}
					os_pr_info(
						"Face ID %d updated in in-memory database\n",
						response.generate_face_id_async_response.face_id);
					/* Flush the face database to disk */
					kt_app_face_db_flush_to_disk(p_app_ctx);
					break;
				case KT_APP_CMD_LIST_FACE_PROFILES:
					/* List the face profiles in the in-memory buffer */
					os_pr_info("--------------------------------\n");
					os_pr_info("Listing %d face profiles\n",
							   response.list_face_profiles_response.face_count);
					for (uint32_t i = 0;
						 i < response.list_face_profiles_response.face_count;
						 i++) {
						os_pr_info("ID[%d]: Name[%s]\n",
								   response.list_face_profiles_response
									   .face_profiles[i]
									   .face_id,
								   response.list_face_profiles_response
									   .face_profiles[i]
									   .face_name);
					}
					os_pr_info("--------------------------------\n");
					break;
				default:
					break;
				}
				break;
			} else {
				cmd_log_write(p_app_ctx, "%s, RESP[%d] fail, error code %d",
							  __func__, cmd, response.ret_code);
			}

			break;
		}

		if (retry_count >= KT_APP_CMD_RESP_RETRY_COUNT) {
			cmd_log_write(p_app_ctx, "%s, RESP[%d] fail, max retries reached",
						  __func__, cmd, KT_APP_CMD_RESP_RETRY_COUNT);
		}
	}

	kt_app_face_db_flush_to_disk(p_app_ctx);

	return NULL;
}