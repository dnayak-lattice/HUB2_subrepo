/******************************************************************************
 * Copyright (c) 2026 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#include "kt_app.h"
#include "kt_app_logger.h"
#include "kt_app_ipc_funcs.h"
#include "kt_app_face_db.h"

/* Kt App Face ID Callback Functions */

#if defined(KT_APP_SIMULATOR_MODE)
/* For simulation to test the IPC and Python clients.
 * This code is not used in the real application.
 *
 * Disable the calls to Kt_AddMetadataBuffer in simulator mode
 */
#define Kt_AddMetadataBuffer(handle, p_buffer, buff_len)
#endif /* KT_APP_SIMULATOR_MODE */


/**
 * Callback function for face ID generation done
 *
 * @param app_handle The App handle
 * @param face_id The ID of the face
 * @param op_status The operation status
 *
 * @return void
 */
void kt_app_generate_face_id_done_cb(App_Handle app_handle,
									 uint32_t   face_id,
									 Kt_RetCode op_status)
{
	struct kt_app_ctx *p_app_ctx = (struct kt_app_ctx *)app_handle;

	os_mutex_lock(&p_app_ctx->kt_app_shmem->mutex);
	p_app_ctx->kt_app_shmem->response.cmd = KT_APP_CMD_GENERATE_FACE_ID_ASYNC;
	p_app_ctx->kt_app_shmem->response.ret_code = KT_APP_SUCCESS;
	p_app_ctx->kt_app_shmem->response.generate_face_id_async_response.face_id =
		face_id;
	p_app_ctx->kt_app_shmem->response.generate_face_id_async_response
		.op_status = op_status;
	os_mutex_unlock(&p_app_ctx->kt_app_shmem->mutex);

	/* Notify any connected req/resp client immediately. */
	if (p_app_ctx->req_resp_ipc_connected &&
		p_app_ctx->req_resp_client_sockfd >= 0) {
		struct {
			uint32_t cmd_u32;
			int32_t  rc_i32;
		} header = {.cmd_u32 = (uint32_t)KT_APP_CMD_GENERATE_FACE_ID_ASYNC,
					.rc_i32  = (int32_t)KT_APP_SUCCESS};

		(void)kt_app_ipc_send_data_on_fd(p_app_ctx->req_resp_client_sockfd,
										 &header, sizeof(header));

		/* Send payload with face_id and op_status */
		struct {
			uint32_t face_id_u32;
			int32_t  op_status_i32;
		} payload = {.face_id_u32   = (uint32_t)face_id,
					 .op_status_i32 = (int32_t)op_status};

		(void)kt_app_ipc_send_data_on_fd(p_app_ctx->req_resp_client_sockfd,
										 &payload, sizeof(payload));
	}

	/**
	 * The Face ID is entered into the in-memory DB with a default name.
	 * This lets external clients rename via UPDATE later.
	 */
	if ((enum kt_app_ret_code)op_status == KT_APP_SUCCESS) {
		if (!kt_app_face_db_add_face_profile(p_app_ctx, face_id, "Default")) {
			os_pr_err("Failed to add face profile to in-memory database\n");
			return;
		}
		cmd_log_write(p_app_ctx, "Face ID generated: %d, op status: %d",
					  face_id, op_status);
	}
}

/**
 * Callback function for face identified
 *
 * @param app_handle The App handle
 * @param p_ppe_data The PPE data
 *
 * @return void
 */
void kt_app_face_identified_cb(App_Handle      app_handle,
							   struct PpeData *p_ppe_data)
{
	struct kt_app_ctx *p_app_ctx = (struct kt_app_ctx *)app_handle;
	(void)p_app_ctx;

	/**
	 * kt_app_main got the buffer back from KtLib, we need to parse the
	 * metadata. The buffer needs to go back to kt_app_main's pool of metadata
	 * buffers. Also, KtLib needs to be supplied with a new buffer.
	 */
	stream_log_write(app_handle, "Face identified, %d faces",
					 p_ppe_data->pd__face_count);

	for (uint32_t i = 0; i < p_ppe_data->pd__face_count; i++) {
		stream_log_write(
			app_handle,
			"--- ID[%d], Ideal[%d], L[%d], T[%d], R[%d], B[%d], PPE[%d, %d, "
			"%d]",
			p_ppe_data->pd__faces[i].fd__face_id,
			p_ppe_data->pd__faces[i].ideal_person,
			p_ppe_data->pd__faces[i].fd__left, p_ppe_data->pd__faces[i].fd__top,
			p_ppe_data->pd__faces[i].fd__right,
			p_ppe_data->pd__faces[i].fd__bottom,
			p_ppe_data->pd__faces[i].fd__safety_hat_on,
			p_ppe_data->pd__faces[i].fd__safety_glasses_on,
			p_ppe_data->pd__faces[i].fd__safety_gloves_on);
	}

	if (p_app_ctx->stream_ipc_connected) {
		/* Send the PPE data to the client */
		kt_app_ipc_send_data_on_fd(p_app_ctx->stream_client_sockfd, p_ppe_data,
								   sizeof(struct PpeData) +
									   p_ppe_data->pd__face_count *
										   sizeof(struct FaceData));
	}

	/* Hand over a new buffer to KtLib */
	uint8_t *p_buffer;
	uint32_t buff_len;
	kt_app_metadata_buffer_queue_get(app_handle, &p_buffer, &buff_len);

	if (NULL == p_buffer) {
		os_pr_err("Failed to get metadata buffer\n");
		return;
	}

	/* Hand over the buffer to KtLib*/
	Kt_AddMetadataBuffer(p_app_ctx->kt_handle, p_buffer, buff_len);

	uint8_t *p_buffer_to_add = (uint8_t *)p_ppe_data;
	uint32_t buff_len_to_add =
		sizeof(struct PpeData) +
		p_ppe_data->pd__face_count * sizeof(struct FaceData);
	/* Add the obtained buffer back to the queue*/
	kt_app_metadata_buffer_queue_add(app_handle, p_buffer_to_add,
									 buff_len_to_add);
}

/**
 * Callback function for face not identified
 *
 * @param app_handle The App handle
 * @param p_ppe_data The PPE data
 *
 * @return void
 */
void kt_app_face_not_identified_cb(App_Handle      app_handle,
								   struct PpeData *p_ppe_data)
{
	struct kt_app_ctx *p_app_ctx = (struct kt_app_ctx *)app_handle;
	(void)p_app_ctx;

	stream_log_write(app_handle, "Face not identified, %d faces",
					 p_ppe_data->pd__face_count);
	for (uint32_t i = 0; i < p_ppe_data->pd__face_count; i++) {
		stream_log_write(
			app_handle,
			"--- ID[%d], Ideal[%d], L[%d], T[%d], R[%d], B[%d], PPE[%d, %d, "
			"%d]",
			p_ppe_data->pd__faces[i].fd__face_id,
			p_ppe_data->pd__faces[i].ideal_person,
			p_ppe_data->pd__faces[i].fd__left, p_ppe_data->pd__faces[i].fd__top,
			p_ppe_data->pd__faces[i].fd__right,
			p_ppe_data->pd__faces[i].fd__bottom,
			p_ppe_data->pd__faces[i].fd__safety_hat_on,
			p_ppe_data->pd__faces[i].fd__safety_glasses_on,
			p_ppe_data->pd__faces[i].fd__safety_gloves_on);
	}

	if (p_app_ctx->stream_ipc_connected) {
		/* Send the PPE data to the client */
		kt_app_ipc_send_data_on_fd(p_app_ctx->stream_client_sockfd, p_ppe_data,
								   sizeof(struct PpeData) +
									   p_ppe_data->pd__face_count *
										   sizeof(struct FaceData));
	}

	/* Hand over a new buffer to KtLib*/
	uint8_t *p_buffer;
	uint32_t buff_len;
	kt_app_metadata_buffer_queue_get(app_handle, &p_buffer, &buff_len);

	if (NULL == p_buffer) {
		os_pr_err("Failed to get metadata buffer\n");
		return;
	}

	/* Hand over the buffer to KtLib*/
	Kt_AddMetadataBuffer(p_app_ctx->kt_handle, p_buffer, buff_len);

	/* Add the obtained buffer to the queue*/
	uint8_t *p_buffer_to_add = (uint8_t *)p_ppe_data;
	uint32_t buff_len_to_add =
		sizeof(struct PpeData) +
		p_ppe_data->pd__face_count * sizeof(struct FaceData);
	kt_app_metadata_buffer_queue_add(app_handle, p_buffer_to_add,
									 buff_len_to_add);
}
