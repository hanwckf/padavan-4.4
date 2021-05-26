/*
 * Private macros for accessing __jmp_buf contents.  kvx version.
 * This file is subject to the terms and conditions of the LGPL V2.1
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2019 Kalray Inc.
 */

#include <bits/wordsize.h>

#if __WORDSIZE == 64

/* We only need to save callee-saved registers plus stackpointer */
# define JB_R12	0 /* stack pointer */
# define JB_R14	1 /* frame pointer */
# define JB_R18	2
# define JB_R19	3
# define JB_R20	4
# define JB_R21	5
# define JB_R22	6
# define JB_R23	7
# define JB_R24	8
# define JB_R25	9
# define JB_R26	10
# define JB_R27	11
# define JB_R28	12
# define JB_R29	13
# define JB_R30	14
# define JB_R31	15

#ifndef  __ASSEMBLER__
#include <setjmp.h>
#include <stdint.h>
#include <sysdep.h>

static inline uintptr_t __attribute__ ((unused))
_jmpbuf_sp (__jmp_buf jmpbuf)
{
  uintptr_t sp = jmpbuf[0].regs[JB_R12];
  return sp;
}
#endif


#else

#error unsupported 32 bit wordsize

#endif
