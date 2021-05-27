/* s_copysignl.c -- long double version of s_copysign.c.
 * Conversion to long double by Ulrich Drepper,
 * Cygnus Support, drepper@cygnus.com.
 */

/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

/*
 * copysignl(long double x, long double y)
 * copysignl(x,y) returns a value with the magnitude of x and
 * with the sign bit of y.
 */

#include <endian.h>
#include <stdint.h>

#if __FLOAT_WORD_ORDER == BIG_ENDIAN

typedef union
{
  long double value;
  struct
  {
    int sign_exponent:16;
    unsigned int empty:16;
    uint32_t msw;
    uint32_t lsw;
  } parts;
} ieee_long_double_shape_type;

#endif

#if __FLOAT_WORD_ORDER == LITTLE_ENDIAN

typedef union
{
  long double value;
  struct
  {
    uint32_t lsw;
    uint32_t msw;
    int sign_exponent:16;
    unsigned int empty:16;
  } parts;
} ieee_long_double_shape_type;

#endif

/* Get int from the exponent of a long double.  */

#define GET_LDOUBLE_EXP(exp,d)					\
do {								\
  ieee_long_double_shape_type ge_u;				\
  ge_u.value = (d);						\
  (exp) = ge_u.parts.sign_exponent;				\
} while (0)

/* Set exponent of a long double from an int.  */

#define SET_LDOUBLE_EXP(d,exp)					\
do {								\
  ieee_long_double_shape_type se_u;				\
  se_u.value = (d);						\
  se_u.parts.sign_exponent = (exp);				\
  (d) = se_u.value;						\
} while (0)

long double copysignl(long double x, long double y);
libc_hidden_proto(copysignl);

long double copysignl(long double x, long double y)
{
	uint32_t es1,es2;
	GET_LDOUBLE_EXP(es1,x);
	GET_LDOUBLE_EXP(es2,y);
	SET_LDOUBLE_EXP(x,(es1&0x7fff)|(es2&0x8000));
        return x;
}

libc_hidden_def(copysignl);
