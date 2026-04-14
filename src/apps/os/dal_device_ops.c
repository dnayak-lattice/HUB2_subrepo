/* *****************************************************************************
 * Copyright (c) 2026 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#include "hub.h"
#include "dal.h"
#include "dal_busses.h"
#include "dal_device_init.h"
#include "dal_uart_ops.h"

/* DAL device operations. */

/**
 * Get the HUB GARD bus type from the DAL handle.
 *
 * @param dal_handle The DAL handle.
 *
 * @return The HUB GARD bus type.
 */
enum hub_gard_bus_types dal_get_bus_type(dal_handle_t dal_handle)
{
	struct dal_bus *p_bus = (struct dal_bus *)dal_handle;
	return p_bus->bus_type;
}

/**
 * Get the bus bandwidth from the DAL handle.
 *
 * @param dal_handle The DAL handle.
 *
 * @return The bus bandwidth.
 */
uint64_t dal_get_bus_bandwidth(dal_handle_t dal_handle)
{
	struct dal_bus *p_bus = (struct dal_bus *)dal_handle;
	return p_bus->bus_bandwidth;
}

/**
 * Initialize the DAL handle.
 *
 * @param dal_handle The DAL handle.
 *
 * @return DAL_SUCCESS if the device is initialized successfully,
 *         DAL_FAILURE_DEVICE_ERROR if the device is not initialized
 * successfully.
 */
enum dal_ret_code dal_init(dal_handle_t dal_handle)
{
	struct dal_bus *p_bus = (struct dal_bus *)dal_handle;
	int32_t         ret;

	/* find the bus type and initialize the bus */
	switch (p_bus->bus_type) {
	case HUB_GARD_BUS_UART:
		ret = dal_uart_init(dal_handle);
		if (ret < 0) {
			return DAL_FAILURE_DEVICE_ERROR;
		}
		break;
	default:
		return DAL_FAILURE_INVALID_ARGS;
	}

	return DAL_SUCCESS;
}

/**
 * Finalize the DAL handle.
 *
 * @param dal_handle The DAL handle.
 *
 * @return DAL_SUCCESS if the device is finalized successfully,
 *         DAL_FAILURE_DEVICE_ERROR if the device is not finalized successfully.
 */
enum dal_ret_code dal_fini(dal_handle_t dal_handle)
{
	struct dal_bus *p_bus = (struct dal_bus *)dal_handle;
	int32_t         ret;

	/* find the bus type and finalize the bus */
	switch (p_bus->bus_type) {
	case HUB_GARD_BUS_UART:
		ret = dal_uart_fini(dal_handle);
		if (ret < 0) {
			return DAL_FAILURE_DEVICE_ERROR;
		}
		break;
	default:
		return DAL_FAILURE_INVALID_ARGS;
	}

	return DAL_SUCCESS;
}

/**
 * Read data from the DAL handle.
 *
 * @param dal_handle The DAL handle.
 * @param p_buffer The buffer to read the data into.
 * @param count The number of bytes to read.
 * @param addr The address to read from.
 * @param timeout_msec The timeout in milliseconds.
 *
 * @return The number of bytes read.
 */
int32_t dal_read(dal_handle_t dal_handle,
				 void        *p_buffer,
				 uint32_t     count,
				 uint32_t     addr,
				 uint32_t     timeout_msec)
{
	int32_t ret;

	if (count == 0) {
		return DAL_SUCCESS;
	}

	struct dal_bus *p_bus = (struct dal_bus *)dal_handle;

	/* find the bus type and read the data */
	switch (p_bus->bus_type) {
	case HUB_GARD_BUS_UART:
		/**
		 * NOTE: UART read expects a timeout in microseconds.
		 * Hence we convert milliseconds to microseconds before the call
		 */
		ret = dal_uart_read(dal_handle, p_buffer, count, addr,
							timeout_msec * 1000);
		if (ret < 0) {
			return DAL_FAILURE_DEVICE_ERROR;
		}
		break;
	default:
		return DAL_FAILURE_INVALID_ARGS;
	}

	return ret;
}

/**
 * Write data to the DAL handle.
 *
 * @param dal_handle The DAL handle.
 * @param p_buffer The buffer to write the data from.
 * @param count The number of bytes to write.
 * @param addr The address to write to.
 * @param timeout_msec The timeout in milliseconds.
 *
 * @return The number of bytes written.
 */
int32_t dal_write(dal_handle_t dal_handle,
				  const void  *p_buffer,
				  uint32_t     count,
				  uint32_t     addr,
				  uint32_t     timeout_msec)
{
	int32_t ret;

	if (count == 0) {
		return DAL_SUCCESS;
	}

	struct dal_bus *p_bus = (struct dal_bus *)dal_handle;

	/* find the bus type and write the data */
	switch (p_bus->bus_type) {
	case HUB_GARD_BUS_UART:
		/**
		 * NOTE: UART write expects a timeout in micro-seconds.
		 * Hence we convert milliseconds to micro-seconds before the call
		 */
		ret = dal_uart_write(dal_handle, p_buffer, count, addr,
							 timeout_msec * 1000);
		if (ret < 0) {
			return DAL_FAILURE_DEVICE_ERROR;
		}
		break;
	default:
		return DAL_FAILURE_INVALID_ARGS;
	}

	return ret;
}

