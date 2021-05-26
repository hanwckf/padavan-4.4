/*
 * renameat() for uClibc
 *
 * Copyright (C) 2009 Analog Devices Inc.
 *
 * Licensed under the LGPL v2.1, see the file COPYING.LIB in this tarball.
 */

#include <stdio.h>
#include <fcntl.h>
#include <sysdep.h>
#include <errno.h>

int
renameat (int oldfd, const char *old, int newfd, const char *new)
{
#ifdef __NR_renameat
  return INLINE_SYSCALL (renameat, 4, oldfd, old, newfd, new);
#else
  return INLINE_SYSCALL (renameat2, 5, oldfd, old, newfd, new, 0);
#endif
}
libc_hidden_def (renameat)
