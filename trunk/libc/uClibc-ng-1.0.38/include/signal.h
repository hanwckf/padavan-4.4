/* Copyright (C) 1991-2003, 2004, 2007, 2009 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

/*
 *	ISO C99 Standard: 7.14 Signal handling <signal.h>
 */

#ifndef	_SIGNAL_H

#if !defined __need_sig_atomic_t && !defined __need_sigset_t
# define _SIGNAL_H
#endif

#include <features.h>

__BEGIN_DECLS

#include <bits/sigset.h>		/* __sigset_t, __sig_atomic_t.  */

/* An integral type that can be modified atomically, without the
   possibility of a signal arriving in the middle of the operation.  */
#if defined __need_sig_atomic_t || defined _SIGNAL_H
# ifndef __sig_atomic_t_defined
#  define __sig_atomic_t_defined
__BEGIN_NAMESPACE_STD
typedef __sig_atomic_t sig_atomic_t;
__END_NAMESPACE_STD
# endif
# undef __need_sig_atomic_t
#endif

#if defined __need_sigset_t || (defined _SIGNAL_H && defined __USE_POSIX)
# ifndef __sigset_t_defined
#  define __sigset_t_defined
typedef __sigset_t sigset_t;
# endif
# undef __need_sigset_t
#endif

#ifdef _SIGNAL_H

#include <bits/types.h>
#include <bits/signum.h>

/* Fake signal functions.  */
#define SIG_ERR    ((__sighandler_t) -1) /* Error return.  */
#define SIG_DFL    ((__sighandler_t) 0)  /* Default action.  */
#define SIG_IGN    ((__sighandler_t) 1)  /* Ignore signal.  */
#ifdef __USE_UNIX98
# define SIG_HOLD  ((__sighandler_t) 2)  /* Add signal to hold mask.  */
#endif
/* Biggest signal number + 1 (including real-time signals).  */
#ifndef _NSIG /* if arch has not defined it in bits/signum.h... */
# define _NSIG 65
#endif
#ifdef __USE_MISC
# define NSIG _NSIG
#endif
/* Real-time signal range */
#define SIGRTMIN   (__libc_current_sigrtmin())
#define SIGRTMAX   (__libc_current_sigrtmax())
/* These are the hard limits of the kernel.  These values should not be
   used directly at user level.  */
#ifndef __SIGRTMIN /* if arch has not defined it in bits/signum.h... */
# define __SIGRTMIN 32
#endif
#define __SIGRTMAX (_NSIG - 1)


#if defined __USE_XOPEN || defined __USE_XOPEN2K
# ifndef __pid_t_defined
typedef __pid_t pid_t;
#  define __pid_t_defined
# endif
#endif
#ifdef __USE_XOPEN
# ifndef __uid_t_defined
typedef __uid_t uid_t;
#  define __uid_t_defined
# endif
#endif	/* Unix98 */

#if defined __USE_POSIX199309 && defined __UCLIBC_HAS_REALTIME__
/* We need `struct timespec' later on.  */
# define __need_timespec
# include <time.h>
#endif

#if defined __USE_POSIX199309 || defined __USE_XOPEN_EXTENDED
/* Get the `siginfo_t' type plus the needed symbols.  */
# include <bits/siginfo.h>
#endif


/* Type of a signal handler.  */
typedef void (*__sighandler_t) (int);

#if defined __UCLIBC_HAS_OBSOLETE_SYSV_SIGNAL__
/* The X/Open definition of `signal' specifies the SVID semantic.  Use
   the additional function `sysv_signal' when X/Open compatibility is
   requested.  */
extern __sighandler_t __sysv_signal (int __sig, __sighandler_t __handler)
     __THROW;
# ifdef __USE_GNU
extern __sighandler_t sysv_signal (int __sig, __sighandler_t __handler)
     __THROW;
# endif
#endif /* __UCLIBC_HAS_OBSOLETE_SYSV_SIGNAL__ */

/* Set the handler for the signal SIG to HANDLER, returning the old
   handler, or SIG_ERR on error.
   By default `signal' has the BSD semantic.  */
__BEGIN_NAMESPACE_STD
#if defined __USE_BSD || !defined __UCLIBC_HAS_OBSOLETE_SYSV_SIGNAL__
extern __sighandler_t signal (int __sig, __sighandler_t __handler)
     __THROW;
libc_hidden_proto(signal)
#else
/* Make sure the used `signal' implementation is the SVID version. */
# ifdef __REDIRECT_NTH
extern __sighandler_t __REDIRECT_NTH (signal,
				      (int __sig, __sighandler_t __handler),
				      __sysv_signal);
# else
#  define signal __sysv_signal
# endif
#endif
__END_NAMESPACE_STD

#if defined __USE_XOPEN && defined __UCLIBC_SUSV3_LEGACY__
/* The X/Open definition of `signal' conflicts with the BSD version.
   So they defined another function `bsd_signal'.  */
extern __sighandler_t bsd_signal (int __sig, __sighandler_t __handler)
     __THROW;
#endif

/* Send signal SIG to process number PID.  If PID is zero,
   send SIG to all processes in the current process's process group.
   If PID is < -1, send SIG to all processes in process group - PID.  */
#ifdef __USE_POSIX
extern int kill (__pid_t __pid, int __sig) __THROW;
libc_hidden_proto(kill)
#endif

#if defined __USE_BSD || defined __USE_XOPEN_EXTENDED
/* Send SIG to all processes in process group PGRP.
   If PGRP is zero, send SIG to all processes in
   the current process's process group.  */
extern int killpg (__pid_t __pgrp, int __sig) __THROW;
#endif

__BEGIN_NAMESPACE_STD
/* Raise signal SIG, i.e., send SIG to yourself.  */
extern int raise (int __sig) __THROW;
libc_hidden_proto(raise)
__END_NAMESPACE_STD

#if 0 /*def __USE_SVID*/
/* SVID names for the same things.  */
extern __sighandler_t ssignal (int __sig, __sighandler_t __handler)
     __THROW;
extern int gsignal (int __sig) __THROW;
#endif /* Use SVID.  */

/* glibc guards the next two wrong with __USE_XOPEN2K */
#if defined __USE_MISC || defined __USE_XOPEN2K8
/* Print a message describing the meaning of the given signal number.  */
extern void psignal (int __sig, const char *__s);
#endif /* Use misc or POSIX 2008.  */

#if 0 /*def __USE_XOPEN2K8*/
/* Print a message describing the meaning of the given signal information.  */
extern void psiginfo (const siginfo_t *__pinfo, const char *__s);
#endif /* POSIX 2008.  */

#ifdef __UCLIBC_SUSV4_LEGACY__
/* The `sigpause' function has two different interfaces.  The original
   BSD definition defines the argument as a mask of the signal, while
   the more modern interface in X/Open defines it as the signal
   number.  We go with the X/Open version.

   This function is a cancellation point and therefore not marked with
   __THROW.  */

# ifdef __USE_XOPEN_EXTENDED
/* Remove a signal from the signal mask and suspend the process.  */
extern int sigpause(int __sig);
# endif
#endif /* __UCLIBC_SUSV4_LEGACY__ */

#if 0 /*def __USE_BSD*/
/* None of the following functions should be used anymore.  They are here
   only for compatibility.  A single word (`int') is not guaranteed to be
   enough to hold a complete signal mask and therefore these functions
   simply do not work in many situations.  Use `sigprocmask' instead.  */

/* Compute mask for signal SIG.  */
# define sigmask(sig)	__sigmask(sig)

/* Block signals in MASK, returning the old mask.  */
extern int sigblock (int __mask) __THROW __attribute_deprecated__;

/* Set the mask of blocked signals to MASK, returning the old mask.  */
extern int sigsetmask (int __mask) __THROW __attribute_deprecated__;

/* Return currently selected signal mask.  */
extern int siggetmask (void) __THROW __attribute_deprecated__;
#endif /* Use BSD.  */


#ifdef __USE_GNU
typedef __sighandler_t sighandler_t;
#endif

/* 4.4 BSD uses the name `sig_t' for this.  */
#ifdef __USE_BSD
typedef __sighandler_t sig_t;
#endif

#ifdef __USE_POSIX

/* Clear all signals from SET.  */
extern int sigemptyset (sigset_t *__set) __THROW __nonnull ((1));

/* Set all signals in SET.  */
extern int sigfillset (sigset_t *__set) __THROW __nonnull ((1));

/* Add SIGNO to SET.  */
extern int sigaddset (sigset_t *__set, int __signo) __THROW __nonnull ((1));
libc_hidden_proto(sigaddset)

/* Remove SIGNO from SET.  */
extern int sigdelset (sigset_t *__set, int __signo) __THROW __nonnull ((1));
libc_hidden_proto(sigdelset)

/* Return 1 if SIGNO is in SET, 0 if not.  */
extern int sigismember (const sigset_t *__set, int __signo)
     __THROW __nonnull ((1));

# ifdef __USE_GNU
/* Return non-empty value is SET is not empty.  */
extern int sigisemptyset (const sigset_t *__set) __THROW __nonnull ((1));

/* Build new signal set by combining the two inputs set using logical AND.  */
extern int sigandset (sigset_t *__set, const sigset_t *__left,
		      const sigset_t *__right) __THROW __nonnull ((1, 2, 3));

/* Build new signal set by combining the two inputs set using logical OR.  */
extern int sigorset (sigset_t *__set, const sigset_t *__left,
		     const sigset_t *__right) __THROW __nonnull ((1, 2, 3));
# endif /* GNU */

/* Get the system-specific definitions of `struct sigaction'
   and the `SA_*' and `SIG_*'. constants.  */
# include <bits/sigaction.h>

/* Get and/or change the set of blocked signals.  */
extern int sigprocmask (int __how, const sigset_t *__restrict __set,
			sigset_t *__restrict __oset) __THROW;
libc_hidden_proto(sigprocmask)

/* Change the set of blocked signals to SET,
   wait until a signal arrives, and restore the set of blocked signals.

   This function is a cancellation point and therefore not marked with
   __THROW.  */
extern int sigsuspend (const sigset_t *__set) __nonnull ((1));
#ifdef _LIBC
extern __typeof(sigsuspend) __sigsuspend_nocancel attribute_hidden;
libc_hidden_proto(sigsuspend)
#endif

/* Get and/or set the action for signal SIG.  */
extern int sigaction (int __sig, const struct sigaction *__restrict __act,
		      struct sigaction *__restrict __oact) __THROW;
#ifdef _LIBC
# if 0 /* this is in headers */
/* In uclibc, userspace struct sigaction is identical to
 * "new" struct kernel_sigaction (one from the Linux 2.1.68 kernel).
 * See sigaction.h
 */
struct old_kernel_sigaction;
extern int __syscall_sigaction(int, const struct old_kernel_sigaction *,
	struct old_kernel_sigaction *) attribute_hidden;
# else /* this is how the function is built */
extern __typeof(sigaction) __syscall_sigaction attribute_hidden;
# endif
# define __need_size_t
# include <stddef.h>
/* candidate for attribute_hidden, if NPTL would behave */
extern int __syscall_rt_sigaction(int, const struct sigaction *,
	struct sigaction *, size_t)
# ifndef __UCLIBC_HAS_THREADS_NATIVE__
		attribute_hidden
# endif
	;
extern __typeof(sigaction) __libc_sigaction;
libc_hidden_proto(sigaction)

# ifdef __mips__
#  define _KERNEL_NSIG_WORDS (_NSIG / _MIPS_SZLONG)
typedef struct {
	unsigned long sig[_KERNEL_NSIG_WORDS];
} kernel_sigset_t;
#  define __SYSCALL_SIGSET_T_SIZE (sizeof(kernel_sigset_t))
# else
#  define __SYSCALL_SIGSET_T_SIZE (_NSIG / 8)
# endif
#endif

/* Put in SET all signals that are blocked and waiting to be delivered.  */
extern int sigpending (sigset_t *__set) __THROW __nonnull ((1));


/* Select any of pending signals from SET or wait for any to arrive.

   This function is a cancellation point and therefore not marked with
   __THROW.  */
extern int sigwait (const sigset_t *__restrict __set, int *__restrict __sig)
     __nonnull ((1, 2));

# if defined __USE_POSIX199309 && defined __UCLIBC_HAS_REALTIME__
/* Select any of pending signals from SET and place information in INFO.

   This function is a cancellation point and therefore not marked with
   __THROW.  */
extern int sigwaitinfo (const sigset_t *__restrict __set,
			siginfo_t *__restrict __info) __nonnull ((1));
#ifdef _LIBC
extern __typeof(sigwaitinfo) __sigwaitinfo attribute_hidden;
#endif

/* Select any of pending signals from SET and place information in INFO.
   Wait the time specified by TIMEOUT if no signal is pending.

   This function is a cancellation point and therefore not marked with
   __THROW.  */
extern int sigtimedwait (const sigset_t *__restrict __set,
			 siginfo_t *__restrict __info,
			 const struct timespec *__restrict __timeout)
     __nonnull ((1));
#ifdef _LIBC
extern __typeof(sigtimedwait) __sigtimedwait_nocancel attribute_hidden;
libc_hidden_proto(sigtimedwait)
#endif

/* Send signal SIG to the process PID.  Associate data in VAL with the
   signal.  */
extern int sigqueue (__pid_t __pid, int __sig, const union sigval __val)
     __THROW;
# endif	/* Use POSIX 199306.  */

#endif /* Use POSIX.  */

#ifdef __USE_BSD

# ifdef __UCLIBC_HAS_SYS_SIGLIST__
/* Names of the signals.  This variable exists only for compatibility.
   Use `strsignal' instead (see <string.h>).  */
#  define _sys_siglist sys_siglist
extern const char *const sys_siglist[_NSIG];
# endif

#ifndef __UCLIBC_STRICT_HEADERS__
/* Structure passed to `sigvec'.  */
struct sigvec
  {
    __sighandler_t sv_handler;	/* Signal handler.  */
    int sv_mask;		/* Mask of signals to be blocked.  */

    int sv_flags;		/* Flags (see below).  */
# define sv_onstack	sv_flags /* 4.2 BSD compatibility.  */
  };

/* Bits in `sv_flags'.  */
# define SV_ONSTACK	(1 << 0)/* Take the signal on the signal stack.  */
# define SV_INTERRUPT	(1 << 1)/* Do not restart system calls.  */
# define SV_RESETHAND	(1 << 2)/* Reset handler to SIG_DFL on receipt.  */
#endif

/* Get machine-dependent `struct sigcontext' and signal subcodes.  */
# include <bits/sigcontext.h>

#endif /*  use BSD.  */


#if defined __USE_BSD || defined __USE_XOPEN_EXTENDED
# define __need_size_t
# include <stddef.h>

# ifdef __UCLIBC_SUSV4_LEGACY__
/* If INTERRUPT is nonzero, make signal SIG interrupt system calls
   (causing them to fail with EINTR); if INTERRUPT is zero, make system
   calls be restarted after signal SIG.  */
extern int siginterrupt (int __sig, int __interrupt) __THROW;
# endif

# include <bits/sigstack.h>
# ifdef __USE_XOPEN
/* This will define `ucontext_t' and `mcontext_t'.  */
/* SuSv4 obsoleted include/ucontext.h */
#  include <sys/ucontext.h>
# endif

# if 0
/* Run signals handlers on the stack specified by SS (if not NULL).
   If OSS is not NULL, it is filled in with the old signal stack status.
   This interface is obsolete and on many platform not implemented.  */
extern int sigstack (struct sigstack *__ss, struct sigstack *__oss)
     __THROW __attribute_deprecated__;
# endif

/* Alternate signal handler stack interface.
   This interface should always be preferred over `sigstack'.  */
extern int sigaltstack (const struct sigaltstack *__restrict __ss,
			struct sigaltstack *__restrict __oss) __THROW;

#endif /* use BSD or X/Open Unix.  */

#if defined __USE_XOPEN_EXTENDED && defined __UCLIBC_HAS_OBSOLETE_BSD_SIGNAL__
/* Simplified interface for signal management.  */

/* Add SIG to the calling process' signal mask.  */
extern int sighold (int __sig) __THROW;

/* Remove SIG from the calling process' signal mask.  */
extern int sigrelse (int __sig) __THROW;

/* Set the disposition of SIG to SIG_IGN.  */
extern int sigignore (int __sig) __THROW;

/* Set the disposition of SIG.  */
extern __sighandler_t sigset (int __sig, __sighandler_t __disp) __THROW;
#endif

#if defined __UCLIBC_HAS_THREADS__ && (defined __USE_POSIX199506 || defined __USE_UNIX98)
/* Some of the functions for handling signals in threaded programs must
   be defined here.  */
# include <bits/pthreadtypes.h>
# include <bits/sigthread.h>
#endif

/* The following functions are used internally in the C library and in
   other code which need deep insights.  */

/* Return number of available real-time signal with highest priority.  */
extern int __libc_current_sigrtmin (void) __THROW;
/* Return number of available real-time signal with lowest priority.  */
extern int __libc_current_sigrtmax (void) __THROW;

#ifdef _LIBC
extern sigset_t _sigintr attribute_hidden;
# include <string.h>
#endif
#endif /* signal.h  */

__END_DECLS

#endif /* not signal.h */
