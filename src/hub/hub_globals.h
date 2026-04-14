/******************************************************************************
 * Copyright (c) 2026 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#ifndef __HUB_GLOBALS_H__
#define __HUB_GLOBALS_H__

#include "hub.h"

/* Hub global variables. */

/**
 * Array of pointers to string representations of
 * various busses supported by HUB.
 *
 * This array is auto-populated by a macro such that the string
 * equals the name of the bus's enum as defined in HUB (hub.h)
 */
extern const char *hub_gard_bus_strings[];

/**
 * Array of pointers to string representations of
 * various bus policies supported by HUB.
 *
 * This array is auto-populated by a macro such that the string
 * equals the name of the bus policy's enum as defined in HUB (hub.h)
 */
extern const char *hub_gard_bus_policy_strings[];

/**
 * Version string for the HUB.
 */
extern char hub_version_string[];

#endif /* __HUB_GLOBALS_H__ */