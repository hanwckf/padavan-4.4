/*
 * This file is subject to the terms and conditions of the LGPL V2.1
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2018 Kalray Inc.
 */

#include <sys/syscall.h>
#include <sysdep.h>

/* We do not want COMPAT to be enabled in our kernel, hence vfork
 * is not available. Use clone to do the same job with appropriate flags */
#define _SIGNAL_H
#include <bits/signum.h>       /* For SIGCHLD */

#define CLONE_VM		0x00000100
#define CLONE_VFORK		0x00004000
#define CLONE_FLAGS_FOR_VFORK	(CLONE_VM|CLONE_VFORK|SIGCHLD)

ENTRY(__vfork)
	make $r0 = CLONE_FLAGS_FOR_VFORK
	/* Not sure if needed to zero-out other parameters but better
	 * be safe than sorry */
	make $r1 = 0
	make $r2 = 0
	;;
	make $r3 = 0
	make $r4 = 0
	;;
	scall SYS_ify(clone)
	;;
	/* If PID < 0 then it's an error, else, simply return */
	cb.dltz $r0 ? err
	;;
	ret
	;;
L(err):
	goto __syscall_error
	;;
	/* Never return */
	errop
	;;
END(__vfork)

weak_alias(__vfork,vfork)
libc_hidden_def(vfork)
