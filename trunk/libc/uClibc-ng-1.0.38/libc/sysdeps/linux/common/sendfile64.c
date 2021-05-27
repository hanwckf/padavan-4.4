/*
 * Copyright (C) 2000-2006 Erik Andersen <andersen@uclibc.org>
 *
 * Licensed under the LGPL v2.1, see the file COPYING.LIB in this tarball.
 */
/* sendfile64 syscall.  Copes with 64 bit and 32 bit machines
 * and on 32 bit machines this sends things into the kernel as
 * two 32-bit arguments (high and low 32 bits of length) that
 * are ordered based on endianess.  It turns out endian.h has
 * just the macro we need to order things, __LONG_LONG_PAIR.
 */

#include <_lfs_64.h>
#include <sys/syscall.h>
#include <bits/wordsize.h>

#if defined __NR_sendfile64 && __WORDSIZE != 64
# include <sys/sendfile.h>
_syscall4(ssize_t,sendfile64, int, out_fd, int, in_fd, __off64_t *, offset, size_t, count)
libc_hidden_def(sendfile64)
#endif
