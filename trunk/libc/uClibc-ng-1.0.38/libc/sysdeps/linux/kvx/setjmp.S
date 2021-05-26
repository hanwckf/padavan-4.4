/*
 * This file is subject to the terms and conditions of the LGPL V2.1
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2018 Kalray Inc.
 */

#include <sysdep.h>

#define _SETJMP_H
#define _ASM
#include <bits/setjmp.h>
#include <libc-symbols.h>

/**
 * Simply save the user context to $r0 (jmpbuf)
 *
 * This function has the following prototype:
 * int __sigsetjmp (r0 = jmp_buf env, r1 = int savemask)
 * At the end, we call sigjmp_save
 * int __sigjmp_save (sigjmp_buf env, int savemask)
 * which will save signal mask if needed (set by setjmp/_setjmp)
 *
 * NOTE: since r0 and r1 are used by __sigjmp_save, we must not
 * clobber them during this function
 */
ENTRY(__sigsetjmp)
	/* Save r20r21r22r23 */
	so (JMPBUF_REGS_OFFSET + QUAD_REG_SIZE)[$r0] = $r20r21r22r23
	get $r40 = $ra
	;;
	/* Save r24r25r26r27 */
	so (JMPBUF_REGS_OFFSET + 2 * QUAD_REG_SIZE) [$r0] = $r24r25r26r27
	get $r41 = $cs
	;;
	copyd $r36 = $sp
	copyd $r37 = $r14
	copyd $r38 = $r18
	copyd $r39 = $r19
	;;
	/* Save r12(sp) r14 r18 r19 stored in $r36r37r38r39 */
	so (JMPBUF_REGS_OFFSET)[$r0] = $r36r37r38r39
	get $r32 = $lc
	;;
	/* Save r28r29r30r31 */
	so (JMPBUF_REGS_OFFSET + 3 * QUAD_REG_SIZE) [$r0] = $r28r29r30r31
	get $r33 = $le
	;;
	/* Save $cs and $ra */
	sq JMPBUF_RA_CS_OFFSET[$r0] = $r40r41
	get $r34 = $ls
	;;
	/* Save $lc, $le and $ls */
	so JMPBUF_LC_LE_LS_OFFSET[$r0] = $r32r33r34r35
	goto __sigjmp_save
	;;
END(__sigsetjmp)
libc_hidden_def(__sigsetjmp)
