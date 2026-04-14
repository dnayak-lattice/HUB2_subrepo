/******************************************************************************
 * Copyright (c) 2026 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

/* DAL UART operations. */

#include "dal_uart_ops.h"

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
int32_t dal_uart_init(dal_handle_t dal_handle)
{
	int32_t         ret, bus_hdl;
	speed_t         speed;

	struct dal_bus *p_bus = (struct dal_bus *)dal_handle;
	if (NULL == p_bus) {
		return DAL_FAILURE_INVALID_ARGS;
	}

	struct dal_bus_uart_props *p_uart_props =
		(struct dal_bus_uart_props *)&(p_bus->props.uart_props);
	if (NULL == p_uart_props) {
		return DAL_FAILURE_INVALID_ARGS;
	}

	/* Check if the device is already open */
	if (1 == p_uart_props->base.is_open) {
		return DAL_SUCCESS;
	}

	/* Open the UART device */
	bus_hdl = open(p_uart_props->bus_dev, O_RDWR);
	if (bus_hdl < 0) {
		return DAL_FAILURE_DEVICE_ERROR;
	}

	/* Set up the UART configuration */
	struct termios tty;
	if (tcgetattr(bus_hdl, &tty) < 0) {
		goto err_dal_uart_init;
	}

	switch (p_uart_props->baud_rate) {
	case DAL_UART_BAUD_RATE_9600:
		speed = B9600;
		break;
	case DAL_UART_BAUD_RATE_115200:
		speed = B115200;
		break;
	case DAL_UART_BAUD_RATE_921600:
		speed = B921600;
		break;
	default:
		return DAL_FAILURE_INVALID_ARGS;
	}

	ret = cfsetispeed(&tty, speed);
	if (0 != ret) {
		goto err_dal_uart_init;
	}
	ret = cfsetospeed(&tty, speed);
	if (0 != ret) {
		goto err_dal_uart_init;
	}

	/**
	 * Termios settings (match with minicom)
	 */
	/* All input flags disabled - completely raw mode */
	tty.c_iflag      = 0x0;
	/* Raw output - no processing */
	tty.c_oflag      = 0x0;
	/* Configure c_cflag: CS8, no parity, 1 stop bit */
	/* Clear size, parity, stop bits, hardware flow control */
	tty.c_cflag     &= ~(CSIZE | PARENB | CSTOPB | CRTSCTS | HUPCL);
	/* Set 8 data bits */
	tty.c_cflag     |= CS8;
	/* Enable CREAD for receiver functionality */
	tty.c_cflag     |= CREAD;
	/* Raw input - no echo, no canonical mode */
	tty.c_lflag      = 0x0;

	/* VMIN=0, VTIME=0: Non-blocking mode */
	/* We use select() in the read function to wait for data with timeout */
	tty.c_cc[VMIN]   = 0;
	tty.c_cc[VTIME]  = 0;

	if (tcsetattr(bus_hdl, TCSANOW, &tty) < 0) {
		goto err_dal_uart_init;
	}

	/* Mark the device as open */
	p_uart_props->base.dev_hdl = bus_hdl;
	p_uart_props->base.is_open = 1;

	return DAL_SUCCESS;

err_dal_uart_init:
	close(bus_hdl);
	p_uart_props->base.dev_hdl = -1;
	p_uart_props->base.is_open = 0;
	return DAL_FAILURE_DEVICE_ERROR;
}

/**
 * Finalize the UART device.
 *
 * @param dal_handle The DAL handle.
 *
 * @return 0 if the UART device is finalized successfully, -1 otherwise.
 */
int32_t dal_uart_fini(dal_handle_t dal_handle)
{
	int32_t         ret;
	struct dal_bus *p_bus = (struct dal_bus *)dal_handle;
	if (NULL == p_bus) {
		return DAL_FAILURE_INVALID_ARGS;
	}

	struct dal_bus_uart_props *p_uart_props =
		(struct dal_bus_uart_props *)&(p_bus->props.uart_props);
	if (NULL == p_uart_props) {
		return DAL_FAILURE_INVALID_ARGS;
	}

	/* Close the UART device */
	ret = close(p_uart_props->base.dev_hdl);
	if (ret < 0) {
		return DAL_FAILURE_DEVICE_ERROR;
	}

	/* Mark the device as closed */
	p_uart_props->base.is_open = 0;
	p_uart_props->base.dev_hdl = -1;

	return DAL_SUCCESS;
}

/**
 * Read data from the UART device.
 *
 * @param dal_handle The DAL handle.
 * @param p_buffer The buffer to read the data into.
 * @param count The number of bytes to read.
 * @param addr The address to read the data from.
 * @param timeout_usec The timeout in micro-seconds.

 * NOTE: This function expects the timeout in micro-seconds.
 *
 * @return Positive number indicates the number of bytes read.
 *         DAL_FAILURE_INVALID_ARGS if the DAL handle is invalid,
 *         DAL_FAILURE_DEVICE_ERROR if the UART device is not read successfully.
 */
int32_t dal_uart_read(dal_handle_t dal_handle,
					  void        *p_buffer,
					  uint32_t     count,
					  uint32_t     addr,
					  uint32_t     timeout_usec)
{
	int32_t         ret;
	struct dal_bus *p_bus = (struct dal_bus *)dal_handle;
	if (NULL == p_bus) {
		return DAL_FAILURE_INVALID_ARGS;
	}

	struct dal_bus_uart_props *p_uart_props =
		(struct dal_bus_uart_props *)&(p_bus->props.uart_props);
	if (NULL == p_uart_props) {
		return DAL_FAILURE_INVALID_ARGS;
	}

	uint32_t       bytes_left = count;
	ssize_t        nread, to_read;
	ssize_t        total_bytes = 0;

	fd_set         read_fds;

	struct timeval timeout;

	const int      max_retries = DAL_UART_READ_MAX_RETRIES;
	int            retry_count = 0;

	do {
		to_read = min_uint32(bytes_left, DAL_GARD_UART_READ_CHUNK_SZ);

		/**
		 * Use select() to wait for data availability.
		 */
		FD_ZERO(&read_fds);
		FD_SET(p_uart_props->base.dev_hdl, &read_fds);
		timeout.tv_sec  = 0;
		timeout.tv_usec = timeout_usec;

		ret = select(p_uart_props->base.dev_hdl + 1, &read_fds, NULL, NULL,
					 &timeout);

		if (-1 == ret) { /* select error */
			os_pr_err("select error\n");
			return total_bytes;
		}

		/* data is available, perform the read */
		if (FD_ISSET(p_uart_props->base.dev_hdl, &read_fds)) {
			nread = read(p_uart_props->base.dev_hdl, p_buffer, to_read);

			if (-1 == nread) { /* read error */
				os_pr_err("read error\n");
				return total_bytes;
			}

			if (0 == nread) { /* EOF or no data */
				/**
				 * eof or no data (shouldn't happen after select, but handle it)
				 */
				retry_count++;
				if (retry_count >= max_retries) {
					os_pr_err("max retries reached\n");
					return total_bytes;
				}
				continue;
			}

			/* reset retry count on successful read */
			retry_count  = 0;

			total_bytes += nread;
			bytes_left  -= nread;
			p_buffer    += nread;
		}
	} while (total_bytes < count);

	return total_bytes;
}

/**
 * Write data to the UART device.
 *
 * @param dal_handle The DAL handle.
 * @param p_buffer The buffer to write the data from.
 * @param count The number of bytes to write.
 * @param addr The address to write the data to.
 * @param timeout_usec The timeout in micro-seconds.
 *
 * NOTE: This function expects the timeout in micro-seconds.
 *
 * @return Positive number indicates the number of bytes written.
 *         DAL_FAILURE_INVALID_ARGS if the DAL handle is invalid,
 *         DAL_FAILURE_DEVICE_ERROR if the UART device is not written successfully.
 */
int32_t dal_uart_write(dal_handle_t dal_handle,
					   const void  *p_buffer,
					   uint32_t     count,
					   uint32_t     addr,
					   uint32_t     timeout_usec)
{
	int32_t         ret;
	struct dal_bus *p_bus = (struct dal_bus *)dal_handle;
	if (NULL == p_bus) {
		return DAL_FAILURE_INVALID_ARGS;
	}

	struct dal_bus_uart_props *p_uart_props =
		(struct dal_bus_uart_props *)&(p_bus->props.uart_props);
	if (NULL == p_uart_props) {
		return DAL_FAILURE_INVALID_ARGS;
	}

	/* Write the data */
	ret = write(p_uart_props->base.dev_hdl, p_buffer, count);
	if (ret < 0) {
		return DAL_FAILURE_DEVICE_ERROR;
	}

	/* Flush the output buffer */
	tcdrain(p_uart_props->base.dev_hdl);

	return ret;
}