/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#include "types.h"
#include <stddef.h>

#include "gard_hub_iface.h"
#include "Kt_Internal.h"
#include "Kt_Assert.h"

/**
 * Application calls this function to start the face identification feature.
 * When this feature is started the Application will start receiving
 * kc__on_face_identified() and kc__on_face_not_identified() callbacks if
 * they have been requested during Kt_Init() by the Application.
 *
 * @param handle is the handle to the KtLib context.
 *
 * @return The status of the face identification start operation.
 */
Kt_RetCode Kt_StartFaceIdentification(Kt_Handle handle)
{
	struct Kt_Context             *ctxt = (struct Kt_Context *)(handle);
	struct _hub_app_layer_requests start_face_id_request = {
		.hub_app_layer_request_id = START_FACE_IDENTIFICATION,
	};
	struct _gard_app_layer_responses start_face_id_response;

	KT__ASSERT(kt_ctxt == ctxt && NULL != ctxt,
			   "Invalid parameter passed to Kt_StartFaceIdentification.");

	KT__ASSERT(ctxt->kc__pipeline_running, "Pipeline is not running.");

	/**
	 * Send Start Face Identification Pipeline request to the App Layer code
	 * running over GARD.
	 */
	hub_app_layer_request(
		ctxt->kc__gard_handle, &start_face_id_request,
		sizeof(start_face_id_request.hub_app_layer_request_id),
		&start_face_id_response,
		sizeof(start_face_id_response.gard_app_layer_response_id) +
			sizeof(start_face_id_response.start_face_identification_response));

	/**
	 * Ensure the App Layer code running over GARD has turned ON the pipeline.
	 */
	KT__ASSERT(
		start_face_id_response.start_face_identification_response.ack_or_nak ==
			ACK_BYTE,
		"Failed to start face identification pipeline.");

	ctxt->kc__face_id_monitoring_active = true;

	return KT_OK;
}

/**
 * Application calls this function to stop the face identification feature.
 * When this feature is stopped the Application will stop receiving
 * kc__on_face_identified() and kc__on_face_not_identified() callbacks.
 *
 * @param handle is the handle to the KtLib context.
 *
 * @return The status of the face identification stop operation.
 */
Kt_RetCode Kt_StopFaceIdentification(Kt_Handle handle)
{
	struct Kt_Context             *ctxt = (struct Kt_Context *)(handle);
	struct _hub_app_layer_requests stop_face_id_request = {
		.hub_app_layer_request_id = STOP_FACE_IDENTIFICATION,
	};
	struct _gard_app_layer_responses stop_face_id_response;

	KT__ASSERT(kt_ctxt == ctxt && NULL != ctxt,
			   "Invalid parameter passed to Kt_StopFaceIdentification.");

	KT__ASSERT(ctxt->kc__face_id_monitoring_active,
			   "Face identification pipeline is not active.");

	/**
	 * Send Stop Face Identification Pipeline request to the App Layer code
	 * running over GARD.
	 */
	hub_app_layer_request(
		ctxt->kc__gard_handle, &stop_face_id_request,
		sizeof(stop_face_id_request.hub_app_layer_request_id),
		&stop_face_id_response,
		sizeof(stop_face_id_response.gard_app_layer_response_id) +
			sizeof(stop_face_id_response.stop_face_identification_response));

	/**
	 * Ensure the App Layer code running over GARD has turned ON the pipeline.
	 */
	KT__ASSERT(
		stop_face_id_response.stop_face_identification_response.ack_or_nak ==
			ACK_BYTE,
		"Failed to stop face identification pipeline.");

	ctxt->kc__face_id_monitoring_active = false;

	return KT_OK;
}

/**
 * Application calls this function to generate a face ID asynchronously.
 * Once the face ID has been generated the generate_done_handler is invoked.
 *
 * Important Note:
 * Please note that the invocation of generate_done_handler
 * App_GenerateFaceIdDone() by KtLib could either happen:
 * 1. while this function is still executing (as far as Application is
 * concerned)
 * 2. after this function has returned back to the Application.
 *
 * The Application should be prepared for both the scenarios and handle them
 * accordingly.
 *
 * @param handle is the handle to the KtLib context.
 * @param generate_done_handler is the callback function invoked when the
 * 		  face ID has been generated.
 *
 * @return The status of the face ID generation operation.
 */
Kt_RetCode Kt_GenerateFaceId_async(Kt_Handle              handle,
								   App_GenerateFaceIdDone generate_done_handler)
{
	struct Kt_Context             *ctxt = (struct Kt_Context *)(handle);
	struct _hub_app_layer_requests generate_face_id_request = {
		.hub_app_layer_request_id = GENERATE_FACE_ID,
	};
	struct _gard_app_layer_responses generate_face_id_response;

	KT__ASSERT(handle != NULL || kt_ctxt == ctxt,
			   "Invalid parameter passed to Kt_GenerateFaceId_async.");

	KT__ASSERT(generate_done_handler != NULL, "Callback handler is NULL.");

	KT__ASSERT(ctxt->kc__face_id_monitoring_active,
			   "Face-ID module is not active.");

	/**
	 * Send Generate Face Identification request to the App Layer code running
	 * over GARD.
	 */
	hub_app_layer_request(
		ctxt->kc__gard_handle, &generate_face_id_request,
		sizeof(generate_face_id_request.hub_app_layer_request_id),
		&generate_face_id_response,
		sizeof(generate_face_id_response.gard_app_layer_response_id) +
			sizeof(generate_face_id_response.generate_face_id_response));

	/* <TBD-SRP> Currently we discard the face location within the frame. */

	/**
	 * The callback handler is invoked for the Application to notify the Face
	 * ID.
	 */
	generate_done_handler(
		ctxt->kc__cfg.kc__app_handle,
		generate_face_id_response.generate_face_id_response.face_id,
		(Kt_RetCode)KT_OK);

	return KT_OK;
}

/**
 * Application calls this function to delete a face ID asynchronously.
 * Once the face ID has been deleted the delete_done_handler is invoked.
 *
 * Important Note:
 * Please note that the invocation of delete_done_handler
 * App_DeleteFaceIdDone() by KtLib could either happen:
 * 1. while this function is still executing (as far as Application is
 *    concerned)
 * 2. after this function has returned back to the Application.
 *
 * The Application should be prepared for both the scenarios and handle them
 * accordingly.
 *
 * @param handle is the handle to the KtLib context.
 * @param face_id is the face ID to be deleted.
 * @param delete_done_handler is the callback function invoked when the face
 * 		  ID has been deleted.
 *
 * @return The status of the face ID deletion operation.
 */
Kt_RetCode Kt_DeleteFaceId_async(Kt_Handle            handle,
								 uint32_t             face_id,
								 App_DeleteFaceIdDone done_handler)
{
	struct Kt_Context             *ctxt = (struct Kt_Context *)(handle);
	struct _hub_app_layer_requests delete_face_id_request = {
		.hub_app_layer_request_id       = DELETE_FACE_ID,
		.delete_face_id_request.face_id = face_id,
	};
	struct _gard_app_layer_responses delete_face_id_response;

	/**
	 * Send Delete Face Identification request to the App Layer code running
	 * over GARD.
	 */
	hub_app_layer_request(
		ctxt->kc__gard_handle, &delete_face_id_request,
		sizeof(delete_face_id_request.hub_app_layer_request_id) +
			sizeof(delete_face_id_request.delete_face_id_request),
		&delete_face_id_response,
		sizeof(delete_face_id_response.gard_app_layer_response_id) +
			sizeof(delete_face_id_response.delete_face_id_response));

	KT__ASSERT(delete_face_id_response.delete_face_id_response.ack_or_nak ==
				   ACK_BYTE,
			   "Failed to delete face ID.");

	/**
	 * The callback handler is invoked for the Application to notify the Face ID
	 * deletion.
	 */
	done_handler(ctxt->kc__cfg.kc__app_handle,
				 (Kt_RetCode)(delete_face_id_response.delete_face_id_response
								  .ack_or_nak == ACK_BYTE)
					 ? KT_OK
					 : KT_ERR_GENERIC);

	return KT_OK;
}

