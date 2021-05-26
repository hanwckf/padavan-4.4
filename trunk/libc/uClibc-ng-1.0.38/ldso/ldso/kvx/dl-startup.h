/*
 * Architecture specific code used by dl-startup.c
 * Copyright (C) 2016 Waldemar Brodkorb <wbx@uclibc-ng.org>
 * Copyright (C) 2018 Kalray Inc.
 *
 * Ported from GNU libc
 * Licensed under the LGPL v2.1, see the file COPYING.LIB in this tarball.
 */

/* Copyright (C) 1995-2016 Free Software Foundation, Inc.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation; either version 2.1 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#include <features.h>

/* This is the first bit of code, ever, executed in user space of a dynamically
 * linked ELF.
 * The kernel jumps on this with the following stack layout:
 * 	argc            argument counter (integer)
 *	argv[0]         program name (pointer)
 *	argv[1..argc-1] program args (pointers)
 * 	NULL
 *	env[0...N]      environment variables (pointers)
 *	NULL
 *	auxvt[0...N]   Auxiliary Vector Table elements (mixed types)
 *
 * We should call _dl_start($sp) (the argument should point to the previously
 * described memory layout).
 *
 * Next we should skip N arguments (N == _dl_skip_args).
 * Those correspond to the arguments which are consumed by the dynamic loader
 * if it is called directly as a program, which is possible when
 * __LDSO_STANDALONE_SUPPORT__ is defined.
 *
 * We eventually end up calling the main executable's _start (from ctr1.S).
 * The address of this _start is returned by _dl_start (in $r0).
 *
 * We should call this with one argument (in $r0): the address of _dl_fini()
 */
__asm__("\
.text							\n\
.globl _start						\n\
.type _start, %function					\n\
_start:							\n\
	copyd $r0 = $sp					\n\
	copyd $r18 = $sp				\n\
	andd $sp = $sp, -32				\n\
	call _dl_start					\n\
	;;						\n\
.globl _dl_start_user					\n\
.type _dl_start_user, %function				\n\
_dl_start_user:						\n\
	pcrel $r1 = @gotaddr() 				\n\
	copyd $r5 = $r0					\n\
	copyd $sp = $r18				\n\
	;;						\n\
	ld $r2 = @gotoff(_dl_skip_args)[$r1]		\n\
	addd $r0 = $r1, @gotoff(_dl_fini) 		\n\
	;;						\n\
	lwz $r3 = 0[$sp]				\n\
	;;						\n\
	sbfw $r4 = $r2, $r3				\n\
	addx8d $sp = $r2, $sp				\n\
	;;						\n\
	sd 0[$sp] = $r4					\n\
	icall $r5					\n\
	;;						\n\
");

/* Get a pointer to the argv array.  On many platforms this can be just
 * the address of the first argument, on other platforms we need to
 * do something a little more subtle here.  */
#define GET_ARGV(ARGVP, ARGS) ARGVP = (((unsigned long*)ARGS)+1)

/* Handle relocation of the symbols in the dynamic loader. */
static __always_inline
void PERFORM_BOOTSTRAP_RELOC(ELF_RELOC *rpnt, ElfW(Addr) *reloc_addr,
	ElfW(Addr) symbol_addr, ElfW(Addr) load_addr, ElfW(Sym) *sym)
{
	switch (ELF_R_TYPE(rpnt->r_info)) {
		case R_KVX_NONE:
			break;
		case R_KVX_JMP_SLOT:
		 	*reloc_addr = symbol_addr + rpnt->r_addend;
		 	break;
		case R_KVX_RELATIVE:
			*reloc_addr = load_addr + rpnt->r_addend;
			break;
		default:
			_dl_exit(1);
	}
}
