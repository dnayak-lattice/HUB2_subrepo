/******************************************************************************
 * Copyright (c) 2026 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#ifndef __KT_APP_CMDS_H__
#define __KT_APP_CMDS_H__

/* Kt App Commands and Return Codes */

/**
 * TBD-DPN: Right now, Python refers to this file - hence keeping this else
 * ideally we should switch to using types.h
 */
#include <stdint.h>

#ifndef PATH_MAX
#define PATH_MAX 256
#endif

/* Socket for request-response communication */
#define KT_APP_IPC_REQ_RESP_SOCKET "/tmp/kt_app_req_resp.sock"

/* Socket for stream communication */
#define KT_APP_IPC_STREAM_SOCKET   "/tmp/kt_app_stream.sock"

/* Commands between Kt App Python and KT App C */
enum kt_app_cmd {
	KT_APP_CMD_NONE                      = 0U, /* No command */
	KT_APP_CMD_START_PIPELINE            = 1U,
	KT_APP_CMD_START_FACE_IDENTIFICATION = 2U,
	KT_APP_CMD_GENERATE_FACE_ID_ASYNC    = 3U,
	KT_APP_CMD_UPDATE_FACE_PROFILE       = 4U,
	KT_APP_CMD_LIST_FACE_PROFILES        = 5U,
	KT_APP_CMD_START_QR_MONITOR          = 6U,
	KT_APP_CMD_STOP_PIPELINE             = 7U,
	KT_APP_CMD_STOP_FACE_IDENTIFICATION  = 8U,
	KT_APP_CMD_DELETE_FACE_ID_ASYNC      = 9U,
	KT_APP_CMD_STOP_QR_MONITOR           = 10U,
	KT_APP_CMD_EXIT                      = 11U,
	NR_KT_APP_CMDS                       = 12U,
};

/**
 * This is the return code for a pending command.
 */
#define KT_APP_RET_CODE_PENDING             0xFFFFF

/* Face ID related constants*/

/* Maximum number of faces in a single request/response */
#define KT_APP_MAX_FACES_IN_METADATA_STREAM 10U

/* Maximum length in bytes of a Face Name*/
#define KT_APP_MAX_FACE_NAME_LENGTH         (PATH_MAX - 1)

/* struct for a face profile - this is how a face profile is stored on disk */
struct kt_app_face_profile {
	uint32_t face_id;
	char     face_name[PATH_MAX];
};

/* Maximum number of face profiles that can be stored on disk */
#define KT_APP_MAX_FACE_PROFILES_ON_DISK 100U

/********************************************************************************
 * IMPORTANT NOTE:
 * The following enum and 2 structs mirror the ones in Kt_Lib.h
 * They should be kept in sync!
 *********************************************************************************/

/* Return codes for IPC operations between Kt App Python and KT App C */
enum kt_app_ret_code {
	/** Operation completed successfully. */
	KT_APP_SUCCESS                              = 0U,

	/** Unspecified / catch-all failure. */
	KT_APP_ERROR_GENERIC                        = -1,

	/** I/O error during SOM communication. */
	KT_APP_ERROR_IO                             = -2,

	/** KtLib has not been initialised (Kt_Init not called or failed). */
	KT_APP_ERROR_NOT_INITED                     = -3,

	/** Requested feature is not supported by the current SOM firmware. */
	KT_APP_ERROR_UNSUPPORTED                    = -4,

	/** Communication with the SOM timed out. */
	KT_APP_ERROR_TIMEOUT                        = -5,

	/* --- QR-related error codes --- */

	/** QR code was detected but could not be decoded. */
	KT_APP_ERROR_QR_DECODE_FAIL                 = -100,

	/* --- Face-ID-related error codes --- */

	/** Face-ID generation/matching failed on the SOM. */
	KT_APP_ERROR_FACE_ID_FAIL                   = -200,

	/** The face is already registered in the gallery. */
	KT_APP_ERROR_FACE_ALREADY_REGISTERED        = -201,

	/** A face-ID generation request is already in progress. */
	KT_APP_ERROR_FACE_ID_GENERATION_IN_PROGRESS = -202,

	/** The specified face was not found in the gallery. */
	KT_APP_ERROR_FACE_NOT_FOUND                 = -203,

	NR_KT_APP_RET_CODES /**< Sentinel; not a runtime return value. */
};

struct kt_app_face_data {
	/* Face ID of the detected person. 0 if person is unidentified. */
	uint32_t face_id;

	/* This person has an ideal pose needed for better identification. */
	uint8_t ideal_person;

	/* Bounding box coordinates w.r.t. the camera image resolution. */
	int32_t left;
	int32_t top;
	int32_t right;
	int32_t bottom;

	uint8_t safety_hat_on;     /* Safety hat detected. */
	uint8_t safety_glasses_on; /* Safety glasses detected. */
	uint8_t safety_gloves_on;  /* Safety gloves detected. */
};

/**
 * PpeData contains information of one or more faces that were detected in
 * the image frame.
 */
struct kt_app_ppe_data {
	/* Count of faces whose information follows this field. */
	uint32_t face_count;

	/* Array of faces. */
	struct kt_app_face_data faces[0];
};

/********************************************************************************
 * END OF IMPORTANT NOTE
 *********************************************************************************/

#pragma pack(push, 1)

/* Request-response communication structures */

struct kt_app_request {
	enum kt_app_cmd cmd;

	union {
		struct _start_pipeline_request {
			uint8_t dummy[0];
		} start_pipeline_request;

		struct _start_face_identification_request {
			uint8_t dummy[0];
		} start_face_identification_request;

		struct _generate_face_id_async_request {
			uint8_t dummy[0];
		} generate_face_id_async_request;

		struct _update_face_profile_request {
			uint32_t face_id;
			char     face_name[KT_APP_MAX_FACE_NAME_LENGTH];
		} update_face_profile_request;

		struct _list_face_profiles_request {
			uint8_t dummy[0];
		} list_face_profiles_request;

		struct _delete_face_id_async_request {
			uint32_t face_id;
		} delete_face_id_async_request;

		struct _stop_qr_monitor_request {
			uint8_t dummy[0];
		} stop_qr_monitor_request;

		struct _stop_pipeline_request {
			uint8_t dummy[0];
		} stop_pipeline_request;

		struct _stop_face_identification_request {
			uint8_t dummy[0];
		} stop_face_identification_request;

		struct _exit_request {
			uint8_t dummy[0];
		} exit_request;
	};
};

struct kt_app_response {
	/* We start with the command that was executed */
	enum kt_app_cmd      cmd;
	enum kt_app_ret_code ret_code;

	union {
		struct _start_pipeline_response {
			uint8_t dummy[0];
		} start_pipeline_response;

		struct _start_face_identification_response {
			uint8_t dummy[0];
		} start_face_identification_response;

		struct _generate_face_id_async_response {
			uint32_t             face_id;
			enum kt_app_ret_code op_status;
		} generate_face_id_async_response;

		struct _update_face_profile_response {
			uint8_t dummy[0];
		} update_face_profile_response;

		struct _list_face_profiles_response {
			uint32_t face_count;
			struct kt_app_face_profile
				face_profiles[KT_APP_MAX_FACE_PROFILES_ON_DISK];
		} list_face_profiles_response;

		struct _delete_face_id_async_response {
			enum kt_app_ret_code op_status;
		} delete_face_id_async_response;

		struct _stop_qr_monitor_response {
			uint8_t dummy[0];
		} stop_qr_monitor_response;

		struct _stop_pipeline_response {
			uint8_t dummy[0];
		} stop_pipeline_response;

		struct _stop_face_identification_response {
			uint8_t dummy[0];
		} stop_face_identification_response;

		struct _exit_response {
			uint8_t dummy[0];
		} exit_response;
	};
};

#pragma pack(pop)

#endif /* __KT_APP_CMDS_H__ */