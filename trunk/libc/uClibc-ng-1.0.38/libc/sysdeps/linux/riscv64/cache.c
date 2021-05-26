/* RISC-V instruction cache flushing VDSO calls
   Copyright (C) 2017-2020 Free Software Foundation, Inc.

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

#include <stdlib.h>
#include <atomic.h>
#include <sys/syscall.h>

typedef int (*func_type) (void *, void *, unsigned long int);

static int
__riscv_flush_icache_syscall (void *start, void *end, unsigned long int flags)
{
  return INLINE_SYSCALL (riscv_flush_icache, 3, start, end, flags);
}

static func_type
__lookup_riscv_flush_icache (void)
{
  /* always call the system call directly.*/
  return &__riscv_flush_icache_syscall;
}

int
__riscv_flush_icache (void *start, void *end, unsigned long int flags)
{
  static volatile func_type cached_func;

  func_type func = atomic_load_relaxed (&cached_func);

  if (!func)
    {
      func = __lookup_riscv_flush_icache ();
      atomic_store_relaxed (&cached_func, func);
    }

  return func (start, end, flags);
}
