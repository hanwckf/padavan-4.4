/*
 * Various assembly language/system dependent hacks that are required
 * so that we can minimize the amount of platform specific code.
 * Copyright (C) 2000-2004 by Erik Andersen <andersen@codepoet.org>
 * Copyright (C) 2019 by Waldemar Brodkorb <wbx@uclibc-ng.org>
 * Ported from GNU C Library
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

/* Define this if the system uses RELOCA.  */
#define ELF_USES_RELOCA

#include <elf.h>
#include <link.h>

/* Initialization sequence for the GOT.  */
#define INIT_GOT(GOT_BASE,MODULE) \
{				\
  GOT_BASE[2] = (unsigned long) _dl_linux_resolve; \
  GOT_BASE[1] = (unsigned long) MODULE; \
}

/* Here we define the magic numbers that this dynamic loader should accept */
#define MAGIC1 EM_RISCV
#undef  MAGIC2

/* Used for error messages */
#define ELF_TARGET "RISC-V"

struct elf_resolve;
unsigned long _dl_linux_resolver(struct elf_resolve * tpnt, int reloc_entry);

#define ELF_MACHINE_JMP_SLOT R_RISCV_JUMP_SLOT

#define elf_machine_type_class(type)                            \
  ((ELF_RTYPE_CLASS_PLT * ((type) == ELF_MACHINE_JMP_SLOT       \
     || (__WORDSIZE == 32 && (type) == R_RISCV_TLS_DTPREL32)    \
     || (__WORDSIZE == 32 && (type) == R_RISCV_TLS_DTPMOD32)    \
     || (__WORDSIZE == 32 && (type) == R_RISCV_TLS_TPREL32)     \
     || (__WORDSIZE == 64 && (type) == R_RISCV_TLS_DTPREL64)    \
     || (__WORDSIZE == 64 && (type) == R_RISCV_TLS_DTPMOD64)    \
     || (__WORDSIZE == 64 && (type) == R_RISCV_TLS_TPREL64)))   \
   | (ELF_RTYPE_CLASS_COPY * ((type) == R_RISCV_COPY)))


/* Return the link-time address of _DYNAMIC.  */
static inline ElfW(Addr)
elf_machine_dynamic (void)
{
  extern ElfW(Addr) _GLOBAL_OFFSET_TABLE_ __attribute__ ((visibility ("hidden")));
  return _GLOBAL_OFFSET_TABLE_;
}


/* Return the run-time load address of the shared object.  */
static __always_inline ElfW(Addr) __attribute__ ((unused))
elf_machine_load_address (void)
{
  ElfW(Addr) load_addr;
  __asm__ ("lla %0, _DYNAMIC" : "=r" (load_addr));
  return load_addr - elf_machine_dynamic ();
}

static __always_inline void
elf_machine_relative(Elf64_Addr load_off, const Elf64_Addr rel_addr,
                     Elf64_Word relative_count)
{
	Elf64_Rela *rpnt = (Elf64_Rela*)rel_addr;
	--rpnt;
	do {
		Elf64_Addr *const reloc_addr = (Elf64_Addr*)(load_off + (++rpnt)->r_offset);

		*reloc_addr = load_off + rpnt->r_addend;
	} while (--relative_count);
}
