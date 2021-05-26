/*
 * This file is subject to the terms and conditions of the LGPL V2.1
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2018 Kalray Inc.
 */

/* Startup code compliant to the ELF KVX ABI */

#include <libc-symbols.h>
#include <features.h>

.type	    main,@function
.type	    __uClibc_main,@function
	
/*
 * When we enter this piece of code, the program stack has been
 * layed out by the kernel like this:
 *        argc            argument counter (integer)
 *        argv[0]         program name (pointer)
 *        argv[1...N]     program args (pointers)
 *        argv[argc-1]    end of args (integer)
 *        NULL
 *        env[0...N]      environment variables (pointers)
 *        NULL
 * 
 *  Moreover, when using dynamic loader, $r0 contains the rtld_fini
 *  address
 *
 * And we need to call the following function:
 *  __uClibc_main (int (*main) (int, char **, char **), int argc,
 *		char **argv, void (*init) (void), void (*fini) (void),
 *		void (*rtld_fini) (void), void *stack_end)
 */
.text
.globl	_start
.type	_start,@function
.align 	8
C_SYMBOL_NAME(_start):
	/* Load argc from stack */
	ld $r1 = 0[$sp]
	/* Load argv addr from stack */
	addd $r2 = $sp, 0x8
#ifdef __PIC__
	pcrel $r7 = @gotaddr()
#endif
	;;
	/* $r0 contains rtld_fini when run by dynamic loader */
	copyd $r5 = $r0
	/* prepare __uClibc_main arg */
#ifndef __PIC__
	make $r3 = _init
	make $r4 = _fini
#endif
	;;
	/* Align stack to 32-byte boundary */
	andd $sp = $sp, -32
	make $r8 = 0
	make $fp = 0
	/* prepare __uClibc_main arg */
#ifdef __PIC__
	ld $r3 = @got(_init)[$r7]
#endif
	;;
	/* Setup stack_end for __uClibc_main */
	copyd $r6 = $sp
	/* Clear compute status */
	set $cs = $r8
#ifdef __PIC__
	ld $r4 = @got(_fini)[$r7]
#endif
	;;
#ifdef __PIC__
	ld $r0 = @got(main)[$r7]
#else
	make $r0 = main
#endif
	goto __uClibc_main
	;;
	/* We should never return ! */
	errop
	;;
