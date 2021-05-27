/* Copyright (C) 1996-2002, 2003, 2004, 2009 Free Software Foundation, Inc.
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

/* All data returned by the network data base library are supplied in
   host order and returned in network order (suitable for use in
   system calls).  */

#ifndef	_NETDB_H
#define	_NETDB_H	1

#include <features.h>

#include <netinet/in.h>
#include <stdint.h>

#ifdef __USE_GNU
# define __need_sigevent_t
# include <bits/siginfo.h>
# define __need_timespec
# include <time.h>
#endif

#include <bits/netdb.h>

/* Absolute file name for network data base files.  */
#define	_PATH_HEQUIV		"/etc/hosts.equiv"
#define	_PATH_HOSTS		"/etc/hosts"
#define	_PATH_NETWORKS		"/etc/networks"
#define	_PATH_NSSWITCH_CONF	"/etc/nsswitch.conf"
#define	_PATH_PROTOCOLS		"/etc/protocols"
#define	_PATH_SERVICES		"/etc/services"


__BEGIN_DECLS

/* Error status for non-reentrant lookup functions.
   We use a macro to access always the thread-specific `h_errno' variable.  */
#define h_errno (*__h_errno_location ())

/* Function to get address of global `h_errno' variable.  */
extern int *__h_errno_location (void) __THROW __attribute__ ((__const__));

/* Macros for accessing h_errno from inside libc.  */
#ifdef _LIBC
# ifdef __UCLIBC_HAS_THREADS__
#  if defined __UCLIBC_HAS_TLS__ \
             && (!defined NOT_IN_libc || defined IS_IN_libpthread)
#   undef h_errno
#   ifndef NOT_IN_libc
#    define h_errno __libc_h_errno
#   else
#    define h_errno h_errno     /* For #ifndef h_errno tests.  */
#   endif
extern __thread int h_errno attribute_tls_model_ie;
#   define __set_h_errno(x) (h_errno = (x))
#  else
static inline int __set_h_errno (int __err)
{
       return *__h_errno_location () = __err;
}
#  endif /* __UCLIBC_HAS_TLS__ */
# else
#  undef h_errno
#  define __set_h_errno(x) (h_errno = (x))
extern int h_errno;
# endif /* __UCLIBC_HAS_THREADS__ */
#endif /* _LIBC */

/* Possible values left in `h_errno'.  */
#define	HOST_NOT_FOUND	1	/* Authoritative Answer Host not found.  */
#define	TRY_AGAIN	2	/* Non-Authoritative Host not found,
				   or SERVERFAIL.  */
#define	NO_RECOVERY	3	/* Non recoverable errors, FORMERR, REFUSED,
				   NOTIMP.  */
#define	NO_DATA		4	/* Valid name, no data record of requested
				   type.  */
#if defined __USE_MISC || defined __USE_GNU
# define NETDB_INTERNAL	-1	/* See errno.  */
# define NETDB_SUCCESS	0	/* No problem.  */
# define NO_ADDRESS	NO_DATA	/* No address, look for MX record.  */
#endif

#ifdef __USE_XOPEN2K
/* Highest reserved Internet port number.  */
# define IPPORT_RESERVED	1024
#endif

#ifdef __USE_GNU
/* Scope delimiter for getaddrinfo(), getnameinfo().  */
# define SCOPE_DELIMITER	'%'
#endif

#if defined __USE_MISC || defined __USE_GNU
/* Print error indicated by `h_errno' variable on standard error.  STR
   if non-null is printed before the error string.  */
extern void herror (const char *__str) __THROW;
libc_hidden_proto(herror)

/* Return string associated with error ERR_NUM.  */
extern const char *hstrerror (int __err_num) __THROW;
#endif


/* Description of data base entry for a single host.  */
struct hostent
{
  char *h_name;			/* Official name of host.  */
  char **h_aliases;		/* Alias list.  */
  int h_addrtype;		/* Host address type.  */
  int h_length;			/* Length of address.  */
  char **h_addr_list;		/* List of addresses from name server.  */
#if defined __USE_MISC || defined __USE_GNU
# define	h_addr	h_addr_list[0] /* Address, for backward compatibility.*/
#endif
};

/* Open host data base files and mark them as staying open even after
   a later search if STAY_OPEN is non-zero.

   This function is a possible cancellation point and therefore not
   marked with __THROW.  */
extern void sethostent (int __stay_open);

/* Close host data base files and clear `stay open' flag.

   This function is a possible cancellation point and therefore not
   marked with __THROW.  */
extern void endhostent (void);

/* Get next entry from host data base file.  Open data base if
   necessary.

   This function is a possible cancellation point and therefore not
   marked with __THROW.  */
extern struct hostent *gethostent (void);

/* Return entry from host data base which address match ADDR with
   length LEN and type TYPE.

   This function is a possible cancellation point and therefore not
   marked with __THROW.  */
extern struct hostent *gethostbyaddr (const void *__addr, __socklen_t __len,
				      int __type);
libc_hidden_proto(gethostbyaddr)

/* Return entry from host data base for host with NAME.

   This function is a possible cancellation point and therefore not
   marked with __THROW.  */
extern struct hostent *gethostbyname (const char *__name);
libc_hidden_proto(gethostbyname)

#ifdef __USE_MISC
/* Return entry from host data base for host with NAME.  AF must be
   set to the address type which is `AF_INET' for IPv4 or `AF_INET6'
   for IPv6.

   This function is not part of POSIX and therefore no official
   cancellation point.  But due to similarity with an POSIX interface
   or due to the implementation it is a cancellation point and
   therefore not marked with __THROW.  */
extern struct hostent *gethostbyname2 (const char *__name, int __af);
libc_hidden_proto(gethostbyname2)

/* Reentrant versions of the functions above.  The additional
   arguments specify a buffer of BUFLEN starting at BUF.  The last
   argument is a pointer to a variable which gets the value which
   would be stored in the global variable `herrno' by the
   non-reentrant functions.

   These functions are not part of POSIX and therefore no official
   cancellation point.  But due to similarity with an POSIX interface
   or due to the implementation they are cancellation points and
   therefore not marked with __THROW.  */
extern int gethostent_r (struct hostent *__restrict __result_buf,
			 char *__restrict __buf, size_t __buflen,
			 struct hostent **__restrict __result,
			 int *__restrict __h_errnop);
libc_hidden_proto(gethostent_r)

extern int gethostbyaddr_r (const void *__restrict __addr, __socklen_t __len,
			    int __type,
			    struct hostent *__restrict __result_buf,
			    char *__restrict __buf, size_t __buflen,
			    struct hostent **__restrict __result,
			    int *__restrict __h_errnop);
libc_hidden_proto(gethostbyaddr_r)

extern int gethostbyname_r (const char *__restrict __name,
			    struct hostent *__restrict __result_buf,
			    char *__restrict __buf, size_t __buflen,
			    struct hostent **__restrict __result,
			    int *__restrict __h_errnop);
libc_hidden_proto(gethostbyname_r)

extern int gethostbyname2_r (const char *__restrict __name, int __af,
			     struct hostent *__restrict __result_buf,
			     char *__restrict __buf, size_t __buflen,
			     struct hostent **__restrict __result,
			     int *__restrict __h_errnop);
libc_hidden_proto(gethostbyname2_r)
#endif	/* misc */


/* Open network data base files and mark them as staying open even
   after a later search if STAY_OPEN is non-zero.

   This function is a possible cancellation point and therefore not
   marked with __THROW.  */
extern void setnetent (int __stay_open);
libc_hidden_proto(setnetent)

/* Close network data base files and clear `stay open' flag.

   This function is a possible cancellation point and therefore not
   marked with __THROW.  */
extern void endnetent (void);
libc_hidden_proto(endnetent)

/* Get next entry from network data base file.  Open data base if
   necessary.

   This function is a possible cancellation point and therefore not
   marked with __THROW.  */
extern struct netent *getnetent (void);

/* Return entry from network data base which address match NET and
   type TYPE.

   This function is a possible cancellation point and therefore not
   marked with __THROW.  */
extern struct netent *getnetbyaddr (uint32_t __net, int __type);

/* Return entry from network data base for network with NAME.

   This function is a possible cancellation point and therefore not
   marked with __THROW.  */
extern struct netent *getnetbyname (const char *__name);

#ifdef	__USE_MISC
/* Reentrant versions of the functions above.  The additional
   arguments specify a buffer of BUFLEN starting at BUF.  The last
   argument is a pointer to a variable which gets the value which
   would be stored in the global variable `herrno' by the
   non-reentrant functions.

   These functions are not part of POSIX and therefore no official
   cancellation point.  But due to similarity with an POSIX interface
   or due to the implementation they are cancellation points and
   therefore not marked with __THROW.  */
extern int getnetent_r (struct netent *__restrict __result_buf,
			char *__restrict __buf, size_t __buflen,
			struct netent **__restrict __result,
			int *__restrict __h_errnop);
libc_hidden_proto(getnetent_r)

extern int getnetbyaddr_r (uint32_t __net, int __type,
			   struct netent *__restrict __result_buf,
			   char *__restrict __buf, size_t __buflen,
			   struct netent **__restrict __result,
			   int *__restrict __h_errnop);
libc_hidden_proto(getnetbyaddr_r)

extern int getnetbyname_r (const char *__restrict __name,
			   struct netent *__restrict __result_buf,
			   char *__restrict __buf, size_t __buflen,
			   struct netent **__restrict __result,
			   int *__restrict __h_errnop);
libc_hidden_proto(getnetbyname_r)
#endif	/* __USE_MISC */


/* Description of data base entry for a single service.  */
struct servent
{
  char *s_name;			/* Official service name.  */
  char **s_aliases;		/* Alias list.  */
  int s_port;			/* Port number.  */
  char *s_proto;		/* Protocol to use.  */
};

/* Open service data base files and mark them as staying open even
   after a later search if STAY_OPEN is non-zero.

   This function is a possible cancellation point and therefore not
   marked with __THROW.  */
extern void setservent (int __stay_open);
libc_hidden_proto(setservent)

/* Close service data base files and clear `stay open' flag.

   This function is a possible cancellation point and therefore not
   marked with __THROW.  */
extern void endservent (void);
libc_hidden_proto(endservent)

/* Get next entry from service data base file.  Open data base if
   necessary.

   This function is a possible cancellation point and therefore not
   marked with __THROW.  */
extern struct servent *getservent (void);

/* Return entry from network data base for network with NAME and
   protocol PROTO.

   This function is a possible cancellation point and therefore not
   marked with __THROW.  */
extern struct servent *getservbyname (const char *__name,
				      const char *__proto);

/* Return entry from service data base which matches port PORT and
   protocol PROTO.

   This function is a possible cancellation point and therefore not
   marked with __THROW.  */
extern struct servent *getservbyport (int __port, const char *__proto);
libc_hidden_proto(getservbyport)


#ifdef	__USE_MISC
/* Reentrant versions of the functions above.  The additional
   arguments specify a buffer of BUFLEN starting at BUF.

   These functions are not part of POSIX and therefore no official
   cancellation point.  But due to similarity with an POSIX interface
   or due to the implementation they are cancellation points and
   therefore not marked with __THROW.  */
extern int getservent_r (struct servent *__restrict __result_buf,
			 char *__restrict __buf, size_t __buflen,
			 struct servent **__restrict __result);
libc_hidden_proto(getservent_r)

extern int getservbyname_r (const char *__restrict __name,
			    const char *__restrict __proto,
			    struct servent *__restrict __result_buf,
			    char *__restrict __buf, size_t __buflen,
			    struct servent **__restrict __result);
libc_hidden_proto(getservbyname_r)

extern int getservbyport_r (int __port, const char *__restrict __proto,
			    struct servent *__restrict __result_buf,
			    char *__restrict __buf, size_t __buflen,
			    struct servent **__restrict __result);
libc_hidden_proto(getservbyport_r)
#endif	/* misc */


/* Description of data base entry for a single service.  */
struct protoent
{
  char *p_name;			/* Official protocol name.  */
  char **p_aliases;		/* Alias list.  */
  int p_proto;			/* Protocol number.  */
};

/* Open protocol data base files and mark them as staying open even
   after a later search if STAY_OPEN is non-zero.

   This function is a possible cancellation point and therefore not
   marked with __THROW.  */
extern void setprotoent (int __stay_open);
libc_hidden_proto(setprotoent)

/* Close protocol data base files and clear `stay open' flag.

   This function is a possible cancellation point and therefore not
   marked with __THROW.  */
extern void endprotoent (void);
libc_hidden_proto(endprotoent)

/* Get next entry from protocol data base file.  Open data base if
   necessary.

   This function is a possible cancellation point and therefore not
   marked with __THROW.  */
extern struct protoent *getprotoent (void);

/* Return entry from protocol data base for network with NAME.

   This function is a possible cancellation point and therefore not
   marked with __THROW.  */
extern struct protoent *getprotobyname (const char *__name);

/* Return entry from protocol data base which number is PROTO.

   This function is a possible cancellation point and therefore not
   marked with __THROW.  */
extern struct protoent *getprotobynumber (int __proto);


#ifdef	__USE_MISC
/* Reentrant versions of the functions above.  The additional
   arguments specify a buffer of BUFLEN starting at BUF.

   These functions are not part of POSIX and therefore no official
   cancellation point.  But due to similarity with an POSIX interface
   or due to the implementation they are cancellation points and
   therefore not marked with __THROW.  */
extern int getprotoent_r (struct protoent *__restrict __result_buf,
			  char *__restrict __buf, size_t __buflen,
			  struct protoent **__restrict __result);
libc_hidden_proto(getprotoent_r)

extern int getprotobyname_r (const char *__restrict __name,
			     struct protoent *__restrict __result_buf,
			     char *__restrict __buf, size_t __buflen,
			     struct protoent **__restrict __result);
libc_hidden_proto(getprotobyname_r)

extern int getprotobynumber_r (int __proto,
			       struct protoent *__restrict __result_buf,
			       char *__restrict __buf, size_t __buflen,
			       struct protoent **__restrict __result);
libc_hidden_proto(getprotobynumber_r)


#ifdef __UCLIBC_HAS_NETGROUP__
/* Establish network group NETGROUP for enumeration.

   This function is not part of POSIX and therefore no official
   cancellation point.  But due to similarity with an POSIX interface
   or due to the implementation it is a cancellation point and
   therefore not marked with __THROW.  */
extern int setnetgrent (const char *__netgroup);

/* Free all space allocated by previous `setnetgrent' call.

   This function is not part of POSIX and therefore no official
   cancellation point.  But due to similarity with an POSIX interface
   or due to the implementation it is a cancellation point and
   therefore not marked with __THROW.  */
extern void endnetgrent (void);

/* Get next member of netgroup established by last `setnetgrent' call
   and return pointers to elements in HOSTP, USERP, and DOMAINP.

   This function is not part of POSIX and therefore no official
   cancellation point.  But due to similarity with an POSIX interface
   or due to the implementation it is a cancellation point and
   therefore not marked with __THROW.  */
extern int getnetgrent (char **__restrict __hostp,
			char **__restrict __userp,
			char **__restrict __domainp);


/* Test whether NETGROUP contains the triple (HOST,USER,DOMAIN).

   This function is not part of POSIX and therefore no official
   cancellation point.  But due to similarity with an POSIX interface
   or due to the implementation it is a cancellation point and
   therefore not marked with __THROW.  */
extern int innetgr (const char *__netgroup, const char *__host,
		    const char *__user, const char *__domain);

/* Reentrant version of `getnetgrent' where result is placed in BUFFER.

   This function is not part of POSIX and therefore no official
   cancellation point.  But due to similarity with an POSIX interface
   or due to the implementation it is a cancellation point and
   therefore not marked with __THROW.  */
extern int getnetgrent_r (char **__restrict __hostp,
			  char **__restrict __userp,
			  char **__restrict __domainp,
			  char *__restrict __buffer, size_t __buflen);
#endif	/* UCLIBC_HAS_NETGROUP */
#endif	/* misc */

#ifdef __UCLIBC__
/* ruserpass - remote password check.
   This function also exists in glibc but is undocumented */
extern int ruserpass(const char *host, const char **aname, const char **apass);
libc_hidden_proto(ruserpass)
#endif

#ifdef __USE_BSD
/* Call `rshd' at port RPORT on remote machine *AHOST to execute CMD.
   The local user is LOCUSER, on the remote machine the command is
   executed as REMUSER.  In *FD2P the descriptor to the socket for the
   connection is returned.  The caller must have the right to use a
   reserved port.  When the function returns *AHOST contains the
   official host name.

   This function is not part of POSIX and therefore no official
   cancellation point.  But due to similarity with an POSIX interface
   or due to the implementation it is a cancellation point and
   therefore not marked with __THROW.  */
extern int rcmd (char **__restrict __ahost, unsigned short int __rport,
		 const char *__restrict __locuser,
		 const char *__restrict __remuser,
		 const char *__restrict __cmd, int *__restrict __fd2p);

#if 0
/* FIXME */
/* This is the equivalent function where the protocol can be selected
   and which therefore can be used for IPv6.

   This function is not part of POSIX and therefore no official
   cancellation point.  But due to similarity with an POSIX interface
   or due to the implementation it is a cancellation point and
   therefore not marked with __THROW.  */
extern int rcmd_af (char **__restrict __ahost, unsigned short int __rport,
		    const char *__restrict __locuser,
		    const char *__restrict __remuser,
		    const char *__restrict __cmd, int *__restrict __fd2p,
		    sa_family_t __af);
#endif

/* Call `rexecd' at port RPORT on remote machine *AHOST to execute
   CMD.  The process runs at the remote machine using the ID of user
   NAME whose cleartext password is PASSWD.  In *FD2P the descriptor
   to the socket for the connection is returned.  When the function
   returns *AHOST contains the official host name.

   This function is not part of POSIX and therefore no official
   cancellation point.  But due to similarity with an POSIX interface
   or due to the implementation it is a cancellation point and
   therefore not marked with __THROW.  */
extern int rexec (char **__restrict __ahost, int __rport,
		  const char *__restrict __name,
		  const char *__restrict __pass,
		  const char *__restrict __cmd, int *__restrict __fd2p);

/* This is the equivalent function where the protocol can be selected
   and which therefore can be used for IPv6.

   This function is not part of POSIX and therefore no official
   cancellation point.  But due to similarity with an POSIX interface
   or due to the implementation it is a cancellation point and
   therefore not marked with __THROW.  */
extern int rexec_af (char **__restrict __ahost, int __rport,
		     const char *__restrict __name,
		     const char *__restrict __pass,
		     const char *__restrict __cmd, int *__restrict __fd2p,
		     sa_family_t __af);
libc_hidden_proto(rexec_af)

/* Check whether user REMUSER on system RHOST is allowed to login as LOCUSER.
   If SUSER is not zero the user tries to become superuser.  Return 0 if
   it is possible.

   This function is not part of POSIX and therefore no official
   cancellation point.  But due to similarity with an POSIX interface
   or due to the implementation it is a cancellation point and
   therefore not marked with __THROW.  */
extern int ruserok (const char *__rhost, int __suser,
		    const char *__remuser, const char *__locuser);

#if 0
/* FIXME */
/* This is the equivalent function where the protocol can be selected
   and which therefore can be used for IPv6.

   This function is not part of POSIX and therefore no official
   cancellation point.  But due to similarity with an POSIX interface
   or due to the implementation it is a cancellation point and
   therefore not marked with __THROW.  */
extern int ruserok_af (const char *__rhost, int __suser,
		       const char *__remuser, const char *__locuser,
		       sa_family_t __af);
#endif

/* Try to allocate reserved port, returning a descriptor for a socket opened
   at this port or -1 if unsuccessful.  The search for an available port
   will start at ALPORT and continues with lower numbers.

   This function is not part of POSIX and therefore no official
   cancellation point.  But due to similarity with an POSIX interface
   or due to the implementation it is a cancellation point and
   therefore not marked with __THROW.  */
extern int rresvport (int *__alport);
libc_hidden_proto(rresvport)

#if 0
/* FIXME */
/* This is the equivalent function where the protocol can be selected
   and which therefore can be used for IPv6.

   This function is not part of POSIX and therefore no official
   cancellation point.  But due to similarity with an POSIX interface
   or due to the implementation it is a cancellation point and
   therefore not marked with __THROW.  */
extern int rresvport_af (int *__alport, sa_family_t __af);
#endif
#endif


/* Extension from POSIX.1g.  */
#ifdef	__USE_POSIX
/* Structure to contain information about address of a service provider.  */
struct addrinfo
{
  int ai_flags;			/* Input flags.  */
  int ai_family;		/* Protocol family for socket.  */
  int ai_socktype;		/* Socket type.  */
  int ai_protocol;		/* Protocol for socket.  */
  socklen_t ai_addrlen;		/* Length of socket address.  */
  struct sockaddr *ai_addr;	/* Socket address for socket.  */
  char *ai_canonname;		/* Canonical name for service location.  */
  struct addrinfo *ai_next;	/* Pointer to next in list.  */
};

/* Possible values for `ai_flags' field in `addrinfo' structure.  */
# define AI_PASSIVE	0x0001	/* Socket address is intended for `bind'.  */
# define AI_CANONNAME	0x0002	/* Request for canonical name.  */
# define AI_NUMERICHOST	0x0004	/* Don't use name resolution.  */
# define AI_V4MAPPED	0x0008	/* IPv4 mapped addresses are acceptable.  */
# define AI_ALL		0x0010	/* Return IPv4 mapped and IPv6 addresses.  */
# define AI_ADDRCONFIG	0x0020	/* Use configuration of this host to choose
				   returned address type..  */
# define AI_NUMERICSERV	0x0400	/* Don't use name resolution.  */

/* Error values for `getaddrinfo' function.  */
# define EAI_BADFLAGS	  -1	/* Invalid value for `ai_flags' field.  */
# define EAI_NONAME	  -2	/* NAME or SERVICE is unknown.  */
# define EAI_AGAIN	  -3	/* Temporary failure in name resolution.  */
# define EAI_FAIL	  -4	/* Non-recoverable failure in name res.  */
# define EAI_FAMILY	  -6	/* `ai_family' not supported.  */
# define EAI_SOCKTYPE	  -7	/* `ai_socktype' not supported.  */
# define EAI_SERVICE	  -8	/* SERVICE not supported for `ai_socktype'.  */
# define EAI_MEMORY	  -10	/* Memory allocation failure.  */
# define EAI_SYSTEM	  -11	/* System error returned in `errno'.  */
# define EAI_OVERFLOW	  -12	/* Argument buffer overflow.  */
# ifdef __USE_GNU
#  define EAI_NODATA	  -5	/* No address associated with NAME.  */
#  define EAI_ADDRFAMILY  -9	/* Address family for NAME not supported.  */
#  define EAI_INPROGRESS  -100	/* Processing request in progress.  */
#  define EAI_CANCELED	  -101	/* Request canceled.  */
#  define EAI_NOTCANCELED -102	/* Request not canceled.  */
#  define EAI_ALLDONE	  -103	/* All requests done.  */
#  define EAI_INTR	  -104	/* Interrupted by a signal.  */
# endif

# ifdef __USE_MISC
#  define NI_MAXHOST      1025
#  define NI_MAXSERV      32
# endif

# define NI_NUMERICHOST	1	/* Don't try to look up hostname.  */
# define NI_NUMERICSERV 2	/* Don't convert port number to name.  */
# define NI_NOFQDN	4	/* Only return nodename portion.  */
# define NI_NAMEREQD	8	/* Don't return numeric addresses.  */
# define NI_DGRAM	16	/* Look up UDP service rather than TCP.  */
#if 0 /* uClibc: not implemented */
/* # ifdef __USE_GNU */
#  define NI_IDN	32	/* Convert name from IDN format.  */
#  define NI_IDN_ALLOW_UNASSIGNED 64 /* Don't reject unassigned Unicode
					code points.  */
#  define NI_IDN_USE_STD3_ASCII_RULES 128 /* Validate strings according to
					     STD3 rules.  */
# endif

/* Translate name of a service location and/or a service name to set of
   socket addresses.

   This function is a possible cancellation point and therefore not
   marked with __THROW.  */
extern int getaddrinfo (const char *__restrict __name,
			const char *__restrict __service,
			const struct addrinfo *__restrict __req,
			struct addrinfo **__restrict __pai);
libc_hidden_proto(getaddrinfo)

/* Free `addrinfo' structure AI including associated storage.  */
extern void freeaddrinfo (struct addrinfo *__ai) __THROW;
libc_hidden_proto(freeaddrinfo)

/* Convert error return from getaddrinfo() to a string.  */
extern const char *gai_strerror (int __ecode) __THROW;

/* Translate a socket address to a location and service name.

   This function is a possible cancellation point and therefore not
   marked with __THROW.  */
extern int getnameinfo (const struct sockaddr *__restrict __sa,
			socklen_t __salen, char *__restrict __host,
			socklen_t __hostlen, char *__restrict __serv,
			socklen_t __servlen, unsigned int __flags);
libc_hidden_proto(getnameinfo)
#endif	/* POSIX */

__END_DECLS

#endif	/* netdb.h */
