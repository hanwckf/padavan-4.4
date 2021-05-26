/*
  (C) Copyright 2019 Kalray S.A.
  This file provides fegetround for the Coolidge processor.
*/

#include <fenv.h>

int fegetround(void)
{
  /* Get all $cs flags (exception flags and rounding mode) */
  fenv_t rm;
  rm = __builtin_kvx_get(KVX_SFR_CS);

  /* Return the rounding mode */
  return rm & FE_RND_MASK;
}
