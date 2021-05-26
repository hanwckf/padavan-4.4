/*
 * This file is subject to the terms and conditions of the LGPL V2.1
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2018 Kalray Inc.
 */

#include <sysdep.h>
#define _ERRNO_H	1
#include <bits/errno.h>

/**
 * Clone system call implementation for kvx
 * int clone (int (*fn)(void *arg), void *child_stack, int flags, void *arg,
 *             pid_t *ptid, struct user_desc *tls, pid_t *ctid);
 * $r0 = fn
 * $r1 = child_stack
 * $r2 = flags
 * $r3 = args
 * $r4 = ptid
 * $r5 = tls
 * $r6 = ctid
 *
 * The kernel expects to find its arguments in the following order:
 * sys_clone(unsigned long clone_flags, unsigned long newsp,
 *		 int __user * parent_tidptr,
 *		 int __user * child_tidptr,
 *		 unsigned long tls)
 *
 * So we have to make a few modifications before calling
 *
 */

ENTRY (__clone)
	/* Check fn and stack to be non-null */
	cb.deqz $r1? L(clone_einval_error)
	/* Align child stack first */
	andd $r1 = $r1, -32
	;;
	cb.deqz $r0? L(clone_einval_error)
	/* Prepare space for child arguments on stack and stay aligned */
	addd $r1 = $r1, -32
	;;
	/* Save fn ($r0) on child stack */
	sd 0[$r1] = $r0
	/* Set clone_flags */
	copyd $r0 = $r2
	;;
	/* Save args ($r3) on child stack */
	sd 8[$r1] = $r3
	/* Set parent_tidptr */
	copyd $r2 = $r4
	/* Set child_tidptr */
	copyd $r3 = $r6
	/* Set tls */ 
	copyd $r4 = $r5
	;;
	scall SYS_ify(clone)
	;;
	/* If 0, then we are the child */
	cb.deqz $r0, L(child_start)
	;;
	/* Else we are the parent, and we need to check for errors */
	cb.dltz $r0, L(clone_error)
	;;
	/* No error ! Yeepa ! */
	ret
	;;
L(child_start):
	/* get fn from stack */
	ld $r1 = 0[$sp]
	;;
	/* Get args from stack */
	ld $r0 = 8[$sp]
	addd $sp = $sp, 32
	;;
	icall $r1
	;;
	scall SYS_ify(exit)
	;;
	/* We should never ever get here ! */
	errop
	;;
L(clone_einval_error):
	make $r0 = -EINVAL
	;;
L(clone_error):
	/* goto __syscall_error but do not use call or $ra will be
	 * destroyed */
	goto __syscall_error
	;;
	/* We will not return here but to clone caller
	 * (stored in $ra) */
	errop
	;;
END(__clone)

libc_hidden_def (__clone)
weak_alias (__clone,clone)
