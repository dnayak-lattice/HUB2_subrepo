/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#include <stdint.h>
#include <string.h>

#include "Kt_Lib.h"
#include "Kt_Internal.h"
#include "Kt_Assert.h"
#include "gard_hub_iface.h"
#include "hmi_metadata_iface.h"

#ifdef KT_DEBUG
uint32_t received_packet_counter = 0;
#endif

/**
 * Disable the zero-length bounds warning for the following function.
 * This is because we know that we will be accessing such a buffer in the code
 * in this file.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wzero-length-bounds"

/**
 * Kt_HandleReceivedMetadata will be called to process the received Metadata
 * from the App Layer running over GARD.
 *
 * @param p_gard_handle is the handle to the GARD device.
 * @param handle is the handle to the KtLib context.
 * @param p_buffer is the pointer to the buffer that contains the received
 * 		  Metadata.
 * @param buffer_size is the size of the buffer that contains the received
 * 		  Metadata.
 *
 * @return True if the metadata is successfully handled, false otherwise.
 */
bool Kt_HandleReceivedMetadata(gard_handle_t p_gard_handle,
							   void         *handle,
							   void         *p_buffer,
							   uint32_t      buffer_size)
{
	struct Kt_Context         *ctxt = (struct Kt_Context *)(handle);
	struct _hmi_metadata_send *p_hmi_metadata_send =
		(struct _hmi_metadata_send *)p_buffer;
	struct app_hmi_metadata_payload *p_hmi_metadata =
		(struct app_hmi_metadata_payload *)p_hmi_metadata_send->metadata;
	struct PpeData         *p_out_data;
	uint32_t                i;
	uint32_t                count_of_identified_users;
	uint32_t                count_of_unidentified_users;
	struct ideal_user_data *p_ideal_data = NULL;
	uint32_t                md_buffer_size;
	uint32_t                ideal_user_index;
	uint32_t                ideal_user_face_id;
	uint32_t                face_id_of_tracked_user;

	/**
	 * At a high level this function does the following:
	 * 1. Acquires Metadata buffer which the App had handed to us previously.
	 *    These buffer is used to provide the Identified and Unidentified Users
	 *    Different buffers are used for each type of User.
	 * 2. The routine parses the HMI metadata sent by the GARD and extracts the
	 *    information to fill the PpeData for the application to consume.
	 *
	 * P.S.: The ideal user is optionally provided by GARD. GARD sends this data
	 * only when the ML network has identified an Ideal User. This routine needs
	 * to look for the ideal user data (if present) as the Ideal User data
	 * provides information that is of critical importance.
	 */
	KT__ASSERT(kt_ctxt == ctxt && ctxt != NULL,
			   "Invalid parameter passed to Kt_HandleReceivedMetadata.");

	KT__ASSERT(p_buffer != NULL && buffer_size > 0,
			   "Invalid parameter passed to Kt_HandleReceivedMetadata.");

	/* Maintain basic sanity. */
	KT__ASSERT((p_hmi_metadata->header.response_type ==
				(uint8_t)APP_HMI_METADATA_RESPONSE_TYPE) &&
			   (p_hmi_metadata->header.response_version ==
				(uint8_t)APP_HMI_METADATA_VERSION));

	KT__ASSERT(p_hmi_metadata->header.nb_users <=
			   MAX_USERS_SUPPORTED_PER_FRAME);

#ifdef KT_DEBUG
	received_packet_counter++;
	os_pr_err("Received %d users in frame # %u.\n",
			  p_hmi_metadata->header.nb_users, received_packet_counter);
#endif

	/* Check if there is nothing to extract from this metadata. */
	if (p_hmi_metadata->header.nb_users == 0) {
		return true;
	}

	p_out_data =
		(struct PpeData *)Kt_GetMetadataBuffer(handle, &md_buffer_size);
	KT__ASSERT(p_out_data != NULL && md_buffer_size > 0,
			   "Failed to get metadata buffer.");

	/**
	 * To identify if Ideal User Data is present we place a pointer to Ideal
	 * Data at the end of the tracked (secondary) users information. If we do
	 * not exceed the buffer size then we assume ideal User data is available.
	 */
	p_ideal_data = (struct ideal_user_data *)&p_hmi_metadata
					   ->tracked_users[p_hmi_metadata->header.nb_users];

	if (((uint8_t *)p_ideal_data) >= (((uint8_t *)p_buffer) + buffer_size)) {
		/* No Ideal User Data present in this buffer. */
		p_ideal_data = NULL;
	} else {
		/* Ensure the data-type matches the expected type. */
		KT__ASSERT(p_ideal_data->ideal_user.data_type ==
				   APP_HMI_IDEAL_USER_OUTPUT_DATA);

		/**
		 * Extract Ideal User index within tracked Users and Face ID from the
		 * index field.
		 */
		ideal_user_index =
			(uint32_t)(p_ideal_data->ideal_user.ideal_user_data.index &
					   APP_HMI_IDEAL_USER_INDEX_SLOT_MASK);

		ideal_user_face_id =
			(uint32_t)((p_ideal_data->ideal_user.ideal_user_data.index >>
						APP_HMI_IDEAL_USER_INDEX_MATCHED_SHIFT) &
					   APP_HMI_IDEAL_USER_INDEX_MATCHED_MASK);

#ifdef KT_DEBUG
		os_pr_err("Received tracked user # %d with face ID %d as Ideal User.\n",
				  ideal_user_index, ideal_user_face_id);
#endif
	}

	/* First circle through all the users who have been identified. */
	count_of_identified_users   = 0;
	count_of_unidentified_users = 0;
	for (i = 0; i < p_hmi_metadata->header.nb_users; i++) {
		face_id_of_tracked_user =
			(uint32_t)(p_hmi_metadata->tracked_users[i].user_data.id >>
					   APP_HMI_USER_OUTPUT_ID_FACE_ID_SHIFT) &
			APP_HMI_USER_OUTPUT_ID_FACE_ID_MASK;
		if (face_id_of_tracked_user == 0) {
			count_of_unidentified_users++;
			continue;
		}

		KT__ASSERT(p_hmi_metadata->tracked_users[i].data_type ==
				   APP_HMI_USER_OUTPUT_DATA);

		/* Initialize Face ID. */
		p_out_data->pd__faces[count_of_identified_users].fd__face_id =
			face_id_of_tracked_user;

		/* Copy coordinates. */
		p_out_data->pd__faces[count_of_identified_users].fd__left =
			p_hmi_metadata->tracked_users[i].user_data.person_left;
		p_out_data->pd__faces[count_of_identified_users].fd__top =
			p_hmi_metadata->tracked_users[i].user_data.person_top;
		p_out_data->pd__faces[count_of_identified_users].fd__right =
			p_hmi_metadata->tracked_users[i].user_data.person_right;
		p_out_data->pd__faces[count_of_identified_users].fd__bottom =
			p_hmi_metadata->tracked_users[i].user_data.person_bottom;

		if (p_ideal_data != NULL && (ideal_user_index == i)) {
			p_out_data->pd__faces[count_of_identified_users].ideal_person =
				true;
			p_out_data->pd__faces[count_of_identified_users].fd__face_id =
				face_id_of_tracked_user;
		} else {
			p_out_data->pd__faces[count_of_identified_users].ideal_person =
				false;
		}

		/* <TBD-SRP> : The PPE data needs to reflect the real attributes. */
		p_out_data->pd__faces[count_of_identified_users].fd__safety_hat_on =
			false;
		p_out_data->pd__faces[count_of_identified_users].fd__safety_glasses_on =
			false;
		p_out_data->pd__faces[count_of_identified_users].fd__safety_gloves_on =
			false;

		count_of_identified_users++;
	}

	p_out_data->pd__face_count = count_of_identified_users;

	/* Send this PpeData of Identified Users to the application. */
	if (count_of_identified_users &&
		ctxt->kc__cfg.kc__callbacks.kc__on_face_identified != NULL) {
		ctxt->kc__cfg.kc__callbacks.kc__on_face_identified(
			ctxt->kc__cfg.kc__app_handle, p_out_data);

		/* New buffer needs to be allocated for sending unidentified users. */
		p_out_data = NULL;
	}

	if (count_of_unidentified_users) {
		/**
		 * If PpeData was sent for Identified Users then get another buffer to
		 * send the unidentified users to the application, or else use the
		 * already allocated PpeData buffer.
		 */
		if (NULL == p_out_data) {
			p_out_data =
				(struct PpeData *)Kt_GetMetadataBuffer(handle, &buffer_size);
		}

		KT__ASSERT(p_out_data != NULL && buffer_size > 0,
				   "Failed to get metadata buffer.");

		/* Now circle through all the users who have not been identified. */
		count_of_unidentified_users = 0;
		for (i = 0; i < p_hmi_metadata->header.nb_users; i++) {
			face_id_of_tracked_user =
				(uint32_t)(p_hmi_metadata->tracked_users[i].user_data.id >>
						   APP_HMI_USER_OUTPUT_ID_FACE_ID_SHIFT) &
				APP_HMI_USER_OUTPUT_ID_FACE_ID_MASK;
			if (face_id_of_tracked_user != 0) {
				continue;
			}

			KT__ASSERT(p_hmi_metadata->tracked_users[i].data_type ==
					   APP_HMI_USER_OUTPUT_DATA);

			/* Initialize Face ID. */
			p_out_data->pd__faces[count_of_identified_users].fd__face_id =
				face_id_of_tracked_user;

			/* Copy coordinates. */
			p_out_data->pd__faces[count_of_unidentified_users].fd__left =
				p_hmi_metadata->tracked_users[i].user_data.person_left;
			p_out_data->pd__faces[count_of_unidentified_users].fd__top =
				p_hmi_metadata->tracked_users[i].user_data.person_top;
			p_out_data->pd__faces[count_of_unidentified_users].fd__right =
				p_hmi_metadata->tracked_users[i].user_data.person_right;
			p_out_data->pd__faces[count_of_unidentified_users].fd__bottom =
				p_hmi_metadata->tracked_users[i].user_data.person_bottom;

			if (p_ideal_data != NULL && (ideal_user_index == i)) {
				p_out_data->pd__faces[count_of_unidentified_users]
					.ideal_person = true;
				p_out_data->pd__faces[count_of_unidentified_users].fd__face_id =
					face_id_of_tracked_user;
			} else {
				p_out_data->pd__faces[count_of_unidentified_users]
					.ideal_person = false;
			}

			/* <TBD-SRP> : The PPE data needs to reflect the real attributes. */
			p_out_data->pd__faces[count_of_unidentified_users]
				.fd__safety_hat_on = false;
			p_out_data->pd__faces[count_of_unidentified_users]
				.fd__safety_glasses_on = false;
			p_out_data->pd__faces[count_of_unidentified_users]
				.fd__safety_gloves_on = false;

			count_of_unidentified_users++;
		}

		p_out_data->pd__face_count = count_of_unidentified_users;

		/**
		 * Send this PpeData of Identified Users to the application by calling
		 * the On_Face_Not_Identified callback (if provided).
		 */
		if (ctxt->kc__cfg.kc__callbacks.kc__on_face_not_identified != NULL) {
			ctxt->kc__cfg.kc__callbacks.kc__on_face_not_identified(
				ctxt->kc__cfg.kc__app_handle, p_out_data);
			p_out_data = NULL;
		}
	}

	return true;
}

#pragma GCC diagnostic pop

