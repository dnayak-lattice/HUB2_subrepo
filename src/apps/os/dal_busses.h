/******************************************************************************
 * Copyright (c) 2026 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#ifndef __DAL_BUSSES_H__
#define __DAL_BUSSES_H__

#include "hub.h"
#include "types.h"
#include "os_threads.h"

/* Details on Device Abstraction Layer (DAL) busses. */

/* fd/handle */
typedef intptr_t dal_os_fd_t;
#define DAL_OS_INVALID_FD ((dal_os_fd_t) - 1)

/* Bus properties for each bus type. */

/**
 * DAL bus base structure - properties that are common to all bus types.
 */
struct dal_bus_base {
	dal_os_fd_t dev_hdl;
	uint8_t     is_open;
};

/* DAL UART bus properties */
struct dal_bus_uart_props {
	struct dal_bus_base base;
	char                bus_dev[PATH_MAX];
	uint32_t            baud_rate;
};

/* DAL bus structure. */
struct dal_bus {
	/**
	 * Type of the bus.
	 */
	enum hub_gard_bus_types bus_type;

	/**
	 * Union of bus properties for each bus type.
	 */
	union {
		struct dal_bus_uart_props uart_props;
		/* TBD-DPN: Add other bus types here */
	} props;

	/**
	 * Mutex to synchronize access to the bus.
	 * NOTE: This is not used in the present release.
	 */
	os_mutex_t bus_mutex;

	/**
	 * Bandwidth of the bus in bytes per second.
	 * NOTE: This is not used in the present release.
	 */
	uint64_t   bus_bandwidth;
};

#endif /* __DAL_BUSSES_H__ */