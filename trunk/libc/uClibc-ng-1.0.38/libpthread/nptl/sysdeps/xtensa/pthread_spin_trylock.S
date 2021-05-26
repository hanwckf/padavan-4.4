/* Copyright (C) 2005 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If
   not, see <http://www.gnu.org/licenses/>.  */

#define _ERRNO_H 1
#include <bits/errno.h>
#include <bits/xtensa-config.h>
#include <sysdep.h>

	.text
ENTRY (pthread_spin_trylock)

#if XCHAL_HAVE_EXCLUSIVE
	memw
	l32ex	a3, a2
	bnez	a3, 1f
	movi	a3, 1
	s32ex	a3, a2
	getex	a3
	addi	a3, a3, -1
	memw
1:
#elif XCHAL_HAVE_S32C1I
	movi	a3, 0
	wsr 	a3, scompare1
	movi	a3, 1
	s32c1i	a3, a2, 0
#else

#error No hardware atomic operations

#endif
	movi	a2, EBUSY
	moveqz	a2, a3, a3

	abi_ret

END (pthread_spin_trylock)
