/*
 * This file is subject to the terms and conditions of the LGPL V2.1
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2020 Kalray Inc.
 */

#include <sys/syscall.h>

#ifdef __NR_cachectl
# include <sys/cachectl.h>
_syscall4(int, cachectl, void *, addr, size_t, len,
	  unsigned long, cache, unsigned long, flags)
#endif
