/* *****************************************************************************
 * Copyright (c) 2026 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#ifndef __DAL_DEVICE_INIT_H__
#define __DAL_DEVICE_INIT_H__

#include "dal.h"

/* Device Abstraction Layer (DAL) device initialization functions. */

/**
 * Initialize the device.
 *
 * @param dal_handle The DAL handle.
 *
 * @return DAL_SUCCESS if the device is initialized successfully,
 *         DAL_FAILURE_DEVICE_ERROR if the device is not initialized successfully.
 */
enum dal_ret_code dal_init(dal_handle_t dal_handle);

/**
 * Finalize the device.
 *
 * @param dal_handle The DAL handle.
 *
 * @return DAL_SUCCESS if the device is finalized successfully,
 *         DAL_FAILURE_DEVICE_ERROR if the device is not finalized successfully.
 */
enum dal_ret_code dal_fini(dal_handle_t dal_handle);

#endif /* __DAL_DEVICE_INIT_H__ */