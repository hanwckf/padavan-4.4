/*
 * Copyright (c) 1983, 1987, 1989
 *    The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Portions Copyright (c) 1996-1999 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

/*
 *	@(#)resolv.h	8.1 (Berkeley) 6/2/93
 *	$BINDId: resolv.h,v 8.31 2000/03/30 20:16:50 vixie Exp $
 */

#ifndef _RESOLV_H_

/* These headers are needed for types used in the `struct res_state'
   declaration.  */
#include <sys/types.h>
#include <netinet/in.h>

#ifndef __need_res_state
# define _RESOLV_H_

# include <sys/param.h>
# include <sys/cdefs.h>
# include <stdio.h>
# include <arpa/nameser.h>
#endif

#ifndef __res_state_defined
# define __res_state_defined

typedef enum { res_goahead, res_nextns, res_modified, res_done, res_error }
	res_sendhookact;

typedef res_sendhookact (*res_send_qhook) (struct sockaddr_in * const *ns,
					   const u_char **query,
					   int *querylen,
					   u_char *ans,
					   int anssiz,
					   int *resplen);

typedef res_sendhookact (*res_send_rhook) (const struct sockaddr_in *ns,
					   const u_char *query,
					   int querylen,
					   u_char *ans,
					   int anssiz,
					   int *resplen);

/*
 * Global defines and variables for resolver stub.
 */
# define MAXNS			3	/* max # name servers we'll track */
# define MAXDFLSRCH		3	/* # default domain levels to try */
# define MAXDNSRCH		6	/* max # domains in search path */
# define LOCALDOMAINPARTS	2	/* min levels in name that is "local" */

# define RES_TIMEOUT		5	/* min. seconds between retries */
# define MAXRESOLVSORT		10	/* number of net to sort on */
# define RES_MAXNDOTS		15	/* should reflect bit field size */
# define RES_MAXRETRANS		30	/* only for resolv.conf/RES_OPTIONS */
# define RES_MAXRETRY		5	/* only for resolv.conf/RES_OPTIONS */
# define RES_DFLRETRY		3	/* Default #/tries. */
/* (glibc uses RES_DFLRETRY of 2 but also does _res.retry = 4 sometimes (!) */
# define RES_MAXTIME		65535	/* Infinity, in milliseconds. */

/* _res (an instance of this structure) uses 0.5kb in bss
 * in "ordinary" libc's (glibc, xBSD). We want to be less wasteful.
 * We (1) shuffle and shrink some integer fields,
 * and (2) can switch off stuff we don't support.
 * Everything inside __UCLIBC_HAS_COMPAT_RES_STATE__
 * is not actually used by uclibc and can be configured off.
 * However, this will prevent some programs from building.
 * Really obscure stuff with no observed users in the wild is under
 * __UCLIBC_HAS_EXTRA_COMPAT_RES_STATE__.
 * I guess it's safe to set that to N.
 */
struct __res_state {
	/*int retrans, retry; - moved, was here */
	u_int32_t options;		/* (was: ulong) option flags - see below. */
	struct sockaddr_in nsaddr_list[MAXNS]; /* address of name server */
#define nsaddr nsaddr_list[0]		/* for backward compatibility */
	char	*dnsrch[MAXDNSRCH + 1];	/* components of domain to search */
	/*char defdname[256]; - moved, was here */
	u_int8_t nscount;		/* (was: int) number of name servers */
	u_int8_t ndots;			/* (was: unsigned:4) threshold for initial abs. query */
	u_int8_t retrans;		/* (was: int) retransmission time interval */
	u_int8_t retry;			/* (was: int) number of times to retransmit */
#ifdef __UCLIBC_HAS_COMPAT_RES_STATE__
	/* googling for "_res.defdname" says it's still sometimes used.
	 * Pity. It's huge, I want to move it to EXTRA_COMPAT... */
	char	defdname[256];		/* default domain (deprecated) */
	u_int8_t nsort;			/* (was: unsigned:4) number of elements in sort_list[] */
	u_int16_t pfcode;		/* (was: ulong) RES_PRF_ flags. Used by dig. */
	unsigned short id;		/* current message id */
	int	res_h_errno;		/* last one set for this context */
	struct {
		struct in_addr	addr;
		u_int32_t	mask;
	} sort_list[MAXRESOLVSORT];
#endif

	/* I assume that the intention is to store all
	 * DNS servers' addresses here, and duplicate in nsaddr_list[]
	 * those which have IPv4 address. In the case of IPv4 address
	 * _u._ext.nsaddrs[x] will point to some nsaddr_list[y],
	 * otherwise it will point into malloc'ed sockaddr_in6.
	 * nscount is the number of IPv4 addresses and _u._ext.nscount
	 * is the number of addresses of all kinds.
	 *
	 * If this differs from established usage and you need
	 * to change this, please describe how it is supposed to work.
	 */
	union {
		struct {
#ifdef __UCLIBC_HAS_IPV6__
			struct sockaddr_in6	*nsaddrs[MAXNS];
#endif
			u_int8_t		nscount; /* (was: u_int16_t) */
#ifdef __UCLIBC_HAS_COMPAT_RES_STATE__
			/* rather obscure, and differs in BSD and glibc */
			u_int16_t		nstimes[MAXNS];
			int			nssocks[MAXNS];
			u_int16_t		nscount6;
			u_int16_t		nsinit;
			/* glibc also has: */
			/*u_int16_t		nsmap[MAXNS];*/
			/*unsigned long long	initstamp;*/
#endif
		} _ext;
	} _u;

#ifdef __UCLIBC_HAS_EXTRA_COMPAT_RES_STATE__
	/* Truly obscure stuff.
	 * Googling for "_res.XXX" for these members
	 * turned up basically empty */
	res_send_qhook qhook;		/* query hook */
	res_send_rhook rhook;		/* response hook */
	int	_vcsock;		/* PRIVATE: for res_send VC i/o */
	unsigned _flags;		/* PRIVATE: see below */
#endif
};

typedef struct __res_state *res_state;
# undef __need_res_state
#endif

#ifdef _RESOLV_H_
/*
 * Revision information.  This is the release date in YYYYMMDD format.
 * It can change every day so the right thing to do with it is use it
 * in preprocessor commands such as "#if (__RES > 19931104)".  Do not
 * compare for equality; rather, use it to determine whether your resolver
 * is new enough to contain a certain feature.
 */

#if 0
#define	__RES	19991006
#else
#define	__RES	19960801
#endif

/*
 * Resolver configuration file.
 * Normally not present, but may contain the address of the
 * inital name server(s) to query and the domain search list.
 */

#ifndef _PATH_RESCONF
#define _PATH_RESCONF        "/etc/resolv.conf"
#endif

struct res_sym {
	int	number;		/* Identifying number, like T_MX */
	char *	name;		/* Its symbolic name, like "MX" */
	char *	humanname;	/* Its fun name, like "mail exchanger" */
};

/*
 * Resolver flags (used to be discrete per-module statics ints).
 */
#define	RES_F_VC	0x00000001	/* socket is TCP */
#define	RES_F_CONN	0x00000002	/* socket is connected */

/* res_findzonecut() options */
#define	RES_EXHAUSTIVE	0x00000001	/* always do all queries */

/*
 * Resolver options (keep these in synch with res_debug.c, please)
 * (which of these do we really implement??)
 */
#define RES_INIT	0x00000001	/* address initialized */
#define RES_DEBUG	0x00000002	/* print debug messages */
#define RES_AAONLY	0x00000004	/* authoritative answers only (!IMPL)*/
#define RES_USEVC	0x00000008	/* use virtual circuit */
#define RES_PRIMARY	0x00000010	/* query primary server only (!IMPL) */
#define RES_IGNTC	0x00000020	/* ignore trucation errors */
#define RES_RECURSE	0x00000040	/* recursion desired */
#define RES_DEFNAMES	0x00000080	/* use default domain name */
#define RES_STAYOPEN	0x00000100	/* Keep TCP socket open */
#define RES_DNSRCH	0x00000200	/* search up local domain tree */
#define	RES_INSECURE1	0x00000400	/* type 1 security disabled */
#define	RES_INSECURE2	0x00000800	/* type 2 security disabled */
#define	RES_NOALIASES	0x00001000	/* shuts off HOSTALIASES feature */
#define	RES_USE_INET6	0x00002000	/* use/map IPv6 in gethostbyname() */
#define RES_ROTATE	0x00004000	/* rotate ns list after each query */
#define	RES_NOCHECKNAME	0x00008000	/* do not check names for sanity. */
#define	RES_KEEPTSIG	0x00010000	/* do not strip TSIG records */
#define	RES_BLAST	0x00020000	/* blast all recursive servers */
#if 0
#define RES_USEBSTRING	0x00040000	/* IPv6 reverse lookup with byte
					   strings */
#define RES_NOIP6DOTINT	0x00080000	/* Do not use .ip6.int in IPv6
					   reverse lookup */

#define RES_DEFAULT	(RES_RECURSE|RES_DEFNAMES|RES_DNSRCH|RES_NOIP6DOTINT)
#else
#define RES_DEFAULT	(RES_RECURSE|RES_DEFNAMES|RES_DNSRCH)
#endif

/*
 * Resolver "pfcode" values.  Used by dig.
 */
#define RES_PRF_STATS	0x00000001
#define RES_PRF_UPDATE	0x00000002
#define RES_PRF_CLASS   0x00000004
#define RES_PRF_CMD	0x00000008
#define RES_PRF_QUES	0x00000010
#define RES_PRF_ANS	0x00000020
#define RES_PRF_AUTH	0x00000040
#define RES_PRF_ADD	0x00000080
#define RES_PRF_HEAD1	0x00000100
#define RES_PRF_HEAD2	0x00000200
#define RES_PRF_TTLID	0x00000400
#define RES_PRF_HEADX	0x00000800
#define RES_PRF_QUERY	0x00001000
#define RES_PRF_REPLY	0x00002000
#define RES_PRF_INIT	0x00004000
/*			0x00008000	*/

/* Things involving an internal (static) resolver context. */
__BEGIN_DECLS
extern struct __res_state *__res_state(void) __attribute__ ((__const__));
__END_DECLS
#define _res (*__res_state())

#if 0
#define fp_nquery		__fp_nquery
#define fp_query		__fp_query
#define hostalias		__hostalias
#define p_query			__p_query
#endif
#define res_close		__res_close
#define res_init		__res_init
#if 0
#define res_isourserver		__res_isourserver
#endif
#define res_mkquery		__res_mkquery
#define res_query		__res_query
#define res_querydomain		__res_querydomain
#define res_search		__res_search
#if 0
#define res_send		__res_send
#endif

__BEGIN_DECLS
#if 0
void		fp_nquery (const u_char *, int, FILE *) __THROW;
void		fp_query (const u_char *, FILE *) __THROW;
const char *	hostalias (const char *) __THROW;
void		p_query (const u_char *) __THROW;
#endif
#ifdef __UCLIBC_HAS_BSD_RES_CLOSE__
void		res_close (void) __THROW;
#endif
int		res_init (void) __THROW;
libc_hidden_proto(res_init)
#if 0
int		res_isourserver (const struct sockaddr_in *) __THROW;
#endif
int		res_mkquery (int, const char *, int, int, const u_char *,
			     int, const u_char *, u_char *, int) __THROW;
int		res_query (const char *, int, int, u_char *, int) __THROW;
libc_hidden_proto(res_query)
int		res_querydomain (const char *, const char *, int, int,
				 u_char *, int) __THROW;
libc_hidden_proto(res_querydomain)
int		res_search (const char *, int, int, u_char *, int) __THROW;
#if 0
int		res_send (const u_char *, int, u_char *, int) __THROW;
#endif
__END_DECLS

#ifdef __UCLIBC_HAS_BSD_B64_NTOP_B64_PTON__
#define b64_ntop		__b64_ntop
#define b64_pton		__b64_pton
#endif
#if 0
#define dn_count_labels		__dn_count_labels
#endif
#define dn_comp			__dn_comp
#define dn_expand		__dn_expand
#define dn_skipname		__dn_skipname
#define res_ninit		__res_ninit
#define res_nclose		__res_nclose
#if 0
#define fp_resstat		__fp_resstat
#define loc_aton		__loc_aton
#define loc_ntoa		__loc_ntoa
#define p_cdname		__p_cdname
#define p_cdnname		__p_cdnname
#define p_class			__p_class
#define p_fqname		__p_fqname
#define p_fqnname		__p_fqnname
#define p_option		__p_option
#define p_secstodate		__p_secstodate
#define p_section		__p_section
#define p_time			__p_time
#define p_type			__p_type
#define p_rcode			__p_rcode
#define putlong			__putlong
#define putshort		__putshort
#define res_dnok		__res_dnok
#define res_hnok		__res_hnok
#define res_hostalias		__res_hostalias
#define res_mailok		__res_mailok
#define res_nameinquery		__res_nameinquery
#define res_nmkquery		__res_nmkquery
#define res_npquery		__res_npquery
#define res_nquery		__res_nquery
#define res_nquerydomain	__res_nquerydomain
#define res_nsearch		__res_nsearch
#define res_nsend		__res_nsend
#define res_nisourserver	__res_nisourserver
#define res_ownok		__res_ownok
#define res_queriesmatch	__res_queriesmatch
#define res_randomid		__res_randomid
#define sym_ntop		__sym_ntop
#define sym_ntos		__sym_ntos
#define sym_ston		__sym_ston
#endif
__BEGIN_DECLS
#if 0
int		res_hnok (const char *) __THROW;
int		res_ownok (const char *) __THROW;
int		res_mailok (const char *) __THROW;
int		res_dnok (const char *) __THROW;
int		sym_ston (const struct res_sym *, const char *, int *) __THROW;
const char *	sym_ntos (const struct res_sym *, int, int *) __THROW;
const char *	sym_ntop (const struct res_sym *, int, int *) __THROW;
#endif
#ifdef __UCLIBC_HAS_BSD_B64_NTOP_B64_PTON__
int		b64_ntop (u_char const *, size_t, char *, size_t) __THROW;
int		b64_pton (char const *, u_char *, size_t) __THROW;
#endif
#if 0
int		loc_aton (const char *ascii, u_char *binary) __THROW;
const char *	loc_ntoa (const u_char *binary, char *ascii) __THROW;
void		putlong (u_int32_t, u_char *) __THROW;
void		putshort (u_int16_t, u_char *) __THROW;
const char *	p_class (int) __THROW;
const char *	p_time (u_int32_t) __THROW;
const char *	p_type (int) __THROW;
const char *	p_rcode (int) __THROW;
const u_char *	p_cdnname (const u_char *, const u_char *, int, FILE *)
     __THROW;
const u_char *	p_cdname (const u_char *, const u_char *, FILE *) __THROW;
const u_char *	p_fqnname (const u_char *cp, const u_char *msg,
			   int, char *, int) __THROW;
const u_char *	p_fqname (const u_char *, const u_char *, FILE *) __THROW;
const char *	p_option (u_long option) __THROW;
char *		p_secstodate (u_long) __THROW;
int		dn_count_labels (const char *) __THROW;
#endif
int		dn_skipname (const u_char *, const u_char *) __THROW;
libc_hidden_proto(dn_skipname)
int		dn_comp (const char *, u_char *, int, u_char **, u_char **)
     __THROW;
libc_hidden_proto(dn_comp)
int		dn_expand (const u_char *, const u_char *, const u_char *,
			   char *, int) __THROW;
libc_hidden_proto(dn_expand)
int		res_ninit (res_state) __THROW;
void		res_nclose (res_state) __THROW;
#if 0
u_int		res_randomid (void) __THROW;
int		res_nameinquery (const char *, int, int,
				 const u_char *, const u_char *) __THROW;
int		res_queriesmatch (const u_char *, const u_char *,
				  const u_char *, const u_char *) __THROW;
const char *	p_section (int section, int opcode) __THROW;
/* Things involving a resolver context. */
int		res_nisourserver (const res_state,
				  const struct sockaddr_in *) __THROW;
void		fp_resstat (const res_state, FILE *) __THROW;
void		res_npquery (const res_state, const u_char *, int, FILE *)
     __THROW;
const char *	res_hostalias (const res_state, const char *, char *, size_t)
     __THROW;
int		res_nquery (res_state, const char *, int, int, u_char *, int)
     __THROW;
int		res_nsearch (res_state, const char *, int, int, u_char *, int)
     __THROW;
int		res_nquerydomain (res_state, const char *, const char *, int,
				  int, u_char *, int) __THROW;
int		res_nmkquery (res_state, int, const char *, int, int,
			      const u_char *, int, const u_char *, u_char *,
			      int) __THROW;
int		res_nsend (res_state, const u_char *, int, u_char *, int)
     __THROW;
#endif
__END_DECLS

# if _LIBC
#  ifdef __UCLIBC_HAS_THREADS__
#   if defined __UCLIBC_HAS_TLS__ \
	       && (!defined NOT_IN_libc || defined IS_IN_libpthread)
#    undef _res
#    ifndef NOT_IN_libc
#     define __resp __libc_resp
#    endif
#    define _res (*__resp)
extern __thread struct __res_state *__resp attribute_tls_model_ie;
#   endif
#  else
#   undef _res
#   define _res (*__resp)
extern struct __res_state *__resp;
#  endif /* __UCLIBC_HAS_THREADS__ */
# endif /* _LIBC */

#endif /* _RESOLV_H_ */

#endif /* !_RESOLV_H_ */
