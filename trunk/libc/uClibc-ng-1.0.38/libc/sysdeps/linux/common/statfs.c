/*
 * statfs() for uClibc
 *
 * Copyright (C) 2000-2006 Erik Andersen <andersen@uclibc.org>
 *
 * Licensed under the LGPL v2.1, see the file COPYING.LIB in this tarball.
 */

#include <sys/syscall.h>
#include <string.h>
#include <sys/param.h>
#include <sys/vfs.h>

extern __typeof(statfs) __libc_statfs attribute_hidden;

#if defined __NR_statfs64 && !defined __NR_statfs

int __libc_statfs(const char *path, struct statfs *buf)
{
	int err = INLINE_SYSCALL(statfs64, 3, path, sizeof(*buf), buf);
	return err;
}
# if defined __UCLIBC_LINUX_SPECIFIC__ || defined __UCLIBC_HAS_THREADS_NATIVE__
/* statfs is used by NPTL, so it must exported in case */
weak_alias(__libc_statfs, statfs)
libc_hidden_def(statfs)
# endif

/* For systems which have both, prefer the old one */
#else

# define __NR___libc_statfs __NR_statfs
_syscall2(int, __libc_statfs, const char *, path, struct statfs *, buf)

# if defined __UCLIBC_LINUX_SPECIFIC__ || defined __UCLIBC_HAS_THREADS_NATIVE__
/* statfs is used by NPTL, so it must exported in case */
weak_alias(__libc_statfs, statfs)
libc_hidden_def(statfs)
# endif

#endif
