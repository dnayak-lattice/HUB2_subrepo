/******************************************************************************
 * Copyright (c) 2026 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#ifndef __DAL_UART_OPS_H__
#define __DAL_UART_OPS_H__

#include <fcntl.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/time.h>

#include "dal.h"
#include "dal_busses.h"
#include "os_funcs.h"
#include "utils.h"

#define DAL_UART_BAUD_RATE_9600     9600
#define DAL_UART_BAUD_RATE_115200   115200
#define DAL_UART_BAUD_RATE_921600   921600

#define DAL_GARD_UART_READ_CHUNK_SZ (64U)

#define DAL_UART_READ_TIMEOUT_SEC   2
#define DAL_UART_READ_MAX_RETRIES   3

/**
 * Initialize the UART device.
 *
 * @param dal_handle The DAL handle.
 *
 * @return DAL_SUCCESS if the UART device is initialized successfully,
 *         DAL_FAILURE_INVALID_ARGS if the DAL handle is invalid,
 *         DAL_FAILURE_DEVICE_ERROR if the UART device is not initialized
 *         successfully.
 */
int32_t dal_uart_init(dal_handle_t dal_handle);

/**
 * Finalize the UART device.
 *
 * @param dal_handle The DAL handle.
 *
 * @return DAL_SUCCESS if the UART device is finalized successfully,
 *         DAL_FAILURE_INVALID_ARGS if the DAL handle is invalid,
 *         DAL_FAILURE_DEVICE_ERROR if the UART device is not finalized
 *         successfully.
 */
int32_t dal_uart_fini(dal_handle_t dal_handle);

/**
 * Read data from the UART device.
 *
 * @param dal_handle The DAL handle.
 * @param p_buffer The buffer to read the data into.
 * @param count The number of bytes to read.
 * @param addr The address to read the data from.
 * @param timeout_usec The timeout in micro-seconds.
 *
 * @return Positive number indicates the number of bytes read.
 *         DAL_FAILURE_INVALID_ARGS if the DAL handle is invalid,
 *         DAL_FAILURE_DEVICE_ERROR if the UART device is not read successfully.
 */
int32_t dal_uart_read(dal_handle_t dal_handle,
					  void        *p_buffer,
					  uint32_t     count,
					  uint32_t     addr,
					  uint32_t     timeout_usec);

/**
 * Write data to the UART device.
 *
 * @param dal_handle The DAL handle.
 * @param p_buffer The buffer to write the data from.
 * @param count The number of bytes to write.
 * @param addr The address to write the data to.
 * @param timeout_usec The timeout in micro-seconds.
 *
 * @return Positive number indicates the number of bytes written.
 *         DAL_FAILURE_INVALID_ARGS if the DAL handle is invalid,
 *         DAL_FAILURE_DEVICE_ERROR if the UART device is not written
 *         successfully.
 */
int32_t dal_uart_write(dal_handle_t dal_handle,
					   const void  *p_buffer,
					   uint32_t     count,
					   uint32_t     addr,
					   uint32_t     timeout_usec);

#endif /* __DAL_UART_OPS_H__ */