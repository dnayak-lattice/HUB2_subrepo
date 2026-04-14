/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#include "Kt_Assert.h"
#include "Kt_Internal.h"
#include "gard_hub_iface.h"

/**
 * Application calls this function to run the dispatcher. The dispatcher
 * performs various tasks internally and invokes the callbacks which the
 * Application had requested during Kt_Init() by the Application.
 *
 * The Dispatcher is run in one of the two modes based on the Application
 * design:
 * 1. Continuous mode: The dispatcher runs continuously until
 *    Kt_StopDispatcher() is called. This is the default mode.
 * 2. One-time mode: The dispatcher runs only once and returns. It performs
 *    the same tasks as it does in the continuous mode, but only once. This
 *    way the Application gets the control of the system and can perform
 *    other tasks before invoking the Dispatcher again.
 *
 * @param handle is the handle to the KtLib context.
 * @param run_once is a flag to indicate if the dispatcher should run only
 * 		  once.
 *
 * @return None.
 */
void Kt_RunDispatcher(Kt_Handle handle, bool run_once)
{
	struct Kt_Context *ctxt = (struct Kt_Context *)(handle);

	KT__ASSERT(NULL != ctxt && kt_ctxt == ctxt,
			   "Invalid parameter passed to Kt_RunDispatcher.");

	ctxt->kc__stop_dispatcher_loop = false;
	do {
		/**
		 * In the current implementation the Generate and Delete face ID
		 * operations are synchronous.
		 * Therefor we do not need to monitor for their completions and invoke
		 * their done callbacks.
		 */

		/**
		 * Execute GARD command if it has arrived on the wire.
		 * Metadata and QR code will be dispatched internally through a nested
		 * chain of calls.
		 */
		hub_check_and_execute_gard_command(ctxt->kc__gard_handle);

		/**
		 * <TBD-SRP>
		 * If there is any other background work fir Kt or HUB to perform then
		 * it should be done here.
		 */

		/* Call the idle callback if it is set. */
		if (ctxt->kc__cfg.kc__callbacks.kc__idle_callback) {
			ctxt->kc__cfg.kc__callbacks.kc__idle_callback(
				ctxt->kc__cfg.kc__app_handle);
		}

	} while (!run_once && !ctxt->kc__stop_dispatcher_loop);
}

/**
 * Application calls this function to exit the continuous Dispatcher loop.
 *
 * @param handle is the handle to the KtLib context.
 *
 * @return None.
 */
void Kt_StopDispatcher(Kt_Handle handle)
{
	struct Kt_Context *ctxt = (struct Kt_Context *)(handle);

	KT__DBG_ASSERT(NULL != ctxt && kt_ctxt == ctxt,
				   "Invalid parameter passed to Kt_StopDispatcher.\n");

	/**
	 * Just set this flag and the dispatcher will exit the loop on the next
	 * iteration.
	 */
	ctxt->kc__stop_dispatcher_loop = true;
}

/**
 * Kt_GetBufferForGardSendData returns an appropriate buffer to the caller (HUB)
 * based on the contents of the peek buffer. The peek buffer is supposed to
 * contain enough of the incoming data sample for this routine to determine the
 * type of data so that the appropriate buffer can be returned to the caller.
 * Typically HUB will read a few sample bytes of incoming data in the peek
 * buffer and call this routine to get the appropriate buffer for reading the
 * complete data from GARD.
 *
 * @param p_gard_handle is the handle to the GARD for which the buffer is
 * requested.
 * @param p_buffer points to the pointer which will point to the buffer to use.
 * @param p_buffer_size points to the variable that will be filled with the size
 * 		  of the buffer.
 * @param p_peek_buffer points to the peek buffer.
 * @param p_peek_buffer_size tells the size of the peek buffer.
 *
 * @return True if the buffer was successfully returned, false otherwise.
 */
bool Kt_GetBufferForGardSendData(gard_handle_t p_gard_handle,
								 void         *app_handle,
								 void        **p_buffer,
								 uint32_t     *p_buffer_size,
								 void         *p_peek_buffer,
								 uint32_t      peek_buffer_size)
{
	struct Kt_Context            *ctxt = (struct Kt_Context *)(app_handle);
	struct _gard_app_layer_sends *p_app_layer_send =
		(struct _gard_app_layer_sends *)p_peek_buffer;

	KT__DBG_ASSERT(
		NULL != ctxt && kt_ctxt == ctxt,
		"Invalid parameter passed to Kt_GetBufferForGardSendData.\n");

	/**
	 * Interpret the peek buffer to determine the buffer to return to HUB.
	 */
	switch (p_app_layer_send->gard_app_layer_send_command) {
	case HMI_METADATA:
		/**
		 * The peek buffer hints metadata.
		 */
		*p_buffer      = ctxt->kc__hmi_metadata_buffer;
		*p_buffer_size = ctxt->kc__hmi_metadata_buffer_size;
		break;
	case QR_CODE_IMAGE:
		/**
		 * The peek buffer contains the QR code image.
		 */
		*p_buffer      = ctxt->kc__qr_image_buffer;
		*p_buffer_size = ctxt->kc__qr_image_buffer_size;
		break;
	default:
		/**
		 * Unsuported command.
		 */
		KT__DBG_ASSERT(false, "Unsupported command: %d",
					   p_app_layer_send->gard_app_layer_send_command);
		return false;
	}

	return true;
}

/**
 * Kt_HandleGardSendData handles the data that was received from GARD (via HUB).
 * Currently we support only two types of data: metadata and QR code image.
 *
 * @param p_gard_handle The GARD handle.
 * @param p_buffer points to the data buffer that contains the data
 *				   received from GARD.
 * @param buffer_size tells the size of the data buffer that contains the data
 * 					  received from GARD.
 *
 * @return True if the data is successfully handled, false otherwise
 */
bool Kt_HandleGardSendData(gard_handle_t p_gard_handle,
						   void         *app_handle,
						   void         *p_buffer,
						   uint32_t      buffer_size)
{
	struct _gard_app_layer_sends *p_app_layer_send =
		(struct _gard_app_layer_sends *)p_buffer;

	KT__DBG_ASSERT(NULL != app_handle,
				   "Invalid parameter passed to Kt_HandleGardSendData.");

	switch (p_app_layer_send->gard_app_layer_send_command) {
	case HMI_METADATA:
		/* Parse the metadata structure to compose the PpeData structure. */
		return Kt_HandleReceivedMetadata(
			p_gard_handle, app_handle, &p_app_layer_send->hmi_metadata_send,
			buffer_size -
				sizeof(p_app_layer_send->gard_app_layer_send_command));
	case QR_CODE_IMAGE:
		/* Handle the QR image that was received from the FW. */
		return Kt_HandleReceivedQRImage(p_gard_handle, app_handle,
										&p_app_layer_send->qr_code_image_send,
										buffer_size) -
			   sizeof(p_app_layer_send->gard_app_layer_send_command);
	}

	return false;
}

/**
 * Kt_HandleGardRequest will be called by HUB when it receives a request from
 * App Layer running over GARD. Currently GARD *does not* call HUB for any such
 * requests. This call is provided as a placeholder for future use.
 *
 * @param p_gard_handle is the handle to the GARD for which the request is
 * received.
 * @param p_request points to the request buffer that contains the request
 * 					received from GARD.
 * @param request_size tells the size of the request buffer that contains the
 * request received from GARD.
 * @param p_response points to the response buffer that contains the
 * 					 response to send to GARD.
 * @param response_size tells the size of the response buffer that contains the
 * 						response to send to GARD.
 *
 * @return True if the request is successfully handled, false otherwise
 */
bool Kt_HandleGardRequest(gard_handle_t p_gard_handle,
						  void         *app_handle,
						  void         *p_request,
						  uint32_t      request_size,
						  void         *p_response,
						  uint32_t      response_size)
{
	struct Kt_Context *ctxt = (struct Kt_Context *)(app_handle);
	KT__DBG_ASSERT(NULL != ctxt && ctxt == kt_ctxt,
				   "Invalid parameter passed to Kt_HandleGardRequest.\n");

	p_gard_handle = p_gard_handle;
	app_handle    = app_handle;
	p_request     = p_request;
	request_size  = request_size;
	p_response    = p_response;
	response_size = response_size;
	ctxt          = ctxt;

	return false;
}

