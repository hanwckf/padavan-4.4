/*
 * This file is subject to the terms and conditions of the LGPL V2.1
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2019 Kalray Inc.
 */

#ifndef _SYS_USER_H
#define _SYS_USER_H	1

struct user_regs_struct
{
  /* GPR */
  unsigned long long gpr_regs[64];

  /* SFR */
  unsigned long lc; 
  unsigned long le; 
  unsigned long ls; 
  unsigned long ra; 

  unsigned long cs; 
  unsigned long spc;
};

#endif
