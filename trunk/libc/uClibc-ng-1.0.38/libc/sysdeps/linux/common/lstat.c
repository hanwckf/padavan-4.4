/*
 * lstat() for uClibc
 *
 * Copyright (C) 2000-2006 Erik Andersen <andersen@uclibc.org>
 *
 * Licensed under the LGPL v2.1, see the file COPYING.LIB in this tarball.
 */

#include <sys/syscall.h>
#include <unistd.h>
#include <sys/stat.h>

#if defined __NR_fstatat64 && !defined __NR_lstat
# include <fcntl.h>

int lstat(const char *file_name, struct stat *buf)
{
	return fstatat(AT_FDCWD, file_name, buf, AT_SYMLINK_NOFOLLOW);
}
libc_hidden_def(lstat)

#elif __WORDSIZE == 64 && defined __NR_newfstatat
# include <fcntl.h>

int lstat(const char *file_name, struct stat *buf)
{
	return fstatat(AT_FDCWD, file_name, buf, AT_SYMLINK_NOFOLLOW);
}
libc_hidden_def(lstat)

#elif defined __NR_statx && defined __UCLIBC_HAVE_STATX__
# include <fcntl.h>
# include <statx_cp.h>

int lstat(const char *file_name, struct stat *buf)
{
      struct statx tmp;
      int rc = INLINE_SYSCALL (statx, 5, AT_FDCWD, file_name,
                               AT_NO_AUTOMOUNT | AT_SYMLINK_NOFOLLOW,
                               STATX_BASIC_STATS, &tmp);
      if (rc == 0)
        __cp_stat_statx ((struct stat *)buf, &tmp);

      return rc;
}
libc_hidden_def(lstat)

/* For systems which have both, prefer the old one */
#else
# include "xstatconv.h"
int lstat(const char *file_name, struct stat *buf)
{
	int result;
# ifdef __NR_lstat64
	/* normal stat call has limited values for various stat elements
	 * e.g. uid device major/minor etc.
	 * so we use 64 variant if available
	 * in order to get newer versions of stat elements
	 */
	struct kernel_stat64 kbuf;
	result = INLINE_SYSCALL(lstat64, 2, file_name, &kbuf);
	if (result == 0) {
		__xstat32_conv(&kbuf, buf);
	}
# else
	struct kernel_stat kbuf;

	result = INLINE_SYSCALL(lstat, 2, file_name, &kbuf);
	if (result == 0) {
		__xstat_conv(&kbuf, buf);
	}
# endif /* __NR_lstat64 */
	return result;
}
libc_hidden_def(lstat)

# if ! defined __NR_fstatat64 && ! defined __NR_lstat64 && ! defined __UCLIBC_HAS_STATX__
strong_alias_untyped(lstat,lstat64)
libc_hidden_def(lstat64)
# endif

#endif /* __NR_fstatat64 */
