/******************************************************************************
 * Copyright (c) 2026 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#include "hub.h"
#include "dal.h"
#include "hub_version.h"
#include "hub_globals.h"
#include "os_funcs.h"

/* Hub pre-initialization functions. */

/**
 * Get the version string of the HUB.
 *
 * @return A pointer to the version string.
 */
const char *hub_get_version_string(void)
{
	os_snprintf(hub_version_string, PATH_MAX, "%s", HUB_VERSION_STRING);

	return hub_version_string;
}

/**
 * Pre-initialize the HUB.
 *
 * @param p_config The configuration for the HUB.
 *
 * @return A pointer to the HUB handle.
 *         NULL if the HUB is not initialized successfully.
 */
hub_handle_t hub_preinit(struct hub_config *p_config)
{
	struct hub_ctx   *p_hub_ctx  = NULL;

	if (NULL == p_config) {
		hub_pr_err("HUB configuration is NULL\n");
		return NULL;
	}

	p_hub_ctx = (struct hub_ctx *)os_calloc(1, sizeof(struct hub_ctx));
	if (p_hub_ctx == NULL) {
		hub_pr_err("Failed to allocate memory for HUB context\n");
		return NULL;
	}

	p_hub_ctx->p_config  = p_config;
	p_hub_ctx->hub_state = HUB_BEGIN;

	p_hub_ctx->hub_state = HUB_PREINIT_DONE;
	return (hub_handle_t)p_hub_ctx;
}
