/* Copyright (C) 2004       Manuel Novoa III    <mjn3@codepoet.org>
 *
 * GNU Library General Public License (LGPL) version 2 or later.
 *
 * Dedicated to Toni.  See uClibc/DEDICATION.mjn3 for details.
 */

#include "_stdio.h"

/* UNSAFE FUNCTION -- do not bother optimizing */

/* disable macro, force actual function call */
#undef getchar_unlocked

char *gets(char *s)
{
	register char *p = s;
	int c;
	__STDIO_AUTO_THREADLOCK_VAR;

	__STDIO_AUTO_THREADLOCK(stdin);

	/* Note: don't worry about performance here... this shouldn't be used!
	 * Therefore, force actual function call. */
	while (((c = getchar_unlocked()) != EOF) && ((*p = c) != '\n')) {
		++p;
	}
	if ((c == EOF) || (s == p)) {
		s = NULL;
	} else {
		*p = 0;
	}

	__STDIO_AUTO_THREADUNLOCK(stdin);

	return s;
}
