/* Copyright (C) 2003 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Jakub Jelinek <jakub@redhat.com>, 2003.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#include "pthreadP.h"

int
pthread_spin_lock (pthread_spinlock_t *lock)
{
  __asm__ __volatile__
    ("1: ldstub [%0], %%g2\n"
     "   orcc   %%g2, 0x0, %%g0\n"
     "   bne,a  2f\n"
     "   ldub   [%0], %%g2\n"
     ".subsection 2\n"
     "2: orcc   %%g2, 0x0, %%g0\n"
     "   bne,a  2b\n"
     "   ldub   [%0], %%g2\n"
     "   b,a    1b\n"
     ".previous"
     : /* no outputs */
     : "r" (lock)
     : "g2", "memory", "cc");
  return 0;
}
