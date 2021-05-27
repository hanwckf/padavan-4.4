/* Note that we use the exact same include guard #define names
 * as asm/posix_types.h.  This will avoid gratuitous conflicts
 * with the posix_types.h kernel header, and will ensure that
 * our private content, and not the kernel header, will win.
 *  -Erik
 */
#ifndef __ASM_AVR32_POSIX_TYPES_H
#define __ASM_AVR32_POSIX_TYPES_H

/*
 * This file is generally used by user-level software, so you need to
 * be a little careful about namespace pollution etc.  Also, we cannot
 * assume GCC is being used.
 */

typedef unsigned long		__kernel_dev_t;
typedef unsigned long		__kernel_ino_t;
typedef unsigned short		__kernel_mode_t;
typedef unsigned short		__kernel_nlink_t;
typedef long			__kernel_off_t;
typedef int			__kernel_pid_t;
typedef unsigned short		__kernel_ipc_pid_t;
typedef unsigned int		__kernel_uid_t;
typedef unsigned int		__kernel_gid_t;
typedef unsigned long		__kernel_size_t;
typedef long			__kernel_ssize_t;
typedef int			__kernel_ptrdiff_t;
typedef long			__kernel_time_t;
typedef long			__kernel_suseconds_t;
typedef long			__kernel_clock_t;
typedef int			__kernel_timer_t;
typedef int			__kernel_clockid_t;
typedef int			__kernel_daddr_t;
typedef char *			__kernel_caddr_t;
typedef unsigned short		__kernel_uid16_t;
typedef unsigned short		__kernel_gid16_t;
typedef unsigned int		__kernel_uid32_t;
typedef unsigned int		__kernel_gid32_t;
typedef unsigned short		__kernel_old_uid_t;
typedef unsigned short		__kernel_old_gid_t;
typedef unsigned short		__kernel_old_dev_t;
typedef long			__kernel_long_t;
typedef unsigned long		__kernel_ulong_t;

#ifdef __GNUC__
typedef long long		__kernel_loff_t;
#endif

typedef struct {
#if defined(__USE_ALL)
	int	val[2];
#else
	int	__val[2];
#endif
} __kernel_fsid_t;

#endif /* __ASM_AVR32_POSIX_TYPES_H */
