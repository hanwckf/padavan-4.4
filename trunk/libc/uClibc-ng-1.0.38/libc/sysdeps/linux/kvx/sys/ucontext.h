/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2018 Kalray Inc.
 */

#ifndef _SYS_UCONTEXT_H
#define _SYS_UCONTEXT_H	1

#include <signal.h>
#include <bits/sigcontext.h>

/* Type for general register.  */
typedef unsigned long greg_t;

/* Number of general registers.  */
#define NGREG	64

typedef struct ucontext {
	unsigned long	  uc_flags;
	struct ucontext  *uc_link;
	stack_t		  uc_stack;
	struct sigcontext uc_mcontext;
	sigset_t	  uc_sigmask;	/* mask last for extensibility */
} ucontext_t;

#endif /* sys/ucontext.h */
