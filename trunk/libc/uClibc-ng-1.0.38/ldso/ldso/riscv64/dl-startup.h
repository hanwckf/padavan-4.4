/*
 * Architecture specific code used by dl-startup.c
 * Copyright (C) 2019 Waldemar Brodkorb <wbx@uclibc-ng.org>
 * Ported from GNU libc
 * Licensed under the LGPL v2.1, see the file COPYING.LIB in this tarball.
 */

/* Copyright (C) 2011-2019 Free Software Foundation, Inc.

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
   <https://www.gnu.org/licenses/>.  */

#include <features.h>
#include <sys/asm.h>

#ifndef _RTLD_PROLOGUE
# define _RTLD_PROLOGUE(entry)                                          \
        ".globl\t" __STRING (entry) "\n\t"                              \
        ".type\t" __STRING (entry) ", @function\n"                      \
        __STRING (entry) ":\n\t"
#endif

#ifndef _RTLD_EPILOGUE
# define _RTLD_EPILOGUE(entry)                                          \
        ".size\t" __STRING (entry) ", . - " __STRING (entry) "\n\t"
#endif

#define STRINGXP(X) __STRING (X)  

__asm__(\
	".text\n\
        " _RTLD_PROLOGUE (_start) "\
        mv a0, sp\n\
        jal _dl_start\n\
        # Stash user entry point in s0.\n\
        mv s0, a0\n\
        # See if we were run as a command with the executable file\n\
        # name as an extra leading argument.\n\
        lw a0, _dl_skip_args\n\
        # Load the original argument count.\n\
        " STRINGXP (REG_L) " a1, 0(sp)\n\
        # Subtract _dl_skip_args from it.\n\
        sub a1, a1, a0\n\
        # Adjust the stack pointer to skip _dl_skip_args words.\n\
        sll a0, a0, " STRINGXP (PTRLOG) "\n\
        add sp, sp, a0\n\
        # Save back the modified argument count.\n\
        " STRINGXP (REG_S) " a1, 0(sp)\n\
        # Pass our finalizer function to _start.\n\
        lla a0, _dl_fini\n\
        # Jump to the user entry point.\n\
        jr s0\n\
        " _RTLD_EPILOGUE (_start) "\
        .previous" \
);

/* Get a pointer to the argv array.  On many platforms this can be just
 * the address of the first argument, on other platforms we need to
 * do something a little more subtle here.  */
#define GET_ARGV(ARGVP, ARGS) ARGVP = (((unsigned long*)ARGS)+1)

/* Function calls are not safe until the GOT relocations have been done.  */
#define NO_FUNCS_BEFORE_BOOTSTRAP

/* Handle relocation of the symbols in the dynamic loader. */
static __always_inline
void PERFORM_BOOTSTRAP_RELOC(ELF_RELOC *rpnt, ElfW(Addr) *reloc_addr,
	ElfW(Addr) symbol_addr, ElfW(Addr) load_addr, ElfW(Addr) *sym)
{
	switch (ELF_R_TYPE(rpnt->r_info)) {
		case R_RISCV_NONE:
			break;
		case R_RISCV_JUMP_SLOT:
			*reloc_addr = symbol_addr + rpnt->r_addend;
			break;
		default:
			_dl_exit(1);
	}
}
