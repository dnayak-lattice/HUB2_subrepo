/******************************************************************************
 * Copyright (c) 2026 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#include "kt_app.h"
#include "kt_app_logger.h"

/**
  * Kt App Main Function Implementation

  * This function interacts with KtLib, which in turn interacts with the
  hardware.
  *
  * 1. It knows the bus type and device node, and it creates a DAL handle
  through a struct dal_bus.
  * 2. It calls the KtLib provided Kt_GetIfaceInfo function to get the bus
  specific DAL bus properties.
  * 3. It is expected to initialize the DAL handle and open the bus device
  thorugh dal_init function.
  * 4. It creates the KtLib configuration structure and populates it with the
  App specific callbakc functions.
  * 5. It initializes KtLib using the Kt_Init function, and gets a Kt_Handle.
  This Kt_Handle is used to interact with KtLib.
  * 6. It adds the metadata buffers to KtLib.
  * 7. It then starts the pipeline(as needed), in particular, face
  identification.
  * 8. Finally, it runs the KtLib's  dispatcher.
  *
  */

/* kt_app_main globals */

/**
 * Communications with GARD FW needs a bus interface defined through a DAL
 * handle.
 * The current release supports only the UART bus.
 *
 * Kt_App has knowledge about the bus type and device node, but not
 * the baud rate at which the UART is running. The baud rate is obtained
 * from the KtLib.
 *
 * IMPORTANT NOTE: The user should change the bus_dev string to the appropriate
 * value before using the Kt App.
 *
 * The value "/dev/ttyUSB2" applies to the Katan carrier board hardware meant
 * for demonstrating the Kt App. Other platforms could have different values for
 * the bus_dev string.
 */
static struct dal_bus dal_bus = {
	.bus_type = HUB_GARD_BUS_UART,
	.props.uart_props =
		{
		.bus_dev   = "/dev/ttyUSB2",
		.baud_rate = 0,
		},
};

/**
 * Main function for the Kt App
 *
 * @param arg The argument
 *
 * @return void * - 0 on success, -1 on failure
 */
void *kt_app_main(void *arg)
{
	struct kt_app_ctx *p_app_ctx             = (struct kt_app_ctx *)arg;
	bool               metadata_queue_inited = false;

	enum dal_ret_code  ret;
	(void)ret;
	uint32_t i;
	(void)i;
	(void)dal_bus;

	/* Print the DAL Bus type and properties */
	os_pr_info("DAL Bus type: ");
	switch (dal_bus.bus_type) {
	case HUB_GARD_BUS_UART:
		os_pr_info("UART, device node: %s\n", dal_bus.props.uart_props.bus_dev);
		break;
	default:
		os_pr_info("Unknown bus type\n");
		goto err_kt_app_main;
	}

	os_pr_info("******************************************************\n");

	/* Check if the shared memory is initialized */
	if (p_app_ctx->kt_app_shmem == NULL) {
		os_pr_err("Shared memory is not initialized\n");
		goto err_kt_app_main;
	}

	/* Initialize the shared memory request and response structures */
	p_app_ctx->kt_app_shmem->request.cmd       = KT_APP_CMD_NONE;
	p_app_ctx->kt_app_shmem->response.ret_code = KT_APP_RET_CODE_PENDING;

	/* Initialize the metadata buffer queue */
	kt_app_metadata_buffer_queue_init((App_Handle)p_app_ctx);
	metadata_queue_inited = true;

#if defined(KT_APP_SIMULATOR_MODE)
	/**
	 * For simulation to test the IPC and Python clients.
	 * This code is not used in the real application.
	 */
	kt_app_idle_cb((App_Handle)p_app_ctx);
	return (void *)0;

#else /* KT_APP_SIMULATOR_MODE */

	/* Interacts with hardware via KtLib */

	/**
	 * This struct holds details on all interfaces supported by the KtLib.
	 */
	struct IfaceData iface_data = {0};

	/**
	 * KtLib gets the bus specific properties (in this case UART baudrate).
	 * It also marks the interface as valid.
	 */
	Kt_GetIfaceInfo(&iface_data);
	if (!iface_data.uart_data.uid__is_valid) {
		os_pr_err("UART interface is not valid\n");
		goto err_kt_app_main;
	}

	/* Kt_Appinitializes the DAL handle and opens the bus device */
	dal_bus.props.uart_props.baud_rate = iface_data.uart_data.uid__baud_rate;
	ret                                = dal_init((dal_handle_t)&dal_bus);
	if (ret != DAL_SUCCESS) {
		os_pr_err("Failed to initialize DAL handle\n");
		goto err_kt_app_main;
	}

	/**
	 * Create the KT config structure.
	 * KtLib callbacks are set to NULL for now.
	 */
	struct KtCallbacks kt_callbacks = {
		.kc__on_qr_output           = NULL,
		.kc__on_qr_failed           = NULL,
		.kc__on_face_identified     = kt_app_face_identified_cb,
		.kc__on_face_not_identified = kt_app_face_not_identified_cb,
		.kc__idle_callback          = kt_app_idle_cb,
	};

	/* KtLib configuration flags - autostart some pipelines*/
	struct KtConfigFlags kt_config_flags = {
		.kcf__auto_start_pipeline       = true,
		.kcf__auto_start_face_id_detect = true,
		.kcf__auto_start_qr_monitor     = false,
	};

	/**
	 * KtConfig is used to configure and initialize KtLib.
	 * It also includes the DAL handle to the bus device.
	 * As of this release, we have a single DAL handle.
	 */
	struct KtConfig kt_config = {
		.kc__config_flags = kt_config_flags,
		.kc__callbacks    = kt_callbacks,
		.kc__app_handle   = (App_Handle)p_app_ctx,
		.kc__dal_count    = 1,
		.kc__dal_handles  = {0},
	};

	/* Add the DAL handle ot the array of data handles for use by KtLib */
	kt_config.kc__dal_handles[0] = (dal_handle_t)&dal_bus;

	/**
	 * Initialize KtLib - this will return a Kt_Handle.
	 * This Kt_Handle is used to interact with KtLib.
	 */
	p_app_ctx->kt_handle         = Kt_Init(&kt_config);
	if (p_app_ctx->kt_handle == NULL) {
		os_pr_err("Failed to initialize KtLib\n");
		goto err_kt_app_main;
	}

	/* Supply KtLib with an initial set of metadata buffers */
	for (i = 0; i < KT_APP_METADATA_BUFFER_POOL_SIZE; i++) {
		Kt_AddMetadataBuffer(p_app_ctx->kt_handle,
							 p_app_ctx->buffer_queue.buffers[i].p_buffer,
							 KT_APP_METADATA_BUFFER_SIZE);
	}

	/* Just a test for idempotency*/
	Kt_StartPipeline(p_app_ctx->kt_handle);

	/* Start Face Identification */
	Kt_StartFaceIdentification(p_app_ctx->kt_handle);

	/**
	 * Run the KtLib dispatcher.
	 *
	 * The second argument here is run_once, which is false.
	 * This means that the dispatcher will run forever until the application is
	 * terminated.

	 * The KtLib dispatcher will call the Kt App priovided callbacks depending
	 * on the events that happen.
	 *
	 * The KtLib dispatcher will also call the Kt Apps's idle callback when
	 * there is no event to process. The Kt App is expected to handle any house
	 * keeping tasks, etc. in the idle callback.
	 */
	Kt_RunDispatcher(p_app_ctx->kt_handle, false);

#endif /* KT_APP_SIMULATOR_MODE */

	return (void *)0;

err_kt_app_main:
	if (metadata_queue_inited) {
		kt_app_metadata_buffer_queue_fini((App_Handle)p_app_ctx);
	}
	p_app_ctx->should_exit = true;
	return (void *)-1;
}
