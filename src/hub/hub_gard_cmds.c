/******************************************************************************
 * Copyright (c) 2026 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#include "hub_req_resp_sm.h"
#include "utils.h"

/* Hub GARD commands. */

/*******************************************************
 * Internal functions.
 *******************************************************/

/**
 * Internal function to send a discover command to the GARD.
 *
 * @param p_gard_handle The handle to the GARD.
 *
 * @return HUB_SUCCESS if the command is sent successfully,
 *         HUB_FAILURE_GARD_DISCOVER if the command is not sent successfully.
 */
static enum hub_ret_code hub_send_discover_cmd(gard_handle_t gard_handle)
{
	struct _gard_hub_requests  discovery_req  = {.command_id = GARD_DISCOVERY};
	struct _gard_hub_responses discovery_resp = {0};

	if (NULL == gard_handle) {
		hub_pr_err("Invalid GARD handle\n");
		return HUB_FAILURE_GARD_DISCOVER;
	}

	dal_handle_t dal_handle =
		((struct hub_gard_info *)gard_handle)->active_dal_bus;
	if (NULL == dal_handle) {
		hub_pr_err("No active DAL bus found for GARD\n");
		return HUB_FAILURE_GARD_DISCOVER;
	}

	struct hub_request_response_data gard_discovery_request_response_data = {
		.p_request      = (uint8_t *)&discovery_req,
		.request_size   = sizeof(discovery_req.command_id),

		.p_send_data    = NULL,
		.send_data_size = 0,

		.p_response     = (uint8_t *)&discovery_resp,
		.response_size  = sizeof(discovery_req.command_id) +
						 sizeof(discovery_resp.gard_discovery_response),

		.p_validation_data    = (uint8_t *)&discovery_req,
		.validation_data_size = sizeof(discovery_req.command_id),

		.should_send_data     = NULL,

		.state                = HUB_REQUEST__SM_START,
	};

	if (hub_execute_hub_request(gard_handle,
								&gard_discovery_request_response_data)) {
		/* Command is executed successfully. Now confirm the response. */
		if ((0 ==
			 os_strncmp(
				 HUB_GARD_DISCOVER_SIGNATURE,
				 (const char *)discovery_resp.gard_discovery_response.signature,
				 sizeof(HUB_GARD_DISCOVER_SIGNATURE))) &&
			(discovery_resp.gard_discovery_response.end_of_data_marker ==
			 END_OF_DATA_MARKER)) {
			hub_pr_dbg(
				"signature: %s\n",
				(const char *)discovery_resp.gard_discovery_response.signature);
			return HUB_SUCCESS;
		}
	}

	hub_pr_err("Discovery command failed\n");
	return HUB_FAILURE_GARD_DISCOVER;
}

/**
 * Get the profile ID of the GARD.
 * Currently, this function is a placeholder and returns a fixed profile ID.
 *
 * @param hub_handle The handle to the HUB.
 * @param index The index of the bus to send the command to.
 * @param p_profile_id A pointer to the profile ID of the GARD.
 *
 * @return HUB_SUCCESS if the profile ID is retrieved successfully,
 *         HUB_FAILURE_GARD_PROFILE_ID if the profile ID is not retrieved
 * successfully.
 */
static enum hub_ret_code hub_get_gard_profile_id(hub_handle_t hub_handle,
												 uint32_t     index,
												 uint32_t    *p_profile_id)
{
	*p_profile_id = 12345;

	return HUB_SUCCESS;
}

/**
 * Internal function to validate the send data response from the GARD.
 *
 * @param p_request_response_data The request response data.
 *
 * @return true if the send data response is valid, false otherwise.
 */
static bool hub_validate_response_start_signal(
	struct hub_request_response_data *p_request_response_data)
{
	struct _gard_hub_responses *p_send_data_resp =
		(struct _gard_hub_responses *)p_request_response_data->p_response;

	if (SEND_DATA == p_send_data_resp->send_data_to_gard_for_offset_response
						 .start_signal.send_data_start) {
		/* We keep the same buffer to receive the response data but change the
		 * response size as needed. */
		p_request_response_data->response_size =
			sizeof(p_send_data_resp->response_id) +
			sizeof(p_send_data_resp->send_data_to_gard_for_offset_response.ack);
		return true;
	}

	return false;
}

/**
 * Internal function to handle the App Layer Send data coming from GARD.
 *
 * @param p_gard_handle The GARD handle.
 *
 * @return True if the App Layer send data is handled successfully, false
 * otherwise.
 */
static bool hub_handle_app_layer_send_data(gard_handle_t p_gard_handle)
{
	struct hub_gard_info     *p_gard = (struct hub_gard_info *)p_gard_handle;
	dal_handle_t              dal_handle;
	int32_t                   nread;
	uint8_t                   peek_buffer[APP_LAYER_PEEK_BUFFER_SIZE];
	void                     *p_buffer    = NULL;
	uint32_t                  buffer_size = 0;
	struct _gard_hub_requests gard_command_request;
	uint32_t                  gard_app_layer_send_payload_size = 0;

	/**
	 * This function performs the following actions:
	 * 1. Reads the GARD command ID and the payload size. Our assumption is that
	 *    we will be receiving the GARD App Layer Send data command and we also
	 *    verify this.
	 * 2. We then read a small sample of the App Layer data in the peek buffer.
	 *    This helps the app Layer identify which buffer should be used to hold
	 *    the data that is arriving from GARD.
	 * 3. We get the buffer and its size from the App Layer.
	 * 4. We read the App Layer data from the GARD in this buffer and call the
	 *    App Layer handle routine to handle the received data.
	 */

	if (NULL == p_gard) {
		hub_pr_err("Invalid GARD handle\n");
		return false;
	}

	dal_handle = p_gard->active_dal_bus;
	if (NULL == dal_handle) {
		hub_pr_err("No active DAL bus found for GARD profile ID: %d\n",
				   p_gard->gard_profile_id);
		return false;
	}

	/* First get the GARD command ID and the payload size. */
	nread = dal_read(dal_handle, &gard_command_request,
					 sizeof(gard_command_request.command_id) +
						 sizeof(gard_command_request.gard_app_layer_send),
					 0, 1000);
	if (nread != sizeof(gard_command_request.command_id) +
					 sizeof(gard_command_request.gard_app_layer_send)) {
		hub_pr_err("read = %d, expected = %lu\n", nread,
				   sizeof(gard_command_request.command_id) +
					   sizeof(gard_command_request.gard_app_layer_send));
		return false;
	}

	/* Re-confirm this is a GARD App Layer Send data command. Also get the
	 * payload that will be arriving from GARD. */
	if (GARD_APP_LAYER_SEND != gard_command_request.command_id) {
		hub_pr_err("Unexpected  GARD command ID: %d\n",
				   gard_command_request.command_id);
		return false;
	}

	/* Move the payload size to a 32-bit variable. */
	((uint8_t *)&gard_app_layer_send_payload_size)[0] =
		gard_command_request.gard_app_layer_send.gard_app_layer_payload_size[0];
	((uint8_t *)&gard_app_layer_send_payload_size)[1] =
		gard_command_request.gard_app_layer_send.gard_app_layer_payload_size[1];
	((uint8_t *)&gard_app_layer_send_payload_size)[2] =
		gard_command_request.gard_app_layer_send.gard_app_layer_payload_size[2];

	if (0 == gard_app_layer_send_payload_size) {
		hub_pr_err("Invalid GARD App Layer Send data payload size: %u\n",
				   gard_app_layer_send_payload_size);
		return false;
	}

	/* Read the peek buffer. */
	nread = dal_read(dal_handle, peek_buffer, sizeof(peek_buffer), 0, 1000);
	if (nread != sizeof(peek_buffer)) {
		/* Failed to read the peek buffer. Just return back to the caller. */
		hub_pr_err("read = %d, expected = %lu\n", nread, sizeof(peek_buffer));
		return false;
	}

	/**
	 * <TBD - SRP>
	 * Ideally these should be asserts. Till the assert support is added to HUB
	 * we catch the error this way.
	 */
	if (NULL == p_gard->p_app_layer_cbs->get_buffer_for_gard_send_data ||
		NULL == p_gard->p_app_layer_cbs->handle_gard_send) {
		hub_pr_err("App Layer Callbacks have not yet been set.\n");
		/* App Layer callbacks are not set. Just return back to the caller. */
		return false;
	}

	if (!p_gard->p_app_layer_cbs->get_buffer_for_gard_send_data(
			p_gard_handle, p_gard->p_app_layer_cbs->app_handle, &p_buffer,
			&buffer_size, peek_buffer, sizeof(peek_buffer))) {
		/* Failed to get the buffer for the App Layer send data. Just return
		 * back to the caller. */
		hub_pr_err("Failed to get the buffer for the App Layer send data.\n");
		return false;
	}

	if (buffer_size < gard_app_layer_send_payload_size) {
		hub_pr_err("Buffer size is less than the GARD App Layer Send data "
				   "payload size: %u < %u\n",
				   buffer_size, gard_app_layer_send_payload_size);
		return false;
	}

	/**
	 * Copy the data from the peek buffer to the receive buffer for
	 * completeness.
	 */
	os_memcpy(p_buffer, peek_buffer, sizeof(peek_buffer));

	/* Read the rest of the data from the GARD. */
	nread = dal_read(dal_handle, p_buffer + sizeof(peek_buffer),
					 gard_app_layer_send_payload_size - sizeof(peek_buffer), 0,
					 1000);
	if (nread != gard_app_layer_send_payload_size - sizeof(peek_buffer)) {
		hub_pr_err("read = %d, expected = %lu\n", nread,
				   gard_app_layer_send_payload_size - sizeof(peek_buffer));
		return false;
	}

	/* Call the App Layer handle routine to handle the data. */
	return p_gard->p_app_layer_cbs->handle_gard_send(
		p_gard_handle, p_gard->p_app_layer_cbs->app_handle, p_buffer,
		gard_app_layer_send_payload_size);
}

/*******************************************************
 * Public functions.
 *******************************************************/

/**
 * Discover the GARDS on the HUB.
 *
 * @param hub_handle The handle to the HUB.
 *
 * @return HUB_SUCCESS if the GARDS are discovered successfully,
 *         HUB_FAILURE_GARD_DISCOVER if the GARDS are not discovered
 */
enum hub_ret_code hub_discover_gards(hub_handle_t hub_handle)
{
	enum hub_ret_code    ret;
	struct hub_ctx      *p_hub_ctx = (struct hub_ctx *)hub_handle;
	struct hub_config   *p_config  = p_hub_ctx->p_config;
	struct hub_gard_info temp_gard;

	if (p_hub_ctx == NULL || p_config == NULL) {
		return HUB_FAILURE_GARD_DISCOVER;
	}

	struct discovered_device {
		dal_handle_t p_dal_bus;
		uint32_t     profile_id;
		uint32_t     is_unique;
	} *p_discovered_devices = NULL;

	p_discovered_devices =
		os_calloc(p_config->num_busses, sizeof(struct discovered_device));
	if (NULL == p_discovered_devices) {
		return HUB_FAILURE_GARD_DISCOVER;
	}

	for (uint32_t i = 0; i < p_config->num_busses; i++) {
		/* Create a temporary GARD handle as discovery call needs it. */
		temp_gard.active_dal_bus = p_config->dal_busses[i];

		/* Send the command */
		ret                      = hub_send_discover_cmd(&temp_gard);
		if (HUB_SUCCESS == ret) {
			uint32_t profile_id;
			hub_pr_dbg("GARD discovered on bus %d\n", i);

			ret = hub_get_gard_profile_id(hub_handle, i, &profile_id);
			if (HUB_SUCCESS == ret) {
				hub_pr_dbg("GARD profile ID: %d\n", profile_id);

				p_discovered_devices[i].p_dal_bus  = p_config->dal_busses[i];
				p_discovered_devices[i].profile_id = profile_id;
				p_discovered_devices[i].is_unique  = 1;
			}

			for (uint32_t j = 0; j < i; j++) {
				if (p_discovered_devices[j].profile_id == profile_id) {
					p_discovered_devices[i].is_unique = 0;
					hub_pr_dbg("Duplicate GARD profile ID: %d on bus %d\n",
							   profile_id, j);
					break;
				}
			}

			if (p_discovered_devices[i].is_unique) {
				p_hub_ctx->num_gards++;
			}
		}
	}

	p_hub_ctx->p_gards = (struct hub_gard_info *)os_calloc(
		p_hub_ctx->num_gards, sizeof(struct hub_gard_info));
	if (NULL == p_hub_ctx->p_gards) {
		return HUB_FAILURE_GARD_DISCOVER;
	}

	uint32_t j = 0;
	for (uint32_t i = 0; i < p_config->num_busses; i++) {
		if (p_discovered_devices[i].is_unique) {
			uint32_t profile_id = p_discovered_devices[i].profile_id;
			struct hub_gard_info *p_gard;
			uint32_t              gard_num_busses = 0;

			p_gard                                = &p_hub_ctx->p_gards[j++];
			p_gard->hub                           = p_hub_ctx;

			/* Get the number of busses for the same profile ID */
			for (uint32_t k = 0; k < p_config->num_busses; k++) {
				if (p_discovered_devices[k].profile_id == profile_id) {
					gard_num_busses++;
				}
			}
			p_gard->num_busses = gard_num_busses;
			hub_pr_dbg("GARD %d has %d busses\n", profile_id, gard_num_busses);

			p_gard->p_dal_busses = (dal_handle_t *)os_calloc(
				gard_num_busses, sizeof(dal_handle_t));
			if (NULL == p_gard->p_dal_busses) {
				hub_pr_err("Failed to allocate memory for GARD busses\n");
				continue;
			}

			/* Record all busses over which the GARD is discovered */
			uint32_t cnt = 0;
			for (uint32_t k = 0; k < p_config->num_busses; k++) {
				if (p_discovered_devices[k].profile_id == profile_id) {
					dal_handle_t dal_handle = p_discovered_devices[k].p_dal_bus;
					p_gard->p_dal_busses[k] = dal_handle;
					hub_pr_dbg(
						"GARD bus: %s\n",
						hub_gard_bus_strings[dal_get_bus_type(dal_handle)]);
					cnt++;
				}
			}

			p_gard->gard_profile_id = profile_id;
		}
	}

	os_free(p_discovered_devices);

	p_hub_ctx->hub_state = HUB_DISCOVER_DONE;

	return HUB_SUCCESS;
}

/**
 * Get the number of GARDS on the HUB.
 *
 * @param hub_handle The handle to the HUB.
 *
 * @return The number of GARDS on the HUB.
 */
uint32_t hub_get_num_gards(hub_handle_t hub_handle)
{
	struct hub_ctx *p_hub_ctx = (struct hub_ctx *)hub_handle;

	if (p_hub_ctx == NULL) {
		return 0;
	}

	return p_hub_ctx->num_gards;
}

/**
 * Get the GARD handle for a given index.
 *
 * @param hub_handle The handle to the HUB.
 * @param gard_index The index of the GARD.
 *
 * @return The GARD handle.
 *         NULL if the GARD handle is not found.
 */
gard_handle_t hub_get_gard_handle(hub_handle_t hub_handle, uint32_t gard_index)
{
	struct hub_ctx *p_hub_ctx = (struct hub_ctx *)hub_handle;

	if (p_hub_ctx == NULL || p_hub_ctx->hub_state != HUB_INIT_DONE) {
		return NULL;
	}

	if (gard_index < 0 || gard_index >= p_hub_ctx->num_gards) {
		hub_pr_err("Invalid GARD index: %d\n", gard_index);
		return NULL;
	}

	return (gard_handle_t) & (p_hub_ctx->p_gards[gard_index]);
}

/**
 * Send a pause pipeline command to the GARD.
 *
 * @param p_gard_handle The GARD handle.
 * @param camera_id The camera ID to pause the pipeline for.
 *
 * @return HUB_SUCCESS if the command is sent successfully,
 *         HUB_FAILURE_SEND_PAUSE_PIPELINE on failure.
 */
enum hub_ret_code hub_send_pause_pipeline_cmd(gard_handle_t p_gard_handle,
											  uint8_t       camera_id)
{
	struct _gard_hub_requests pause_pipeline_req = {
		.command_id             = PAUSE_PIPELINE,
		.pause_pipeline_request = {.camera_id          = camera_id,
								   .rsvd1              = 0,
								   .end_of_data_marker = END_OF_DATA_MARKER}};
	struct _gard_hub_responses       pause_pipeline_resp = {.response_id =
																PAUSE_PIPELINE};

	struct hub_request_response_data gard_pause_pipeline_request_response_data =
		{
		.p_request    = (uint8_t *)&pause_pipeline_req,
		.request_size = sizeof(pause_pipeline_req.command_id) +
						sizeof(pause_pipeline_req.pause_pipeline_request),

		.p_app_request        = NULL,
		.app_request_size     = 0,

		.p_validation_data    = (uint8_t *)&pause_pipeline_req,
		.validation_data_size = sizeof(pause_pipeline_req.command_id),

		.p_send_data          = NULL,
		.send_data_size       = 0,

		.p_response           = (uint8_t *)&pause_pipeline_resp,
		.response_size        = sizeof(pause_pipeline_resp.response_id) +
						 sizeof(pause_pipeline_resp.pause_pipeline_response),

		.p_app_response    = NULL,
		.app_response_size = 0,

		.should_send_data  = NULL,

		.state             = HUB_REQUEST__SM_START,
		};

	if (hub_execute_hub_request(p_gard_handle,
								&gard_pause_pipeline_request_response_data)) {
		/* Command is executed successfully. Now confirm the response. */
		if (ACK_BYTE ==
			pause_pipeline_resp.pause_pipeline_response.ack_or_nak) {
			return HUB_SUCCESS;
		}
	}

	hub_pr_err("Pause Pipeline command failed\n");
	return HUB_FAILURE_SEND_PAUSE_PIPELINE;
}

/**
 * Send resume pipeline command to GARD.
 *
 * @param: p_gard_handle is the GARD handle
 * @param: camera_id is the camera ID
 *
 * @return: HUB_SUCCESS on success, HUB_FAILURE_SEND_RESUME_PIPELINE on
 * failure
 */
enum hub_ret_code hub_send_resume_pipeline_cmd(gard_handle_t p_gard_handle,
											   uint8_t       camera_id)
{
	struct _gard_hub_requests resume_pipeline_req = {
		.command_id              = RESUME_PIPELINE,
		.resume_pipeline_request = {.camera_id          = camera_id,
									.rsvd1              = 0,
									.end_of_data_marker = END_OF_DATA_MARKER}};
	struct _gard_hub_responses resume_pipeline_resp = {.response_id =
														   RESUME_PIPELINE};

	struct hub_request_response_data
		gard_resume_pipeline_request_response_data = {
		.p_request    = (uint8_t *)&resume_pipeline_req,
		.request_size = sizeof(resume_pipeline_req.command_id) +
						sizeof(resume_pipeline_req.resume_pipeline_request),

		.p_app_request        = NULL,
		.app_request_size     = 0,

		.p_validation_data    = (uint8_t *)&resume_pipeline_req,
		.validation_data_size = sizeof(resume_pipeline_req.command_id),

		.p_send_data          = NULL,
		.send_data_size       = 0,

		.p_response           = (uint8_t *)&resume_pipeline_resp,
		.response_size        = sizeof(resume_pipeline_resp.response_id) +
						 sizeof(resume_pipeline_resp.resume_pipeline_response),

		.p_app_response    = NULL,
		.app_response_size = 0,

		.should_send_data  = NULL,

		.state             = HUB_REQUEST__SM_START,
		};

	if (hub_execute_hub_request(p_gard_handle,
								&gard_resume_pipeline_request_response_data)) {
		/* Command is executed successfully. Now confirm the response. */
		if (ACK_BYTE ==
			resume_pipeline_resp.resume_pipeline_response.ack_or_nak) {
			return HUB_SUCCESS;
		}
	}

	hub_pr_err("Resume Pipeline command failed\n");
	return HUB_FAILURE_SEND_RESUME_PIPELINE;
}

/**
 * Write a given 32-bit value to a register address inside the GARD memory map
 * represented by the GARD handle.
 *
 * @param p_gard_handle The GARD handle.
 * @param reg_addr The register address inside the GARD memory map.
 * @param value The 32-bit value to write to the register.
 *
 * @return HUB_SUCCESS if the register is written successfully,
 *         HUB_FAILURE_WRITE_REG if the register is not written successfully.
 */
enum hub_ret_code hub_write_gard_reg(gard_handle_t  p_gard_handle,
									 uint32_t       reg_addr,
									 const uint32_t value)
{
	struct _gard_hub_requests write_reg_req = {
		.command_id = WRITE_REG_VALUE_TO_GARD_AT_OFFSET,
		.write_reg_value_to_gard_at_offset_request = {
		.offset_address     = reg_addr,
		.data               = value,
		.end_of_data_marker = END_OF_DATA_MARKER}};
	struct _gard_hub_responses write_reg_resp = {
		.response_id = WRITE_REG_VALUE_TO_GARD_AT_OFFSET};

	struct hub_request_response_data gard_write_reg_request_response_data = {
		.p_request = (uint8_t *)&write_reg_req,
		.request_size =
			sizeof(write_reg_req.command_id) +
			sizeof(write_reg_req.write_reg_value_to_gard_at_offset_request),

		.p_app_request        = NULL,
		.app_request_size     = 0,

		.p_validation_data    = (uint8_t *)&write_reg_req,
		.validation_data_size = sizeof(write_reg_req.command_id),

		.p_send_data          = NULL,
		.send_data_size       = 0,

		.p_response           = (uint8_t *)&write_reg_resp,
		.response_size =
			sizeof(write_reg_resp.response_id) +
			sizeof(write_reg_resp.write_reg_value_to_gard_at_offset_response),

		.p_app_response    = NULL,
		.app_response_size = 0,

		.should_send_data  = NULL,

		.state             = HUB_REQUEST__SM_START,
	};

	if (hub_execute_hub_request(p_gard_handle,
								&gard_write_reg_request_response_data)) {
		/* Command is executed successfully. Now confirm the response. */
		if (ACK_BYTE ==
			write_reg_resp.write_reg_value_to_gard_at_offset_response.ack) {
			return HUB_SUCCESS;
		}
	}

	hub_pr_err("Write Register command failed\n");
	return HUB_FAILURE_WRITE_REG;
}

/**
 * Read a given 32-bit value from a register address inside the GARD memory map
 * represented by the GARD handle.
 *
 * @param p_gard_handle The GARD handle.
 * @param reg_addr The register address inside the GARD memory map.
 * @param p_value A pointer to a 32-bit value to read from the register.
 *
 * @return HUB_SUCCESS if the register is read successfully,
 *         HUB_FAILURE_READ_REG if the register is not read successfully.
 */
enum hub_ret_code hub_read_gard_reg(gard_handle_t p_gard_handle,
									uint32_t      reg_addr,
									uint32_t     *p_value)
{
	struct _gard_hub_requests read_reg_req = {
		.command_id = READ_REG_VALUE_FROM_GARD_AT_OFFSET,
		.read_reg_value_from_gard_at_offset_request = {
		.offset_address     = reg_addr,
		.end_of_data_marker = END_OF_DATA_MARKER}};
	struct _gard_hub_responses read_reg_resp = {
		.response_id = READ_REG_VALUE_FROM_GARD_AT_OFFSET};

	struct hub_request_response_data gard_read_reg_request_response_data = {
		.p_request = (uint8_t *)&read_reg_req,
		.request_size =
			sizeof(read_reg_req.command_id) +
			sizeof(read_reg_req.read_reg_value_from_gard_at_offset_request),

		.p_app_request        = NULL,
		.app_request_size     = 0,

		.p_validation_data    = (uint8_t *)&read_reg_req,
		.validation_data_size = sizeof(read_reg_req.command_id),

		.p_send_data          = NULL,
		.send_data_size       = 0,

		.p_response           = (uint8_t *)&read_reg_resp,
		.response_size =
			sizeof(read_reg_resp.response_id) +
			sizeof(read_reg_resp.read_reg_value_from_gard_at_offset_response),

		.p_app_response    = NULL,
		.app_response_size = 0,

		.should_send_data  = NULL,

		.state             = HUB_REQUEST__SM_START,
	};

	if (hub_execute_hub_request(p_gard_handle,
								&gard_read_reg_request_response_data)) {
		/* Command is executed successfully. Now confirm the response. */
		if (read_reg_resp.read_reg_value_from_gard_at_offset_response
				.end_of_data_marker == END_OF_DATA_MARKER) {
			*p_value = read_reg_resp.read_reg_value_from_gard_at_offset_response
						   .reg_value;
			return HUB_SUCCESS;
		}
	}

	hub_pr_err("Read Register command failed\n");
	return HUB_FAILURE_READ_REG;
}

/**
 * Send data to the GARD.
 *
 * @param p_gard_handle The GARD handle.
 * @param p_buffer The buffer to send to the GARD.
 * @param addr The address in the GARD memory map to send the data to.
 * @param count The number of bytes to send.
 *
 * @return HUB_SUCCESS if the data is sent successfully,
 *         HUB_FAILURE_SEND_DATA if the data is not sent successfully.
 */
enum hub_ret_code hub_send_data_to_gard(gard_handle_t p_gard_handle,
										const void   *p_buffer,
										uint32_t      addr,
										uint32_t      count)
{
	struct _gard_hub_requests send_data_req = {
		.command_id                           = SEND_DATA_TO_GARD_FOR_OFFSET,
		.send_data_to_gard_for_offset_request = {.cmd = {.offset_address = addr,
														 .data_size    = count,
														 .control_code = 0,
														 .mtu_size     = 0}}};
	struct _gard_hub_responses send_data_resp = {
		.response_id = SEND_DATA_TO_GARD_FOR_OFFSET};

	struct hub_request_response_data gard_send_data_request_response_data = {
		.p_request = (uint8_t *)&send_data_req,
		.request_size =
			sizeof(send_data_req.command_id) +
			sizeof(send_data_req.send_data_to_gard_for_offset_request.cmd),

		.p_app_request        = NULL,
		.app_request_size     = 0,

		.p_validation_data    = (uint8_t *)&send_data_req,
		.validation_data_size = sizeof(send_data_req.command_id),

		.p_send_data          = (uint8_t *)p_buffer,
		.send_data_size       = count,

		.p_response           = (uint8_t *)&send_data_resp,
		.response_size =
			sizeof(send_data_resp.response_id) +
			sizeof(send_data_resp.send_data_to_gard_for_offset_response
					   .start_signal),

		.p_app_response    = NULL,
		.app_response_size = 0,

		.should_send_data  = hub_validate_response_start_signal,

		.state             = HUB_REQUEST__SM_START,
	};

	if (hub_execute_hub_request(p_gard_handle,
								&gard_send_data_request_response_data)) {
		/* Command is executed successfully. Now confirm the response. */
		if (ACK_BYTE ==
			send_data_resp.send_data_to_gard_for_offset_response.ack.opt_ack) {
			return HUB_SUCCESS;
		}
	}

	hub_pr_err("Send Data command failed\n");
	return HUB_FAILURE_SEND_DATA;
}

/**
 * Receive data from the GARD.
 *
 * @param p_gard_handle The GARD handle.
 * @param p_buffer The buffer to receive the data into.
 * @param addr The address in the GARD memory map to receive the data from.
 * @param count The number of bytes to receive.
 *
 * @return HUB_SUCCESS if the data is received successfully,
 *         HUB_FAILURE_RECV_DATA if the data is not received successfully.
 */
enum hub_ret_code hub_recv_data_from_gard(gard_handle_t p_gard_handle,
										  void         *p_buffer,
										  uint32_t      addr,
										  uint32_t      count)
{
	struct _gard_hub_requests recv_data_req = {
		.command_id                            = RECV_DATA_FROM_GARD_AT_OFFSET,
		.recv_data_from_gard_at_offset_request = {.offset_address = addr,
												  .data_size      = count,
												  .control_code   = 0,
												  .mtu_size       = 0}};

	struct _gard_hub_responses recv_data_resp = {
		.response_id = RECV_DATA_FROM_GARD_AT_OFFSET};

	struct hub_request_response_data gard_recv_data_request_response_data = {
		.p_request = (uint8_t *)&recv_data_req,
		.request_size =
			sizeof(recv_data_req.command_id) +
			sizeof(recv_data_req.recv_data_from_gard_at_offset_request),

		.p_app_request        = NULL,
		.app_request_size     = 0,

		.p_validation_data    = (uint8_t *)&recv_data_req,
		.validation_data_size = sizeof(recv_data_req.command_id),

		.p_send_data          = NULL,
		.send_data_size       = 0,

		.p_response           = (uint8_t *)&recv_data_resp,
		.response_size =
			sizeof(recv_data_resp.response_id) +
			sizeof(recv_data_resp.recv_data_from_gard_at_offset_response),

		.p_app_response    = NULL,
		.app_response_size = 0,

		.should_send_data  = NULL,

		.state             = HUB_REQUEST__SM_START,
	};

	if (hub_execute_hub_request(p_gard_handle,
								&gard_recv_data_request_response_data)) {
		/* Command is executed successfully. Now confirm the response. */
		if (END_OF_DATA_MARKER ==
			recv_data_resp.recv_data_from_gard_at_offset_response.eod
				.end_of_data_marker) {
			return HUB_SUCCESS;
		}
	}

	hub_pr_err("Receive Data command failed\n");
	return HUB_FAILURE_RECV_DATA;
}

/**
 * Execute an App Layer request
 *
 * @param p_gard_handle The GARD handle.
 * @param app_request_buffer The pointer to the App Layer request buffer.
 * @param app_request_buffer_size The size of the App Layer request buffer.
 * @param app_response_buffer The pointer to the App Layer response buffer.
 * @param app_response_buffer_size The size of the App Layer response buffer.
 */
bool hub_app_layer_request(gard_handle_t p_gard_handle,
						   void         *app_request_buffer,
						   uint32_t      app_request_buffer_size,
						   void         *app_response_buffer,
						   uint32_t      app_response_buffer_size)
{
	struct _gard_hub_requests hub_app_layer_request_req = {
		.command_id = HUB_APP_LAYER_REQUEST,
		.hub_app_layer_request =
			{
			.hub_app_layer_payload_size[0] =
				((uint8_t *)&app_request_buffer_size)[0],
			.hub_app_layer_payload_size[1] =
				((uint8_t *)&app_request_buffer_size)[1],
			.hub_app_layer_payload_size[2] =
				((uint8_t *)&app_request_buffer_size)[2],
			},
	};

	struct _gard_hub_responses       gard_app_layer_response_resp;

	struct hub_request_response_data hub_app_layer_request_response_data = {
		.p_request    = (uint8_t *)&hub_app_layer_request_req,
		.request_size = sizeof(hub_app_layer_request_req.command_id) +
						sizeof(hub_app_layer_request_req.hub_app_layer_request),

		.p_app_request        = app_request_buffer,
		.app_request_size     = app_request_buffer_size,

		.p_validation_data    = (uint8_t *)&hub_app_layer_request_req,
		.validation_data_size = sizeof(hub_app_layer_request_req.command_id),

		.p_send_data          = NULL,
		.send_data_size       = 0,

		.p_response           = (uint8_t *)&gard_app_layer_response_resp,
		.response_size =
			sizeof(gard_app_layer_response_resp.response_id) +
			sizeof(gard_app_layer_response_resp.gard_app_layer_response),

		.p_app_response    = app_response_buffer,
		.app_response_size = app_response_buffer_size,

		.should_send_data  = NULL,

		.state             = HUB_REQUEST__SM_START,
	};

	if (hub_execute_hub_request(p_gard_handle,
								&hub_app_layer_request_response_data)) {
		/**
		 * Command is executed successfully. The App Layer response is already
		 * in the response buffer. Return success so that the caller can analyze
		 * the response.
		 */
		return true;
	}

	return false;
}

/**
 * Check if GARD sent a GARD (or App Layer) command and execute it. This could
 * either be a command or data transfer.
 *
 * @param p_gard_handle The GARD handle.
 *
 * @return True if a GARD command was found and executed successfully, false
 * otherwise.
 */
bool hub_check_and_execute_gard_command(gard_handle_t p_gard_handle)
{
	struct hub_gard_info      *p_gard = (struct hub_gard_info *)p_gard_handle;
	dal_handle_t               dal_handle = p_gard->active_dal_bus;
	int32_t                    nread, nwrite;
	uint8_t                    gard_command_byte;
	struct _gard_hub_requests  gard_command_request;
	struct _gard_hub_responses gard_command_response;
	bool                       status;

	/**
	 * This function performs the following actions:
	 * 1. Reads one byte to see if GARD sent a command. If no command is found
	 *    it returns.
	 * 2. If the command is Originator Sync indicating that GARD is interested
	 *    in sending data to the App Layer, then we first send an Originator
	 *    Sync response to GARD so that GARD can send the App Layer Send Data.
	 * 3. We then read and handle the App Layer Send Data request.
	 */
	if (p_gard->command_execution) {
		/* Command execution is already in progress. We should not execute
		 * another command. */
		return false;
	}

	if (NULL == dal_handle) {
		hub_pr_err("No active DAL bus found for GARD profile ID: %d\n",
				   p_gard->gard_profile_id);
		return false;
	}

	/* Read one byte to see if GARD sent a command. */
	nread = dal_read(dal_handle, &gard_command_byte, sizeof(gard_command_byte),
					 0, POLL_TIME_TO_RECV_GARD_CMND_IN_US);
	if (nread != sizeof(gard_command_byte)) {
		/* Nothing found. Just return back to the caller. */
		return false;
	}

	/* Compose the request for the command that we will receive. */
	gard_command_request.command_id = gard_command_byte;

	switch (gard_command_request.command_id) {
	case GARD_APP_LAYER_REQUEST:
		/* App Layer request command found. Currently not implemented. */
		break;

	case ORIGINATOR_SYNC:
		/**
		 * Originator sync command found. This one should be from GARD, or else
		 * we would never be here.
		 */

		/* Read the originator's ID.*/
		nread = dal_read(
			dal_handle,
			&gard_command_request.originator_sync_request.originator_id,
			sizeof(gard_command_request.originator_sync_request.originator_id),
			0, 1000);
		if (nread !=
			sizeof(
				gard_command_request.originator_sync_request.originator_id)) {
			/* Nothing found. Just return back to the caller. */
			return false;
		}

		/** Confirm Originator ID is GARD. */
		if (ORIGINATOR_ID__GARD !=
			gard_command_request.originator_sync_request.originator_id) {
			/* Originator ID is not GARD. Just return back to the caller. */
			return false;
		}

		/**
		 * Originator ID is GARD. Now we compose the response Originator Sync
		 * before looking for the App Command that we will receive.
		 */
		gard_command_response.response_id = ORIGINATOR_SYNC;
		gard_command_response.originator_sync_response.originator_id =
			ORIGINATOR_ID__GARD;

		nwrite = dal_write(
			dal_handle, &gard_command_response,
			sizeof(gard_command_response.response_id) +
				sizeof(gard_command_response.originator_sync_response),
			0, 0);
		if (nwrite !=
			sizeof(gard_command_response.response_id) +
				sizeof(gard_command_response.originator_sync_response)) {
			/* Failed to write the response. Just return back to the caller. */
			return false;
		}

		p_gard->command_execution = true;

		/* Now handle the App Layer Send data. */
		status = hub_handle_app_layer_send_data(p_gard_handle);

		p_gard->command_execution = false;

		return status;
	}

	return false;
}

/**
 * Set the App Layer callbacks.
 *
 * @param p_gard_handle The GARD handle.
 * @param p_callbacks The pointer to the App Layer callbacks.
 *
 * @return True if the App Layer callbacks are set successfully, false
 * otherwise.
 */
bool hub_set_app_layer_callbacks(gard_handle_t                   p_gard_handle,
								 struct hub_app_layer_callbacks *p_callbacks)
{
	struct hub_gard_info *p_gard = (struct hub_gard_info *)p_gard_handle;

	if (NULL == p_callbacks) {
		return false;
	}

	if (NULL == p_gard->p_app_layer_cbs) {
		p_gard->p_app_layer_cbs = os_malloc(sizeof(*p_gard->p_app_layer_cbs));
	}

	*p_gard->p_app_layer_cbs = *p_callbacks;

	return true;
}

