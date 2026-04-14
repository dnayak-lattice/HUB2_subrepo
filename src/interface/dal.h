/******************************************************************************
 * Copyright (c) 2026 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#ifndef __DAL_H__
#define __DAL_H__

#include "types.h"

#include "os_threads.h"

/* DAL return codes*/
enum dal_ret_code {
	DAL_SUCCESS                  = 0,
	/* DAL failure marker */
	DAL_FAILURE_MARKER           = -500,
	/* DAL async return codes*/
	DAL_ASYNC_PENDING            = -400,
	DAL_ASYNC_COMPLETED          = -399,
	/* DAL failure codes */
	DAL_FAILURE_INVALID_ARGS     = -200,
	DAL_FAILURE_DEVICE_NOT_FOUND = -199,
	DAL_FAILURE_DEVICE_BUSY      = -198,
	DAL_FAILURE_DEVICE_ERROR     = -197,
	DAL_FAILURE_DEVICE_TIMEOUT   = -196,
};

/* Opaque handle to a DAL instance */
typedef void *dal_handle_t;

/**
 * Opaque handle to a HUB instance ; forward decalaration, actual definition in
 * hub.h
 */
typedef void *hub_handle_t;

/**
 * Get the HUB GARD bus type from the DAL handle.
 *
 * @param dal_handle The DAL handle.
 *
 * @return The HUB GARD bus type.
 */
enum hub_gard_bus_types dal_get_bus_type(dal_handle_t dal_handle);

/**
 * Get the bus bandwidth from the DAL handle.
 *
 * @param dal_handle The DAL handle.
 *
 * @return The bus bandwidth.
 */
uint64_t dal_get_bus_bandwidth(dal_handle_t dal_handle);

/**
 * dal_write() will write the data to the device at the address referenced by
 * the dal_handle. The data is written to the device in a synchronous manner.

 * In case of success, the function will return number of bytes written.

 * In case of an error, the function will return a negative error code
 * corresponding to dal_ret_code. The function caller is expected to check the
 * return value and handle the error accordingly.
 *
 * @param dal_handle The DAL handle.
 * @param p_buffer Pointer to the buffer to write the data from.
 * @param count Number of bytes to write.
 * @param addr The address in the device memory map to write the data to.
 * @param timeout_msec Timeout in milliseconds.
 *
 * @return number of bytes written on success, a negative error code on failure
 *         corresponding to dal_ret_code
 */
int32_t dal_write(dal_handle_t dal_handle,
				  const void  *p_buffer,
				  uint32_t     count,
				  uint32_t     addr,
				  uint32_t     timeout_msec);

/**
 * dal_read() will read the data from the device at the address referenced by
 * the dal_handle. The data is read from the device in a synchronous manner.
 *
 * In case of success, the function will return number of bytes read.
 *
 * In case of an error, the function will return a negative error code
 * corresponding to dal_ret_code. The function caller is expected to check the
 * return value and handle the error accordingly.
 *
 * @param dal_handle The DAL handle.
 * @param p_buffer Pointer to the buffer to read the data into.
 * @param count Number of bytes to read.
 * @param addr The address in the device memory map to read the data from.
 * @param timeout_msec Timeout in milliseconds.
 *
 * @return number of bytes read on success, a negative error code on failure
 *         corresponding to dal_ret_code
 */
int32_t dal_read(dal_handle_t dal_handle,
				 void        *p_buffer,
				 uint32_t     count,
				 uint32_t     addr,
				 uint32_t     timeout_msec);

/**
 * These functions are intended for future use. They are not implemented in the
 * present release.
 */

/**
 * Completion callback function given to async reads/writes
 *
 * This function is called when the async read/write is completed.
 * The async caller function gets the number of bytes written/read by the async
 * read/write operation as an input argument to this callback function.
 *
 * @param p_context Context pointer passed to the async read/write
 * @param bytes_done Number of bytes successfully written/read
 *
 * @return DAL_ASYNC_COMPLETED on success, a corresponding DAL failure code on
 * failure
 */
typedef enum dal_ret_code (*dal_completion_callback_t)(void   *p_context,
													   int32_t bytes_done);

/**
 * dal_write_async() will write the data to the device at the address referenced
 * by the dal_handle. The data is written to the device in an asynchronous
 * manner.
 *
 * In case of success, the function will return DAL_ASYNC_PENDING. After
 * the write is completed, the callback function will be called.

 * In case of an error, the function will return a corresponding DAL failure
 * code on failure. The callback function will not be called in case of an
 error.
 *
 * @param dal_handle The DAL handle.
 * @param p_buffer Pointer to the buffer to write the data from.
 * @param count Number of bytes to write.
 * @param addr The address in the device memory map to write the data to.
 * @param timeout_msec Timeout in milliseconds.
 * @param p_callback Callback function to be called when the write is completed.
 * @param p_context Context pointer to be passed to the callback function.
 *
 * @return DAL_ASYNC_PENDING on success, a corresponding DAL failure code on
 * failure
 */
int32_t dal_write_async(dal_handle_t              dal_handle,
						const void               *p_buffer,
						uint32_t                  count,
						uint32_t                  addr,
						uint32_t                  timeout_msec,
						dal_completion_callback_t p_callback,
						void                     *p_context);

/**
 * dal_read_async() will read the data from the device at the address referenced
 * by the dal_handle. The data is read from the device in an asynchronous
 * manner.
 *
 * In case of success, the function will return DAL_ASYNC_PENDING.
 *
 * In case of an error, the function will return a corresponding DAL failure
 * code on failure. The callback function will not be called in case of an
 * error.
 *
 * @param dal_handle The DAL handle.
 * @param p_buffer Pointer to the buffer to read the data into.
 * @param count Number of bytes to read.
 * @param addr The address in the device memory map to read the data from.
 * @param timeout_msec Timeout in milliseconds.
 * @param p_callback Callback function to be called when the read is completed.
 * @param p_context Context pointer to be passed to the callback function.
 *
 * @return DAL_ASYNC_PENDING on success, a corresponding DAL failure code on
 * failure
 */
int32_t dal_read_async(dal_handle_t              dal_handle,
					   void                     *p_buffer,
					   uint32_t                  count,
					   uint32_t                  addr,
					   uint32_t                  timeout_msec,
					   dal_completion_callback_t p_callback,
					   void                     *p_context);

#endif /* __DAL_H__ */