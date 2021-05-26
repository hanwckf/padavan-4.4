/*
 * Copyright (C) 2020 Kalray Inc.
 *
 * Licensed under the LGPL v2.1 or later, see the file COPYING.LIB
 * in this tarball.
 */

#include <sysdep.h>

.align 16
ENTRY(memcpy)
	cb.deqz $r2? .Lreturn
	compd.geu $r3 = $r2, 256
	copyd $r6 = $r0
	;;
	cb.deqz $r3? .Lremaining_256
	;;
	lq.u $r32r33 = 0[$r1]
	addd $r2 = $r2, -256
	;;
	lq.u $r34r35 = 16[$r1]
	;;
	lq.u $r36r37 = 32[$r1]
	srld $r7 = $r2, 8
	;;
	lq.u $r38r39 = 48[$r1]
	;;
	lq.u $r40r41 = 64[$r1]
	;;
	lq.u $r42r43 = 80[$r1]
	;;
	lq.u $r44r45 = 96[$r1]
	;;
	lq.u $r46r47 = 112[$r1]
	;;
	lq.u $r48r49 = 128[$r1]
	;;
	lq.u $r50r51 = 144[$r1]
	;;
	lq.u $r52r53 = 160[$r1]
	;;
	lq.u $r54r55 = 176[$r1]
	;;
	lq.u $r56r57 = 192[$r1]
	;;
	lq.u $r58r59 = 208[$r1]
	compd.geu $r3 = $r2, 256
	;;
	lq.u $r60r61 = 224[$r1]
	;;
	lq.u $r62r63 = 240[$r1]
	addd $r1 = $r1, 256
	;;
	cb.deqz $r7? .Lstreaming_loop_end
	;;
	loopdo $r7? .Lstreaming_loop_end
		;;
		sq 0[$r0] = $r32r33
		addd $r2 = $r2, -256
		;;
		lq.u $r32r33 = 0[$r1]
		;;
		sq 16[$r0] = $r34r35
		;;
		lq.u $r34r35 = 16[$r1]
		;;
		sq 32[$r0] = $r36r37
		;;
		lq.u $r36r37 = 32[$r1]
		;;
		sq 48[$r0] = $r38r39
		;;
		lq.u $r38r39 = 48[$r1]
		;;
		sq 64[$r0] = $r40r41
		;;
		lq.u $r40r41 = 64[$r1]
		;;
		sq 80[$r0] = $r42r43
		;;
		lq.u $r42r43 = 80[$r1]
		;;
		sq 96[$r0] = $r44r45
		;;
		lq.u $r44r45 = 96[$r1]
		;;
		sq 112[$r0] = $r46r47
		;;
		lq.u $r46r47 = 112[$r1]
		;;
		sq 128[$r0] = $r48r49
		;;
		lq.u $r48r49 = 128[$r1]
		;;
		sq 144[$r0] = $r50r51
		;;
		lq.u $r50r51 = 144[$r1]
		;;
		sq 160[$r0] = $r52r53
		;;
		lq.u $r52r53 = 160[$r1]
		;;
		sq 176[$r0] = $r54r55
		;;
		lq.u $r54r55 = 176[$r1]
		;;
		sq 192[$r0] = $r56r57
		;;
		lq.u $r56r57 = 192[$r1]
		;;
		sq 208[$r0] = $r58r59
		;;
		lq.u $r58r59 = 208[$r1]
		;;
		sq 224[$r0] = $r60r61
		;;
		lq.u $r60r61 = 224[$r1]
		;;
		sq 240[$r0] = $r62r63
		addd $r0 = $r0, 256
		;;
		lq.u $r62r63 = 240[$r1]
		addd $r1 = $r1, 256
		;;
	.Lstreaming_loop_end:
	sq 0[$r0] = $r32r33
	;;
	sq 16[$r0] = $r34r35
	;;
	sq 32[$r0] = $r36r37
	;;
	sq 48[$r0] = $r38r39
	;;
	sq 64[$r0] = $r40r41
	;;
	sq 80[$r0] = $r42r43
	;;
	sq 96[$r0] = $r44r45
	;;
	sq 112[$r0] = $r46r47
	;;
	sq 128[$r0] = $r48r49
	;;
	sq 144[$r0] = $r50r51
	;;
	sq 160[$r0] = $r52r53
	;;
	sq 176[$r0] = $r54r55
	;;
	sq 192[$r0] = $r56r57
	;;
	sq 208[$r0] = $r58r59
	;;
	sq 224[$r0] = $r60r61
	;;
	sq 240[$r0] = $r62r63
	addd $r0 = $r0, 256
	;;
.Lremaining_256:
	andd $r11 = $r2, 16
	srld $r7 = $r2, 5
	;;
	cb.deqz $r7? .Lloop_32_end
	;;
	loopdo $r7? .Lloop_32_end
		;;
		lo $r32r33r34r35 = 0[$r1]
		addd $r1 = $r1, 32
		addd $r2 = $r2, -32
		;;
		so 0[$r0] = $r32r33r34r35
		addd $r0 = $r0, 32
		;;
	.Lloop_32_end:
	andd $r10 = $r2, 8
	andd $r9 = $r2, 4
	cb.deqz $r11? .Lloop_remaining_16
	lq.u.dnez $r11? $r32r33 = 0[$r1]
	;;
	sq 0[$r0] = $r32r33
	addd $r1 = $r1, 16
	addd $r0 = $r0, 16
	;;
.Lloop_remaining_16:
	andd $r8 = $r2, 2
	andd $r7 = $r2, 1
	cb.deqz $r10? .Lloop_remaining_8
	ld.dnez $r10? $r32 = 0[$r1]
	;;
	sd 0[$r0] = $r32
	addd $r1 = $r1, 8
	addd $r0 = $r0, 8
	;;
.Lloop_remaining_8:
	cb.deqz $r9? .Lloop_remaining_4
	lwz.dnez $r9? $r32 = 0[$r1]
	;;
	sw 0[$r0] = $r32
	addd $r1 = $r1, 4
	addd $r0 = $r0, 4
	;;
.Lloop_remaining_4:
	cb.deqz $r8? .Lloop_remaining_2
	lhz.dnez $r8? $r32 = 0[$r1]
	;;
	sh 0[$r0] = $r32
	addd $r1 = $r1, 2
	addd $r0 = $r0, 2
	;;
.Lloop_remaining_2:
	lbz.dnez $r7? $r32 = 0[$r1]
	;;
	sb.dnez $r7? 0[$r0] = $r32
	;;
.Lreturn:
	copyd $r0 = $r6
	ret
	;;
END(memcpy)

libc_hidden_def(memcpy)
