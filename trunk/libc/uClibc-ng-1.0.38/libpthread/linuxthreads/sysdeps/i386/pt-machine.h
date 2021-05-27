/* Machine-dependent pthreads configuration and inline functions.
   i386 version.
   Copyright (C) 1996-2001, 2002, 2003 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Richard Henderson <rth@tamu.edu>.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation; either version 2.1 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If
   not, see <http://www.gnu.org/licenses/>.  */

#ifndef _PT_MACHINE_H
#define _PT_MACHINE_H	1

#include <features.h>

#ifndef __ASSEMBLER__
#ifndef PT_EI
# define PT_EI __extern_always_inline __attribute__((visibility("hidden")))
#endif

extern long int testandset (int *spinlock);
extern int __compare_and_swap (long int *p, long int oldval, long int newval);

/* Get some notion of the current stack.  Need not be exactly the top
   of the stack, just something somewhere in the current frame.  */
#define CURRENT_STACK_FRAME  __builtin_frame_address (0)


/* See if we can optimize for newer cpus... */
#if defined __GNUC__ && __GNUC__ >= 2 && \
   (defined __i486__ || defined __pentium__ || defined __pentiumpro__ || defined __pentium4__ || \
    defined __athlon__ || defined __k8__)

/* Spinlock implementation; required.  */
PT_EI long int
testandset (int *spinlock)
{
  long int ret;

  __asm__ __volatile__ (
	"xchgl %0, %1"
	: "=r" (ret), "=m" (*spinlock)
	: "0" (1), "m" (*spinlock)
	: "memory");

  return ret;
}

/* Compare-and-swap for semaphores.  It's always available on i686.  */
#define HAS_COMPARE_AND_SWAP

PT_EI int
__compare_and_swap (long int *p, long int oldval, long int newval)
{
  char ret;
  long int readval;

  __asm__ __volatile__ ("lock; cmpxchgl %3, %1; sete %0"
			: "=q" (ret), "=m" (*p), "=a" (readval)
			: "r" (newval), "m" (*p), "a" (oldval)
			: "memory");
  return ret;
}

#if defined(__ASSUME_LDT_WORKS) && __ASSUME_LDT_WORKS > 0
#include "useldt.h"
#endif

/* The P4 and above really want some help to prevent overheating.  */
#define BUSY_WAIT_NOP	__asm__ ("rep; nop")


#else /* Generic i386 implementation */

extern int compare_and_swap_is_available (void);

/* Spinlock implementation; required.  */
PT_EI long int
testandset (int *spinlock)
{
  long int ret;

  __asm__ __volatile__(
       "xchgl %0, %1"
       : "=r"(ret), "=m"(*spinlock)
       : "0"(1), "m"(*spinlock)
       : "memory");

  return ret;
}


/* Compare-and-swap for semaphores.
   Available on the 486 and above, but not on the 386.
   We test dynamically whether it's available or not. */

#define HAS_COMPARE_AND_SWAP
#define TEST_FOR_COMPARE_AND_SWAP

PT_EI int
__compare_and_swap (long int *p, long int oldval, long int newval)
{
  char ret;
  long int readval;

  __asm__ __volatile__ ("lock; cmpxchgl %3, %1; sete %0"
			: "=q" (ret), "=m" (*p), "=a" (readval)
			: "r" (newval), "m" (*p), "a" (oldval)
			: "memory");
  return ret;
}

PT_EI int
compare_and_swap_is_available (void)
{
  int changed;
  int oldflags;
  /* get EFLAGS */
  __asm__ __volatile__ ("pushfl; popl %0" : "=r" (oldflags) : );
  /* Flip AC bit in EFLAGS.  */
  __asm__ __volatile__ ("pushl %0; popfl" : : "r" (oldflags ^ 0x40000) : "cc");
  /* reread EFLAGS */
  __asm__ __volatile__ ("pushfl; popl %0" : "=r" (changed) : );
  /* See if bit changed.  */
  changed = (changed ^ oldflags) & 0x40000;
  /* Restore EFLAGS.  */
  __asm__ __volatile__ ("pushl %0; popfl" : : "r" (oldflags) : "cc");
  /* If the AC flag did not change, it's a 386 and it lacks cmpxchg.
     Otherwise, it's a 486 or above and it has cmpxchg.  */
  return changed != 0;
}
#endif /* Generic i386 implementation */

#endif /* __ASSEMBLER__ */

#endif /* pt-machine.h */
