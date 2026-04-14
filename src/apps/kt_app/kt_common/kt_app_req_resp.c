/******************************************************************************
 * Copyright (c) 2026 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#include "kt_app_req_resp.h"

/* Kt App Request Response Functions */

/**
 * kt_app_send_request is called by the kt_app_interactive and the kt_ipc
 * threads to send requests to the kt_app_main thread.
 *
 * @param app_handle The App handle
 * @param p_request The request to send
 *
 * @return void
 */
void kt_app_send_request(App_Handle             app_handle,
						 struct kt_app_request *p_request)
{
	struct kt_app_ctx   *p_app_ctx    = (struct kt_app_ctx *)app_handle;
	struct kt_app_shmem *p_kt_app_shmem = p_app_ctx->kt_app_shmem;

	os_mutex_lock(&p_kt_app_shmem->mutex);
	os_memcpy(&p_kt_app_shmem->request, p_request, sizeof(struct kt_app_request));
	p_kt_app_shmem->cmd_pending = true;
	os_mutex_unlock(&p_kt_app_shmem->mutex);
}

/**
 * kt_app_get_response is called by the kt_app_interactive and the kt_ipc
 * threads to get the response from the kt_app_main thread.
 *
 * @param app_handle The App handle
 * @param p_response The response to get
 *
 * @return void
 */
void kt_app_get_response(App_Handle              app_handle,
						 struct kt_app_response *p_response)
{
	struct kt_app_ctx   *p_app_ctx    = (struct kt_app_ctx *)app_handle;
	struct kt_app_shmem *p_kt_app_shmem = p_app_ctx->kt_app_shmem;

	/* Copy the response from the shared memory */
	os_mutex_lock(&p_kt_app_shmem->mutex);
	os_memcpy(p_response, &p_kt_app_shmem->response,
			  sizeof(struct kt_app_response));
	os_mutex_unlock(&p_kt_app_shmem->mutex);
}