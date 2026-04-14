/******************************************************************************
 * Copyright (c) 2026 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#include "Kt_Assert.h"

/**
 * kt_assert_failed() will print the user message and stay in an endless loop.
 * This function is used during debugging phase to catch the assert failures.
 *
 * @param assert_expr 	contains the failed assertion expression as a string
 * @param filename 		points to the name of the file from where this function
 *                 		is called
 * @param lineno 		is the line number in the file from where this function
 *               		is called
 *
 * @return Nothing
 */
void kt_assert_failed(const i8 *assert_expr,
					  const i8 *filename,
					  const u32 lineno,
					  ...)
{
	/**
	 * <TBD-SRP> Currently this function is implementedas an endless while loop.
	 * In future we may implement this to generate a crash dump for post-mortem
	 * analysis.
	 */
	(void)assert_expr;
	(void)filename;
	(void)lineno;

	while (true) {
		/* Endless loop */
	}
}