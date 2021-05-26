/*
  (C) Copyright 2019 Kalray S.A.
  This file provides fesetround for the Coolidge processor.
*/

#include <fenv.h>

int fesetround(int rounding_mode)
{
  /* Mask round to be sure only valid rounding bits are set */
  rounding_mode &= FE_RND_MASK;

  /* Set rounding mode bit-fields of $cs, with 'rounding_mode' as a
     set mask and FE_RND_MASK as a clear mask. */
  __builtin_kvx_wfxl(KVX_SFR_CS, ((long long)rounding_mode << 32) | FE_RND_MASK);

  /* The above insn cannot fail (while the OS allows access to the
     floating-point exception flags of the $cs register). Return
     success. */
  return 0;
}
