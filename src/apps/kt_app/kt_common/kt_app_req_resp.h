/******************************************************************************
 * Copyright (c) 2026 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#ifndef __KT_APP_COMMON_H__
#define __KT_APP_COMMON_H__

#include "kt_app.h"
#include "kt_app_cmds.h"
#include "kt_app_logger.h"

/* Kt App Request Response Functions */

/**
 * Send a request to the kt_app_main thread
 *
 * @param app_handle The App handle
 * @param p_request The request to send
 *
 * @return void
 */
void kt_app_send_request(App_Handle             app_handle,
						 struct kt_app_request *p_request);

/**
 * Get a response from the kt_app_main thread
 *
 * @param app_handle The App handle
 * @param p_response The response to get
 *
 * @return void
 */
void kt_app_get_response(App_Handle              app_handle,
						 struct kt_app_response *p_response);
						 
#endif /* __KT_APP_REQ_RESP_H__ */