/******************************************************************************
 * Copyright (c) 2026 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#ifndef __HUB_REQ_RESP_SM_H__
#define __HUB_REQ_RESP_SM_H__

#include "hub.h"
#include "dal.h"
#include "gard_hub_iface.h"

/* Hub request-response state machine. */
enum hub_request_sm {
	HUB_REQUEST__SM_START,
	HUB_REQUEST__SM_SEND_REQUEST,
	HUB_REQUEST__SM_SEND_APP_REQUEST,
	HUB_REQUEST__SM_RECV_PARTIAL_RESPONSE,
	HUB_REQUEST__SM_VALIDATE_PARTIAL_RESPONSE,
	HUB_REQUEST__SM_RECV_RESPONSE,
	HUB_REQUEST__SM_RECV_APP_RESPONSE,
	HUB_REQUEST__SM_SEND_DATA,
	HUB_REQUEST__SM_RECV_OPTIONAL_ACK,
	HUB_REQUEST__SM_COMPLETE,
	HUB_REQUEST__SM_SERVICE_GARD_REQUEST,
};

#define POLL_TIME_TO_RECV_GARD_CMND_IN_US 1

struct hub_request_response_data {
	/***************************************************************************
	 * The variables in this part of the structure are shared between the
	 * callers and hub_execute_hub_request.
	 **************************************************************************/

	/* Request data and its size that is sent to be sent to GARD. */
	uint8_t *p_request;
	uint32_t request_size;

	/* If App request is to be sent then this buffer contains the request. */
	uint8_t *p_app_request;
	uint32_t app_request_size;

	/**
	 * Validation data and its size that is used to validate the response data.
	 * If the received response data does not match the validation data, for
	 * size validation_data_size then it is assumed to be a different request
	 * from GARD and is handled differently.
	 */
	uint8_t *p_validation_data;
	uint32_t validation_data_size;

	/**
	 * If any data is to be sent to GARD then this is it. Currently if data is
	 * sent by GARD then it is received in the response buffer and not in this
	 * buffer.
	 */
	uint8_t *p_send_data;
	uint32_t send_data_size;

	/**
	 * Response data and its size that is received from GARD. If there are
	 * intermediate responses which could be different in sizes then the first
	 * time this structure is setup with the first intermediate response. When
	 * the response handler is called by hub_execute_hub_request then the
	 * response handler code should update the response data size with the
	 * response size of the subsequent response that is expected to be received
	 * from GARD.
	 */
	uint8_t *p_response;
	uint32_t response_size;

	/* If App response is expected then this buffer should catch it. */
	uint8_t *p_app_response;
	uint32_t app_response_size;

	/**
	 * A callback function provided by the caller which helps
	 * hub_execute_hub_request() determine if data is to be sent to GARD.
	 * If the callback function is NULL then data will be sent to GARD by
	 * default.
	 * If the callback function returns true then data is to be sent to
	 * GARD.
	 * If the callback function returns false then data is not to be sent
	 * to GARD.
	 */
	bool (*should_send_data)(
		struct hub_request_response_data *p_request_response_data);

	/***************************************************************************
	 * The variables in this part of the structure are used explicitly by
	 * hub_execute_hub_request.
	 **************************************************************************/

	/**
	 * This variable is used internally by hub_execute_hub_request. The caller
	 * should not initialize this variable to HUB_REQUEST_SM_START.
	 */
	enum hub_request_sm state;

	/***************************************************************************
	 * The variables in this part of the structure are used explicitly by
	 * the caller and his helper routines.
	 **************************************************************************/
	gard_handle_t p_gard_handle;
};

/**
 * hub_execute_hub_request is a helper function to execute a request-response
 * operation between the HUB and the GARD. This function captures the common
 * code for sending the request and receiving the response.
 *
 * @param p_gard_handle The GARD handle.
 * @param p_request_response_data The request response data.
 *
 * @return true if the request is executed successfully, false on failure.
 */
bool hub_execute_hub_request(
	gard_handle_t                     p_gard_handle,
	struct hub_request_response_data *p_request_response_data);

#endif /* __HUB_REQ_RESP_SM_H__ */