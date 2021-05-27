/* Store current floating-point environment.
   Copyright (C) 2013 Imagination Technologies Ltd.
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
   License along with the GNU C Library; see the file COPYING.LIB.  If
   not, see <http://www.gnu.org/licenses/>.  */

#include <fenv.h>

int
fegetenv (fenv_t *envp)
{
  unsigned int txdefr;
  unsigned int txmode;

  __asm__ ("MOV %0,TXDEFR" : "=r" (txdefr));
  __asm__ ("MOV %0,TXMODE" : "=r" (txmode));

  envp->txdefr = txdefr;
  envp->txmode = txmode;

  /* Success.  */
  return 0;
}
