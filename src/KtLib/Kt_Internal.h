/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#ifndef __KT_INTERNAL_H__
#define __KT_INTERNAL_H__

#include "types.h"

#include "hub.h"
#include "Kt_Lib.h"

#ifdef __cplusplus
extern "C" {
#endif

	/**
	 * This is the structure that holds the linked list management data for the
	 * buffers.
	 */
	struct BufInfo {
		/* Linked list pointer to next node. */
		struct BufInfo *bi__next;

		/* Size of this buffer in bytes. */
		uint32_t bi__buff_len;
	};

	/**
	 * This is the Fifo structure that holds the buffers in a linked list.
	 */
	struct BufFifo {
		/* Points to the next buffer to be dequeued from the FiFo for usage. */
		struct BufInfo *p_head;

		/**
		 * Points to the last buffer in the queue. New buffer to be added in the
		 * Fifo is added after this one.
		 */
		struct BufInfo *p_tail;
	};

/**
 * We do not expect the HMI metadata and QR image to be larger than these sizes.
 * If they are then bump up these values accordingly.
 */
#define MAX_HMI_METADATA_BUFFER_SIZE 2048

#define MAX_QR_IMAGE_BUFFER_SIZE     (1920 * 1080 * 1)

	/**
	 * Kt_Context is the internal context for the KtLib. It holds all the data
	 * structures needed for KtLib management.
	 */
	struct Kt_Context {
		/* HUB handle. */
		hub_handle_t kc__hub_handle;

		/* GARD handle. Currently we only support one GARD device. */
		gard_handle_t kc__gard_handle;

		/**
		 * KtConfig structure which the Application had provided to us with its
		 * desired configuration. We keep a copy of it for referencing some of
		 * the fields that we could need at runtime.
		 */
		struct KtConfig kc__cfg;

		/**
		 * Fifo to hold metadata buffers for PpeData / FaceData payloads.
		 */
		struct BufFifo kc__meta_buf_fifo;

		/**
		 * Fifo to hold decoded QR string buffers.
		 */
		struct BufFifo kc__qr_buf_fifo;

		/**
		 * Flag to indicate if the pipeline is currently running on GARD.
		 */
		bool kc__pipeline_running;

		/**
		 * Flag to indicate if the dispatcher should stop running.
		 */
		bool kc__stop_dispatcher_loop;

		/**
		 * The HMI metadata buffer is used to hold the metadata arriving from
		 * the App Layer running over GARD. KtLib then parses this received HMI
		 * metadata in this buffer to compose the PpeData structure.
		 */
		uint8_t *kc__hmi_metadata_buffer;
		uint32_t kc__hmi_metadata_buffer_size;

		/**
		 * The QR image buffer is used to hold the QR code image arrivingfrom
		 * the App Layer running over GARD. KtLib then decodes this received QR
		 * code image in this buffer to get the decoded string.
		 */
		uint8_t *kc__qr_image_buffer;
		uint32_t kc__qr_image_buffer_size;

		/**
		 * Flag to indicate if the face identification monitoring is currently
		 * active.
		 */
		bool kc__face_id_monitoring_active;

		/**
		 * Flag to indicate if the QR monitoring is currently active.
		 */
		bool kc__qr_monitor_active;
	};

	/**
	 * Global pointer to the Kt_Context.
	 */
	extern struct Kt_Context *kt_ctxt;

	/**
	 * Kt_BufFifo_Init initializes the buffer Fifo.
	 */
	void Kt_BufFifo_Init(struct BufFifo *p_buf_fifo);

	/**
	 * Kt_GetMetadataBuffer returns the oldest metadata buffer from the metadata
	 * buffer fifo.
	 */
	void *Kt_GetMetadataBuffer(Kt_Handle handle, uint32_t *p_buffer_size);

	/**
	 * Kt_GetQrStringBuffer returns the oldest QR string buffer from the QR
	 * string buffer fifo.
	 */
	void *Kt_GetQrStringBuffer(Kt_Handle handle, uint32_t *p_buffer_size);

	/**
	 * Kt_ConvertFixedPointToInteger converts a fixed point value to an unsigned
	 * 32-bit integer.
	 */
	uint32_t Kt_ConvertFixedPointToInteger(uint8_t  frac_bits,
										   uint32_t fixed_point_value);

	/**
	 * Kt_GetBufferForGardSendData will be called by HUB when it receives data
	 * from GARD. HUB expects Kt layer to read the peek buffer for determinig
	 * the buffer to return to HUB for receiving the data from GARD.
	 */
	bool Kt_GetBufferForGardSendData(gard_handle_t p_gard_handle,
									 void         *handle,
									 void        **p_buffer,
									 uint32_t     *p_buffer_size,
									 void         *p_peek_buffer,
									 uint32_t      peek_buffer_size);

	/**
	 * Kt_HandleGardSendData will be called by HUB when it receives data from
	 * GARD. HUB expects Kt layer to handle the data.
	 */
	bool Kt_HandleGardSendData(gard_handle_t p_gard_handle,
							   void         *handle,
							   void         *p_buffer,
							   uint32_t      buffer_size);

	/**
	 * Kt_HandleGardRequest will be called by HUB when it receives a request
	 * from GARD. HUB expects Kt layer to handle the request.
	 */
	bool Kt_HandleGardRequest(gard_handle_t p_gard_handle,
							  void         *handle,
							  void         *p_request,
							  uint32_t      request_size,
							  void         *p_response,
							  uint32_t      response_size);

	/**
	 * Kt_HandleReceivedMetadata will be called to process the received Metadata
	 * from the App Layer running over GARD.
	 */
	bool Kt_HandleReceivedMetadata(gard_handle_t p_gard_handle,
								   void         *handle,
								   void         *p_buffer,
								   uint32_t      buffer_size);

	/**
	 * Kt_HandleQRImage will be called to process the received QR image from the
	 * App Layer running over GARD.
	 */
	bool Kt_HandleReceivedQRImage(gard_handle_t p_gard_handle,
								  void         *handle,
								  void         *p_buffer,
								  uint32_t      buffer_size);
#ifdef __cplusplus
}
#endif

#endif /* __KT_TYPES_H__ */
