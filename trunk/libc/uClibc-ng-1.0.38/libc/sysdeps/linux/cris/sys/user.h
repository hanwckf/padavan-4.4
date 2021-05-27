#ifndef __ASM_CRIS_USER_H
#define __ASM_CRIS_USER_H

/* User-mode register used for core dumps. */

struct user_fpregs {
};

struct user_regs_struct {
	unsigned long r0;	/* General registers. */
	unsigned long r1;
	unsigned long r2;
	unsigned long r3;
	unsigned long r4;
	unsigned long r5;
	unsigned long r6;
	unsigned long r7;
	unsigned long r8;
	unsigned long r9;
	unsigned long r10;
	unsigned long r11;
	unsigned long r12;
	unsigned long r13;
	unsigned long sp;	/* R14, Stack pointer. */
	unsigned long acr;	/* R15, Address calculation register. */
	unsigned long bz;	/* P0, Constant zero (8-bits). */
	unsigned long vr;	/* P1, Version register (8-bits). */
	unsigned long pid;	/* P2, Process ID (8-bits). */
	unsigned long srs;	/* P3, Support register select (8-bits). */
	unsigned long wz;	/* P4, Constant zero (16-bits). */
	unsigned long exs;	/* P5, Exception status. */
	unsigned long eda;	/* P6, Exception data address. */
	unsigned long mof;	/* P7, Multiply overflow regiter. */
	unsigned long dz;	/* P8, Constant zero (32-bits). */
	unsigned long ebp;	/* P9, Exception base pointer. */
	unsigned long erp;	/* P10, Exception return pointer. */
	unsigned long srp;	/* P11, Subroutine return pointer. */
	unsigned long nrp;	/* P12, NMI return pointer. */
	unsigned long ccs;	/* P13, Condition code stack. */
	unsigned long usp;	/* P14, User mode stack pointer. */
	unsigned long spc;	/* P15, Single step PC. */
};

/*
 * Core file format: The core file is written in such a way that gdb
 * can understand it and provide useful information to the user (under
 * linux we use the `trad-core' bfd).  The file contents are as follows:
 *
 *  upage: 1 page consisting of a user struct that tells gdb
 *	what is present in the file.  Directly after this is a
 *	copy of the task_struct, which is currently not used by gdb,
 *	but it may come in handy at some point.  All of the registers
 *	are stored as part of the upage.  The upage should always be
 *	only one page long.
 *  data: The data segment follows next.  We use current->end_text to
 *	current->brk to pick up all of the user variables, plus any memory
 *	that may have been sbrk'ed.  No attempt is made to determine if a
 *	page is demand-zero or if a page is totally unused, we just cover
 *	the entire range.  All of the addresses are rounded in such a way
 *	that an integral number of pages is written.
 *  stack: We need the stack information in order to get a meaningful
 *	backtrace.  We need to write the data from usp to
 *	current->start_stack, so we round each of these in order to be able
 *	to write an integer number of pages.
 */

struct user {
	struct user_regs_struct	regs;		/* entire machine state */
	size_t		u_tsize;		/* text size (pages) */
	size_t		u_dsize;		/* data size (pages) */
	size_t		u_ssize;		/* stack size (pages) */
	unsigned long	start_code;		/* text starting address */
	unsigned long	start_data;		/* data starting address */
	unsigned long	start_stack;		/* stack starting address */
	long int	signal;			/* signal causing core dump */
	unsigned long	u_ar0;			/* help gdb find registers */
	unsigned long	magic;			/* identifies a core file */
	char		u_comm[32];		/* user command name */
};

#endif /* __ASM_CRIS_USER_H */
