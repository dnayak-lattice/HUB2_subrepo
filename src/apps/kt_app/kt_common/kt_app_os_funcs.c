/******************************************************************************
 * Copyright (c) 2026 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#include "os_funcs.h"

/**
 * These functions are implementations of OS specific features as advertised by
 * KtLib in its requirements.
 *
 * Important: Note that these functions follow the KtLib naming convention for
 * the functions. The other components (hub, apps [including kt_app]) follow a
 * different naming convention. As a consequence, the functions in this file are
 * mere wrappers around the OS functions offered by apps/os/app_os_funcs.c.
 */

/**
 * Allocate memory
 *
 * @param app_ctxt The app context
 * @param byte_count The number of bytes to allocate
 *
 * @return void *
 */
void *Os_MemAlloc(App_Handle app_ctxt, uint32_t byte_count)
{
	return os_malloc(byte_count);
}

/**
 * Free memory
 *
 * @param app_ctxt The app context
 * @param p_mem The memory to free
 *
 * @return void
 */
void Os_MemFree(App_Handle app_ctxt, void *p_mem)
{
	os_free(p_mem);
}

/**
 * Set memory
 *
 * @param app_ctxt The app context
 * @param p_mem The memory to set
 * @param value The value to set the memory to
 * @param size The number of bytes to set
 *
 * @return void
 */
void Os_MemSet(App_Handle app_ctxt, void *p_mem, uint8_t value, uint32_t size)
{
	os_memset(p_mem, value, size);
}