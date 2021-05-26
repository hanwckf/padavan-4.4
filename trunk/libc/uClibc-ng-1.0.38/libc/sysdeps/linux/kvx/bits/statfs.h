/* Copyright (C) 1997, 2000 Free Software Foundation, Inc.
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
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#ifndef _SYS_STATFS_H
# error "Never include <bits/statfs.h> directly; use <sys/statfs.h> instead."
#endif

#include <bits/types.h>  /* for __fsid_t and __fsblkcnt_t*/

struct statfs
  {
    long int f_type;
    long int f_bsize;
    __fsblkcnt64_t f_blocks;
    __fsblkcnt64_t f_bfree;
    __fsblkcnt64_t f_bavail;
    __fsfilcnt64_t f_files;
    __fsfilcnt64_t f_ffree;

	/* Linux specials */
    __fsid_t f_fsid;
    long int f_namelen;
    long int f_frsize;
    long int f_flags;
    long int f_spare[4];
  };

#ifdef __USE_LARGEFILE64
struct statfs64
  {
    long int f_type;
    long int f_bsize;
    __fsblkcnt64_t f_blocks;
    __fsblkcnt64_t f_bfree;
    __fsblkcnt64_t f_files;
    __fsfilcnt64_t f_ffree;
    __fsfilcnt64_t f_bavail;

	/* Linux specials */
    __fsid_t f_fsid;
    long int f_namelen;
    long int f_frsize;
    long int f_flags;
    long int f_spare[4];
  };
#endif

/* Tell code we have these members.  */
#define _STATFS_F_NAMELEN
#define _STATFS_F_FRSIZE
