/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#include "types.h"

#include "os_funcs.h"

#include "hub.h"
#include "Kt_Lib.h"
#include "Kt_Internal.h"
#include "Kt_Assert.h"
#include "Kt_Version.h"

struct Kt_Context *kt_ctxt = NULL;

const char *kt_version_string = KT_VERSION_STRING;

/**
 * Get the version string of the KtLib.
 *
 * @return A pointer to the version string.
 */
const char *Kt_GetVersionString(void)
{
	return kt_version_string;
}

/**
 * Application calls this function to get the information about the possible
 * interfaces on which the GARD firmware can be communicated with. It also
 * provides some more information that the Application can use to filter out
 * non-GARD devices. Some of the examples are as below:
 * 1) UART - If UART is supported then the UART information lists the baud
 *			 rate at which the UART needs to be set. If specific flow
 *			 control is expected then even those values will be filled so
 *			 that the Application can setup the UART hardware before
 *			 providing the UART DAL to Kt_Init().
 * 2) USB - If USB is supported then the USB speed and USB device IDs (PID
 * 			and VID) are provided which the Application can use to filter
 * 			out "other" USB devices and only provide the DAL for the USB
 * 			while possibly could contain GARD hardware.
 * 3) I2C - If I2C is supported then the target ID of the GARD hardware is
 * 			provided that the Application can use to probe on all the
 * 			available I2C buses. Application can then create a DAL for this
 * 			I2C device and provide it to Kt_Init().
 * Note: In the current GARD I2C/USB is not provided but the above write-up is
 * provided as an example.
 */
void Kt_GetIfaceInfo(struct IfaceData *p_if_data)
{
	KT__ASSERT(p_if_data != NULL, "p_if_data is NULL");

	/* Currently only UART is supported. */
	p_if_data->uart_data.uid__is_valid  = true;
	p_if_data->uart_data.uid__baud_rate = 921600;

	p_if_data->usb_data.uid__is_valid   = false;

	return;
}

/**
 * Application calls this function to Initialize the Kt library. Before
 * calling this function the Application initializes KtConfig structure and
 * provides it to this function. In KtConfig structure the Application
 * requests the features that it needs the KtLib to confgure as.
 * The Application also provides the DAL handle(s) for the hardware
 * interfaces that can be used by KtLib to reach the GARD hardware.
 * KtLib uses this opportunity to discover the GARD hardware and configure
 * the library accordingly. All the DAL handles on which GARD is not
 * available are passed back to the Application in the same KtConfig
 * structure.
 *
 * @param p_cfg is the pointer to the initialized KtConfig structure.
 *
 * @return The handle to the KtLib context if the initialization is
 * 		   successful; NULL if the initialization fails.
 */
Kt_Handle Kt_Init(const struct KtConfig *p_cfg)
{
	struct hub_config hub_config = {
		.num_busses = 0,
	};
	enum hub_ret_code              ret_hub_code;
	struct hub_app_layer_callbacks app_layer_cbs = {
		.app_handle                    = kt_ctxt,
		.get_buffer_for_gard_send_data = Kt_GetBufferForGardSendData,
		.handle_gard_send              = Kt_HandleGardSendData,
		.handle_gard_request           = Kt_HandleGardRequest,
	};

	KT__ASSERT(p_cfg != NULL, "p_cfg is NULL");

	KT__ASSERT(
		kt_ctxt == NULL,
		"Kt_Init is called multiple times without calling Kt_Fini() first.\n.");

	kt_ctxt = (struct Kt_Context *)Os_MemAlloc(p_cfg->kc__app_handle,
											   sizeof(*kt_ctxt));

	KT__ASSERT(kt_ctxt != NULL, "Failed to allocate KtLib context.\n");

	/* Initialize the context. */
	Os_MemSet(p_cfg->kc__app_handle, kt_ctxt, 0, sizeof(*kt_ctxt));

	/* Keep a copy of the configuration in the context for easy access. */
	kt_ctxt->kc__cfg                  = *p_cfg;

	kt_ctxt->kc__stop_dispatcher_loop = false;

	/**
	 * Since we just allocated space for the context pointer, add it to the
	 * callbacks strucuture
	 */
	app_layer_cbs.app_handle          = kt_ctxt;

	/**
	 * Initialize the HUB configuration with all the DAL handles provided by the
	 * application.
	 */
	hub_config.num_busses             = p_cfg->kc__dal_count;
	for (uint32_t i = 0; i < p_cfg->kc__dal_count; i++) {
		hub_config.dal_busses[i] = p_cfg->kc__dal_handles[i];
	}

	/* Initialize HUB */
	kt_ctxt->kc__hub_handle = hub_preinit(&hub_config);
	KT__ASSERT(kt_ctxt->kc__hub_handle != NULL, "Failed to initialize HUB.\n");

	/* Discover GARDS on the HUB */
	ret_hub_code = hub_discover_gards(kt_ctxt->kc__hub_handle);
	KT__ASSERT(ret_hub_code == HUB_SUCCESS, "Failed to discover GARDS.\n");

	/* Initialize the HUB */
	ret_hub_code = hub_init(kt_ctxt->kc__hub_handle);
	KT__ASSERT(ret_hub_code == HUB_SUCCESS, "Failed to initialize HUB.\n");

	/**
	 * <SRP-TBD> At this point we should know (from HUB) which DAL handles he
	 * will be using so that the remaining handles we will send back to the
	 * caller in the same p_cfg->kc__dal_handles array indicating that these are
	 * not needed by KtLib (or HUB).
	 * This code of getting the unused DAL handles from HUB still needs to be
	 * implemented.
	 */

	/**
	 * Get the GARD connected to HUB. Currently we support just one GARD
	 * device.
	 */

	KT__ASSERT(hub_get_num_gards(kt_ctxt->kc__hub_handle) == 1,
			   "We support one GARD.\n");

	kt_ctxt->kc__gard_handle = hub_get_gard_handle(kt_ctxt->kc__hub_handle, 0);

	KT__ASSERT(kt_ctxt->kc__gard_handle != NULL,
			   "Failed to get GARD handle.\n");

	/* Initialize the metadata and QR string buffer Fifos. */
	Kt_BufFifo_Init(&kt_ctxt->kc__meta_buf_fifo);
	Kt_BufFifo_Init(&kt_ctxt->kc__qr_buf_fifo);

	/* <SRP-TBD> Any other KtLib related initialization should be done here */

	/**
	 * Allocate memory for holding the HMI metadat and QR image that will be
	 * sent by our peer App Layer running over GARD.
	 */
	kt_ctxt->kc__hmi_metadata_buffer = (uint8_t *)Os_MemAlloc(
		p_cfg->kc__app_handle, MAX_HMI_METADATA_BUFFER_SIZE);
	KT__ASSERT(kt_ctxt->kc__hmi_metadata_buffer != NULL,
			   "Failed to allocate memory for HMI metadata buffer.\n");
	kt_ctxt->kc__hmi_metadata_buffer_size = MAX_HMI_METADATA_BUFFER_SIZE;

	kt_ctxt->kc__qr_image_buffer =
		(uint8_t *)Os_MemAlloc(p_cfg->kc__app_handle, MAX_QR_IMAGE_BUFFER_SIZE);
	KT__ASSERT(kt_ctxt->kc__qr_image_buffer != NULL,
			   "Failed to allocate memory for QR image buffer.\n");
	kt_ctxt->kc__qr_image_buffer_size = MAX_QR_IMAGE_BUFFER_SIZE;

	/* Hook up the callback handlers. */
	hub_set_app_layer_callbacks(kt_ctxt->kc__gard_handle, &app_layer_cbs);

	/* Handle auto-start flags. */

	/**
	 * <TBD-SRP> Currently the pipeline is already running. So we ignore this
	 * flag.
	 */
	if (p_cfg->kc__config_flags.kcf__auto_start_pipeline) {
		KT__ASSERT(Kt_StartPipeline((Kt_Handle)kt_ctxt) == KT_OK,
				   "Failed to start pipeline.");
	}

	/* Auto-start Face Identification. */
	if (p_cfg->kc__config_flags.kcf__auto_start_face_id_detect) {
		(void)Kt_StartFaceIdentification((Kt_Handle)kt_ctxt);
	}

	/* Auto-start QR Detection. */
	if (p_cfg->kc__config_flags.kcf__auto_start_qr_monitor) {
		(void)Kt_StartQrMonitor((Kt_Handle)kt_ctxt);
	}

	return (Kt_Handle)kt_ctxt;
}

/**
 * Application calls this function when it no longer needs to use KtLib and
 * needs to release all the KtLib alliocated resources.
 *
 * @param handle is the handle to the KtLib context.
 *
 * @return None
 */
void Kt_Fini(Kt_Handle handle)
{
	struct Kt_Context *ctxt = (struct Kt_Context *)(handle);
	App_Handle         app_handle;

	KT__ASSERT(kt_ctxt == ctxt, "Invalid parameter passed to Kt_Fini.");

	app_handle = ctxt->kc__cfg.kc__app_handle;

	if (ctxt->kc__qr_monitor_active) {
		(void)Kt_StopQrMonitor(kt_ctxt);
	}

	if (ctxt->kc__face_id_monitoring_active) {
		(void)Kt_StopFaceIdentification(kt_ctxt);
	}

	Kt_StopPipeline(kt_ctxt);

	/* Release all buffers that the App had provided to us. */
	Kt_ReleaseMetadataBuffers(kt_ctxt);
	Kt_ReleaseQrStringBuffers(kt_ctxt);

	Os_MemFree(app_handle, ctxt);
}

