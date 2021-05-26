/*
 * fstat64() for uClibc
 *
 * Copyright (C) 2000-2006 Erik Andersen <andersen@uclibc.org>
 *
 * Licensed under the LGPL v2.1, see the file COPYING.LIB in this tarball.
 */

#include <_lfs_64.h>
#include <sys/syscall.h>

#ifdef __NR_fstat64
# include <unistd.h>
# include <sys/stat.h>
# include "xstatconv.h"
# define __NR___syscall_fstat64 __NR_fstat64
static __always_inline _syscall2(int, __syscall_fstat64,
				 int, filedes, struct kernel_stat64 *, buf)

int fstat64(int fd, struct stat64 *buf)
{
#ifdef __ARCH_HAS_DEPRECATED_SYSCALLS__
	int result;
	struct kernel_stat64 kbuf;

	result = __syscall_fstat64(fd, &kbuf);
	if (result == 0) {
		__xstat64_conv(&kbuf, buf);
	}
	return result;

#else
	return __syscall_fstat64(fd, buf);
#endif
}
libc_hidden_def(fstat64)

#elif __NR_statx && defined __UCLIBC_HAVE_STATX__
# include <fcntl.h>
# include <statx_cp.h>

int fstat64(int fd, struct stat64 *buf)
{
      struct statx tmp;
      int rc = INLINE_SYSCALL (statx, 5, fd, "", AT_EMPTY_PATH,
                               STATX_BASIC_STATS, &tmp);
      if (rc == 0)
        __cp_stat_statx ((struct stat64 *)buf, &tmp);

      return rc;
}
libc_hidden_def(fstat64)
#endif
