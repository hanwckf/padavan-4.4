/*
 * This file is subject to the terms and conditions of the LGPL V2.1
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2020 Kalray Inc.
 */

#ifndef _SYS_CACHECTL_H
#define _SYS_CACHECTL_H	1

#include <asm/cachectl.h>

__BEGIN_DECLS

extern int cachectl(void *addr, size_t len, unsigned long cache,
		    unsigned long flags);

__END_DECLS

#endif
