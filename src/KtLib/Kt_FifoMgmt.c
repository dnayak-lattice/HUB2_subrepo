/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#include <string.h>

#include "Kt_Lib.h"
#include "Kt_Internal.h"
#include "Kt_Assert.h"

/**
 * Every buffer that the Application provides to us is held in a Fifo while they
 * wait to be used. This Fifo is created using a single linked list where the
 * linked list management data (consisting of the next pointer and the buffer
 * size) is stored in the buffer itself since we "own" the buffer when the
 * Application provided it to us.
 */

/**
 * Initialzes the buffer Fifos.
 *
 * @param None.
 *
 * @return None.
 */
void Kt_BufFifo_Init(struct BufFifo *p_buf_fifo)
{
	KT__ASSERT(NULL != p_buf_fifo, "p_buf_fifo is NULL");
	p_buf_fifo->p_head = NULL;
	p_buf_fifo->p_tail = NULL;
}

/**
 * Returns true if the buffer Fifo is empty.
 *
 * @param p_buf_fifo  Buffer Fifo.
 *
 * @return True if empty, false otherwise.
 */
static bool Kt_BufFifo_IsEmpty(const struct BufFifo *p_buf_fifo)
{
	return p_buf_fifo->p_head == NULL;
}

/**
 * Kt_GetBufferFromFifo removes and returns the oldest buffer from the Fifo.
 * This routine currently is not thread safe.
 *
 * @param p_buf_fifo  Buffer Fifo.
 * @param p_buffer_size Optional pointer to the size of the buffer.
 *
 * @return None.
 */
static void *Kt_GetBufferFromFifo(struct BufFifo *p_buf_fifo,
								  uint32_t       *p_buffer_size)
{
	struct BufInfo *p_buf_info;

	KT__ASSERT(NULL != p_buf_fifo, "p_buf_fifo is NULL");
	KT__ASSERT(NULL != p_buf_fifo->p_head, "p_buf_fifo is empty");

	p_buf_info         = p_buf_fifo->p_head;
	p_buf_fifo->p_head = p_buf_info->bi__next;
	if (p_buf_fifo->p_head == NULL) {
		p_buf_fifo->p_tail = NULL;
	}
	p_buf_info->bi__next = NULL;

	if (p_buffer_size) {
		*p_buffer_size = p_buf_info->bi__buff_len;
	}

	return (void *)p_buf_info;
}

/**
 * Kt_AddBufferToFifo adds a buffer to end of the Fifo.
 * This routine currently is not thread safe.
 *
 * @param p_buf_fifo  Buffer Fifo.
 * @param p_buffer    Buffer to add.
 * @param buff_len    Length of the buffer.
 *
 * @return True if added, false otherwise.
 */
static bool Kt_AddBufferToFifo(struct BufFifo *p_buf_fifo,
							   void           *p_buffer,
							   uint32_t        buff_len)
{
	struct BufInfo *p_buf_info;

	KT__ASSERT(NULL != p_buf_fifo, "p_buf_fifo is NULL");
	KT__ASSERT(NULL != p_buffer, "p_buffer is NULL");
	KT__ASSERT(buff_len >= MIN_BUF_SIZE, "buff_len is less than MIN_BUF_SIZE");

	p_buf_info               = (struct BufInfo *)p_buffer;
	p_buf_info->bi__next     = NULL;
	p_buf_info->bi__buff_len = buff_len;

	if (p_buf_fifo->p_head == NULL) {
		p_buf_fifo->p_head = p_buf_info;
	} else {
		p_buf_fifo->p_tail->bi__next = p_buf_info;
	}
	p_buf_fifo->p_tail = p_buf_info;

	return true;
}

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
						  uint32_t  buff_len)
{
	struct Kt_Context *ctxt = (struct Kt_Context *)(handle);
	KT__ASSERT(kt_ctxt == ctxt && ctxt != NULL,
			   "Invalid parameter passed to Kt_AddMetadataBuffer.");

	Kt_AddBufferToFifo(&ctxt->kc__meta_buf_fifo, p_buffer, buff_len);
}

/**
 * Kt_GetMetadataBuffer returns the oldest metadata buffer from the metadata
 * buffer fifo. This fifo was built by the buffers provided by the Application
 * via the Kt_AddMetadataBuffer() function.
 *
 * @param handle is the handle to the library context.
 *
 * @return The pointer to the metadata buffer.
 */
void *Kt_GetMetadataBuffer(Kt_Handle handle, uint32_t *p_buffer_size)
{
	struct Kt_Context *ctxt = (struct Kt_Context *)(handle);
	KT__ASSERT(kt_ctxt == ctxt && ctxt != NULL,
			   "Invalid parameter passed to Kt_GetMetadataBuffer.");

	if (Kt_BufFifo_IsEmpty(&ctxt->kc__meta_buf_fifo)) {
		return NULL;
	}

	return Kt_GetBufferFromFifo(&ctxt->kc__meta_buf_fifo, p_buffer_size);
}

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
void Kt_ReleaseMetadataBuffers(Kt_Handle handle)
{
	struct Kt_Context *ctxt = (struct Kt_Context *)(handle);

	KT__ASSERT(kt_ctxt == ctxt && ctxt != NULL,
			   "Invalid parameter passed to Kt_ReleaseMetadataBuffers.");

	while (!Kt_BufFifo_IsEmpty(&ctxt->kc__meta_buf_fifo)) {
		Os_MemFree(ctxt->kc__cfg.kc__app_handle,
				   Kt_GetBufferFromFifo(&ctxt->kc__meta_buf_fifo, NULL));
	}
}

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
						  uint32_t  buff_len)
{
	struct Kt_Context *ctxt = (struct Kt_Context *)(handle);
	KT__ASSERT(kt_ctxt == ctxt && ctxt != NULL,
			   "Invalid parameter passed to Kt_AddQrStringBuffer.");

	Kt_AddBufferToFifo(&ctxt->kc__qr_buf_fifo, p_buffer, buff_len);
}

/**
 * Kt_GetQrStringBuffer returns the oldest QR string buffer from the QR string
 * buffer fifo. This fifo was built by the buffers provided by the Application
 * via the Kt_AddQrStringBuffer() function.
 *
 * @param handle is the handle to the KtLib context.
 * @param p_buffer_size is the pointer to the size of the QR string buffer.
 *
 * @return The pointer to the QR string buffer.
 */
void *Kt_GetQrStringBuffer(Kt_Handle handle, uint32_t *p_buffer_size)
{
	struct Kt_Context *ctxt = (struct Kt_Context *)(handle);
	KT__ASSERT(kt_ctxt == ctxt && ctxt != NULL,
			   "Invalid parameter passed to Kt_GetQrStringBuffer.");

	if (Kt_BufFifo_IsEmpty(&ctxt->kc__qr_buf_fifo)) {
		return NULL;
	}

	return Kt_GetBufferFromFifo(&ctxt->kc__qr_buf_fifo, p_buffer_size);
}

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
void Kt_ReleaseQrStringBuffers(Kt_Handle handle)
{
	struct Kt_Context *ctxt = (struct Kt_Context *)(handle);

	KT__ASSERT(kt_ctxt == ctxt && ctxt != NULL,
			   "Invalid parameter passed to Kt_ReleaseQrStringBuffers.");

	while (!Kt_BufFifo_IsEmpty(&ctxt->kc__qr_buf_fifo)) {
		Os_MemFree(ctxt->kc__cfg.kc__app_handle,
				   Kt_GetBufferFromFifo(&ctxt->kc__qr_buf_fifo, NULL));
	}
}

