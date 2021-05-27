/*
 * timerfd_create() / timerfd_settime() / timerfd_gettime() for uClibc
 *
 * Copyright (C) 2009 Stephan Raue <stephan@openelec.tv>
 *
 * Licensed under the LGPL v2.1, see the file COPYING.LIB in this tarball.
 */

#include <sys/syscall.h>
#include <sys/timerfd.h>

/*
 * timerfd_create()
 */
#ifdef __NR_timerfd_create
_syscall2(int, timerfd_create, int, clockid, int, flags)
#endif

/*
 * timerfd_settime()
 */
#ifdef __NR_timerfd_settime
_syscall4(int,timerfd_settime, int, ufd, int, flags, const struct itimerspec *, utmr, struct itimerspec *, otmr)
#endif

/*
 * timerfd_gettime()
 */
#ifdef __NR_timerfd_gettime
_syscall2(int, timerfd_gettime, int, ufd, struct itimerspec *, otmr)
#endif
