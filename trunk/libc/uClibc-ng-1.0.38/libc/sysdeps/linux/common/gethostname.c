/*
 * Copyright (C) 2000-2006 Erik Andersen <andersen@uclibc.org>
 *
 * Licensed under the LGPL v2.1, see the file COPYING.LIB in this tarball.
 */

#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <errno.h>



int
gethostname(char *name, size_t len)
{
  struct utsname uts;

  if (name == NULL) {
    __set_errno(EINVAL);
    return -1;
  }

  if (uname(&uts) == -1) return -1;

  if (strlen(uts.nodename)+1 > len) {
    __set_errno(EINVAL);
    return -1;
  }
  strcpy(name, uts.nodename);
  return 0;
}
libc_hidden_def(gethostname)
