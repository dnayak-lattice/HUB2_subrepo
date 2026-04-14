/******************************************************************************
 * Copyright (c) 2026 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#ifndef __UTILS_H__
#define __UTILS_H__

/**
 * This file contains utility functions that are used throughout the project.
 */

#define STR_HELPER(x) #x
#define STR(x)        STR_HELPER(x)

static inline uint32_t min_uint32(uint32_t a, uint32_t b)
{
	return a < b ? a : b;
}

static inline uint32_t max_uint32(uint32_t a, uint32_t b)
{
	return a > b ? a : b;
}

#endif /* __UTILS_H__ */

