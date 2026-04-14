/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#ifndef __KT_LIB_H__
#define __KT_LIB_H__

#include "types.h"
#include "dal.h"
#include "os_funcs.h"

/**
 * This file serves as a contract between KtLib and the Application. This file
 * lists all the functions and data structures that both KtLib and the
 * Application need to access.
 *
 * This file contains the data structures and the functions that are useful for
 * an Application intending to integrate KtLib.
 *
 * The application must implement the Os_* functions declared at the bottom
 * of this header to supply platform-specific memory and I/O services.
 */

/* Maximum number of DAL handles that can be used by KtLib. */
#define DAL_MAX_COUNT                 10

/* Minimum size of any buffer to be provided to KtLib */
#define MIN_BUF_SIZE                  16

/* Maximum number of users that we support detecting in any frame. */
#define MAX_USERS_SUPPORTED_PER_FRAME 20

#ifdef __cplusplus
extern "C" {
#endif

	/* Kt_Handle an opaque data structure that is passed to Kt functions. */
	typedef void *Kt_Handle;

	/**
	 * App_Handle is an opaque data structure that is passed to App functions.
	 */
	typedef void *App_Handle;

	/* SRP-TBD - The return code distribution needs to be revisited. */
	/**
	 * @brief Result codes for KtLib operations and callbacks.
	 *
	 * Zero (@ref KT_OK) means success. Negative values classify failures;
	 * subsets are reserved for QR (@c -100..) and face-ID (@c -200..) as listed
	 * below. Callbacks that take @ref Kt_RetCode use these same constants to
	 * report hub or library outcomes.
	 */
	typedef enum Kt_RetCode {
		/** Operation completed successfully. */
		KT_OK                                 = 0,

		/** Unspecified / catch-all failure. */
		KT_ERR_GENERIC                        = -1,

		/** I/O error during SOM communication. */
		KT_ERR_IO                             = -2,

		/** KtLib has not been initialised (Kt_Init not called or failed). */
		KT_ERR_NOT_INITED                     = -3,

		/** Requested feature is not supported by the current SOM firmware. */
		KT_ERR_UNSUPPORTED                    = -4,

		/** Communication with the SOM timed out. */
		KT_ERR_TIMEOUT                        = -5,

		/* --- QR-related error codes --- */

		/** QR code was detected but could not be decoded. */
		KT_ERR_QR_DECODE_FAIL                 = -100,

		/* --- Face-ID-related error codes --- */

		/** Face-ID generation/matching failed on the SOM. */
		KT_ERR_FACE_ID_FAIL                   = -200,

		/** The face is already registered in the gallery. */
		KT_ERR_FACE_ALREADY_REGISTERED        = -201,

		/** A face-ID generation request is already in progress. */
		KT_ERR_FACE_ID_GENERATION_IN_PROGRESS = -202,

		/** The specified face was not found in the gallery. */
		KT_ERR_FACE_NOT_FOUND                 = -203,

		NR_KT_RET_CODES /**< Sentinel; not a runtime return value. */
	} Kt_RetCode;

	/**
	 * Information for each person detected in an image frame.
	 */
	typedef struct FaceData {
		/* Face ID of the detected person. 0 if person is unidentified. */
		uint32_t fd__face_id;

		/* This person has an ideal pose needed for better identification. */
		bool ideal_person;

		/* Bounding box coordinates w.r.t. the camera image resolution. */
		int32_t fd__left;
		int32_t fd__top;
		int32_t fd__right;
		int32_t fd__bottom;

		bool    fd__safety_hat_on;     /* Safety hat detected. */
		bool    fd__safety_glasses_on; /* Safety glasses detected. */
		bool    fd__safety_gloves_on;  /* Safety gloves detected. */
	} FaceData;

	/**
	 * PpeData contains information of one or more faces that were detected in
	 * the image frame.
	 */
	struct PpeData {
		/* Count of faces whose information follows this field. */
		uint32_t pd__face_count;

		/* Array of faces. */
		struct FaceData pd__faces[];
	};

	/**
	 * App_QrOutput is the prototype of the QR output callback handler.
	 * This handler is called when QR code was detected and successfully
	 * decoded. The decoded string is returned in p_buffer. This buffer was
	 * already provided to KtLib by calling Kt_AddQRStringBuffer(). If no buffer
	 * is available then Kt Lib will silently ignore the QR code.
	 *
	 * @param app_handle is needed by the Application to access his internal
	 * 		  structures.
	 * @param p_buffer points to the buffer that contains the decoded string.
	 * 		  This buffer was allocated and provided to Kt Lib by the
	 * 		  Application.
	 * @param buf_len contains the length of the buffer pointed by p_buffer.
	 *
	 * @return None
	 */
	typedef void (*App_QrOutput)(App_Handle     app_handle,
								 const uint8_t *p_buffer,
								 const uint32_t buf_len);

	/**
	 * App_QrDecodeFailed is the prototype of the QR decode failed callback
	 * handler.
	 * This callback handler is called when QR code was detected but could not
	 * be decoded. This could help the App hint the User to better align the
	 * QR code in front of the camera sensor.
	 *
	 * @param app_handle is needed by the Application to access his internal
	 * 		  structures.
	 * @param error_code is the error code indicating the reason for decode
	 * 		  failure.
	 *
	 * @return None
	 */
	typedef void (*App_QrDecodeFailed)(App_Handle app_handle,
									   Kt_RetCode error_code);

	/**
	 * App_FaceIdentified is the prototype of the Face ID detected callback
	 * handler.
	 * This callback handler is called when one or more faces were recognized.
	 * The face ID and PPE information of those individuals will be placed in
	 * the metadata buffer (as an array of struct FaceData) up to the size of
	 * the buffer and this callback will be invoked. If the buffer will not fit
	 * all the faces then the remaining faces will be silently dropped.
	 *
	 * @param app_handle is needed by the Application to access his internal
	 * 		  structures.
	 * @param p_ppe_data points to the buffer containing the PpeData.
	 * 		  This buffer was allocated and provided to Kt Lib by the
	 * 		  Application.
	 *
	 * @return None
	 */
	typedef void (*App_FaceIdentified)(App_Handle      app_handle,
									   struct PpeData *p_ppe_data);

	/**
	 * App_FaceNotIdentified is the prototype of the face not detected callback
	 * handler.
	 * This callback handler is called when one or more faces were detected but
	 * not recognized. The PPE information for those individuals will be placed
	 * in the metadata buffer (as an array of struct FaceData) up to the size of
	 * the metadata buffer and this callback will be invoked. If the buffer will
	 * not fit all the faces then the remaining faces will be silently dropped.
	 *
	 * @param app_handle is needed by the Application to access his internal
	 * 		  structures.
	 * @param p_ppe_data points to the buffer containing the PpeData.
	 * 		  This buffer was allocated and provided to Kt Lib by the
	 * 		  Application.
	 *
	 * @return None
	 */
	typedef void (*App_FaceNotIdentified)(App_Handle      app_handle,
										  struct PpeData *p_ppe_data);

	/**
	 * App_IdleCallback is the prototype of the Idle (or Background) callback
	 * handler.
	 * This callback handler is periodically invoked by KtLib for the App to
	 * execute. The App can use it for performing any Background work. The App
	 * should perform minimal work and return from this handler so that events
	 * from the KtLib are not missed. If the App needs to execute a long running
	 * function then it is advised that the function be logically split in such
	 * a way that the complete function executes over multiple App_IdleCallback
	 * invocations by KtLib.
	 *
	 * @param app_handle is needed by the Application to access his internal
	 * 		  structures.
	 *
	 * @return None
	 */
	typedef void (*App_IdleCallback)(App_Handle app_handle);

	/**
	 * App_GenerateFaceIdDone is the prototype of the Face ID generation done
	 * callback handler.
	 * This callback handler is called by KtLib when the Face ID generation that
	 * the Application had requested by calling Kt_GenerateFaceId_async is
	 * completed. The face ID and the operation status are provided as
	 * parameters to the callback.
	 *
	 * Note: It is important to note that the invocation of this done_handler
	 * could be either while Kt_GenerateFaceId_async is still executing (as far
	 * as Application is concerned) or after it completed and returned back to
	 * the Application.
	 *
	 * @param app_handle is needed by the Application to access his internal
	 * 		  structures.
	 * @param face_id is the ID assigned by the firmware to the person for whom
	 * 		  the Kt_GenerateFaceId_async() was called.
	 * @param op_status is the status of the Generate Face ID operation.
	 *
	 * @return None
	 */
	typedef void (*App_GenerateFaceIdDone)(App_Handle app_handle,
										   uint32_t   face_id,
										   Kt_RetCode op_status);

	/**
	 * App_DeleteFaceIdDone is the prototype of the Face ID deletion done
	 * callback handler.
	 * This callback handler is called by KtLib when the Face ID deletion that
	 * the Application had requested by calling Kt_DeleteFaceId_async is
	 * completed. The operation status is provided as a parameter to the
	 * callback.
	 *
	 * Important Note:
	 * Please note that the invocation of this done_handler could either happen:
	 * 1. while Kt_GenerateFaceId_async() function is still executing (as far as
	 * 	  Application is concerned)
	 * 2. after Kt_GenerateFaceId_async() function has returned back to the
	 * 	  Application.
	 *
	 * @param app_handle is needed by the Application to access his internal
	 * 		  structures.
	 * @param op_status is the status of the Delete Face ID operation.
	 *
	 * @return None
	 */
	typedef void (*App_DeleteFaceIdDone)(App_Handle app_handle,
										 Kt_RetCode op_status);

	/**
	 * Below are the data structures that are used to describe the interface
	 * to the GARD hardware. Here we have UartIfaceData and UsbIfaceData which
	 * are used to describe the interface requirements to reach the GARD
	 * hardware.
	 * Calling Kt_GetIfaceInfo will populate the IfaceData structure with the
	 * interface requirements. For eg. UART could list the baud rate and other
	 * parameters which need to be set to match the peer port at the other end.
	 * The application should use this information to configure the interface
	 * (in this case UART) to reach the GARD hardware. The application will
	 * provide a DAL handle for this UART interface that KtLib will use to
	 * communicate with the GARD hardware.
	 */
	struct UartIfaceData {
		/* true if this structure has valid data. */
		bool uid__is_valid;

		/* Line speed in bits per second. */
		uint32_t uid__baud_rate;
	};

	/**
	 * UsbIfaceData describes how Lattice device is to be identified over USB
	 * lanes.
	 */
	struct UsbIfaceData {
		bool uid__is_valid; /* true if this structure has valid data. */

		/* USB parameters used for identification. */
		uint16_t uid__vid_of_device;
		uint16_t uid__pid_of_device;
	};

	/**
	 * All the possible Interfaces that can be used to reach GARD are decribed
	 * here. Each of these has the first parameter as is_valid which will be set
	 * to true by Kt_GetIfaceInfo indicating that structure contains valid
	 * interface requirements.
	 */
	struct IfaceData {
		struct UartIfaceData uart_data;
		struct UsbIfaceData  usb_data;
	};

	/**
	 * Set to '1' the features that the Application needs be started
	 * automatically by Kt_Init().
	 */
	struct KtConfigFlags {
		uint32_t kcf__auto_start_pipeline       : 1;

		uint32_t kcf__auto_start_face_id_detect : 1;

		uint32_t kcf__auto_start_qr_monitor     : 1;
	};

	/**
	 * Application should provide the callback handler functions for the
	 * callbacks that is needs to be notified about, for the ones it does not
	 * need to be notified the callback variables should be set to NULL.
	 *
	 */
	struct KtCallbacks {
		/* Callback when QR code was detected and decoded successfully. */
		App_QrOutput kc__on_qr_output;

		/* Callback when QR code was detected but decoding failed. */
		App_QrDecodeFailed kc__on_qr_failed;

		/* Callback when one (or more) Face ID was detected in the frame. */
		App_FaceIdentified kc__on_face_identified;

		/**
		 * Callback when one (or more) face was found but Face ID is not
		 * assigned to it.
		 */
		App_FaceNotIdentified kc__on_face_not_identified;

		/* Callback called periodically to allow App to run. */
		App_IdleCallback kc__idle_callback;
	};

	/**
	 * Application initializes KtConfig to register its desired configuration
	 * with KtLib.
	 */
	struct KtConfig {
		struct KtConfigFlags kc__config_flags;

		struct KtCallbacks   kc__callbacks;

		App_Handle           kc__app_handle;

		uint32_t             kc__dal_count;

		dal_handle_t         kc__dal_handles[DAL_MAX_COUNT];
	};

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
	 * Note: In the current GARD I2C is not provided but the above write-up is
	 * provided as an example.
	 */
	void Kt_GetIfaceInfo(struct IfaceData *p_if_data);

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
	Kt_Handle Kt_Init(const struct KtConfig *p_cfg);

	/**
	 * Application calls this function when it no longer needs to use KtLib and
	 * needs to release all the KtLib alliocated resources.
	 *
	 * @param handle is the handle to the KtLib context.
	 *
	 * @return None
	 */
	void Kt_Fini(Kt_Handle handle);

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
	Kt_RetCode Kt_StartPipeline(Kt_Handle handle);

	/**
	 * Application calls this function to stop the pipeline. When the pipeline
	 * is stoped the face identification and the QR monitoring are also stopped.
	 *
	 * @param handle is the handle to the KtLib context.
	 *
	 * @return The status of the pipeline stop operation.
	 */
	Kt_RetCode Kt_StopPipeline(Kt_Handle handle);

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
	Kt_RetCode Kt_StartFaceIdentification(Kt_Handle handle);

	/**
	 * Application calls this function to stop the face identification feature.
	 * When this feature is stopped the Application will stop receiving
	 * kc__on_face_identified() and kc__on_face_not_identified() callbacks.
	 *
	 * @param handle is the handle to the KtLib context.
	 *
	 * @return The status of the face identification stop operation.
	 */
	Kt_RetCode Kt_StopFaceIdentification(Kt_Handle handle);

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
	Kt_RetCode
		Kt_GenerateFaceId_async(Kt_Handle              handle,
								App_GenerateFaceIdDone generate_done_handler);

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
									 App_DeleteFaceIdDone delete_done_handler);

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
	Kt_RetCode Kt_StartQrMonitor(Kt_Handle handle);

	/**
	 * Application calls this function to stop the QR monitoring feature.
	 * When this feature is stopped the Application will stop receiving
	 * kc__on_qr_output() and kc__on_qr_failed() callbacks.
	 *
	 * @param handle is the handle to the KtLib context.
	 *
	 * @return The status of the QR monitoring stop operation.
	 */
	Kt_RetCode Kt_StopQrMonitor(Kt_Handle handle);

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
	void Kt_RunDispatcher(Kt_Handle handle, bool run_once);

	/**
	 * Application calls this function to exit the continuous Dispatcher loop.
	 *
	 * @param handle is the handle to the KtLib context.
	 *
	 * @return None.
	 */
	void Kt_StopDispatcher(Kt_Handle handle);

	/**
	 * Application calls this function to provide a buffer to KtLib for storing
	 * the decoded QR code strings. Once this buffer is provided to KtLib,
	 * KtLib will own this buffer and the Application is not supposed to access
	 * it till the buffer is returned to the Application via the
	 * kc__on_qr_output() callback.
	 *
	 * @param handle is the handle to the internal KtLib context.
	 * @param p_buffer points to the buffer that will hold the QR decoded
	 * 		  string.
	 * @param buff_len is the length of the buffer in bytes.
	 *
	 * @return None.
	 */
	void Kt_AddQrStringBuffer(Kt_Handle handle,
							  uint8_t  *p_buffer,
							  uint32_t  buff_len);

	/**
	 * Application calls this function to request KtLib for releasing all the
	 * buffers that Application had provided to KtLib by calling
	 * Kt_AddQrStringBuffer(). This function is usually called during the
	 * shutdown of the Application to release the allocated resources.
	 *
	 * @param handle is the handle to the KtLib context.
	 *
	 * @return None.
	 */
	void Kt_ReleaseQrStringBuffers(Kt_Handle handle);

	/**
	 * Application calls this function to provide a buffer to KtLib for
	 * composing and returning the PpeData metadata to the Application. Once
	 * this buffer is provided to KtLib, KtLib will own this buffer and the
	 * Application is not supposed to access it till the buffer is returned to
	 * the Application via the kc__on_metadata_output() callback.
	 *
	 * @param handle is the handle to the internal KtLib context.
	 * @param p_buffer points to the buffer that will hold the PpeData metadata.
	 * @param buff_len is the length of the buffer in bytes.
	 *
	 * @return None.
	 */
	void Kt_AddMetadataBuffer(Kt_Handle handle,
							  uint8_t  *p_buffer,
							  uint32_t  buff_len);

	/**
	 * Application calls this function to request KtLib for releasing all the
	 * buffers that Application had provided to KtLib by calling
	 * Kt_AddMetadataBuffer(). This function is usually called during the
	 * shutdown of the Application to release the allocated resources.
	 *
	 * @param handle is the handle to the KtLib context.
	 *
	 * @return None.
	 */
	void Kt_ReleaseMetadataBuffers(Kt_Handle handle);

	/**
	 * Get the version string of the KtLib.
	 *
	 * @return A pointer to the version string.
	 */
	const char *Kt_GetVersionString(void);

#ifdef __cplusplus
}
#endif

#endif /* __KT_LIB_H__ */
