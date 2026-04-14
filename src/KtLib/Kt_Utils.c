/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#include "types.h"

#include "Kt_Internal.h"
#include "Kt_Lib.h"

/**
 * Kt_ConvertFixedPointToInteger converts a fixed point value to an unsigned
 * 32-bit integer.
 *
 * @param frac_bits is the number of fractional bits in the fixed point value.
 * @param fixed_point_value is the fixed point value to convert.
 *
 * @return The unsigned 32-bit integer value.
 */
uint32_t Kt_ConvertFixedPointToInteger(uint8_t  frac_bits,
									   uint32_t fixed_point_value)
{
	int fracMask = (1 << frac_bits) - 1;
	int halfFrac = 1 << (frac_bits - 1);
	int fracPart = fixed_point_value & fracMask;
	int floor    = fixed_point_value >> frac_bits;

	if (fracPart == halfFrac) {
		if (floor % 2 == 0) {
			return floor;
		} else {
			return floor + 1;
		}
	} else if (fracPart > halfFrac) {
		return floor + 1;
	} else {
		return floor;
	}
}

