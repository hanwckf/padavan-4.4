/*
  (C) Copyright 2019 Kalray S.A.
  This file provides fesetexceptflag for the Coolidge processor.
*/

#include <fenv.h>

int fesetexceptflag(const fexcept_t *flagp, int excepts)
{
  /* Mask excepts to be sure only supported flag bits are set */
  excepts &= FE_ALL_EXCEPT;

  /* Set the requested flags */
  fexcept_t flags = (*flagp & excepts);

  /* Set $cs with 'flags' as a set mask and FE_ALL_EXCEPT as a clear
     mask. */
  __builtin_kvx_wfxl(KVX_SFR_CS, (long long)flags << 32 | FE_ALL_EXCEPT);

  /* The above insn cannot fail (while the OS allows access to the
     floating-point exception flags of the $cs register). Return
     success. */
  return 0;
}
