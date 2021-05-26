/*
 * Copyright (C) 2018 Kalray Inc.	
 * Licensed under the LGPL v2.1, see the file COPYING.LIB in this tarball.
 */

#if defined __UCLIBC_HAS_TLS__
#error NOT IMPLEMENTED: THIS IS A SKELETON 
	.text

	.hidden _dl_tlsdesc_return
	.global	_dl_tlsdesc_return
	.type	_dl_tlsdesc_return,%function
	.align 2
_dl_tlsdesc_return:
	errop
	;;
.size	_dl_tlsdesc_return, .-_dl_tlsdesc_return

#ifdef SHARED

	.hidden _dl_tlsdesc_dynamic
	.global	_dl_tlsdesc_dynamic
	.type	_dl_tlsdesc_dynamic,%function
	cfi_startproc
	.align 2
_dl_tlsdesc_dynamic:
	errop
	;;
	cfi_endproc
	.size	_dl_tlsdesc_dynamic, .-_dl_tlsdesc_dynamic

#endif // SHARED
#endif // __UCLIBC_HAS_TLS__
