/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

       .text
       .global __enable_dcache
       .global __disable_dcache__inner_flush_dcache_L1__inner_clean_dcache_L2
.equ C1_IBIT ,  0x00001000
.equ C1_CBIT ,  0x00000004
.equ PSR_F_BIT, 0x00000040
.equ PSR_I_BIT, 0x00000080

__enable_dcache:
    MRC p15,0,r0,c1,c0,0
    ORR r0,r0,#C1_CBIT
    dsb
    MCR p15,0,r0,c1,c0,0
    dsb
    isb
    BX lr

__disable_dcache__inner_flush_dcache_L1__inner_clean_dcache_L2:
/*******************************************************************************
 * push stack                                                                  *
 ******************************************************************************/
        push    {r0,r1,r2,r3,r4,r5,r7,r9,r10,r11,r14}
/*******************************************************************************
 * __disable_dcache                                                            *
 ******************************************************************************/
        MRC p15,0,r0,c1,c0,0
        BIC r0,r0,#C1_CBIT
        dsb
        MCR p15,0,r0,c1,c0,0
        dsb
        isb
/*
 * Erratum:794322,An instruction fetch can be allocated into the L2 cache after
 * the cache is disabled Status. This erratum can be avoided by inserting both
 * of the following after the SCTLR.C bit is cleared to 0, and before the
 * caches are cleaned or invalidated:
 * 1) A TLBIMVA operation to any address.
 * 2) A DSB instruction.
 */
        MCR p15,0,r0,c8,c7,1
        dsb
        isb
/*******************************************************************************
 * __inner_flush_dcache_L1                                                     *
 ******************************************************************************/
        dmb                                     @ ensure ordering with previous memory accesses
        mrc     p15, 1, r0, c0, c0, 1           @ read clidr
        ands    r3, r0, #0x7000000              @ extract loc from clidr
        mov     r3, r3, lsr #23                 @ left align loc bit field
        beq     DF1C2_L1_finished               @ if loc is 0, then no need to clean
        mov     r10, #0                         @ start clean at cache level 1
DF1C2_L1_loop1:
        add     r2, r10, r10, lsr #1            @ work out 3x current cache level
        mov     r1, r0, lsr r2                  @ extract cache type bits from clidr
        and     r1, r1, #7                      @ mask of the bits for current cache only
        cmp     r1, #2                          @ see what cache we have at this level
        blt     DF1C2_L1_skip                   @ skip if no cache, or just i-cache
#ifdef CONFIG_ARM_ERRATA_814220
	dsb
#endif
        mcr     p15, 2, r10, c0, c0, 0          @ select current cache level in cssr
        isb                                     @ isb to sych the new cssr&csidr
        mrc     p15, 1, r1, c0, c0, 0           @ read the new csidr
        and     r2, r1, #7                      @ extract the length of the cache lines
        add     r2, r2, #4                      @ add 4 (line length offset)
        ldr     r4, =0x3ff
        ands    r4, r4, r1, lsr #3              @ find maximum number on the way size
        clz     r5, r4                          @ find bit position of way size increment
        ldr     r7, =0x7fff
        ands    r7, r7, r1, lsr #13             @ extract max number of the index size
DF1C2_L1_loop2:
        mov     r9, r4                          @ create working copy of max way size
DF1C2_L1_loop3:
        orr     r11, r10, r9, lsl r5            @ factor way and cache number into r11
        orr     r11, r11, r7, lsl r2            @ factor index number into r11
#if 1
        mcr     p15, 0, r11, c7, c10, 2         @ clean by set/way
        mcr     p15, 0, r11, c7, c6, 2          @ invalidate by set/way
#endif

#if 0
        mcr     p15, 0, r11, c7, c14, 2         @ clean & invalidate by set/way
#endif
        subs    r9, r9, #1                      @ decrement the way
        bge     DF1C2_L1_loop3
        subs    r7, r7, #1                      @ decrement the index
        bge     DF1C2_L1_loop2
DF1C2_L1_skip:
        @add     r10, r10, #2                   @ increment cache number
        @cmp     r3, r10
        @bgt     DF1C2_L1_loop1
DF1C2_L1_finished:
        mov     r10, #0                         @ swith back to cache level 0
        mcr     p15, 2, r10, c0, c0, 0          @ select current cache level in cssr
        dsb
        isb
/*******************************************************************************
 * clrex                                                                       *
 ******************************************************************************/
        clrex
/*******************************************************************************
 * __inner_clean_dcache_L2                                                     *
 ******************************************************************************/
        dmb                                     @ ensure ordering with previous memory accesses
        mrc     p15, 1, r0, c0, c0, 1           @ read clidr
        ands    r3, r0, #0x7000000              @ extract loc from clidr
        mov     r3, r3, lsr #23                 @ left align loc bit field
        beq     DF1C2_L2_cl_finished            @ if loc is 0, then no need to clean
        mov     r10, #2                         @ start clean at cache level 2
DF1C2_L2_cl_loop1:
        add     r2, r10, r10, lsr #1            @ work out 3x current cache level
        mov     r1, r0, lsr r2                  @ extract cache type bits from clidr
        and     r1, r1, #7                      @ mask of the bits for current cache only
        cmp     r1, #2                          @ see what cache we have at this level
        blt     DF1C2_L2_cl_skip                @ skip if no cache, or just i-cache
#ifdef CONFIG_ARM_ERRATA_814220
	dsb
#endif
        mcr     p15, 2, r10, c0, c0, 0          @ select current cache level in cssr
        isb                                     @ isb to sych the new cssr&csidr
        mrc     p15, 1, r1, c0, c0, 0           @ read the new csidr
        and     r2, r1, #7                      @ extract the length of the cache lines
        add     r2, r2, #4                      @ add 4 (line length offset)
        ldr     r4, =0x3ff
        ands    r4, r4, r1, lsr #3              @ find maximum number on the way size
        clz     r5, r4                          @ find bit position of way size increment
        ldr     r7, =0x7fff
        ands    r7, r7, r1, lsr #13             @ extract max number of the index size
DF1C2_L2_cl_loop2:
        mov     r9, r4                          @ create working copy of max way size
DF1C2_L2_cl_loop3:
        orr     r11, r10, r9, lsl r5            @ factor way and cache number into r11
        orr     r11, r11, r7, lsl r2            @ factor index number into r11
        mcr     p15, 0, r11, c7, c10, 2         @ clean by set/way
        subs    r9, r9, #1                      @ decrement the way
        bge     DF1C2_L2_cl_loop3
        subs    r7, r7, #1                      @ decrement the index
        bge     DF1C2_L2_cl_loop2
DF1C2_L2_cl_skip:
        @add     r10, r10, #2                   @ increment cache number
        @cmp     r3, r10
        @bgt     DF1C2_L2_cl_loop1
DF1C2_L2_cl_finished:
        mov     r10, #0                         @ swith back to cache level 0
        mcr     p15, 2, r10, c0, c0, 0          @ select current cache level in cssr
        dsb
        isb
/*******************************************************************************
 * pop  stack                                                                  *
 ******************************************************************************/
        pop     {r0,r1,r2,r3,r4,r5,r7,r9,r10,r11,r14}
        bx      lr


        .end
