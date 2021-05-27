/* Copyright (C) 2004, 2005 Free Software Foundation, Inc.
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

#ifndef _MQUEUE_H
#define _MQUEUE_H	1

#include <features.h>
#if defined __UCLIBC_HAS_REALTIME__ || \
	defined __UCLIBC_HAS_ADVANCED_REALTIME__
#include <sys/types.h>
#include <fcntl.h>
#define __need_sigevent_t
#include <bits/siginfo.h>
#define __need_timespec
#include <time.h>
/* Get the definition of mqd_t and struct mq_attr.  */
#include <bits/mqueue.h>
#endif

__BEGIN_DECLS
#if defined __UCLIBC_HAS_REALTIME__
/* Establish connection between a process and a message queue NAME and
   return message queue descriptor or (mqd_t) -1 on error.  OFLAG determines
   the type of access used.  If O_CREAT is on OFLAG, the third argument is
   taken as a `mode_t', the mode of the created message queue, and the fourth
   argument is taken as `struct mq_attr *', pointer to message queue
   attributes.  If the fourth argument is NULL, default attributes are
   used.  */
extern mqd_t mq_open (const char *__name, int __oflag, ...) __THROW;

/* Removes the association between message queue descriptor MQDES and its
   message queue.  */
extern int mq_close (mqd_t __mqdes) __THROW;

/* Query status and attributes of message queue MQDES.  */
extern int mq_getattr (mqd_t __mqdes, struct mq_attr *__mqstat) __THROW;

/* Set attributes associated with message queue MQDES and if OMQSTAT is
   not NULL also query its old attributes.  */
extern int mq_setattr (mqd_t __mqdes,
		       const struct mq_attr *__restrict __mqstat,
		       struct mq_attr *__restrict __omqstat) __THROW;

/* Remove message queue named NAME.  */
extern int mq_unlink (const char *__name) __THROW;

/* Register notification issued upon message arrival to an empty
   message queue MQDES.  */
extern int mq_notify (mqd_t __mqdes, const struct sigevent *__notification)
     __THROW;

/* Receive the oldest from highest priority messages in message queue
   MQDES.  */
extern ssize_t mq_receive (mqd_t __mqdes, char *__msg_ptr, size_t __msg_len,
			   unsigned int *__msg_prio);

/* Add message pointed by MSG_PTR to message queue MQDES.  */
extern int mq_send (mqd_t __mqdes, const char *__msg_ptr, size_t __msg_len,
		    unsigned int __msg_prio);
#endif

#if defined __USE_XOPEN2K && defined __UCLIBC_HAS_ADVANCED_REALTIME__
/* Receive the oldest from highest priority messages in message queue
   MQDES, stop waiting if ABS_TIMEOUT expires.  */
extern ssize_t mq_timedreceive (mqd_t __mqdes, char *__restrict __msg_ptr,
				size_t __msg_len,
				unsigned int *__restrict __msg_prio,
				const struct timespec *__restrict __abs_timeout);

/* Add message pointed by MSG_PTR to message queue MQDES, stop blocking
   on full message queue if ABS_TIMEOUT expires.  */
extern int mq_timedsend (mqd_t __mqdes, const char *__msg_ptr,
			 size_t __msg_len, unsigned int __msg_prio,
			 const struct timespec *__abs_timeout);
#endif

__END_DECLS

#endif /* mqueue.h */
