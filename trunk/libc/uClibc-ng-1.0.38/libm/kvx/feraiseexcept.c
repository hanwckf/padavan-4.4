/*
  (C) Copyright 2019 Kalray S.A.
  This file provides feraiseexcept for the Coolidge processor.
*/

#include <fenv.h>

int feraiseexcept(int excepts)
{
  /* Mask excepts to be sure only supported flag bits are set */
  excepts &= FE_ALL_EXCEPT;

  /* Set $cs with 'excepts' as a set mask. */
  __builtin_kvx_wfxl(KVX_SFR_CS, (long long)excepts << 32);

  /* C99 requirements are met. The flags are raised at the same time
     so order is preserved. FE_INEXACT is not raised if one of the
     exceptions is FE_OVERFLOW or FE_UNDERFLOW. */

  /* The above insn cannot fail (while the OS allows access to the
     floating-point exception flags of the $cs register). Return
     success. */
  return 0;
}
