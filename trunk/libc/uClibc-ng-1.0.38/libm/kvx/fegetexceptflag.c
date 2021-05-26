/*
  (C) Copyright 2019 Kalray S.A.
  This file provides fegetexceptflag for the Coolidge processor.
*/

#include <fenv.h>

int fegetexceptflag(fexcept_t *flagp, int excepts)
{
  /* Mask excepts to be sure only supported flag bits are set */
  excepts &= FE_ALL_EXCEPT;

  /* Get the current exception flags of the $cs register. */
  fexcept_t flags;
  flags = __builtin_kvx_get(KVX_SFR_CS);

  /* Return the requested flags in flagp */
  *flagp = flags & excepts;

  /* The above insn cannot fail (while the OS allows access to the
     floating-point exception flags of the $cs register). Return
     success. */
  return 0;
}
