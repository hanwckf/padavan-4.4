/* Copyright (C) 2010-2012 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Maxim Kuvyrkov <maxim@codesourcery.com>, 2010.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library.  If not, see
   <http://www.gnu.org/licenses/>.  */

#ifndef _KVX_BITS_ATOMIC_H
#define _KVX_BITS_ATOMIC_H

#include <stdint.h>

typedef int8_t atomic8_t;
typedef uint8_t uatomic8_t;
typedef int_fast8_t atomic_fast8_t;
typedef uint_fast8_t uatomic_fast8_t;

typedef int16_t atomic16_t;
typedef uint16_t uatomic16_t;
typedef int_fast16_t atomic_fast16_t;
typedef uint_fast16_t uatomic_fast16_t;

typedef int32_t atomic32_t;
typedef uint32_t uatomic32_t;
typedef int_fast32_t atomic_fast32_t;
typedef uint_fast32_t uatomic_fast32_t;

typedef int64_t atomic64_t;
typedef uint64_t uatomic64_t;
typedef int_fast64_t atomic_fast64_t;
typedef uint_fast64_t uatomic_fast64_t;

typedef intptr_t atomicptr_t;
typedef uintptr_t uatomicptr_t;
typedef intmax_t atomic_max_t;
typedef uintmax_t uatomic_max_t;


#ifndef atomic_full_barrier
# define atomic_full_barrier() do { atomic_read_barrier();		\
					atomic_write_barrier(); } while(0)
#endif

#ifndef atomic_read_barrier
# define atomic_read_barrier() __builtin_kvx_dinval()
#endif

#ifndef atomic_write_barrier
# define atomic_write_barrier() __builtin_kvx_fence()
#endif

/*
 * On kvx, we have a boolean compare and swap which means that the operation
 * returns only the success of operation.
 * If operation succeeds, this is simple, we just need to return the provided
 * old value. However, if it fails, we need to load the value to return it for
 * the caller. If the loaded value is different from the "old" provided by the
 * caller, we can return it since it will mean it failed.
 * However, if for some reason the value we read is equal to the old value
 * provided by the caller, we can't simply return it or the caller will think it
 * succeeded. So if the value we read is the same as the "old" provided by
 * the caller, we try again until either we succeed or we fail with a different
 * value than the provided one.
 */
#define __cmpxchg(ptr, old, new, op_suffix, load_suffix)		\
({									\
	register unsigned long __rn __asm__("r62");			\
	register unsigned long __ro __asm__("r63");			\
	__asm__ __volatile__ (						\
		/* Fence to guarantee previous store to be committed */	\
		"fence\n"						\
		/* Init "expect" with previous value */			\
		"copyd $r63 = %[rOld]\n"				\
		";;\n"							\
		"1:\n"							\
		/* Init "update" value with new */			\
		"copyd $r62 = %[rNew]\n"				\
		";;\n"							\
		"acswap" #op_suffix " 0[%[rPtr]], $r62r63\n"		\
		";;\n"							\
		/* if acswap succeeds, simply return */			\
		"cb.dnez $r62? 2f\n"					\
		";;\n"							\
		/* We failed, load old value */				\
		"l"  #op_suffix  #load_suffix" $r63 = 0[%[rPtr]]\n"	\
		";;\n"							\
		/* Check if equal to "old" one */			\
		"compd.ne $r62 = $r63, %[rOld]\n"			\
		";;\n"							\
		/* If different from "old", return it to caller */	\
		"cb.deqz $r62? 1b\n"					\
		";;\n"							\
		"2:\n"							\
		: "+r" (__rn), "+r" (__ro)				\
		: [rPtr] "r" (ptr), [rOld] "r" (old), [rNew] "r" (new)	\
		: "memory");						\
	(__ro);								\
})

#define cmpxchg(ptr, o, n)						\
({									\
	unsigned long __cmpxchg__ret;					\
	switch (sizeof(*(ptr))) {					\
	case 4:								\
		__cmpxchg__ret = __cmpxchg((ptr), (o), (n), w, s);	\
		break;							\
	case 8:								\
		__cmpxchg__ret = __cmpxchg((ptr), (o), (n), d, );	\
		break;							\
	}								\
	(__typeof(*(ptr))) (__cmpxchg__ret);				\
})

#define atomic_compare_and_exchange_val_acq(mem, newval, oldval)	\
	cmpxchg((mem), (oldval), (newval))


#define atomic_exchange_acq(mem, newval)				\
({									\
	unsigned long __aea__ret, __aea__old;					\
	volatile __typeof((mem)) __aea__m = (mem);				\
	do {								\
		__aea__old = *__aea__m;						\
		__aea__ret = atomic_compare_and_exchange_val_acq((mem),	\
						(newval), (__aea__old));\
	} while (__aea__old != __aea__ret);					\
	(__aea__old);							\
})

#endif
