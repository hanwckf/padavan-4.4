/*
 * Track misc arch-specific features that aren't config options
 */

#ifndef _BITS_UCLIBC_ARCH_FEATURES_H
#define _BITS_UCLIBC_ARCH_FEATURES_H

/* instruction used when calling abort() to kill yourself */
#define __UCLIBC_ABORT_INSTRUCTION__ "call_pal 0"

/* can your target use syscall6() for mmap ? */
#define __UCLIBC_MMAP_HAS_6_ARGS__

/* does your target use statx */
#undef __UCLIBC_HAVE_STATX__

/* does your target align 64bit values in register pairs ? (32bit arches only) */
#undef __UCLIBC_SYSCALL_ALIGN_64BIT__

/* does your target have a broken create_module() ? */
#define __UCLIBC_SLIGHTLY_BROKEN_CREATE_MODULE__

/* does your target have to worry about older [gs]etrlimit() ? */
#undef __UCLIBC_HANDLE_OLDER_RLIMIT__

/* does your target have an asm .set ? */
#undef __UCLIBC_HAVE_ASM_SET_DIRECTIVE__

/* define if target supports .weak */
#define __UCLIBC_HAVE_ASM_WEAK_DIRECTIVE__

/* define if target supports .weakext */
#undef __UCLIBC_HAVE_ASM_WEAKEXT_DIRECTIVE__

/* define if target supports CFI pseudo ops */
#undef __UCLIBC_HAVE_ASM_CFI_DIRECTIVES__

/* define if target supports IEEE signed zero floats */
#define __UCLIBC_HAVE_SIGNED_ZERO__

/* only weird assemblers generally need this */
#undef __UCLIBC_ASM_LINE_SEP__

#endif /* _BITS_UCLIBC_ARCH_FEATURES_H */
