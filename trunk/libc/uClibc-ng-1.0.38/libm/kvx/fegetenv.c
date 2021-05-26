/*
  (C) Copyright 2019 Kalray S.A.
  This file provides fegetenv for the Coolidge processor.
*/

#include <fenv.h>

int fegetenv(fenv_t *envp)
{
  /* Get the current environment ($cs) */
  fenv_t fe;
  fe = __builtin_kvx_get(KVX_SFR_CS);

  /* Mask $cs status to keep exception flags and rounding mode only. */
  *envp = (fe & (FE_ALL_EXCEPT | FE_RND_MASK));

  /* The above insn cannot fail (while the OS allows access to the
     floating-point exception flags of the $cs register). Return
     success. */
  return 0;
}
