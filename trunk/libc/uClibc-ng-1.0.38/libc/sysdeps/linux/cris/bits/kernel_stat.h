/* Taken from linux/include/asm-cris/stat.h */

#ifndef _BITS_STAT_STRUCT_H
#define _BITS_STAT_STRUCT_H

struct kernel_stat {
	unsigned short st_dev;
	unsigned short __pad1;
	unsigned long st_ino;
	unsigned short st_mode;
	unsigned short st_nlink;
	unsigned short st_uid;
	unsigned short st_gid;
	unsigned short st_rdev;
	unsigned short __pad2;
	unsigned long  st_size;
	unsigned long  st_blksize;
	unsigned long  st_blocks;
	struct timespec st_atim;
	struct timespec st_mtim;
	struct timespec st_ctim;
	unsigned long  __uclibc_unused4;
	unsigned long  __uclibc_unused5;
};

/* This matches struct kernel_stat64 in glibc2.1, hence the absolutely
 * insane amounts of padding around dev_t's.
 */
struct kernel_stat64 {
	unsigned short	st_dev;
	unsigned char	__pad0[10];

#define _HAVE_STAT64___ST_INO
	unsigned long	__st_ino;

	unsigned int	st_mode;
	unsigned int	st_nlink;

	unsigned long	st_uid;
	unsigned long	st_gid;

	unsigned short	st_rdev;
	unsigned char	__pad3[10];

	long long	st_size;
	unsigned long	st_blksize;

	unsigned long	st_blocks;	/* Number 512-byte blocks allocated. */
	unsigned long	__pad4;		/* future possible st_blocks high bits */

	struct timespec	st_atim;
	struct timespec	st_mtim;
	struct timespec	st_ctim;

	unsigned long long	st_ino;
};

#endif
