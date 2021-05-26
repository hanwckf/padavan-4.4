/* Copyright (C) 1997, 1998, 1999, 2000, 2001 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#ifndef _FENV_H
# error "Never use <bits/fenv.h> directly; include <fenv.h> instead."
#endif

/* $cs register number for use in kvx builtins */
#define KVX_SFR_CS 4

/* Each core of the Coolidge processor has a coprocessor. They share
   the CS register but have distinct bit-fields for their
   floating-point environment. This implementation synchronizes them
   in such a way that they cannot be managed separately. */

/* Compute Status ($cs) register contains the following bit-fields for
   floating-point execption flags.

   Bit-field Condition of the IEEE 754 binary floating-point standard
   --------- --------------------------------------------------------
   IO        Invalid Operation
   DZ        Divide by Zero
   OV        Overflow
   UN        Underflow
   IN        Inexact
   XIO       Invalid Operation (coprocessor)
   XDZ       Divide by Zero (coprocessor)
   XOV       Overflow (coprocessor)
   XUN       Underflow (coprocessor)
   XIN       Inexact (coprocessor) */

#define _FE_INVALID   0x02
#define _FE_DIVBYZERO 0x04
#define _FE_OVERFLOW  0x08
#define _FE_UNDERFLOW 0x10
#define _FE_INEXACT   0x20

#define _FE_X_INVALID   0x0200
#define _FE_X_DIVBYZERO 0x0400
#define _FE_X_OVERFLOW  0x0800
#define _FE_X_UNDERFLOW 0x1000
#define _FE_X_INEXACT   0x2000

#define FE_INVALID   (_FE_INVALID   | _FE_X_INVALID)
#define FE_DIVBYZERO (_FE_DIVBYZERO | _FE_X_DIVBYZERO)
#define FE_OVERFLOW  (_FE_OVERFLOW  | _FE_X_OVERFLOW)
#define FE_UNDERFLOW (_FE_UNDERFLOW | _FE_X_UNDERFLOW)
#define FE_INEXACT   (_FE_INEXACT   | _FE_X_INEXACT)

#define FE_ALL_EXCEPT (FE_INVALID|FE_DIVBYZERO|FE_OVERFLOW|FE_UNDERFLOW|FE_INEXACT)

/* Compute Status ($cs) register contains the following bit-fields for
   floating-point rounding modes.

   Following table describes both the RM and XRM (coproc) bit-fields.

   Value Rounding Mode of the IEEE 754 binary floating-point standard
   ----- ------------------------------------------------------------
   0b00  to nearest even
   0b01  toward +inf
   0b10  toward -inf
   0b11  toward zero */

#define _FE_TONEAREST  0
#define _FE_UPWARD     1
#define _FE_DOWNWARD   2
#define _FE_TOWARDZERO 3

#define _FE_X_TONEAREST  0
#define _FE_X_UPWARD     1
#define _FE_X_DOWNWARD   2
#define _FE_X_TOWARDZERO 3


#define FE_TONEAREST  ((_FE_TONEAREST  << 16) | (_FE_X_TONEAREST  << 20))
#define FE_UPWARD     ((_FE_UPWARD     << 16) | (_FE_X_UPWARD     << 20))
#define FE_DOWNWARD   ((_FE_DOWNWARD   << 16) | (_FE_X_DOWNWARD   << 20))
#define FE_TOWARDZERO ((_FE_TOWARDZERO << 16) | (_FE_X_TOWARDZERO << 20))

#define FE_RND_MASK FE_TOWARDZERO

/* The type representing all floating-point status flags collectively.
   The environment is simply a copy from the FPU related bits in the
   CS register, but can be improved in the future. */
typedef unsigned int fexcept_t;
/* The type representing the entire floating-point environment.  The
   environment is simply a copy from the FPU related bits in the CS
   register. */
typedef unsigned int fenv_t;

extern const fenv_t __fe_dfl_env;
#define FE_DFL_ENV  __fe_dfl_env
