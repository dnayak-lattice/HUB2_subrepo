/******************************************************************************
 * Copyright (c) 2026 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#include "hub.h"
#include "dal.h"
#include "hub_globals.h"
#include "os_funcs.h"

/* Hub initialization functions. */

/**
 * Initialize the HUB
 *
 * @param hub_handle The handle to the HUB.
 *
 * @return HUB_SUCCESS if the HUB is initialized successfully,
 *         HUB_FAILURE_INIT if the HUB is not initialized successfully.
 */
enum hub_ret_code hub_init(hub_handle_t hub_handle)
{
	struct hub_ctx         *p_hub_ctx  = (struct hub_ctx *)hub_handle;
	struct hub_gard_info   *p_gard     = NULL;
	dal_handle_t            dal_handle = NULL;
	enum hub_gard_bus_types bus_type   = HUB_GARD_BUS_UNKNOWN;

	if (NULL == p_hub_ctx || HUB_DISCOVER_DONE != p_hub_ctx->hub_state) {
		hub_pr_err("Invalid HUB handle or GARDs not discovered\n");
		return HUB_FAILURE_INIT;
	}

	hub_pr_dbg("Initializing HUB...\n");

	for (uint32_t i = 0; i < p_hub_ctx->num_gards; i++) {
		p_gard             = &p_hub_ctx->p_gards[i];

		/* <SRP-TBD> This hard-coding needs to be changed in future. */
		p_gard->bus_policy = HUB_GARD_BUS_POLICY_USE_UART;
		hub_pr_dbg("Initializing GARD %d...\n", p_gard->gard_profile_id);

		switch (p_gard->bus_policy) {
		case HUB_GARD_BUS_POLICY_USE_UART:
			for (uint32_t k = 0; k < p_gard->num_busses; k++) {
				dal_handle = p_gard->p_dal_busses[k];
				bus_type   = dal_get_bus_type(dal_handle);
				if (HUB_GARD_BUS_UART == bus_type) {
					p_gard->active_dal_bus = dal_handle;
					hub_pr_info("Active bus: %s\n",
								hub_gard_bus_strings[bus_type]);
					break;
				}
			}
			break;
		default:
			hub_pr_err("Invalid bus policy: %d\n", p_gard->bus_policy);
			return HUB_FAILURE_INIT;
		}

		if (NULL == p_gard->active_dal_bus) {
			hub_pr_err("No active DAL bus found for GARD profile ID: %d\n",
					   p_gard->gard_profile_id);
			return HUB_FAILURE_INIT;
		}

		/* Currently there is no command execution happening. */
		p_gard->command_execution = false;
	}

	p_hub_ctx->hub_state = HUB_INIT_DONE;

	return HUB_SUCCESS;
}