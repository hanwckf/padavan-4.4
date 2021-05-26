/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2018 Kalray Inc.
 */

#ifndef _BITS_SETJMP_H
#define _BITS_SETJMP_H  1

#if !defined _SETJMP_H && !defined _PTHREAD_H
# error "Never include <bits/setjmp.h> directly; use <setjmp.h> instead."
#endif

#define SIZE_OF_REG 8

/* Size of a quad reg (can't use sizeof(uint64_t) since it will be in asm */
#define QUAD_REG_SIZE (4 * SIZE_OF_REG)


#define JMPBUF_RA_CS_OFFSET  0
#define JMPBUF_LC_LE_LS_OFFSET (2 * SIZE_OF_REG)
/* Start offset of  regs[] in __jmp_buf struct */
#define JMPBUF_REGS_OFFSET   (JMPBUF_LC_LE_LS_OFFSET + (4 * SIZE_OF_REG))

#ifndef _ASM
typedef struct
  {
    /* Return address */
    unsigned long ra;
    unsigned long cs;

    /* Store lc, le, ls into this buf */
    unsigned long lc_le_ls[4];

    /* Callee-saved GPR registers:
     * r12(sp) r14 r18 r19 r20 r21 r22 r23 r24 r25 r26 r27 r28 r29 r30 r31
     */
    unsigned long regs[16];

  } __jmp_buf[1] __attribute__((__aligned__ (8)));

#endif

#endif  /* bits/setjmp.h */
