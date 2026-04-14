/******************************************************************************
 * Copyright (c) 2026 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#include "hub_req_resp_sm.h"

/* Hub request-response state machine functions. */

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
	struct hub_request_response_data *p_request_response_data)
{
	struct hub_gard_info *p_gard       = (struct hub_gard_info *)p_gard_handle;
	bool                  stay_in_loop = false;
	int32_t               nwrite, nread;

	/**
	 * This function performs the following actions:
	 * 1. Sends the request to the GARD. It is assumed that the request is
	 *    properly prepared by the caller.
	 * 2. Receives partial response from the GARD (up to validation data size).
	 * 3. Validates the partial response against the validation data.
	 * 4. If the validation is successful, then the full response is received
	 *    from the GARD and execution happens as described in step 3, otherwise
	 *    it is assumed that GARD has also sent command at the same time and the
	 *    Default Originator rules are applied to move forward as documented
	 *    from step 7 of the Default Originator rules.
	 * 5. If there is no data_buffer provided then the communication is
	 *    complete.
	 *    This is the case when the command is not expected to send any data to
	 *    the GARD.
	 * 6. If data_buffer is provided then the data is sent to GARD and if an
	 *    ack is expected from GARD then the ack is placced in the same response
	 *    buffer and the communication is complete.
	 * 7. If a conflict is encounetered then we find out who is the current
	 *    Default originator; if the Default originator is HUB then we read and
	 *    discard what GARD has sent and instead wait for the response from
	 *    GARD. On the other hand if the Default originator is GARD then we
	 *    receive the request from GARD and call the GARD request helper
	 *    routine. Once this completes we re-send the original request to GARD.
	 */
	if (NULL == p_gard || NULL == p_request_response_data) {
		hub_pr_err("Invalid GARD handle or request response data\n");
		return false;
	}

	dal_handle_t dal_handle = p_gard->active_dal_bus;
	if (NULL == dal_handle) {
		hub_pr_err("No active DAL bus found for GARD profile ID: %d\n",
				   p_gard->gard_profile_id);
		return false;
	}

	do {
		/* By default we DO NOT stay in the loop. */
		stay_in_loop = false;

		switch (p_request_response_data->state) {
		case HUB_REQUEST__SM_START:
		case HUB_REQUEST__SM_SEND_REQUEST:
			nwrite = dal_write(dal_handle, p_request_response_data->p_request,
							   p_request_response_data->request_size, 0, 0);
			if (nwrite != p_request_response_data->request_size) {
				hub_pr_err("write = %d, expected = %u\n", nwrite,
						   p_request_response_data->request_size);
				return false;
			}

			/* Command exection has started. */
			p_gard->command_execution = true;

			if (NULL == p_request_response_data->p_app_request) {
				/* There is App Layer request that needs to be sent out. */
				p_request_response_data->state =
					HUB_REQUEST__SM_RECV_PARTIAL_RESPONSE;
				stay_in_loop = true;
				break;
			}

			/* Fall through to send App request */

		case HUB_REQUEST__SM_SEND_APP_REQUEST:
			nwrite =
				dal_write(dal_handle, p_request_response_data->p_app_request,
						  p_request_response_data->app_request_size, 0, 0);
			if (nwrite != p_request_response_data->app_request_size) {
				hub_pr_err("write = %d, expected = %u\n", nwrite,
						   p_request_response_data->app_request_size);
				return false;
			}

			p_request_response_data->state =
				HUB_REQUEST__SM_RECV_PARTIAL_RESPONSE;

			/* Fall through to receive partial response */

		case HUB_REQUEST__SM_RECV_PARTIAL_RESPONSE:

			/* Read only up to validation data size */
			nread = dal_read(dal_handle, p_request_response_data->p_response,
							 p_request_response_data->validation_data_size, 0,
							 1000);
			if (nread != p_request_response_data->validation_data_size) {
				hub_pr_err("read = %d, expected = %u\n", nread,
						   p_request_response_data->validation_data_size);
				return false;
			}

			p_request_response_data->state =
				HUB_REQUEST__SM_VALIDATE_PARTIAL_RESPONSE;

			/* Fall through to validate partial response */

		case HUB_REQUEST__SM_VALIDATE_PARTIAL_RESPONSE:
			if (0 != os_memcmp(p_request_response_data->p_request,
							   p_request_response_data->p_validation_data,
							   p_request_response_data->validation_data_size)) {
				hub_pr_dbg("Possibly a different request from GARD\n");
				p_request_response_data->state =
					HUB_REQUEST__SM_SERVICE_GARD_REQUEST;
				stay_in_loop = true;
				break;
			} else {
				/**
				 * Looks like a response for our command. receive the rest of
				 * the response.
				 */
				p_request_response_data->state = HUB_REQUEST__SM_RECV_RESPONSE;
			}

			/* Fall through to receive the rest of the response. */

		case HUB_REQUEST__SM_RECV_RESPONSE:
			/* Receive the response from GARD. */
			nread = dal_read(dal_handle,
							 p_request_response_data->p_response +
								 p_request_response_data->validation_data_size,
							 p_request_response_data->response_size -
								 p_request_response_data->validation_data_size,
							 0, 1000);
			if (nread != p_request_response_data->response_size -
							 p_request_response_data->validation_data_size) {
				hub_pr_err("read = %d, expected = %u\n", nread,
						   p_request_response_data->response_size);
				return false;
			}

			if (NULL != p_request_response_data->p_send_data) {
				/* Data is provided to be sent to GARD. */
				p_request_response_data->state = HUB_REQUEST__SM_SEND_DATA;
				stay_in_loop                   = true;
			} else if (NULL != p_request_response_data->p_app_response) {
				/* App Layer response is also expected. Now we receive it. */
				p_request_response_data->state =
					HUB_REQUEST__SM_RECV_APP_RESPONSE;
				stay_in_loop = true;
			} else {
				/**
				 * Done with this command. The caller can analyze the data after
				 * we return.
				 */
				p_request_response_data->state = HUB_REQUEST__SM_COMPLETE;
				stay_in_loop                   = true;
			}

			break;

		case HUB_REQUEST__SM_RECV_APP_RESPONSE:
			/* Receive the App Layer response. */
			nread =
				dal_read(dal_handle, p_request_response_data->p_app_response,
						 p_request_response_data->app_response_size, 0, 1000);
			if (nread != p_request_response_data->app_response_size) {
				hub_pr_err("read = %d, expected = %u\n", nread,
						   p_request_response_data->app_response_size);
				return false;
			}

			p_request_response_data->state = HUB_REQUEST__SM_COMPLETE;
			stay_in_loop                   = true;
			break;

		case HUB_REQUEST__SM_SEND_DATA:
			if ((NULL != p_request_response_data->should_send_data) &&
				(!p_request_response_data->should_send_data(
					p_request_response_data))) {
				/**
				 * Data is provided to be sent to GARD but the GARD response
				 * suggests otherwise. Currently we skip sending the data and
				 * complete the command.
				 */
				p_request_response_data->state = HUB_REQUEST__SM_COMPLETE;
				stay_in_loop                   = true;
				break;
			}

			/**
			 * Send data to GARD.
			 */
			nwrite = dal_write(dal_handle, p_request_response_data->p_send_data,
							   p_request_response_data->send_data_size, 0, 0);
			if (nwrite != p_request_response_data->send_data_size) {
				hub_pr_err("write = %d, expected = %u\n", nwrite,
						   p_request_response_data->send_data_size);
				return false;
			}

			/* Fall through to receive optional ACK from GARD. */

			p_request_response_data->state = HUB_REQUEST__SM_RECV_OPTIONAL_ACK;

			/* Fall through to receive optional ACK from GARD. */

		case HUB_REQUEST__SM_RECV_OPTIONAL_ACK:
			/* SRP - TBD. */
			break;

		case HUB_REQUEST__SM_COMPLETE:
			/* Command is complete. */
			p_gard->command_execution = false;
			return true;

		case HUB_REQUEST__SM_SERVICE_GARD_REQUEST:
			break;
		}
	} while (stay_in_loop);

	return true;
}