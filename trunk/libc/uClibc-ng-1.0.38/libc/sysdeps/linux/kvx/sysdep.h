/*
 * This file is subject to the terms and conditions of the LGPL V2.1
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2018 Kalray Inc.
 */

#ifndef _LINUX_KVX_SYSDEP_H
#define _LINUX_KVX_SYSDEP_H 1

#include <common/sysdep.h>

#define SYS_ify(syscall_name)  (__NR_##syscall_name)

#ifdef	__ASSEMBLER__

# define _ENTRY(name)                    \
  .align 8;                              \
  .globl C_SYMBOL_NAME(name);            \
  .func  C_SYMBOL_NAME(name);            \
  .type  C_SYMBOL_NAME(name), @function; \
C_SYMBOL_NAME(name):		   	 \
	        cfi_startproc;

/* Define an entry point visible from C.  */
# ifdef PIC
# define ENTRY(name)                    \
  .pic					\
  _ENTRY(name)

# else
# define ENTRY(name) _ENTRY(name)
# endif

#endif

/* Local label name for asm code.  */
# ifndef L
#  define L(name) $L##name
# endif

#undef END
#define END(name) \
  cfi_endproc;        \
  .endfunc;           \
  .size C_SYMBOL_NAME(name), .-C_SYMBOL_NAME(name)

#endif //_LINUX_KVX_SYSDEP_H
