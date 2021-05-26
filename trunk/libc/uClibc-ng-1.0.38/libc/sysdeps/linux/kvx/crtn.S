/*
 * This file is subject to the terms and conditions of the LGPL V2.1
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2018 Kalray Inc.
 */

	.section .init
	.align 8
	.global	_init
	.type	 _init,@function
	# EPILOGUE
	lq $r14r15 = 0[$sp]
	;;
	set $ra = $r15
	addd $sp = $sp, 32
	;;
	ret
	;;

	.section .fini
	.align 8
	.global	_fini
	.type	 _fini,@function
	# EPILOGUE
	lq $r14r15 = 0[$sp]
	;;
	set $ra = $r15
	addd $sp = $sp, 32
	;;
	ret
	;;
