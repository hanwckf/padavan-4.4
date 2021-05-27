/*
 * libc/sysdeps/linux/microblaze/bits/endian.h -- Define processor endianess
 *
 *  Copyright (C) 2003  John Williams <jwilliams@itee.uq.edu.au>
 *  Copyright (C) 2001  NEC Corporation
 *  Copyright (C) 2001  Miles Bader <miles@gnu.org>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License.  See the file COPYING.LIB in the main
 * directory of this archive for more details.
 *
 * Written by Miles Bader <miles@gnu.org>
 * Microblaze port by John Williams
 */

#ifndef _ENDIAN_H
# error "Never use <bits/endian.h> directly; include <endian.h> instead."
#endif

/* Note: Toolchain supplies _BIG_ENDIAN or _LITTLE_ENDIAN */
#if defined(_BIG_ENDIAN)
# define __BYTE_ORDER __BIG_ENDIAN
#elif defined(_LITTLE_ENDIAN)
# define __BYTE_ORDER __LITTLE_ENDIAN
#else
# error "Endianness is unknown"
#endif
