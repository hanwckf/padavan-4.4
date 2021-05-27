/* Definitions for BSD-style memory management.
   Copyright (C) 1994-2000, 2003, 2004, 2005 Free Software Foundation, Inc.
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

#ifndef	_SYS_MMAN_H
#define	_SYS_MMAN_H	1

#include <features.h>
#include <bits/types.h>
#define __need_size_t
#include <stddef.h>

#ifndef __off_t_defined
# ifndef __USE_FILE_OFFSET64
typedef __off_t off_t;
# else
typedef __off64_t off_t;
# endif
# define __off_t_defined
#endif

#ifndef __mode_t_defined
typedef __mode_t mode_t;
# define __mode_t_defined
#endif

#include <bits/mman.h>

/* Return value of `mmap' in case of an error.  */
#define MAP_FAILED	((void *) -1)

__BEGIN_DECLS
/* Map addresses starting near ADDR and extending for LEN bytes.  from
   OFFSET into the file FD describes according to PROT and FLAGS.  If ADDR
   is nonzero, it is the desired mapping address.  If the MAP_FIXED bit is
   set in FLAGS, the mapping will be at ADDR exactly (which must be
   page-aligned); otherwise the system chooses a convenient nearby address.
   The return value is the actual mapping address chosen or MAP_FAILED
   for errors (in which case `errno' is set).  A successful `mmap' call
   deallocates any previous mapping for the affected region.  */

#ifndef __USE_FILE_OFFSET64
extern void *mmap (void *__addr, size_t __len, int __prot,
		   int __flags, int __fd, __off_t __offset) __THROW;
libc_hidden_proto(mmap)
#else
# ifdef __REDIRECT_NTH
extern void * __REDIRECT_NTH (mmap,
			      (void *__addr, size_t __len, int __prot,
			       int __flags, int __fd, __off64_t __offset),
			      mmap64);
# else
#  define mmap mmap64
# endif
#endif
#ifdef __USE_LARGEFILE64
extern void *mmap64 (void *__addr, size_t __len, int __prot,
		     int __flags, int __fd, __off64_t __offset) __THROW;
#endif

/* Deallocate any mapping for the region starting at ADDR and extending LEN
   bytes.  Returns 0 if successful, -1 for errors (and sets errno).  */
extern int munmap (void *__addr, size_t __len) __THROW;
libc_hidden_proto(munmap)

/* Change the memory protection of the region starting at ADDR and
   extending LEN bytes to PROT.  Returns 0 if successful, -1 for errors
   (and sets errno).  */
extern int mprotect (void *__addr, size_t __len, int __prot) __THROW;

#ifdef __ARCH_USE_MMU__

/* Synchronize the region starting at ADDR and extending LEN bytes with the
   file it maps.  Filesystem operations on a file being mapped are
   unpredictable before this is done.  Flags are from the MS_* set.

   This function is a cancellation point and therefore not marked with
   __THROW.  */
extern int msync (void *__addr, size_t __len, int __flags);

#else

/* On no-mmu systems you can't have real private mappings.  */
static __inline__ int msync (void *__addr, size_t __len, int __flags) { return 0; }

#endif

#ifdef __ARCH_USE_MMU__
#if defined __USE_BSD && (defined __UCLIBC_LINUX_SPECIFIC__ || defined __UCLIBC_HAS_THREADS_NATIVE__)
/* Advise the system about particular usage patterns the program follows
   for the region starting at ADDR and extending LEN bytes.  */
extern int madvise (void *__addr, size_t __len, int __advice) __THROW;
#endif
#if defined __USE_XOPEN2K && defined __UCLIBC_HAS_ADVANCED_REALTIME__
/* This is the POSIX name for this function.  */
extern int posix_madvise (void *__addr, size_t __len, int __advice) __THROW;
#endif
#endif /* __ARCH_USE_MMU__ */


#if defined __UCLIBC_HAS_REALTIME__
# ifdef __ARCH_USE_MMU__

/* Guarantee all whole pages mapped by the range [ADDR,ADDR+LEN) to
   be memory resident.  */
extern int mlock (const void *__addr, size_t __len) __THROW;

/* Unlock whole pages previously mapped by the range [ADDR,ADDR+LEN).  */
extern int munlock (const void *__addr, size_t __len) __THROW;

/* Cause all currently mapped pages of the process to be memory resident
   until unlocked by a call to the `munlockall', until the process exits,
   or until the process calls `execve'.  */
extern int mlockall (int __flags) __THROW;

/* All currently mapped pages of the process' address space become
   unlocked.  */
extern int munlockall (void) __THROW;

#else

/* On no-mmu systems, memory cannot be swapped out, so
 * these functions will always succeed.  */
static __inline__ int mlock (const void *__addr, size_t __len) { return 0; }
static __inline__ int munlock (const void *__addr, size_t __len) { return 0; }
static __inline__ int mlockall (int __flags) { return 0; }
static __inline__ int munlockall (void) { return 0; }
#endif
#endif /* __UCLIBC_HAS_REALTIME__ */

#if defined __USE_MISC && defined __UCLIBC_BSD_SPECIFIC__
/* mincore returns the memory residency status of the pages in the
   current process's address space specified by [start, start + len).
   The status is returned in a vector of bytes.  The least significant
   bit of each byte is 1 if the referenced page is in memory, otherwise
   it is zero.  */
extern int mincore (void *__start, size_t __len, unsigned char *__vec)
     __THROW;
#endif

#ifdef __USE_GNU
/* Remap pages mapped by the range [ADDR,ADDR+OLD_LEN) to new length
   NEW_LEN.  If MREMAP_MAYMOVE is set in FLAGS the returned address
   may differ from ADDR.  If MREMAP_FIXED is set in FLAGS the function
   takes another paramter which is a fixed address at which the block
   resides after a successful call.  */
extern void *mremap (void *__addr, size_t __old_len, size_t __new_len,
		     int __flags, ...) __THROW;
libc_hidden_proto(mremap)

#ifdef __UCLIBC_LINUX_SPECIFIC__
/* Remap arbitrary pages of a shared backing store within an existing
   VMA.  */
extern int remap_file_pages (void *__start, size_t __size, int __prot,
			     size_t __pgoff, int __flags) __THROW;
#endif
#endif


/* Open shared memory segment.  */
extern int shm_open (const char *__name, int __oflag, mode_t __mode);

/* Remove shared memory segment.  */
extern int shm_unlink (const char *__name);

__END_DECLS

#endif	/* sys/mman.h */
