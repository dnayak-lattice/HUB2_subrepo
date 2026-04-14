/******************************************************************************
 * Copyright (c) 2026 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#include "kt_app.h"

/* Kt App Buffer Management Functions*/

/**
 * Initialize the metadata buffer queue

 * @param app_handle The App handle
 *
 * @return void
 */
void kt_app_metadata_buffer_queue_init(App_Handle app_handle)
{
	struct kt_app_ctx *p_app_ctx = (struct kt_app_ctx *)app_handle;
	os_memset(&p_app_ctx->buffer_queue, 0, sizeof(metadata_buffer_queue_t));

	for (uint32_t i = 0; i < KT_APP_METADATA_BUFFER_POOL_SIZE; i++) {
		p_app_ctx->buffer_queue.buffers[i].p_buffer =
			os_malloc(KT_APP_METADATA_BUFFER_SIZE);
		if (NULL == p_app_ctx->buffer_queue.buffers[i].p_buffer) {
			os_pr_err("Failed to allocate memory for metadata buffer %d\n", i);
			return;
		}
		p_app_ctx->buffer_queue.buffers[i].buff_len =
			KT_APP_METADATA_BUFFER_SIZE;
	}
	p_app_ctx->buffer_queue.tail = 0;
	p_app_ctx->buffer_queue.head = 0;
	os_mutex_init(&p_app_ctx->buffer_queue.mutex);
}

/**
 * Finalize the metadata buffer queue
 *
 * @param app_handle The App handle
 *
 * @return void
 */
void kt_app_metadata_buffer_queue_fini(App_Handle app_handle)
{
	struct kt_app_ctx *p_app_ctx = (struct kt_app_ctx *)app_handle;
	for (uint32_t i = 0; i < KT_APP_METADATA_BUFFER_POOL_SIZE; i++) {
		os_free(p_app_ctx->buffer_queue.buffers[i].p_buffer);
	}
	os_mutex_destroy(&p_app_ctx->buffer_queue.mutex);
}

/**
 * Add a buffer to the metadata buffer queue
 *
 * @param app_handle The App handle
 * @param p_buffer The buffer to add
 * @param buff_len The length of the buffer
 *
 * @return void
 */
void kt_app_metadata_buffer_queue_add(App_Handle app_handle,
									  uint8_t   *p_buffer,
									  uint32_t   buff_len)
{
	struct kt_app_ctx       *p_app_ctx      = (struct kt_app_ctx *)app_handle;

	metadata_buffer_queue_t *p_buffer_queue = &p_app_ctx->buffer_queue;

	os_mutex_lock(&p_buffer_queue->mutex);
	uint32_t tail = p_buffer_queue->tail;
	if (tail == p_buffer_queue->head) {
		os_pr_err("Metadata buffer queue is full\n");
		os_mutex_unlock(&p_buffer_queue->mutex);
		return;
	}
	p_buffer_queue->buffers[tail].p_buffer = p_buffer;
	p_buffer_queue->buffers[tail].buff_len = buff_len;
	p_buffer_queue->tail = (tail + 1) % KT_APP_METADATA_BUFFER_POOL_SIZE;
	os_mutex_unlock(&p_buffer_queue->mutex);
}

/**
 * Get a buffer from the metadata buffer queue
 *
 * @param app_handle The App handle
 * @param p_buffer The buffer to get
 * @param buff_len The length of the buffer
 *
 * @return void
 */
void kt_app_metadata_buffer_queue_get(App_Handle app_handle,
									  uint8_t  **p_buffer,
									  uint32_t  *buff_len)
{
	struct kt_app_ctx       *p_app_ctx      = (struct kt_app_ctx *)app_handle;

	metadata_buffer_queue_t *p_buffer_queue = &p_app_ctx->buffer_queue;

	os_mutex_lock(&p_buffer_queue->mutex);
	uint32_t next_head =
		(p_buffer_queue->head + 1) % KT_APP_METADATA_BUFFER_POOL_SIZE;
	if (next_head == p_buffer_queue->tail) {
		os_mutex_unlock(&p_buffer_queue->mutex);
		os_pr_err("Metadata buffer queue is empty\n");
		*p_buffer = NULL;
		*buff_len = 0;
		return;
	}
	*p_buffer            = p_buffer_queue->buffers[next_head].p_buffer;
	*buff_len            = p_buffer_queue->buffers[next_head].buff_len;
	p_buffer_queue->head = next_head;
	os_mutex_unlock(&p_buffer_queue->mutex);
}