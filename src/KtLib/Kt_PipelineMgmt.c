/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#include "types.h"
#include <string.h>

#include "hub.h"
#include "Kt_Lib.h"
#include "Kt_Internal.h"
#include "Kt_Assert.h"

/**
 * Application calls this function to start the pipeline. The pipeline needs
 * to be started before the Application can start the face identification
 * or the QR monitoring. The Application can control the pipeline to control
 * the power consumption of the GARD hardware in times of low activity.
 *
 * @param handle is the handle to the KtLib context.
 *
 * @return The status of the pipeline start operation.
 */
Kt_RetCode Kt_StartPipeline(Kt_Handle handle)
{
	struct Kt_Context *ctxt = (struct Kt_Context *)(handle);

	KT__ASSERT(kt_ctxt == ctxt,
			   "Invalid parameter passed to Kt_StartPipeline.");

	if (ctxt->kc__pipeline_running) {
		os_pr_info("Pipeline is already running.");
		return KT_OK;
	}

	/* <SRP-TBD> Currently the pipeline is ALWAYS running. */

	ctxt->kc__pipeline_running = true;
	return KT_OK;
}

/**
 * Application calls this function to stop the pipeline. When the pipeline
 * is stoped the face identification and the QR monitoring are also stopped.
 *
 * @param handle is the handle to the KtLib context.
 *
 * @return The status of the pipeline stop operation.
 */
Kt_RetCode Kt_StopPipeline(Kt_Handle handle)
{
	struct Kt_Context *ctxt = (struct Kt_Context *)(handle);

	KT__ASSERT(kt_ctxt == ctxt, "Invalid parameter passed to Kt_StopPipeline.");

	if (!ctxt->kc__pipeline_running) {
		return KT_OK;
	}

	/* < SRP-TBD> Currently we do not have a way of stopping the pipeline. */

	ctxt->kc__pipeline_running = false;
	return KT_OK;
}

