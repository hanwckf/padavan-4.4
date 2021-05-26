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
 * void __longjmp(__jmp_buf __env, int __val)
 */
ENTRY(__longjmp)
	/* Load $ra and $csinto r40r41 */
	lq $r40r41 = JMPBUF_RA_CS_OFFSET[$r0]
	;;
	/* Load $r36r37r38r39 with r12(sp) r14 r18 r19 */
	lo $r36r37r38r39 = (JMPBUF_REGS_OFFSET)[$r0]
	set $ra = $r40
	;;
	/* Load $lc, $le and $ls */
	lo $r32r33r34r35 = JMPBUF_LC_LE_LS_OFFSET[$r0]
	copyd $sp = $r36
	copyd $r14 = $r37
	set $cs = $r41
	;;
	/* Load r20r21r22r23 */
	lo $r20r21r22r23 = (JMPBUF_REGS_OFFSET + QUAD_REG_SIZE)[$r0]
	copyd $r18 = $r38
	copyd $r19 = $r39
	set $lc = $r32
	;;
	/* Load r24r25r26r27 */
	lo $r24r25r26r27 = (JMPBUF_REGS_OFFSET + 2 * QUAD_REG_SIZE)[$r0]
	set $le = $r33
	;;
	/* Load r28r29r30r31 */
	lo $r28r29r30r31 = (JMPBUF_REGS_OFFSET + 3 * QUAD_REG_SIZE)[$r0]
	set $ls = $r34
	/* Copy retval */
	copyd $r0 = $r1
	;;
	/* According to man, if retval is equal to 0, then we should return 1 */
	cmoved.deqz $r0? $r0 = 1
	ret
	;;
END(__longjmp)
libc_hidden_def(__longjmp)
