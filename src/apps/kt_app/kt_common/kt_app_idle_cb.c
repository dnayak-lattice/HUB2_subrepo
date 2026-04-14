/******************************************************************************
 * Copyright (c) 2026 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#include "kt_app.h"
#include "kt_app_cmds.h"
#include "kt_app_logger.h"
#include "kt_app_face_db.h"

#if !defined(KT_APP_SIMULATOR_MODE)
/* Interacts with hardware via KtLib */

/**
 * kt_app_execute_cmd is called by the kt_app_idle_cb thread to execute the
 * commands from the kt_app_interactive or the kt_app_ipc thread. This function
 * exchanges data with the kt_app_interactive or the kt_app_ipc thread via the
 * shared memory.
 *
 * @param app_handle The App handle
 *
 * @return void
 */
static void kt_app_execute_cmd(App_Handle app_handle)
{
	struct kt_app_ctx   *p_app_ctx = (struct kt_app_ctx *)app_handle;
	enum kt_app_cmd      cmd;
	enum kt_app_ret_code ret_code;

	os_mutex_lock(&p_app_ctx->kt_app_shmem->mutex);
	if (!p_app_ctx->kt_app_shmem->cmd_pending) {
		os_mutex_unlock(&p_app_ctx->kt_app_shmem->mutex);
		return;
	}
	cmd = p_app_ctx->kt_app_shmem->request.cmd;
	os_mutex_unlock(&p_app_ctx->kt_app_shmem->mutex);

	switch (cmd) {
	case KT_APP_CMD_START_PIPELINE:
		ret_code = Kt_StartPipeline(p_app_ctx->kt_handle);
		break;
	case KT_APP_CMD_START_FACE_IDENTIFICATION:
		ret_code = Kt_StartFaceIdentification(p_app_ctx->kt_handle);
		break;
	case KT_APP_CMD_GENERATE_FACE_ID_ASYNC:
		ret_code = Kt_GenerateFaceId_async(p_app_ctx->kt_handle,
										   kt_app_generate_face_id_done_cb);
		break;
	case KT_APP_CMD_UPDATE_FACE_PROFILE:
		/* Perform the in-memory update */
		bool updated = kt_app_face_db_update_face_profile(
			(App_Handle)p_app_ctx,
			p_app_ctx->kt_app_shmem->request.update_face_profile_request
				.face_id,
			p_app_ctx->kt_app_shmem->request.update_face_profile_request
				.face_name);
		ret_code = updated ? KT_APP_SUCCESS : KT_APP_ERROR_GENERIC;
		if (KT_APP_SUCCESS == ret_code) {
			/* Flush the face database to disk */
			kt_app_face_db_flush_to_disk(p_app_ctx);
		}
		break;
	case KT_APP_CMD_LIST_FACE_PROFILES:
		/* Fill the response with the in-memory buffer */
		os_mutex_lock(&p_app_ctx->kt_app_shmem->mutex);
		uint32_t face_count = p_app_ctx->face_profile_count;
		if (face_count > KT_APP_MAX_FACE_PROFILES_ON_DISK) {
			face_count = KT_APP_MAX_FACE_PROFILES_ON_DISK;
		}
		p_app_ctx->kt_app_shmem->response.list_face_profiles_response
			.face_count = face_count;
		for (uint32_t i = 0; i < face_count; i++) {
			p_app_ctx->kt_app_shmem->response.list_face_profiles_response
				.face_profiles[i]
				.face_id = p_app_ctx->face_profiles[i].face_id;
			os_memcpy(
				p_app_ctx->kt_app_shmem->response.list_face_profiles_response
					.face_profiles[i]
					.face_name,
				p_app_ctx->face_profiles[i].face_name,
				sizeof(p_app_ctx->kt_app_shmem->response
						   .list_face_profiles_response.face_profiles[i]
						   .face_name));
		}
		os_mutex_unlock(&p_app_ctx->kt_app_shmem->mutex);
		ret_code = KT_APP_SUCCESS;
		break;
	case KT_APP_CMD_START_QR_MONITOR:
		ret_code = Kt_StartQrMonitor(p_app_ctx->kt_handle);
		break;
	case KT_APP_CMD_EXIT:
		Kt_ReleaseMetadataBuffers(p_app_ctx->kt_handle);
		Kt_ReleaseQrStringBuffers(p_app_ctx->kt_handle);
		Kt_Fini(p_app_ctx->kt_handle);
		Kt_StopDispatcher(p_app_ctx->kt_handle);
		ret_code               = KT_OK;
		p_app_ctx->should_exit = true;
		break;
	default:
		cmd_log_write(p_app_ctx, "%s, CMD[%d] unknown", __func__, cmd);
		break;
	}

	cmd_log_write(p_app_ctx, "%s, CMD[%d] response: ret_code[%d]", __func__,
				  cmd, ret_code);
	/* Update the response structure in the shared memory */
	os_mutex_lock(&p_app_ctx->kt_app_shmem->mutex);
	p_app_ctx->kt_app_shmem->response.cmd      = cmd;
	p_app_ctx->kt_app_shmem->response.ret_code = ret_code;
	p_app_ctx->kt_app_shmem->cmd_pending       = false;
	os_mutex_unlock(&p_app_ctx->kt_app_shmem->mutex);
}

/**
 * kt_app_idle_cb is called periodically by the dispatcher.
 *
 * It interacts with the kt_interactive thread to get commands from the user,
 * and respond with return codes, via the share memory. It can also be used to
 * perform any app specific tasks that need to be done periodically.
 *
 * Currently, the idle callback only calls the kt_app_execute_cmd function to
 * execute the commands from the kt_app_interactive or the kt_app_ipc thread.
 *
 * @param app_handle The App handle
 *
 * @return void
 */
void kt_app_idle_cb(App_Handle app_handle)
{
	kt_app_execute_cmd((App_Handle)app_handle);
}

#else /* KT_APP_SIMULATOR_MODE */

/**
 * For simulation to test the IPC and Python clients.
 *
 * This code is not used in the real application.
 */

#include <stdlib.h>

/**
 * Generate PPE data for simulation
 *
 * @param app_handle The App handle
 *
 * @return struct PpeData * The PPE data
 */
static struct PpeData *generate_ppe_data(App_Handle app_handle)
{
	uint32_t        face_count = (uint32_t)rand() % 10 + 1;
	static uint8_t *buffer;

	buffer                   = (uint8_t *)Os_MemAlloc(app_handle,
													  sizeof(struct PpeData) +
														  face_count * sizeof(struct FaceData));

	struct PpeData *ppe_data = (struct PpeData *)buffer;
	ppe_data->pd__face_count = face_count;
	for (uint32_t i = 0; i < face_count; i++) {
		ppe_data->pd__faces[i].fd__face_id           = i + 1;
		ppe_data->pd__faces[i].ideal_person          = rand() % 2 == 0;
		ppe_data->pd__faces[i].fd__left              = rand() % 100 + 1;
		ppe_data->pd__faces[i].fd__top               = rand() % 100 + 1;
		ppe_data->pd__faces[i].fd__right             = rand() % 100 + 1;
		ppe_data->pd__faces[i].fd__bottom            = rand() % 100 + 1;
		ppe_data->pd__faces[i].fd__safety_hat_on     = rand() % 2 == 0;
		ppe_data->pd__faces[i].fd__safety_glasses_on = rand() % 2 == 0;
		ppe_data->pd__faces[i].fd__safety_gloves_on  = rand() % 2 == 0;
	}

	return (struct PpeData *)buffer;
}

/**
 * kt_app_execute_cmd is called by the kt_app_idle_cb thread to execute the
 * commands from the kt_app_interactive or the kt_app_ipc thread. This function
 * exchanges data with the kt_app_interactive or the kt_app_ipc thread via the
 * shared memory.
 *
 * @param app_handle The App handle
 *
 * @return void
 */
static void kt_app_execute_cmd(App_Handle app_handle)
{
	struct kt_app_ctx   *p_app_ctx = (struct kt_app_ctx *)app_handle;
	enum kt_app_cmd      cmd;
	enum kt_app_ret_code ret_code = KT_APP_SUCCESS;

	os_mutex_lock(&p_app_ctx->kt_app_shmem->mutex);
	if (!p_app_ctx->kt_app_shmem->cmd_pending) {
		os_mutex_unlock(&p_app_ctx->kt_app_shmem->mutex);
		return;
	}
	cmd = p_app_ctx->kt_app_shmem->request.cmd;
	os_mutex_unlock(&p_app_ctx->kt_app_shmem->mutex);

	switch (cmd) {
	case KT_APP_CMD_START_PIPELINE:
		ret_code = KT_APP_SUCCESS;
		break;
	case KT_APP_CMD_START_FACE_IDENTIFICATION:
		/* Set the return code to KT_OK but after a small delay */
		os_sleep_ms(1000);
		ret_code = KT_APP_SUCCESS;
		break;
	case KT_APP_CMD_GENERATE_FACE_ID_ASYNC:
		/**
		 * Since it's async, we return with pending right away and call the
		 * callback to notify the user that the face ID generation is done.
		 */
		os_mutex_lock(&p_app_ctx->kt_app_shmem->mutex);
		p_app_ctx->kt_app_shmem->response.ret_code = KT_APP_RET_CODE_PENDING;
		os_mutex_unlock(&p_app_ctx->kt_app_shmem->mutex);

		os_sleep_ms(1000);
		static uint32_t face_id = 1;
		kt_app_generate_face_id_done_cb((App_Handle)p_app_ctx, face_id++,
										KT_APP_SUCCESS);
		ret_code = KT_APP_SUCCESS;
		break;
	case KT_APP_CMD_UPDATE_FACE_PROFILE:
		/* Strict update: only update existing profiles; fail if not found */
		{
			uint32_t face_id = p_app_ctx->kt_app_shmem->request
								   .update_face_profile_request.face_id;
			const char *face_name = p_app_ctx->kt_app_shmem->request
										.update_face_profile_request.face_name;
			bool found = kt_app_face_db_update_face_profile(app_handle, face_id,
															face_name);
			ret_code   = found ? KT_APP_SUCCESS : KT_APP_ERROR_GENERIC;
		}
		break;
	case KT_APP_CMD_LIST_FACE_PROFILES:
		/* Fill the response with the in-memory buffer */
		os_mutex_lock(&p_app_ctx->kt_app_shmem->mutex);
		uint32_t face_count = p_app_ctx->face_profile_count;
		if (face_count > KT_APP_MAX_FACE_PROFILES_ON_DISK) {
			face_count = KT_APP_MAX_FACE_PROFILES_ON_DISK;
		}
		p_app_ctx->kt_app_shmem->response.list_face_profiles_response
			.face_count = face_count;
		for (uint32_t i = 0; i < face_count; i++) {
			p_app_ctx->kt_app_shmem->response.list_face_profiles_response
				.face_profiles[i]
				.face_id = p_app_ctx->face_profiles[i].face_id;
			os_memcpy(
				p_app_ctx->kt_app_shmem->response.list_face_profiles_response
					.face_profiles[i]
					.face_name,
				p_app_ctx->face_profiles[i].face_name,
				sizeof(p_app_ctx->kt_app_shmem->response
						   .list_face_profiles_response.face_profiles[i]
						   .face_name));
		}
		os_mutex_unlock(&p_app_ctx->kt_app_shmem->mutex);
		ret_code = KT_APP_SUCCESS;
		break;
	case KT_APP_CMD_START_QR_MONITOR:
		/* Set the return code to failure  */
		os_sleep_ms(2000);
		ret_code = KT_APP_ERROR_GENERIC;
		break;
	case KT_APP_CMD_EXIT:
		p_app_ctx->should_exit = true;
		ret_code               = KT_APP_SUCCESS;
		break;
	default:
		cmd_log_write(p_app_ctx, "%s, CMD[%d] unknown", __func__, cmd);
		ret_code = KT_APP_ERROR_GENERIC;
		break;
	}

	cmd_log_write(p_app_ctx, "%s, CMD[%d] response: ret_code[%d]", __func__,
				  cmd, ret_code);

	/* Update the final ret code in the shared memory */
	os_mutex_lock(&p_app_ctx->kt_app_shmem->mutex);
	p_app_ctx->kt_app_shmem->response.cmd      = cmd;
	p_app_ctx->kt_app_shmem->response.ret_code = ret_code;
	p_app_ctx->kt_app_shmem->cmd_pending       = false;
	os_mutex_unlock(&p_app_ctx->kt_app_shmem->mutex);
}

/**
 * kt_app_idle_cb is called periodically by the dispatcher.
 *
 * It interacts with the kt_interactive thread to get commands from the user,
 * and respond with return codes, via the share memory. It can also be used to
 * perform any app specific tasks that need to be done periodically.
 *
 * @param app_handle The App handle
 *
 * @return void
 */
void kt_app_idle_cb(App_Handle app_handle)
{
	struct kt_app_ctx *p_app_ctx = (struct kt_app_ctx *)app_handle;
	while (!p_app_ctx->should_exit) {
		kt_app_execute_cmd(app_handle);

		/* Simulate the stream meadata send from KtLib */
		struct PpeData *ppe_data = generate_ppe_data(app_handle);
		/* Randomly alternate between face identified and face not identified */
		if (rand() % 2 == 0) {
			kt_app_face_identified_cb((App_Handle)p_app_ctx, ppe_data);
		} else {
			kt_app_face_not_identified_cb((App_Handle)p_app_ctx, ppe_data);
		}

		os_sleep_ms(500);
	}
}

#endif /* KT_APP_SIMULATOR_MODE */