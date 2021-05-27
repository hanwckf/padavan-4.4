/* Copyright (C) 1996, 1997, 1998, 1999, 2003 Free Software Foundation, Inc.
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

/* Declaration of types and functions for shadow password suite.  */

#ifndef _SHADOW_H
#define _SHADOW_H	1

#include <features.h>

#include <paths.h>

#define	__need_FILE
#include <stdio.h>
#define __need_size_t
#include <stddef.h>

/* Paths to the user database files.  */
#define	SHADOW _PATH_SHADOW


__BEGIN_DECLS

/* Structure of the password file.  */
struct spwd
  {
    char *sp_namp;		/* Login name.  */
    char *sp_pwdp;		/* Encrypted password.  */
    long int sp_lstchg;		/* Date of last change.  */
    long int sp_min;		/* Minimum number of days between changes.  */
    long int sp_max;		/* Maximum number of days between changes.  */
    long int sp_warn;		/* Number of days to warn user to change
				   the password.  */
    long int sp_inact;		/* Number of days the account may be
				   inactive.  */
    long int sp_expire;		/* Number of days since 1970-01-01 until
				   account expires.  */
    unsigned long int sp_flag;	/* Reserved.  */
  };


/* Open database for reading.

   This function is not part of POSIX and therefore no official
   cancellation point.  But due to similarity with an POSIX interface
   or due to the implementation it is a cancellation point and
   therefore not marked with __THROW.  */
extern void setspent (void);

/* Close database.

   This function is not part of POSIX and therefore no official
   cancellation point.  But due to similarity with an POSIX interface
   or due to the implementation it is a cancellation point and
   therefore not marked with __THROW.  */
extern void endspent (void);

/* Get next entry from database, perhaps after opening the file.

   This function is not part of POSIX and therefore no official
   cancellation point.  But due to similarity with an POSIX interface
   or due to the implementation it is a cancellation point and
   therefore not marked with __THROW.  */
extern struct spwd *getspent (void);

/* Get shadow entry matching NAME.

   This function is not part of POSIX and therefore no official
   cancellation point.  But due to similarity with an POSIX interface
   or due to the implementation it is a cancellation point and
   therefore not marked with __THROW.  */
extern struct spwd *getspnam (const char *__name);

/* Read shadow entry from STRING.

   This function is not part of POSIX and therefore no official
   cancellation point.  But due to similarity with an POSIX interface
   or due to the implementation it is a cancellation point and
   therefore not marked with __THROW.  */
extern struct spwd *sgetspent (const char *__string);

/* Read next shadow entry from STREAM.

   This function is not part of POSIX and therefore no official
   cancellation point.  But due to similarity with an POSIX interface
   or due to the implementation it is a cancellation point and
   therefore not marked with __THROW.  */
extern struct spwd *fgetspent (FILE *__stream);

/* Write line containing shadow password entry to stream.

   This function is not part of POSIX and therefore no official
   cancellation point.  But due to similarity with an POSIX interface
   or due to the implementation it is a cancellation point and
   therefore not marked with __THROW.  */
extern int putspent (const struct spwd *__p, FILE *__stream);


#ifdef __USE_MISC
/* Reentrant versions of some of the functions above.

   These functions are not part of POSIX and therefore no official
   cancellation point.  But due to similarity with an POSIX interface
   or due to the implementation they are cancellation points and
   therefore not marked with __THROW.  */
extern int getspent_r (struct spwd *__result_buf, char *__buffer,
		       size_t __buflen, struct spwd **__result);
libc_hidden_proto(getspent_r)

extern int getspnam_r (const char *__name, struct spwd *__result_buf,
		       char *__buffer, size_t __buflen,
		       struct spwd **__result);
libc_hidden_proto(getspnam_r)

extern int sgetspent_r (const char *__string, struct spwd *__result_buf,
			char *__buffer, size_t __buflen,
			struct spwd **__result);
libc_hidden_proto(sgetspent_r)

extern int fgetspent_r (FILE *__stream, struct spwd *__result_buf,
			char *__buffer, size_t __buflen,
			struct spwd **__result);
libc_hidden_proto(fgetspent_r)
#endif	/* misc */


/* The simple locking functionality provided here is not suitable for
   multi-threaded applications.  */

/* Protect password file against multi writers.  */
extern int lckpwdf (void) __THROW;

/* Unlock password file.  */
extern int ulckpwdf (void) __THROW;

__END_DECLS

#endif /* shadow.h */
