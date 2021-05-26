/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2018 Kalray Inc.
 */

#ifndef _BITS_SYSCALLS_H
#define _BITS_SYSCALLS_H
#ifndef _SYSCALL_H
# error "Never use <bits/syscalls.h> directly; include <sys/syscall.h> instead."
#endif

#ifndef __ASSEMBLER__

#define INTERNAL_SYSCALL_NCS(name, err, nr, args...)                    \
 	({									\
		register long _ret __asm__("r0");			\
		register unsigned long _scno  __asm__("r6") = name;	\
		LOAD_ARGS_##nr (args)					\
		__asm__ __volatile__("scall %[r_scno]"			\
				     : "=r" (_ret)			\
				     : [r_scno] "r" (_scno) ASM_ARGS_##nr \
				     : ASM_CLOBBER_##nr);		\
		_ret;							\
	})

/* Mark all argument registers as per ABI in the range r1-r5 as
   clobbered when they are not used for the invocation of the scall */
#define ASM_CLOBBER_6 "cc", "memory",					\
    "r7", "r8", "r9", "r10", "r11", /* unused argument registers */ \
    "r15", /* struct pointer */						\
    "r16", "r17", /* veneer registers */				\
    "r32", "r33", "r34", "r35", "r36", "r37", "r38", "r39", /* 32->63 are caller-saved */ \
    "r40", "r41", "r42", "r43", "r44", "r45", "r46", "r47",		\
    "r48", "r49", "r50", "r51", "r52", "r53", "r54", "r55",		\
    "r56", "r57", "r58", "r59", "r60", "r61", "r62", "r63"
#define ASM_CLOBBER_5 "r5", ASM_CLOBBER_6
#define ASM_CLOBBER_4 "r4", ASM_CLOBBER_5
#define ASM_CLOBBER_3 "r3", ASM_CLOBBER_4
#define ASM_CLOBBER_2 "r2", ASM_CLOBBER_3
#define ASM_CLOBBER_1 "r1", ASM_CLOBBER_2
#define ASM_CLOBBER_0 ASM_CLOBBER_1

#define LOAD_ARGS_0()
#define ASM_ARGS_0

#define LOAD_ARGS_1(a1)                                 \
	LOAD_ARGS_0();					\
	_ret  = (long) a1;
#define ASM_ARGS_1      ASM_ARGS_0, "r"(_ret)

#define LOAD_ARGS_2(a1, a2)                             \
	LOAD_ARGS_1(a1);				\
	register long _a2 __asm__("r1") = (long) a2;
#define ASM_ARGS_2      ASM_ARGS_1, "r"(_a2)

#define LOAD_ARGS_3(a1, a2, a3)                         \
	LOAD_ARGS_2(a1, a2);				\
	register long _a3 __asm__("r2") = (long) a3;
#define ASM_ARGS_3      ASM_ARGS_2, "r"(_a3)

#define LOAD_ARGS_4(a1, a2, a3, a4)                     \
	LOAD_ARGS_3(a1, a2, a3);			\
	register long _a4 __asm__("r3") = (long) a4;
#define ASM_ARGS_4      ASM_ARGS_3, "r"(_a4)

#define LOAD_ARGS_5(a1, a2, a3, a4, a5)                 \
	LOAD_ARGS_4(a1, a2, a3, a4);			\
	register long _a5 __asm__("r4") = (long) a5;
#define ASM_ARGS_5      ASM_ARGS_4, "r"(_a5)

#define LOAD_ARGS_6(a1, a2, a3, a4, a5, a6)             \
	LOAD_ARGS_5(a1, a2, a3, a4, a5);		\
	register long _a6 __asm__("r5") = (long) a6;
#define ASM_ARGS_6      ASM_ARGS_5, "r"(_a6)

#endif /* __ASSEMBLER__ */
#endif /* _BITS_SYSCALLS_H */
