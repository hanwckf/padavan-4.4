/* Copyright (C) 2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@redhat.com>, 2002.

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

#include <string.h>
#include <pthreadP.h>


int
__pthread_mutexattr_init (
     pthread_mutexattr_t *attr)
{
  if (sizeof (struct pthread_mutexattr) != sizeof (pthread_mutexattr_t))
    memset (attr, '\0', sizeof (*attr));

  /* We use bit 31 to signal whether the mutex is going to be
     process-shared or not.  By default it is zero, i.e., the mutex is
     not process-shared.  */
  ((struct pthread_mutexattr *) attr)->mutexkind = PTHREAD_MUTEX_NORMAL;

  return 0;
}
strong_alias (__pthread_mutexattr_init, pthread_mutexattr_init)
