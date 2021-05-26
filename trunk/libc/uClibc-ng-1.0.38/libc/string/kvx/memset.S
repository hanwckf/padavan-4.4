/*
 * Copyright (C) 2019 Kalray Inc.
 *
 * Licensed under the LGPL v2.1 or later, see the file COPYING.LIB
 * in this tarball.
 */

#define REPLICATE_BYTE_MASK	0x0101010101010101
#define MIN_SIZE_FOR_ALIGN	128

/*
 * Optimized memset for kvx architecture
 *
 * In order to optimize memset on kvx, we can use various things:
 * - conditionnal store which avoid branch penalty
 * - store half/word/double/quad/octuple to store up to 16 bytes at a time
 * - hardware loop for steady cases.
 *
 * First,  we start by checking if the size is below a minimum size. If so, we
 * skip the alignment part. Indeed, the kvx supports misalignment and the
 * penalty for letting it do unaligned accesses is lower than trying to
 * realigning us. So for small sizes, we don't even bother to realign.
 * In order to create the 64 bits pattern, we use sbmm to replicate the pattern
 * on all bits on a register in one call.
 * Once alignment has been reached, we can do the hardware loop using store
 * octuple in order to optimize throughput. Care must be taken to align hardware
 * loops on at least 8 bytes for performances.
 * Once the main loop has been done, we finish the copy by checking length to do
 * the necessary calls to store remaining bytes.
 */

#include <sysdep.h>

.align 16
ENTRY(memset)
	/* Preserve return value */
	copyd $r3 = $r0
	/* Replicate the first pattern byte on all bytes */
	sbmm8 $r32 = $r1, REPLICATE_BYTE_MASK
	/* Check if length < MIN_SIZE_FOR_ALIGN */
	compd.geu $r7 = $r2, MIN_SIZE_FOR_ALIGN
	/* Invert address to compute what we need to copy to be aligned on 32 bytes */
	negd $r5 = $r0
	;;
	/* Check if we are aligned on 32 bytes */
	andw $r9 = $r0, 0x1F
	/* Compute the length that will be copied to align on 32 bytes boundary */
	andw $r6 = $r5, 0x1F
	/*
	 * If size < MIN_SIZE_FOR_ALIGN bits, directly go to so, it will be done
	 * unaligned but that is still better that what we can do with sb
	 */
	cb.deqz $r7? .Laligned_32
	;;
	/* Remove unaligned part from length */
	sbfd $r2 = $r6, $r2
	/* If we are already aligned on 32 bytes, jump to main "so" loop */
	cb.deqz $r9? .Laligned_32
	/* Check if we need to copy 1 byte */
	andw $r4 = $r5, (1 << 0)
	;;
	/* If we are not aligned, store byte */
	sb.dnez $r4? [$r0] = $r32
	/* Check if we need to copy 2 bytes */
	andw $r4 = $r5, (1 << 1)
	/* Add potentially copied part for next store offset */
	addd $r0 = $r0, $r4
	;;
	sh.dnez $r4? [$r0] = $r32
	/* Check if we need to copy 4 bytes */
	andw $r4 = $r5, (1 << 2)
	addd $r0 = $r0, $r4
	;;
	sw.dnez $r4? [$r0] = $r32
	/* Check if we need to copy 8 bytes */
	andw $r4 = $r5, (1 << 3)
	addd $r0 = $r0, $r4
	/* Copy second part of pattern for sq */
	copyd $r33 = $r32
	;;
	sd.dnez $r4? [$r0] = $r32
	/* Check if we need to copy 16 bytes */
	andw $r4 = $r5, (1 << 4)
	addd $r0 = $r0, $r4
	;;
	sq.dnez $r4? [$r0] = $r32r33
	addd $r0 = $r0, $r4
	;;
.Laligned_32:
	/* Copy second part of pattern for sq */
	copyd $r33 = $r32
	/* Prepare amount of data for 32 bytes store */
	srld $r10 = $r2, 5
	nop
	nop
	;;
	copyq $r34r35 = $r32, $r33
	/* Remaining bytes for 16 bytes store */
	andw $r8 = $r2, (1 << 4)
	make $r11 = 32
	/* Check if there are enough data for 32 bytes store */
	cb.deqz $r10? .Laligned_32_done
	;;
	loopdo $r10, .Laligned_32_done
		;;
		so 0[$r0] = $r32r33r34r35
		addd $r0 = $r0, $r11
		;;
	.Laligned_32_done:
	/*
	 * Now that we have handled every aligned bytes using 'so', we can
	 * handled the remainder of length using store by decrementing size
	 * We also exploit the fact we are aligned to simply check remaining
	 * size */
	sq.dnez $r8? [$r0] = $r32r33
	addd $r0 = $r0, $r8
	/* Remaining bytes for 8 bytes store */
	andw $r8 = $r2, (1 << 3)
	cb.deqz $r2? .Lmemset_done
	;;
	sd.dnez $r8? [$r0] = $r32
	addd $r0 = $r0, $r8
	/* Remaining bytes for 4 bytes store */
	andw $r8 = $r2, (1 << 2)
	;;
	sw.dnez $r8? [$r0] = $r32
	addd $r0 = $r0, $r8
	/* Remaining bytes for 2 bytes store */
	andw $r8 = $r2, (1 << 1)
	;;
	sh.dnez $r8? [$r0] = $r32
	addd $r0 = $r0, $r8
	;;
	sb.odd $r2? [$r0] = $r32
	/* Restore original value */
	copyd $r0 = $r3
	ret
	;;
.Lmemset_done:
	/* Restore original value */
	copyd $r0 = $r3
	ret
	;;
END(memset)

libc_hidden_def(memset)
