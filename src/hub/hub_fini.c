/******************************************************************************
 * Copyright (c) 2026 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#include "hub.h"
#include "dal.h"

/* Hub finalization functions. */

/**
 * Clean up the HUB and free the resources.
 * This function should be called after the HUB is no longer needed.
 *
 * @param hub_handle The handle to the HUB.
 *
 * @return HUB_SUCCESS if the HUB is cleaned up successfully,
 *         HUB_FAILURE_FINI if the HUB is not cleaned up successfully.
 */
enum hub_ret_code hub_fini(hub_handle_t hub_handle)
{
	struct hub_ctx    *p_hub_ctx = (struct hub_ctx *)hub_handle;
	struct hub_config *p_config  = p_hub_ctx->p_config;

	if (p_hub_ctx == NULL) {
		hub_pr_err("HUB context is NULL\n");
		return HUB_FAILURE_FINI;
	}

	if (p_config == NULL) {
		hub_pr_err("HUB configuration is NULL\n");
		return HUB_FAILURE_FINI;
	}

	/* Free the memory allocated in the GARD structures */
	for (uint32_t i = 0; i < p_hub_ctx->num_gards; i++) {
		struct hub_gard_info *p_gard = &p_hub_ctx->p_gards[i];
		if (p_gard->p_dal_busses != NULL) {
			os_free(p_gard->p_dal_busses);
		}
	}

	/* Free the memory allocated in the hub_config structure */

	/* Free the GARDs */
	if (p_hub_ctx->p_gards != NULL) {
		os_free(p_hub_ctx->p_gards);
	}

	os_free(p_hub_ctx);

	return HUB_SUCCESS;
}