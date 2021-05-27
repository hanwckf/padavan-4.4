/* Copyright (C) 1996,97,98,99,2000,01,02 Free Software Foundation, Inc.
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
 *	ISO C99 Standard: 7.25
 *	Wide character classification and mapping utilities  <wctype.h>
 */

#ifndef _WCTYPE_H

#include <features.h>
#include <bits/types.h>

#ifndef __UCLIBC_HAS_WCHAR__
#error Attempted to include wctype.h when uClibc built without wide char support.
#endif

#ifndef __need_iswxxx
# define _WCTYPE_H	1

/* We try to get wint_t from <stddef.h>, but not all GCC versions define it
   there.  So define it ourselves if it remains undefined.  */
# define __need_wint_t
# include <stddef.h>
# ifndef _WINT_T
/* Integral type unchanged by default argument promotions that can
   hold any value corresponding to members of the extended character
   set, as well as at least one value that does not correspond to any
   member of the extended character set.  */
#  define _WINT_T
typedef unsigned int wint_t;
# else
#  ifdef __USE_ISOC99
__USING_NAMESPACE_C99(wint_t)
#  endif
# endif

/* Constant expression of type `wint_t' whose value does not correspond
   to any member of the extended character set.  */
# ifndef WEOF
#  define WEOF (0xffffffffu)
# endif
#endif
#undef __need_iswxxx


/* The following part is also used in the <wcsmbs.h> header when compiled
   in the Unix98 compatibility mode.  */
#ifndef __iswxxx_defined
# define __iswxxx_defined	1

__BEGIN_NAMESPACE_C99
/* Scalar type that can hold values which represent locale-specific
   character classifications.  */
/* uClibc note: glibc uses - typedef unsigned long int wctype_t; */
typedef unsigned int wctype_t;
__END_NAMESPACE_C99

# ifndef _ISwbit
#  define _ISwbit(bit)	(1 << (bit))

enum
{
  __ISwupper = 0,			/* UPPERCASE.  */
  __ISwlower = 1,			/* lowercase.  */
  __ISwalpha = 2,			/* Alphabetic.  */
  __ISwdigit = 3,			/* Numeric.  */
  __ISwxdigit = 4,			/* Hexadecimal numeric.  */
  __ISwspace = 5,			/* Whitespace.  */
  __ISwprint = 6,			/* Printing.  */
  __ISwgraph = 7,			/* Graphical.  */
  __ISwblank = 8,			/* Blank (usually SPC and TAB).  */
  __ISwcntrl = 9,			/* Control character.  */
  __ISwpunct = 10,			/* Punctuation.  */
  __ISwalnum = 11,			/* Alphanumeric.  */

  _ISwupper = _ISwbit (__ISwupper),	/* UPPERCASE.  */
  _ISwlower = _ISwbit (__ISwlower),	/* lowercase.  */
  _ISwalpha = _ISwbit (__ISwalpha),	/* Alphabetic.  */
  _ISwdigit = _ISwbit (__ISwdigit),	/* Numeric.  */
  _ISwxdigit = _ISwbit (__ISwxdigit),	/* Hexadecimal numeric.  */
  _ISwspace = _ISwbit (__ISwspace),	/* Whitespace.  */
  _ISwprint = _ISwbit (__ISwprint),	/* Printing.  */
  _ISwgraph = _ISwbit (__ISwgraph),	/* Graphical.  */
  _ISwblank = _ISwbit (__ISwblank),	/* Blank (usually SPC and TAB).  */
  _ISwcntrl = _ISwbit (__ISwcntrl),	/* Control character.  */
  _ISwpunct = _ISwbit (__ISwpunct),	/* Punctuation.  */
  _ISwalnum = _ISwbit (__ISwalnum)	/* Alphanumeric.  */
};
# endif /* Not _ISwbit  */


__BEGIN_DECLS

__BEGIN_NAMESPACE_C99
/*
 * Wide-character classification functions: 7.15.2.1.
 */

/* Test for any wide character for which `iswalpha' or `iswdigit' is
   true.  */
extern int iswalnum (wint_t __wc) __THROW;
libc_hidden_proto(iswalnum)

/* Test for any wide character for which `iswupper' or 'iswlower' is
   true, or any wide character that is one of a locale-specific set of
   wide-characters for which none of `iswcntrl', `iswdigit',
   `iswpunct', or `iswspace' is true.  */
extern int iswalpha (wint_t __wc) __THROW;

/* Test for any control wide character.  */
extern int iswcntrl (wint_t __wc) __THROW;

/* Test for any wide character that corresponds to a decimal-digit
   character.  */
extern int iswdigit (wint_t __wc) __THROW;

/* Test for any wide character for which `iswprint' is true and
   `iswspace' is false.  */
extern int iswgraph (wint_t __wc) __THROW;

/* Test for any wide character that corresponds to a lowercase letter
   or is one of a locale-specific set of wide characters for which
   none of `iswcntrl', `iswdigit', `iswpunct', or `iswspace' is true.  */
extern int iswlower (wint_t __wc) __THROW;
libc_hidden_proto(iswlower)

/* Test for any printing wide character.  */
extern int iswprint (wint_t __wc) __THROW;

/* Test for any printing wide character that is one of a
   locale-specific et of wide characters for which neither `iswspace'
   nor `iswalnum' is true.  */
extern int iswpunct (wint_t __wc) __THROW;

/* Test for any wide character that corresponds to a locale-specific
   set of wide characters for which none of `iswalnum', `iswgraph', or
   `iswpunct' is true.  */
extern int iswspace (wint_t __wc) __THROW;
libc_hidden_proto(iswspace)

/* Test for any wide character that corresponds to an uppercase letter
   or is one of a locale-specific set of wide character for which none
   of `iswcntrl', `iswdigit', `iswpunct', or `iswspace' is true.  */
extern int iswupper (wint_t __wc) __THROW;
libc_hidden_proto(iswupper)

/* Test for any wide character that corresponds to a hexadecimal-digit
   character equivalent to that performed be the functions described
   in the previous subclause.  */
extern int iswxdigit (wint_t __wc) __THROW;

/* Test for any wide character that corresponds to a standard blank
   wide character or a locale-specific set of wide characters for
   which `iswalnum' is false.  */
# ifdef __USE_ISOC99
extern int iswblank (wint_t __wc) __THROW;
# endif

/*
 * Extensible wide-character classification functions: 7.15.2.2.
 */

/* Construct value that describes a class of wide characters identified
   by the string argument PROPERTY.  */
extern wctype_t wctype (const char *__property) __THROW;
libc_hidden_proto(wctype)

/* Determine whether the wide-character WC has the property described by
   DESC.  */
extern int iswctype (wint_t __wc, wctype_t __desc) __THROW;
libc_hidden_proto(iswctype)
__END_NAMESPACE_C99


/*
 * Wide-character case-mapping functions: 7.15.3.1.
 */

__BEGIN_NAMESPACE_C99
/* Scalar type that can hold values which represent locale-specific
   character mappings.  */
/* uClibc note: glibc uses - typedef const __int32_t *wctrans_t; */
typedef unsigned int wctrans_t;
__END_NAMESPACE_C99
#ifdef __USE_GNU
__USING_NAMESPACE_C99(wctrans_t)
#endif

__BEGIN_NAMESPACE_C99
/* Converts an uppercase letter to the corresponding lowercase letter.  */
extern wint_t towlower (wint_t __wc) __THROW;
libc_hidden_proto(towlower)

/* Converts an lowercase letter to the corresponding uppercase letter.  */
extern wint_t towupper (wint_t __wc) __THROW;
libc_hidden_proto(towupper)
__END_NAMESPACE_C99

__END_DECLS

#endif	/* need iswxxx.  */


/* The remaining definitions and declarations must not appear in the
   <wcsmbs.h> header.  */
#ifdef _WCTYPE_H

/*
 * Extensible wide-character mapping functions: 7.15.3.2.
 */

__BEGIN_DECLS

__BEGIN_NAMESPACE_C99
/* Construct value that describes a mapping between wide characters
   identified by the string argument PROPERTY.  */
extern wctrans_t wctrans (const char *__property) __THROW;
libc_hidden_proto(wctrans)

/* Map the wide character WC using the mapping described by DESC.  */
extern wint_t towctrans (wint_t __wc, wctrans_t __desc) __THROW;
libc_hidden_proto(towctrans)
__END_NAMESPACE_C99

#if defined(__USE_GNU) && defined(__UCLIBC_HAS_XLOCALE__)
/* Declare the interface to extended locale model.  */
#  include <xlocale.h>

/* Test for any wide character for which `iswalpha' or `iswdigit' is
   true.  */
extern int iswalnum_l (wint_t __wc, __locale_t __locale) __THROW;

/* Test for any wide character for which `iswupper' or 'iswlower' is
   true, or any wide character that is one of a locale-specific set of
   wide-characters for which none of `iswcntrl', `iswdigit',
   `iswpunct', or `iswspace' is true.  */
extern int iswalpha_l (wint_t __wc, __locale_t __locale) __THROW;

/* Test for any control wide character.  */
extern int iswcntrl_l (wint_t __wc, __locale_t __locale) __THROW;

/* Test for any wide character that corresponds to a decimal-digit
   character.  */
extern int iswdigit_l (wint_t __wc, __locale_t __locale) __THROW;

/* Test for any wide character for which `iswprint' is true and
   `iswspace' is false.  */
extern int iswgraph_l (wint_t __wc, __locale_t __locale) __THROW;

/* Test for any wide character that corresponds to a lowercase letter
   or is one of a locale-specific set of wide characters for which
   none of `iswcntrl', `iswdigit', `iswpunct', or `iswspace' is true.  */
extern int iswlower_l (wint_t __wc, __locale_t __locale) __THROW;

/* Test for any printing wide character.  */
extern int iswprint_l (wint_t __wc, __locale_t __locale) __THROW;

/* Test for any printing wide character that is one of a
   locale-specific et of wide characters for which neither `iswspace'
   nor `iswalnum' is true.  */
extern int iswpunct_l (wint_t __wc, __locale_t __locale) __THROW;

/* Test for any wide character that corresponds to a locale-specific
   set of wide characters for which none of `iswalnum', `iswgraph', or
   `iswpunct' is true.  */
extern int iswspace_l (wint_t __wc, __locale_t __locale) __THROW;
libc_hidden_proto(iswspace_l)

/* Test for any wide character that corresponds to an uppercase letter
   or is one of a locale-specific set of wide character for which none
   of `iswcntrl', `iswdigit', `iswpunct', or `iswspace' is true.  */
extern int iswupper_l (wint_t __wc, __locale_t __locale) __THROW;

/* Test for any wide character that corresponds to a hexadecimal-digit
   character equivalent to that performed be the functions described
   in the previous subclause.  */
extern int iswxdigit_l (wint_t __wc, __locale_t __locale) __THROW;

/* Test for any wide character that corresponds to a standard blank
   wide character or a locale-specific set of wide characters for
   which `iswalnum' is false.  */
extern int iswblank_l (wint_t __wc, __locale_t __locale) __THROW;

/* Construct value that describes a class of wide characters identified
   by the string argument PROPERTY.  */
extern wctype_t wctype_l (const char *__property, __locale_t __locale)
     __THROW;

/* Determine whether the wide-character WC has the property described by
   DESC.  */
extern int iswctype_l (wint_t __wc, wctype_t __desc, __locale_t __locale)
     __THROW;
libc_hidden_proto(iswctype_l)


/*
 * Wide-character case-mapping functions.
 */

/* Converts an uppercase letter to the corresponding lowercase letter.  */
extern wint_t towlower_l (wint_t __wc, __locale_t __locale) __THROW;
libc_hidden_proto(towlower_l)

/* Converts an lowercase letter to the corresponding uppercase letter.  */
extern wint_t towupper_l (wint_t __wc, __locale_t __locale) __THROW;
libc_hidden_proto(towupper_l)

/* Construct value that describes a mapping between wide characters
   identified by the string argument PROPERTY.  */
extern wctrans_t wctrans_l (const char *__property, __locale_t __locale)
     __THROW;

/* Map the wide character WC using the mapping described by DESC.  */
extern wint_t towctrans_l (wint_t __wc, wctrans_t __desc,
			   __locale_t __locale) __THROW;
libc_hidden_proto(towctrans_l)

# endif /* Use GNU.  */

__END_DECLS

#endif	/* __WCTYPE_H defined.  */

#endif /* wctype.h  */
