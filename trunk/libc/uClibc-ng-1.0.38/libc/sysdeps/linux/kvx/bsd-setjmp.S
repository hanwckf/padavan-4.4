/*
 * This file is subject to the terms and conditions of the LGPL V2.1
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2018 Kalray Inc.
 */

#include <sysdep.h>

ENTRY(setjmp)
	/* Set second argument to 1 */
	make $r1 = 1
	;;
	goto HIDDEN_JUMPTARGET(__sigsetjmp)
	;;
END(setjmp)
