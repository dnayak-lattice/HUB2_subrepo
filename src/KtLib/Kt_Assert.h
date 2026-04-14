/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#ifndef __KT_ASSERT_H__
#define __KT_ASSERT_H__

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Runtime assert should be used sparingly.*/
#define KT__ASSERT(expr, ...)                                                  \
	do {                                                                       \
		if (!(expr)) {                                                         \
			kt_assert_failed(#expr, __FILE__, __LINE__, ##__VA_ARGS__);        \
		}                                                                      \
	} while (0)

/* Runtime assert to be used for DEBUG builds.*/
#ifdef KT_DEBUG
#define KT__DBG_ASSERT(expr, ...) KT__ASSERT(expr, __VA_ARGS__)
#else
#define KT__DBG_ASSERT(expr, ...) /* Do nothing */
#endif

/* Compile-time asserts. */
#if (__STDC_VERSION__ >= 201112)
/**
 * This macro has to be defined here since the static assert function name is
 * different in C11 and C++11
 */
#define static_assert _Static_assert
#else
#ifndef __cplusplus
/* Currently we do not support. */
#error "Run compiler with flag -std=c11"
#endif
#endif

#define KT__CASSERT(expr, msg) static_assert((expr), msg)

	/**
	 * kt_assert_failed() is used during development phase to catch the assert
	 * failures.
	 */
	/* The attribute noreturn is for code coverage */
	void kt_assert_failed(const char    *assert_expr,
						  const char    *filename,
						  const uint32_t lineno,
						  ...) __attribute__((noreturn));

#ifdef __cplusplus
}
#endif

#endif /* __KT_ASSERT_H__ */