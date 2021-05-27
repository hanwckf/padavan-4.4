/* Assembler macros for CRIS.
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   see <http://www.gnu.org/licenses/>.  */

#ifndef _SYSDEP_H_
#define _SYSDEP_H_

#include <common/sysdep.h>

#ifndef C_LABEL

/* Define a macro we can use to construct the asm name for a C symbol.  */
#ifdef  __STDC__
#define C_LABEL(name)           name##:
#else
#define C_LABEL(name)           name/**/:
#endif

#endif /* C_LABEL */

#define __STR(x) #x
#define STR(x) __STR(x)

/* Mark the end of function named SYM.  This is used on some platforms
   to generate correct debugging information.  */
#ifndef END
#define END(sym)
#endif

#undef SYS_ify
#define SYS_ify(syscall_name)   (__NR_##syscall_name)

#ifdef __ASSEMBLER__

#undef SYS_ify
#define SYS_ify(syscall_name)   __NR_##syscall_name

/* Syntactic details of assembly-code.  */

/* It is *not* generally true that "ELF uses byte-counts for .align, most
   others use log2 of count of bytes", like some neighboring configs say.
   See "align" in gas/read.c which is not overridden by
   gas/config/obj-elf.c.  It takes a log2 argument.  *Some* targets
   override it to take a byte argument.  People should read source instead
   of relying on hearsay.  */
#define ALIGNARG(log2) log2

#define ASM_TYPE_DIRECTIVE(name,typearg) .type name,typearg
#define ASM_SIZE_DIRECTIVE(name) .size name,.-name

/* The non-PIC jump is preferred, since it does not stall, and does not
   invoke generation of a PLT.  These macros assume that $r0 is set up as
   GOT register.  */
#ifdef __arch_v32
#ifdef __PIC__
#define PLTJUMP(_x) \
  ba C_SYMBOL_NAME (_x):PLT				@ \
  nop

#define PLTCALL(_x) \
  bsr C_SYMBOL_NAME (_x):PLT				@ \
  nop

#define SETUP_PIC \
  subq 4,$sp						@ \
  move.d $r0,[$sp]					@ \
  lapc _GLOBAL_OFFSET_TABLE_,$r0

#define TEARDOWN_PIC move.d [$sp+],$r0
#else
#define PLTJUMP(_x) \
  ba C_SYMBOL_NAME (_x)					@ \
  nop

#define PLTCALL(_x) \
  bsr  C_SYMBOL_NAME (_x)				@ \
  nop

#define SETUP_PIC
#define TEARDOWN_PIC
#endif

#else

#ifdef __PIC__
#define PLTJUMP(_x) \
  add.d C_SYMBOL_NAME (_x):PLT,$pc

#define PLTCALL(_x) \
  jsr [$r0+C_SYMBOL_NAME (_x):GOTPLT16]

#define SETUP_PIC \
  push $r0						@ \
  move.d $pc,$r0					@ \
  sub.d .:GOTOFF,$r0

#define TEARDOWN_PIC pop $r0
#else
#define PLTJUMP(_x) jump C_SYMBOL_NAME (_x)
#define PLTCALL(_x) jsr  C_SYMBOL_NAME (_x)
#define SETUP_PIC
#define TEARDOWN_PIC
#endif

#endif /* __arch_v32 */

/* Define an entry point visible from C.  */
#define ENTRY(name) \
  .text							@ \
  .globl C_SYMBOL_NAME (name) 		@ \
  ASM_TYPE_DIRECTIVE (C_SYMBOL_NAME (name), function)	@ \
  .align ALIGNARG (2) 					@ \
  C_LABEL(name)

#define HIDDEN_ENTRY(name) \
  .text							@ \
  .globl C_SYMBOL_NAME (name) 		@ \
  .hidden C_SYMBOL_NAME (name)				@ \
  ASM_TYPE_DIRECTIVE (C_SYMBOL_NAME (name), function)	@ \
  .align ALIGNARG (2) 					@ \
  C_LABEL(name)

#undef END
#define END(name) \
  ASM_SIZE_DIRECTIVE (C_SYMBOL_NAME (name))

#define PSEUDO(name, syscall_name, args) \
  ENTRY (name)						@ \
  DOARGS_##args						@ \
  movu.w SYS_ify (syscall_name),$r9			@ \
  break 13						@ \
  cmps.w -4096,$r10					@ \
  bhs   0f						@ \
  nop							@ \
  UNDOARGS_return_##args

#define PSEUDO_END(name) \
0:							@ \
  SETUP_PIC						@ \
  PLTJUMP (__syscall_error)				@ \
  END (name)

#endif /* __ASSEMBLER__ */
#endif /* _SYSDEP_H_ */
