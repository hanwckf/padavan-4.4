/* Get a thread's floating-point register set.
   Copyright (C) 1999, 2001, 2003 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1999.

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

#include "thread_dbP.h"


td_err_e
td_thr_getfpregs (const td_thrhandle_t *th, prfpregset_t *regset)
{
  struct _pthread_descr_struct pds;

  LOG ("td_thr_getfpregs");

  if (th->th_unique == NULL)
    {
      /* No data yet.  Use the main thread.  */
      pid_t pid = ps_getpid (th->th_ta_p->ph);
      if (ps_lgetfpregs (th->th_ta_p->ph, pid, regset) != PS_OK)
	return TD_ERR;
      return TD_OK;
    }

  /* We have to get the state and the PID for this thread.  */
  if (ps_pdread (th->th_ta_p->ph, th->th_unique, &pds,
		 sizeof (struct _pthread_descr_struct)) != PS_OK)
    return TD_ERR;

  /* If the thread already terminated we return all zeroes.  */
  if (pds.p_terminated)
    memset (regset, '\0', sizeof (*regset));
  /* Otherwise get the register content through the callback.  */
  else
    {
      pid_t pid = pds.p_pid ?: ps_getpid (th->th_ta_p->ph);

      if (ps_lgetfpregs (th->th_ta_p->ph, pid, regset) != PS_OK)
	return TD_ERR;
    }

  return TD_OK;
}
