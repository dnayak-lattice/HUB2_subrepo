/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#include "types.h"

#include "gard_hub_iface.h"
#include "Kt_Internal.h"
#include "Kt_Assert.h"

/**
 * Application calls this function to start the QR monitoring feature.
 * Once this feature is started the Application will start receiving
 * kc__on_qr_output() and kc__on_qr_failed() callbacks if they have been
 * requested during Kt_Init() by the Application.
 *
 * @param handle is the handle to the KtLib context.
 *
 * @return The status of the QR monitoring start operation.
 */
Kt_RetCode Kt_StartQrMonitor(Kt_Handle handle)
{
	struct Kt_Context             *ctxt = (struct Kt_Context *)(handle);
	struct _hub_app_layer_requests start_qr_monitoring_request = {
		.hub_app_layer_request_id = START_QR_MONITORING,
	};
	struct _gard_app_layer_responses start_qr_monitoring_response;

	KT__ASSERT(kt_ctxt == ctxt && NULL != ctxt,
			   "Invalid parameter passed to Kt_StartQrMonitor.");

	KT__ASSERT(ctxt->kc__pipeline_running, "Pipeline is not running.");

	/**
	 * Send Start QR Monitoring request to the App Layer code running over GARD.
	 */
	hub_app_layer_request(
		ctxt->kc__gard_handle, &start_qr_monitoring_request,
		sizeof(start_qr_monitoring_request.hub_app_layer_request_id),
		&start_qr_monitoring_response,
		sizeof(start_qr_monitoring_response.gard_app_layer_response_id) +
			sizeof(start_qr_monitoring_response.start_qr_monitoring_response));

	/**
	 * Ensure the App Layer code running over GARD has turned ON the QR
	 * monitoring.
	 */
	KT__ASSERT(
		start_qr_monitoring_response.start_qr_monitoring_response.ack_or_nak ==
			ACK_BYTE,
		"Failed to start QR monitoring pipeline.");

	ctxt->kc__qr_monitor_active = true;

	return KT_OK;
}

/**
 * Application calls this function to stop the QR monitoring feature.
 * When this feature is stopped the Application will stop receiving
 * kc__on_qr_output() and kc__on_qr_failed() callbacks.
 *
 * @param handle is the handle to the KtLib context.
 *
 * @return The status of the QR monitoring stop operation.
 */
Kt_RetCode Kt_StopQrMonitor(Kt_Handle handle)
{
	struct Kt_Context             *ctxt = (struct Kt_Context *)(handle);
	struct _hub_app_layer_requests stop_qr_monitoring_request = {
		.hub_app_layer_request_id = STOP_QR_MONITORING,
	};
	struct _gard_app_layer_responses stop_qr_monitoring_response;

	KT__ASSERT(kt_ctxt == ctxt && NULL != ctxt,
			   "Invalid parameter passed to Kt_StopQrMonitor.");

	KT__ASSERT(ctxt->kc__qr_monitor_active,
			   "QR monitoring pipeline is not active.");

	/**
	 * Send Stop QR Monitoring request to the App Layer code running over GARD.
	 */
	hub_app_layer_request(
		ctxt->kc__gard_handle, &stop_qr_monitoring_request,
		sizeof(stop_qr_monitoring_request.hub_app_layer_request_id),
		&stop_qr_monitoring_response,
		sizeof(stop_qr_monitoring_response.gard_app_layer_response_id) +
			sizeof(stop_qr_monitoring_response.stop_qr_monitoring_response));

	/**
	 * Ensure the App Layer code running over GARD has turned OFF the QR
	 * monitoring.
	 */
	KT__ASSERT(
		stop_qr_monitoring_response.stop_qr_monitoring_response.ack_or_nak ==
			ACK_BYTE,
		"Failed to stop QR monitoring pipeline.");

	ctxt->kc__qr_monitor_active = false;

	return KT_OK;
}

