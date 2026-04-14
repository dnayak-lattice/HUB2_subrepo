/******************************************************************************
 * Copyright (c) 2026 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#include "os_funcs.h"

/* OS Abstracted print/scan related functions implemented by the Application. */

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
int os_snprintf(char *str, size_t size, const char *format, ...)
{
	va_list ap;
	va_start(ap, format);

	int ret = vsnprintf(str, size, format, ap);
	va_end(ap);

	return ret;
}

/**
 * Scan a string into variables.
 *
 * @param format The format string.
 * @param ... The variables to scan into.
 *
 * @return The number of items scanned.
 */
int os_scanf(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);

	int ret = vscanf(format, ap);
	va_end(ap);

	return ret;
}

/**
 * Compare two strings.
 *
 * @param str1 The first string.
 * @param str2 The second string.
 * @param n The number of characters to compare.
 *
 * @return The result of the comparison.
 */
int os_strncmp(const char *str1, const char *str2, size_t n)
{
	return strncmp(str1, str2, n);
}

/**
 * Allocate memory using malloc.
 *
 * @param nmemb The number of elements to allocate.
 * @param size The size of each element.
 *
 * @return A pointer to the allocated memory.
 */
void *os_malloc(size_t size)
{
	return malloc(size);
}

/**
 * Allocate memory using calloc (memory is zeroed out)
 *
 * @param nmemb The number of elements to allocate.
 * @param size The size of each element.
 *
 * @return A pointer to the allocated memory.
 */
void *os_calloc(size_t nmemb, size_t size)
{
	void *ptr = malloc(nmemb * size);
	if (NULL == ptr) {
		return NULL;
	}

	memset(ptr, 0, nmemb * size);
	return ptr;
}

/**
 * Free memory.
 *
 * @param ptr The pointer to the memory to free.
 *
 * @return void.
 */
void os_free(void *ptr)
{
	free(ptr);
}

/**
 * Sleep for a given number of milliseconds.
 *
 * @param ms The number of milliseconds to sleep.
 *
 * @return void.
 */
void os_sleep_ms(uint32_t ms)
{
	usleep(ms * 1000);
}

/**
 * Set memory to a given value.
 *
 * @param s The pointer to the memory to set.
 * @param c The value to set the memory to.
 * @param n The number of bytes to set.
 *
 * @return void.
 */
void os_memset(void *s, int c, size_t n)
{
	memset(s, c, n);
}

/**
 * Copy memory from one location to another.
 *
 * @param dest The destination pointer.
 * @param src The source pointer.
 * @param n The number of bytes to copy.
 *
 * @return void.
 */
void os_memcpy(void *dest, void *src, size_t n)
{
	memcpy(dest, src, n);
}

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
int os_memcmp(const void *m1, const void *m2, size_t n)
{
	return memcmp(m1, m2, n);
}