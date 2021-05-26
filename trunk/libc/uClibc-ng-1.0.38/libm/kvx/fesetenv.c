/*
  (C) Copyright 2019 Kalray S.A.
  This file provides fesetenv for the Coolidge processor.
*/

#include <fenv.h>

int fesetenv(const fenv_t *envp)
{
  /* Mask *envp to be sure only valid bits are set */
  fenv_t fe = *envp;
  fe &= (FE_ALL_EXCEPT|FE_RND_MASK);

  /* Set exception flags and rounding mode bit-fields of $cs, with
     'fe' as a set mask and FE_ALL_EXCEPT|FE_RND_MASK as a clear
     mask. */
  __builtin_kvx_wfxl(KVX_SFR_CS, ((long long)fe << 32) | FE_ALL_EXCEPT | FE_RND_MASK);

  /* The above insn cannot fail (while the OS allows access to the
     floating-point exception flags of the $cs register). Return
     success. */
  return 0;
}
