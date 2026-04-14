/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#include "Kt_Lib.h"
#include "Kt_Internal.h"
#include "Kt_Assert.h"

/**
 * Kt_HandleReceivedQRImage will be called to process the received QR image from
 * the App Layer running over GARD.
 *
 * @param p_gard_handle The GARD handle.
 * @param p_handle The handle to the Kt_Context.
 * @param p_buffer The pointer to the buffer that contains the received QR
 * image.
 * @param buffer_size The size of the buffer that contains the received QR
 * image.
 *
 * @return True if the QR image is successfully handled, false otherwise.
 */
bool Kt_HandleReceivedQRImage(gard_handle_t p_gard_handle,
							  void         *handle,
							  void         *p_buffer,
							  uint32_t      buffer_size)
{
	p_gard_handle = p_gard_handle;
	handle        = handle;
	p_buffer      = p_buffer;
	buffer_size   = buffer_size;

	return false;
}
