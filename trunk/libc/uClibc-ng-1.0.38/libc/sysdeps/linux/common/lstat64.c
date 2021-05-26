/*
 * lstat64() for uClibc
 *
 * Copyright (C) 2000-2006 Erik Andersen <andersen@uclibc.org>
 *
 * Licensed under the LGPL v2.1, see the file COPYING.LIB in this tarball.
 */

#include <_lfs_64.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/stat.h>

#if defined __NR_fstatat64 && !defined __NR_lstat
# include <fcntl.h>

int lstat64(const char *file_name, struct stat64 *buf)
{
	return fstatat64(AT_FDCWD, file_name, buf, AT_SYMLINK_NOFOLLOW);
}
libc_hidden_def(lstat64)

#elif __WORDSIZE == 64 && defined __NR_newfstatat
# include <fcntl.h>

int lstat64(const char *file_name, struct stat64 *buf)
{
	return fstatat64(AT_FDCWD, file_name, buf, AT_SYMLINK_NOFOLLOW);
}
libc_hidden_def(lstat64)

#elif defined __NR_statx && defined __UCLIBC_HAVE_STATX__
# include <fcntl.h>
# include <statx_cp.h>

int lstat64(const char *file_name, struct stat64 *buf)
{
      struct statx tmp;
      int rc = INLINE_SYSCALL (statx, 5, AT_FDCWD, file_name,
                               AT_NO_AUTOMOUNT | AT_SYMLINK_NOFOLLOW,
                               STATX_BASIC_STATS, &tmp);
      if (rc == 0)
        __cp_stat64_statx ((struct stat64 *)buf, &tmp);

      return rc;
}
libc_hidden_def(lstat64)

/* For systems which have both, prefer the old one */
#elif defined __NR_lstat64
# include "xstatconv.h"
# define __NR___syscall_lstat64 __NR_lstat64
static __always_inline _syscall2(int, __syscall_lstat64, const char *, file_name,
				 struct kernel_stat64 *, buf)

int lstat64(const char *file_name, struct stat64 *buf)
{
	int result;
	struct kernel_stat64 kbuf;

	result = __syscall_lstat64(file_name, &kbuf);
	if (result == 0) {
		__xstat64_conv(&kbuf, buf);
	}
	return result;
}
libc_hidden_def(lstat64)
#endif
