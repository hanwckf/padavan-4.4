/* Copyright (C) 1991, 1994, 1996, 1997, 2003 Free Software Foundation, Inc.
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

#include <string.h>


/* Return the length of the maximum initial segment of S
   which contains no characters from REJECT.  */
size_t strcspn (const char *s, const char *reject)
{
  size_t count = 0;

  while (*s != '\0')
    if (strchr (reject, *s++) == NULL)
      ++count;
    else
      return count;

  return count;
}
libc_hidden_def(strcspn)
