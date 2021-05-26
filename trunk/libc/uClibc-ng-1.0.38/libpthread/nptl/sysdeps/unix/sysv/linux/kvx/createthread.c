/*
 * This file is subject to the terms and conditions of the LGPL V2.1
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2019 Kalray Inc.
 */

/* Value passed to 'clone' for initialization of the thread register.  */
#define TLS_VALUE ((void *) (pd) \
		   + TLS_PRE_TCB_SIZE + TLS_INIT_TCB_SIZE)

/* Get the real implementation.	 */
#include <sysdeps/pthread/createthread.c>
