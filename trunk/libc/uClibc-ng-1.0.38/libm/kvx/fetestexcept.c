/*
  (C) Copyright 2019 Kalray S.A.
  This file provides fesetexcept for the Coolidge processor.
*/

#include <fenv.h>

int fetestexcept(int excepts)
{
  /* Mask excepts to be sure only supported flag bits are set */
  excepts &= FE_ALL_EXCEPT;

  /* Get the current exception flags of the $cs register. */
  fexcept_t flags;
  flags = __builtin_kvx_get(KVX_SFR_CS);

  /* Return the floating-point exception macros that are both included
     in excepts and correspond to the floating-point exceptions
     currently set. */
  return (flags & excepts);
}
