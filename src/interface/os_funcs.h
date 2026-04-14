/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#ifndef __OS_FUNCS_H__
#define __OS_FUNCS_H__

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>

#include "types.h"

/**
 * This header file contains OS specific functions, support and features that
 * have to be implemented by the application. This header has 2 sections:
 * 1. Functions used by non-KtLib components such as HUB, apps, etc.
 * 2. Functions used by KtLib.
 */

/******************************************************************************
 * Functions used by non-KtLib components such as HUB, apps, etc.
 *
 * These functions are implemented in apps/os/app_os_funcs.c. These functions
 * follow the HUB / apps naming convention.
 ******************************************************************************/

/* OS Abstracted print/scan related functions implemented by the Application. */

/* Helper macros for printing to the console. */
#define os_fprintf(stream, fmt, args...) fprintf(stream, fmt, ##args);
#define os_pr_info(fmt, args...)         os_fprintf(stdout, fmt, ##args);
#define os_pr_err(fmt, args...)          os_fprintf(stderr, fmt, ##args);

/**
 * Print a formatted string to a string.
 *
 * @param str The string to print to.
 * @param size The size of the string.
 * @param format The format string.
 * @param args The arguments.
 *
 * @return The number of characters printed.
 */
int os_snprintf(char *str, size_t size, const char *format, ...);

/**
 * Scan a string into variables.
 *
 * @param format The format string.
 * @param ... The variables to scan into.
 *
 * @return The number of items scanned.
 */
int os_scanf(const char *format, ...);

/**
 * Compare two strings.
 *
 * @param str1 The first string.
 * @param str2 The second string.
 * @param n The number of characters to compare.
 *
 * @return The result of the comparison.
 */
int os_strncmp(const char *str1, const char *str2, size_t n);

/* OS Abstracted memory management functions implemented by the Application. */

/**
 * Allocate memory using malloc.
 *
 * @param size The size of the memory to allocate.
 *
 * @return A pointer to the allocated memory.
 */
void *os_malloc(size_t size);

/**
 * Allocate memory using calloc (memory is zeroed out)
 *
 * @param nmemb The number of elements to allocate.
 * @param size The size of each element.
 *
 * @return A pointer to the allocated memory.
 */
void *os_calloc(size_t nmemb, size_t size);

/**
 * Free memory.
 *
 * @param ptr The pointer to the memory to free.
 *
 * @return void.
 */
void os_free(void *ptr);

/**
 * Sleep for a given number of milliseconds.
 *
 * @param ms The number of milliseconds to sleep.
 *
 * @return void.
 */
void os_sleep_ms(uint32_t ms);

/**
 * Copy memory from one location to another.
 *
 * @param dest The destination pointer.
 * @param src The source pointer.
 * @param n The number of bytes to copy.
 *
 * @return void.
 */
void os_memcpy(void *dest, void *src, size_t n);

/**
 * Compares data in two memory areas upto n bytes.
 *
 * @param m1 First memory area
 * @param m2 Second memory area
 * @param n The number of bytes to compare.
 *
 * @return returns an integer less than, equal to, or greater than zero if the
 * first n bytes of m1 are found, respectively, to be less than, to match, or be
 * greater than the first n bytes of s2
 *
 */
int os_memcmp(const void *m1, const void *m2, size_t n);

/**
 * Set memory to a given value.
 *
 * @param s The pointer to the memory to set.
 * @param c The value to set the memory to.
 * @param n The number of bytes to set.
 *
 * @return void.
 */
void os_memset(void *s, int c, size_t n);

/******************************************************************************
 * Functions used by KtLib.
 *
 * These functions follow the KtLib naming convention for the functions. These
 * functions are implemented in apps/kt_app/kt_common/kt_app_os_funcs.c.
 ******************************************************************************/

/**
 * App_Handle is an opaque data structure that is passed to App functions. This
 * is a forward declaration. The formal definition is in Kt_Lib.h.
 */
typedef void *App_Handle;

/**
 * Allocate memory
 *
 * @param app_ctxt The app context
 * @param byte_count The number of bytes to allocate
 *
 * @return void *
 */
void *Os_MemAlloc(App_Handle app_ctxt, uint32_t byte_count);

/**
 * Free memory
 *
 * @param app_ctxt The app context
 * @param p_mem The memory to free
 *
 * @return void
 */
void Os_MemFree(App_Handle app_ctxt, void *p_mem);

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
void Os_MemSet(App_Handle app_ctxt, void *p_mem, uint8_t value, uint32_t size);

#endif /* __OS_FUNCS_H__ */