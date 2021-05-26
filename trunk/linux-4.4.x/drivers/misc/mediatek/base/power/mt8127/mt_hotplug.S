/*
 * Copyright (c) 2015 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

	.text

	.global __switch_to_smp
	.global __switch_to_amp

.equ ACTLR_SMP_BIT ,0x00000040

__switch_to_smp:
	MRC	p15,0,r0,c1,c0,1
	ORR	r0,r0,#ACTLR_SMP_BIT
	MCR	p15,0,r0,c1,c0,1
	BX	lr

__switch_to_amp:
	MRC	p15,0,r0,c1,c0,1
	BIC	r0,r0,#ACTLR_SMP_BIT
	MCR	p15,0,r0,c1,c0,1
	BX	lr
