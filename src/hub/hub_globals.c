/******************************************************************************
 * Copyright (c) 2026 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#include "hub.h"

/* Hub global variables. */

/**
 * Array of pointers to string representations of
 * various busses supported by HUB.
 *
 * This array is auto-populated by a macro such that the string
 * equals the name of the bus's enum as defined in HUB (hub.h)
 */
const char *hub_gard_bus_strings[]        = {FOR_EACH_BUS(GEN_BUS_STRING)};

/**
 * Array of pointers to string representations of
 * various bus policies supported by HUB.
 *
 * This array is auto-populated by a macro such that the string
 * equals the name of the bus policy's enum as defined in HUB (hub.h)
 */
const char *hub_gard_bus_policy_strings[] = {
	FOR_EACH_BUS_POLICY(GEN_BUS_POLICY_STRING)};

/**
 * Version string for the HUB.
 */
char hub_version_string[PATH_MAX];