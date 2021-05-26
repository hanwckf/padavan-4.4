/*
  (C) Copyright 2019 Kalray S.A.
  This file provides feclearexcept for the Coolidge processor.
*/

#include <fenv.h>

int feclearexcept(int excepts)
{
  /* Mask excepts to be sure only supported flag bits are set */
  excepts &= FE_ALL_EXCEPT;

  /* Set $cs with 'excepts' as a clear mask. */
  __builtin_kvx_wfxl(KVX_SFR_CS, excepts);

  /* The above insn cannot fail (while the OS allows access to the
     floating-point exception flags of the $cs register). Return
     success. */
  return 0;
}
