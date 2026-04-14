/******************************************************************************
 * Copyright (c) 2026 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#ifndef HMI_METADATA_IFACE_H
#define HMI_METADATA_IFACE_H

/**
 * Make sure the compiler search path uses the types.h in the interface folder
 * for maximum compatibility between GARD and HUB.
 */
#include "types.h"

/*******************************************************************************
 * Hub / bus framing (cross-reference; full enums live in gard_hub_iface.h)
 *
 * - Top-level GARD command for this stream: GARD_APP_LAYER_SEND = 0x07.
 * - Inner app-layer message id byte (first byte of app-layer payload after
 *   GARD framing): hub_app_layer_request_id = HMI_METADATA = 0x07.
 * - See gard_hub_iface.h: enum hub_app_layer_request_ids, struct
 * _gard_app_layer_send.
 ******************************************************************************/

/** Matches sample_app_module.c BuildMetadataPayload
 * (APP_METADATA_RESPONSE_TYPE). */
#define APP_HMI_METADATA_RESPONSE_TYPE     1U

/** Matches sample_app_module.c BuildMetadataPayload (APP_METADATA_VERSION).
 * Bump when layout changes. */
#define APP_HMI_METADATA_VERSION           8U

/**
 * Hub enum value for the inner app-layer id when sending this blob inside
 * GARD_APP_LAYER_SEND (see hub_app_layer_request_ids::HMI_METADATA).
 */
#define APP_HMI_METADATA_HUB_REQUEST_ID    0x07u

/**
 * Fixed-point scale used for many int32 fields: ML pipeline uses 10 fractional
 * bits; confidence-like values are often (raw_fp * 100) in .n form (see GARD
 * sample_app_module BuildMetadataPayload).
 */
#define APP_HMI_METADATA_FRAC_BITS         10U

/*******************************************************************************
 * HMI metadata application payload (GARD App Module -> Host peer)
 *
 * This header describes the packed binary layout of the HMI metadata blob that
 * travels inside a GARD_APP_LAYER_SEND transaction:
 *
 *   1. Optional bus-level preamble (see gard_hub_iface.h):
 *      - ORIGINATOR_SYNC exchange (command_id 0x0Cu), then
 *   2. Top-level GARD frame:
 *      - command_id = GARD_APP_LAYER_SEND (0x07u),
 *      - 3-byte little-endian app-layer payload length,
 *      - inner app-layer message:
 *          - hub_app_layer_request_id = HMI_METADATA (0x07u)
 *            (see enum hub_app_layer_request_ids in gard_hub_iface.h),
 *          - 3-byte little-endian metadata byte length,
 *          - metadata bytes = the layout documented **below** (this file).
 *
 * Multi-byte integers in the metadata body are little-endian. Structures that
 * describe wire layout are declared under #pragma pack(1).
 *
 * On the wire the metadata is a **variable-length** byte stream:
 *   - One fixed header (struct app_hmi_metadata_header).
 *   - Exactly header.nb_users contiguous secondary-user records (each starts
 *     with output_data_type = USER_OUTPUT_DATA).
 *   - Zero or more optional trailing records; each is a uint8_t data_type
 *     (values IDEAL_USER_OUTPUT_DATA / IDEAL_HAND_OUTPUT_DATA) followed by the
 *     matching body struct. The GARD reference firmware currently emits
 *     IDEAL_USER_OUTPUT_DATA when the ideal user is available; IDEAL_HAND
 *     is reserved for future use.
 *
 * **Single layout authority:** The packed `#pragma pack(1)` structures in
 * this file define the host-visible byte contract. Any `reserved_*` members
 * occupy space in that layout (alignment / future use); hosts must parse in
 * strict struct field order and length—including those bytes—unless a
 * specific product note documents that a given GARD build omits them. For the
 * reference serializer, compare field-by-field with
 * `sample_app_module.c` `BuildMetadataPayload()`.
 ******************************************************************************/

/**
 * Maximum number of secondary (tracked) user records the protocol supports
 * in this revision. Must match the GARD pipeline tracked-user cap (USER_CAP).
 * Hosts must reject or truncate if header.nb_users exceeds this value.
 */
#define APP_HMI_METADATA_MAX_TRACKED_USERS 10U

/**
 * Recommended maximum size of the metadata byte array inside the HMI_METADATA
 * message (3-byte size field in the GARD_APP_LAYER_SEND inner payload).
 * GARD firmware uses this as a buffer cap when building the stream.
 */
#define APP_HMI_METADATA_MAX_PAYLOAD_BYTES 1024U

/**
 * Identifies each typed record embedded in the metadata byte stream after the
 * fixed header. Values align with the GARD App Module serializer
 * (data_transfer_structs.h: USER_OUTPUT_DATA / IDEAL_USER_OUTPUT_DATA / …).
 */
enum app_hmi_metadata_output_data_type {
	/** One struct app_hmi_user_output_data (per-tracked-user person/face box).
	 */
	APP_HMI_USER_OUTPUT_DATA       = 0,

	/** Ideal-user sideband: struct app_hmi_ideal_user_output_data. */
	APP_HMI_IDEAL_USER_OUTPUT_DATA = 1,

	/** Hand sideband: struct app_hmi_ideal_hand_output_data (optional /
	 * future). */
	APP_HMI_IDEAL_HAND_OUTPUT_DATA = 2,
};

/**
 * ideal_user_output_data.status — Face ID / gallery state for the selected
 * ideal user (see track_users.h user_fid_status_t; sample_app_module
 * ctxt->ideal_user_status).
 */
enum app_hmi_ideal_user_fid_status {
	APP_HMI_USER_REGISTERED         = 0,
	APP_HMI_USER_UNREGISTERED       = 1,
	APP_HMI_USER_STATUS_UNKNOWN     = 2,
	APP_HMI_USER_REQUIREMENTS_UNMET = 3,
	APP_HMI_USER_FID_DISABLED       = 4,
	APP_HMI_USER_NO_GALLERY         = 5,
};

/**
 * user_output_data.personDistance — coarse person distance class
 * (hmi_face_id.h: person_distance_t; sample_app_module
 * ctxt->person_distance[i]).
 */
enum app_hmi_person_distance_class {
	APP_HMI_DISTANCE_CLOSE = 0,
	APP_HMI_DISTANCE_MID   = 1,
	APP_HMI_DISTANCE_FAR   = 2,
};

/**
 * user_output_data.personPose — frontal vs not (sample_app_module
 * user->isFrontal ? 0 : 1).
 */
enum app_hmi_person_pose_code {
	APP_HMI_PERSON_POSE_FRONTAL     = 0,
	APP_HMI_PERSON_POSE_NOT_FRONTAL = 1,
};

/**
 * Legacy CLNX-style registration tri-state (data_transfer_structs.h
 * PersonRegistrationStatus). Not all GARD paths use this for ideal_user.status;
 * prefer app_hmi_ideal_user_fid_status for Face ID metadata.
 */
enum app_hmi_person_registration_status {
	APP_HMI_RS_REGISTERED   = 0,
	APP_HMI_RS_UNREGISTERED = 1,
	APP_HMI_RS_UNKNOWN      = 2,
};

/**
 * Bit layout of user_output_data.id (int32_t, little-endian on wire).
 * Encoded by EncodeUserOutputId() (app_hmi.c).
 *
 *   bits 0..7:   tracker userID (masked 0xff)
 *   bits 16..19: gallery face id nibble (userFaceID & 0xf)
 *   bits 24..27: userFaceIDStatus nibble (user_fid_status_t & 0xf)
 *   bit 31:      sentinel (always 1 in current GARD)
 */
#define APP_HMI_USER_OUTPUT_ID_USER_ID_SHIFT        0
#define APP_HMI_USER_OUTPUT_ID_USER_ID_MASK         0xffu

#define APP_HMI_USER_OUTPUT_ID_FACE_ID_SHIFT        16
#define APP_HMI_USER_OUTPUT_ID_FACE_ID_MASK         0xfu

#define APP_HMI_USER_OUTPUT_ID_FACE_ID_STATUS_SHIFT 24
#define APP_HMI_USER_OUTPUT_ID_FACE_ID_STATUS_MASK  0xfu

#define APP_HMI_USER_OUTPUT_ID_SENTINEL_SHIFT       31u
#define APP_HMI_USER_OUTPUT_ID_SENTINEL_MASK        (1u << APP_HMI_USER_OUTPUT_ID_SENTINEL_SHIFT)
/** Same as APP_HMI_USER_OUTPUT_ID_SENTINEL_SHIFT (for `1u <<` expressions). */
#define APP_HMI_USER_OUTPUT_ID_SENTINEL_BIT         APP_HMI_USER_OUTPUT_ID_SENTINEL_SHIFT

/**
 * Bit layout of ideal_user_output_data.index (int32_t, LE).
 * Packed in BuildIdealUserMetadata() (app_hmi.c).
 *
 *   bits 0..7:   ideal_user_idx (tracked user slot) & 0xff
 *   bits 8..15:  FaceIdGetCurrentMatchedUserId() & 0xff
 *   bits 16..23: FaceIdGetLastRegisteredUserId() & 0xff
 *   bits 24..27: GetNbUsers(gallery) & 0xf
 *   bits 28..31: GetGallerySize(gallery) & 0xf
 */
#define APP_HMI_IDEAL_USER_INDEX_SLOT_SHIFT         0
#define APP_HMI_IDEAL_USER_INDEX_SLOT_MASK          0xffu

#define APP_HMI_IDEAL_USER_INDEX_MATCHED_SHIFT      8
#define APP_HMI_IDEAL_USER_INDEX_MATCHED_MASK       0xffu

#define APP_HMI_IDEAL_USER_INDEX_LAST_REG_SHIFT     16
#define APP_HMI_IDEAL_USER_INDEX_LAST_REG_MASK      0xffu

#define APP_HMI_IDEAL_USER_INDEX_NB_USERS_SHIFT     24
#define APP_HMI_IDEAL_USER_INDEX_NB_USERS_MASK      0xfu

#define APP_HMI_IDEAL_USER_INDEX_GALLERY_SIZE_SHIFT 28
#define APP_HMI_IDEAL_USER_INDEX_GALLERY_SIZE_MASK   0xfu

#pragma pack(1)

/**
 * Fixed prefix of the HMI metadata payload. Immediately followed on the wire by
 * nb_users instances of struct app_hmi_secondary_user_record (each record's
 * data_type must be APP_HMI_USER_OUTPUT_DATA in conforming streams).
 */
struct app_hmi_metadata_header {
	/**
	 * Application-specific response discriminator (GARD HMI uses a small
	 * integer version of the CLNX-style type byte).
	 * Must match APP_HMI_METADATA_RESPONSE_TYPE in current GARD builds.
	 */
	uint8_t response_type;

	/** Schema / layout version of the metadata body. Must match
	 * APP_HMI_METADATA_VERSION. */
	uint8_t response_version;

	/** Reserved for alignment and future use. */
	uint8_t reserved_1[2];

	/** Source image width in pixels (int32_t, LE). */
	int32_t image_width;

	/** Source image height in pixels (int32_t, LE). */
	int32_t image_height;

	/** Region-of-interest left coordinate (inclusive), pixels, LE. */
	int32_t roi_left;

	/** Region-of-interest top coordinate (inclusive), pixels, LE. */
	int32_t roi_top;

	/** Region-of-interest right coordinate (inclusive), pixels, LE. */
	int32_t roi_right;

	/** Region-of-interest bottom coordinate (inclusive), pixels, LE. */
	int32_t roi_bottom;

	/**
	 * Number of secondary user records that follow the header.
	 * Must be <= APP_HMI_METADATA_MAX_TRACKED_USERS.
	 */
	int32_t nb_users;
};

/**
 * Packed person and face summary for one tracked user, CLNX-compatible field
 * ordering.
 */
struct app_hmi_user_output_data {
	/** Packed id; decode with APP_HMI_USER_OUTPUT_ID_* bit defines. */
	int32_t id;
	/** 0 = no person, 1 = person present. */
	int32_t person_available;
	/** Confidence * 100 style int (see APP_HMI_METADATA_FRAC_BITS). */
	int32_t person_confidence;
	/** Person box pixel coords (integer). */
	int32_t person_left;
	int32_t person_top;
	int32_t person_right;
	int32_t person_bottom;
	/** app_hmi_person_pose_code */
	int32_t person_pose;
	int32_t person_frontal_pose_confidence;
	int32_t person_not_frontal_pose_confidence;
	/** app_hmi_person_distance_class */
	int32_t person_distance;
	/** 0 = no face, 1 = face present. */
	int32_t face_available;
	int32_t face_confidence;
	int32_t face_left;
	int32_t face_top;
	int32_t face_right;
	int32_t face_bottom;
	/** Rounded face distance / 10 in GARD serializer (see sample_app_module).
	 */
	int32_t face_distance;
};

/**
 * One secondary-user wire record: type tag + user_output_data body.
 * For records immediately after the header, data_type is
 * APP_HMI_USER_OUTPUT_DATA.
 */
struct app_hmi_secondary_user_record {
	/** APP_HMI_USER_OUTPUT_DATA for each of header.nb_users records. */
	uint8_t data_type;
	/** Reserved for alignment and future use. */
	uint8_t                         reserved_2[3];
	struct app_hmi_user_output_data user_data;
};

/** Ideal-user pose / ID sideband (one record when data_type is
 * APP_HMI_IDEAL_USER_OUTPUT_DATA). */
struct app_hmi_ideal_user_output_data {
	/** Packed index; decode with APP_HMI_IDEAL_USER_INDEX_* defines. */
	int32_t index;
	/** app_hmi_ideal_user_fid_status */
	int32_t status;
	/** Head pose angles as fixed-point .n (APP_HMI_METADATA_FRAC_BITS), degrees
	 * scale. */
	int32_t face_yaw;
	int32_t face_pitch;
	int32_t face_roll;
	/** Landmarks confidence * 100 style int. */
	int32_t landmarks_confidence;
};

/**
 * Optional hand-detection sideband (when data_type is
 * APP_HMI_IDEAL_HAND_OUTPUT_DATA).
 */
struct app_hmi_ideal_hand_output_data {
	int16_t hand_validation_score;
	/** Reserved for alignment and future use. */
	uint8_t reserved_4[2];
	int32_t hand_left;
	int32_t hand_top;
	int32_t hand_right;
	int32_t hand_bottom;
	int32_t hand_landmarks_x[10];
	int32_t hand_landmarks_y[10];
};

/**
 * Trailing typed records (per-record: uint8_t data_type then reserved + body).
 * Pairs are grouped in struct ideal_user_data (see below).
 */
struct app_hmi_ideal_user_record {
	uint8_t data_type;
	/** Reserved for alignment and future use. */
	uint8_t                               reserved_3[3];
	struct app_hmi_ideal_user_output_data ideal_user_data;
};

struct app_hmi_ideal_hand_record {
	uint8_t data_type;
	/** Reserved for alignment and future use. */
	uint8_t                               reserved_5[3];
	struct app_hmi_ideal_hand_output_data ideal_hand_data;
};

/**
 * Optional ideal-user and ideal-hand records grouped for hosts.
 */
struct ideal_user_data {
	struct app_hmi_ideal_user_record ideal_user;
	struct app_hmi_ideal_hand_record ideal_hand;
};

/**
 * Metadata blob layout: `header`, then `header.nb_users` elements of
 * `tracked_users`, then `ideal_user_data` (firmware may use only
 * `ideal_user_data.ideal_user` on the wire today; `ideal_hand` is reserved).
 * `tracked_users[0]` is a flexible array; the work buffer must be large enough
 * for `sizeof(header) + nb_users * sizeof(secondary record) + sizeof(ideal_user_data)`.
 */
struct app_hmi_metadata_payload {
	struct app_hmi_metadata_header       header;
	struct app_hmi_secondary_user_record tracked_users[0];
	struct ideal_user_data               ideal_user_data;
};

#pragma pack()

#endif /* HMI_METADATA_IFACE_H */
