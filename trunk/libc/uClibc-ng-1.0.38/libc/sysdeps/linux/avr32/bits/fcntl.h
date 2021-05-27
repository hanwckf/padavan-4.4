#ifndef _FCNTL_H
# error "Never use <bits/fcntl.h> directly; include <fcntl.h> instead."
#endif

#include <sys/types.h>
#ifdef __USE_GNU
# include <bits/uio.h>
#endif

/*
 * open/fcntl - O_SYNC is only implemented on blocks devices and on files
 * located on an ext2 file system
 */
#define O_ACCMODE	00000003
#define O_RDONLY	00000000
#define O_WRONLY	00000001
#define O_RDWR		00000002
#define O_CREAT		00000100	/* not fcntl */
#define O_EXCL		00000200	/* not fcntl */
#define O_NOCTTY	00000400	/* not fcntl */
#define O_TRUNC		00001000	/* not fcntl */
#define O_APPEND	00002000
#define O_NONBLOCK	00004000
#define O_NDELAY	O_NONBLOCK
#define O_SYNC		00010000
#define O_ASYNC		00020000

#ifdef __USE_XOPEN2K8
# define O_DIRECTORY	00200000	/* direct disk access */
# define O_NOFOLLOW	00400000	/* don't follow links */
# define O_CLOEXEC      02000000	/* set close_on_exec */
#endif

#ifdef __USE_GNU
# define O_DIRECT	00040000	/* must be a directory */
# define O_NOATIME	01000000	/* don't set atime */
# define O_PATH        010000000	/* Resolve pathname but do not open file.  */
#endif

#ifdef __USE_LARGEFILE64
# define O_LARGEFILE	00100000
#endif

/* For now Linux has synchronisity options for data and read operations.
   We define the symbols here but let them do the same as O_SYNC since
   this is a superset.	*/
#if defined __USE_POSIX199309 || defined __USE_UNIX98
# define O_DSYNC	O_SYNC	/* Synchronize data.  */
# define O_RSYNC	O_SYNC	/* Synchronize read operations.	 */
#endif

#define F_DUPFD		0	/* dup */
#define F_GETFD		1	/* get close_on_exec */
#define F_SETFD		2	/* set/clear close_on_exec */
#define F_GETFL		3	/* get file->f_flags */
#define F_SETFL		4	/* set file->f_flags */

#ifndef __USE_FILE_OFFSET64
# define F_GETLK	5
# define F_SETLK	6
# define F_SETLKW	7
#else
# define F_GETLK	F_GETLK64
# define F_SETLK	F_SETLK64
# define F_SETLKW	F_SETLKW64
#endif
#define F_GETLK64	12	/*  using 'struct flock64' */
#define F_SETLK64	13
#define F_SETLKW64	14

#if defined __USE_BSD || defined __USE_XOPEN2K
# define F_SETOWN	8	/*  for sockets. */
# define F_GETOWN	9	/*  for sockets. */
#endif

#ifdef __USE_GNU
# define F_SETSIG	10	/*  for sockets. */
# define F_GETSIG	11	/*  for sockets. */
#endif

#ifdef __USE_GNU
# define F_SETLEASE	1024	/* Set a lease.	 */
# define F_GETLEASE	1025	/* Enquire what lease is active.  */
# define F_NOTIFY	1026	/* Request notfications on a directory.	 */
#endif

/* for F_[GET|SET]FL */
#define FD_CLOEXEC	1	/* actually anything with low bit set goes */

/* for posix fcntl() and lockf() */
#define F_RDLCK		0
#define F_WRLCK		1
#define F_UNLCK		2

/* for old implementation of bsd flock () */
#define F_EXLCK		4	/* or 3 */
#define F_SHLCK		8	/* or 4 */

/* for leases */
#define F_INPROGRESS	16

#ifdef __USE_BSD
/* operations for bsd flock(), also used by the kernel implementation */
# define LOCK_SH	1	/* shared lock */
# define LOCK_EX	2	/* exclusive lock */
# define LOCK_NB	4	/* or'd with one of the above to prevent
				   blocking */
# define LOCK_UN	8	/* remove lock */
#endif

#ifdef __USE_GNU
# define LOCK_MAND	32	/* This is a mandatory flock */
# define LOCK_READ	64	/* ... Which allows concurrent
				       read operations */
# define LOCK_WRITE	128	/* ... Which allows concurrent
				       write operations */
# define LOCK_RW	192	/* ... Which allows concurrent
				       read & write ops */
#endif

#ifdef __USE_GNU
/* Types of directory notifications that may be requested with F_NOTIFY.  */
# define DN_ACCESS	0x00000001	/* File accessed.  */
# define DN_MODIFY	0x00000002	/* File modified.  */
# define DN_CREATE	0x00000004	/* File created.  */
# define DN_DELETE	0x00000008	/* File removed.  */
# define DN_RENAME	0x00000010	/* File renamed.  */
# define DN_ATTRIB	0x00000020	/* File changed attibutes.  */
# define DN_MULTISHOT	0x80000000	/* Don't remove notifier.  */
#endif

struct flock {
	short		l_type;
	short		l_whence;
#ifndef __USE_FILE_OFFSET64
	__off_t		l_start;
	__off_t		l_len;
#else
	__off64_t	l_start;
	__off64_t	l_len;
#endif
	__pid_t		l_pid;
};

#ifdef __USE_LARGEFILE64
struct flock64 {
	short		l_type;
	short		l_whence;
	__off64_t	l_start;
	__off64_t	l_len;
	__pid_t		l_pid;
};
#endif

/* Define some more compatibility macros to be backward compatible with
 *    BSD systems which did not managed to hide these kernel macros.  */
#ifdef  __USE_BSD
# define FAPPEND        O_APPEND
# define FFSYNC         O_FSYNC
# define FASYNC         O_ASYNC
# define FNONBLOCK      O_NONBLOCK
# define FNDELAY        O_NDELAY
#endif /* Use BSD.  */

/* Advise to `posix_fadvise'.  */
#ifdef __USE_XOPEN2K
# define POSIX_FADV_NORMAL      0 /* No further special treatment.  */
# define POSIX_FADV_RANDOM      1 /* Expect random page references.  */
# define POSIX_FADV_SEQUENTIAL  2 /* Expect sequential page references.  */
# define POSIX_FADV_WILLNEED    3 /* Will need these pages.  */
# define POSIX_FADV_DONTNEED    4 /* Don't need these pages.  */
# define POSIX_FADV_NOREUSE     5 /* Data will be accessed once.  */
#endif


#if defined __USE_GNU && defined __UCLIBC_LINUX_SPECIFIC__
/* Flags for SYNC_FILE_RANGE.  */
# define SYNC_FILE_RANGE_WAIT_BEFORE	1 /* Wait upon writeout of all pages
					     in the range before performing
					     the write */
# define SYNC_FILE_RANGE_WRITE		2 /* Initiate writeout of all those
					     dirty pages in the range which are
					     not presently under writeback */
# define SYNC_FILE_RANGE_WAIT_AFTER	4 /* Wait upon writeout of all pages
					     in the range after performing the
					     write */

/* Flags for splice() and vmsplice() */
# define SPLICE_F_MOVE		1	/* Move pages instead of copying */
# define SPLICE_F_NONBLOCK	2	/* Don't block on the pipe splicing
					   (but we may still block on the fd
					   we splice from/to) */
# define SPLICE_F_MORE		4	/* Expect more data */
# define SPLICE_F_GIFT		8	/* Pages passed in are a gift */

__BEGIN_DECLS

/* Provide kernel hint to read ahead. */
extern ssize_t readahead (int __fd, __off64_t __offset, size_t __count) __THROW;

/* Selective file content synch'ing */
extern int sync_file_range (int __fd, __off64_t __from, __off64_t __to,
			    unsigned int __flags);

/* Splice address range into a pipe */
extern ssize_t vmsplice (int __fdout, const struct iovec *__iov,
			 size_t __count, unsigned int __flags);

/* Splice two files together */
extern ssize_t splice (int __fdin, __off64_t *__offin, int __fdout,
		       __off64_t *__offout, size_t __len,
		       unsigned int __flags);

/* In-kernel implementation of tee for pipe buffers */
extern ssize_t tee (int __fdin, int __fdout, size_t __len,
		    unsigned int __flags);

__END_DECLS
#endif
