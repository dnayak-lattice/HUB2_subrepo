/******************************************************************************
 * Copyright (c) 2026 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#ifndef __KT_VERSION_H__
#define __KT_VERSION_H__

#include "utils.h"

/* Make changes for versioning here - do not change any other lines */
#define KT_VERSION_MAJOR  1
#define KT_VERSION_MINOR  0
#define KT_VERSION_BUGFIX 0
/* Make changes for versioning here - do not change any other lines */

#define STR_MAJOR         STR(KT_VERSION_MAJOR)
#define STR_MINOR         STR(KT_VERSION_MINOR)
#define STR_BUGFIX        STR(KT_VERSION_BUGFIX)

#define JOIN_SYMBOL(_1, _2, _3, symbol)                                        \
	_1 STR(symbol)                                                             \
	_2 STR(symbol) _3

#define KT_VERSION_STRING JOIN_SYMBOL(STR_MAJOR, STR_MINOR, STR_BUGFIX, .)

#endif /* __KT_VERSION_H__ */