/*
  srelay.h:
  $Id: srelay.h,v 1.36 2017/09/01 08:39:12 bulkstream Exp $
         common definitions.

Copyright (C) 2001-2010 Tomo.M (author).
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. Neither the name of the author nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>

#if HAVE_CONFIG_H
#include <config.h>
#endif

#if defined(FREEBSD) || defined(SOLARIS) || defined(MACOSX)
#include <sys/filio.h>
#endif
#if defined(LINUX) || defined(MACOSX)
#include <sys/ioctl.h>
#endif

#if HAVE_LIMITS_H
# include <limits.h>
#endif

#if HAVE_SYS_RESOURCE_H
# include <sys/resource.h>
#endif

#ifdef LINUX
#define __USE_XOPEN
#endif

#include <unistd.h>

#ifndef HAVE_U_INT8_T
# ifdef HAVE_UINT8_T
typedef    uint8_t         u_int8_t;
# else
typedef    unsigned char   u_int8_t;
# endif
#endif
#ifndef HAVE_U_INT16_T
# ifdef HAVE_UINT16_T
typedef    uint16_t        u_int16_t;
# else
typedef    unsigned short  u_int16_t;
# endif
#endif
#ifndef HAVE_U_INT32_T
# ifdef HAVE_UINT32_T
typedef    uint32_t        u_int32_t;
# else
typedef    unsigned long   u_int32_t;
# endif
#endif

#ifndef HAVE_SOCKLEN_T
typedef    u_int32_t    socklen_t;
#endif

#ifdef SOLARIS
# ifndef AF_INET6
#  include <v6defs.h>
# endif
#endif

#define version  "srelay 0.4.8p3 2018/08/05 (Tomo.M)"

#ifndef SYSCONFDIR
# define SYSCONFDIR "/usr/local/etc"
#endif
#define CONFIG    SYSCONFDIR "/srelay.conf"
#define PWDFILE   SYSCONFDIR "/srelay.passwd"
#define PIDFILE   "/var/run/srelay.pid"
#define WORKDIR0  "/var/run"
#define WORKDIR1  "/var/tmp"

#define S4DEFUSR  "user"

#define BUFSIZE    8192

#define PROCUID  65534
#define PROCGID  65534

#ifdef SOLARIS
# undef PROCUID
# undef PROCGID
# define PROCUID  60001
# define PROCGID  60001
#endif

#ifdef FD_SETSIZE
#define MAX_FD FD_SETSIZE
#else
#define MAX_FD 1024
#endif

/* Fixed maximum proxy route entry */
#define MAX_ROUTE  256

/* Fixed maximum listen sockets */
#define MAX_SOCKS  256

/* default socks port */
#define SOCKS_PORT    1080

/* idle timeout minutes 0 = never timeout */
#define IDLE_TIMEOUT  0

/* default maximum number of child process */
#define MAX_CHILD     100

/* Solaris did not define this */
#ifndef IPPORT_RESERVEDSTART
# define IPPORT_RESERVEDSTART 600
#endif

#ifdef USE_THREAD
# include <pthread.h>

extern pthread_t main_thread;  /* holding the main thread ID */
extern int threading;

# if (MAX_FD > 22)
#   define THREAD_LIMIT   (MAX_FD - 20)/2
# else
#   define THREAD_LIMIT    1     /* wooo !!! */
# endif
# define MAX_THREAD (THREAD_LIMIT > 64 ? 64 : THREAD_LIMIT)

# define MUTEX_LOCK(mutex) \
    if (threading) { \
      pthread_mutex_lock(&mutex); \
    }
# define MUTEX_UNLOCK(mutex) \
    if (threading) { \
      pthread_mutex_unlock(&mutex); \
    }

#else  /* !USE_THREAD */

# define MUTEX_LOCK(mutex)
# define MUTEX_UNLOCK(mutex)

#endif

/* syslog facilities */
#ifdef LOCAL_FAC
# define LOCALFAC  LOCAL_FAC
#else
# define LOCALFAC  0
#endif
#ifdef SYSLOG_FAC
#  define SYSLOGFAC (SYSLOG_FAC|LOCALFAC)
#else
#  define SYSLOGFAC (LOG_DAEMON|LOCALFAC)
#endif

enum { norm=0, warn, crit };

#define DEBUG(level, ...)           \
  do {                      \
    if (verbosity >= (level))       \
        msg_out(norm, __VA_ARGS__); \
  } while(0);

/*
 *  SOCKS protocol related definitions
 */

/*  address types */
#define S5ATIPV4    1
#define S5ATFQDN    3
#define S5ATIPV6    4

#define S4ATIPV4    1
#define S4ATFQDN    3

/* authentication  methods */
#define S5ANOAUTH     0
#define S5AGSSAPI     1
#define S5AUSRPAS     2
#define S5ACHAP       3
#define S5ANOTACC     0xff

/* SOCKSv5 request code */
#define S5REQ_CONN    1
#define S5REQ_BIND    2
#define S5REQ_UDPA    3

/* SOCKSv5 reply code */
#define S5AGRANTED    0
#define S5EGENERAL    1
#define S5ENOTALOW    2
#define S5ENETURCH    3 
#define S5EHOSURCH    4
#define S5ECREFUSE    5
#define S5ETTLEXPR    6
#define S5EUNSUPRT    7
#define S5EUSATYPE    8
#define S5EINVADDR    9

/* SOCKSv4 request code */
#define S4REQ_CONN    1
#define S4REQ_BIND    2

/* SOCKSv4 reply code */
#define S4AGRANTED    90
#define S4EGENERAL    91
#define S4ECNIDENT    92
#define S4EIVUSRID    93

typedef union {
  struct sockaddr sa;
  struct sockaddr_in v4;
  struct sockaddr_in6 v6;
  struct sockaddr_storage ss;
} SockAddr;

#define V4_LEN	sizeof(struct sockaddr_in)
#define V6_LEN	sizeof(struct sockaddr_in6)
#define SS_LEN	sizeof(struct sockaddr_storage)

typedef struct {            /* binary format of SOCKS address */
  u_int8_t      atype;
  union {
    u_int8_t    ip4[4];   /* NBO */
    struct {
      u_int8_t  ip6[16];  /* NBO */
      u_int32_t scope;
    } _ip6;
    struct {
      u_int8_t  _nlen;
      u_int8_t  _name[255];
    } _fqdn;
  } _addr;
#define v4_addr   _addr.ip4
#define v6_addr   _addr._ip6.ip6
#define v6_scope  _addr._ip6.scope
#define len_fqdn  _addr._fqdn._nlen
#define fqdn      _addr._fqdn._name
} bin_addr;

enum { ANY = 0, TCP = 6, UDP = 17 };
enum { SOCKS = 0, HTTP, SOCKSv4, SOCKSv5 };
enum { DIRECT = 0, PROXY, PROXY1 };
#define  USER_PASS_MAX   255

typedef struct {
  bin_addr	proxy;		/* proxy address */
  u_int16_t	pport;		/* proxy port (HBO) */
  int		pproto;		/* proxy protocol (0:socks, 1:HTTP, ..) */
} PROXY_INFO;

typedef struct {
  bin_addr	dest;		/* destination address */
  int		mask;		/* destination address mask len */
  u_int16_t	port_l;		/* port range low  (HostByteOrder) */
  u_int16_t	port_h;		/* port range high (HBO)*/
  int		proto;		/* IP PROTO ANY/TCP/UDP */
  int		hops;		/* relaying hops(direct=0, proxy, 2proxy*/
  PROXY_INFO    *prx;
} ROUTE_INFO;

typedef struct {
  SockAddr	addr;
  socklen_t	len;
} HADDR;

typedef struct {
  char	addr[NI_MAXHOST];	/* client (downstream peer) IP */
  char	name[NI_MAXHOST];	/* client (downstream peer) NAME */
} CL_INFO;

typedef struct {
  HADDR	mys;			/* my upstream socket name */
  HADDR	myc;			/* my downstream socket name */
  HADDR	prs;			/* upstream peer socket name */
  HADDR	prc;			/* downstream peer socket name */
} SOCK_INFO;

typedef struct {
  u_long upl;			/* upload byte count */
  u_long dnl;			/* download byte count */
  u_long bc;			/* total byte count */
  struct timeval start;		/* transfer start timestamp */
  struct timeval end;		/* transfer end timestamp */
} LOGINFO;

typedef struct {
  int		len;		/* socks udp header length */
  u_int8_t	data[300];	/* socks udp header data */
} UDPH;

typedef struct {
  int		d;		/* downward socket */
  int		u;		/* upward socket */
  SOCK_INFO	si;		/* udp-relay socket info */
  bin_addr	dest;		/* udp-relay proxy destination */
  u_int16_t	port;		/* udp-relay proxy dest port */
  UDPH		sv;		/* saved socks udp header */
} UDP_ATTR;

typedef struct {
  int		ver;		/* client socks version (4,5) */
  int		req;		/* request CONN/BIND/UDP */
  bin_addr 	dest;		/* destination address */
  u_int16_t	port;		/* destination port (host byte order) */
  u_int8_t	u_len;		/* user name length (socks v4) */
  char		user[USER_PASS_MAX];  /* user name (socks v4) */ 
  UDP_ATTR	*udp;		/* proxy info used for udp-relay */
} SOCKS_REQ;

typedef struct {
  int		s;		/* client socket */
  int		r;		/* forwarding socket */
  SOCKS_REQ	sr;		/* socks protocol request */
  int           hops;           /* proxy hops */
  int           cur;            /* current proxy hop */
  PROXY_INFO    *prx;           /* proxy route */
  SOCK_INFO	*si;		/* socket names */
} SOCKS_STATE;

struct user_pass {
  char		user[USER_PASS_MAX];
  int		ulen;
  char		pass[USER_PASS_MAX];
  int		plen;
};

#ifndef SIGFUNC_DEFINED
typedef void            (*sigfunc_t)();
#endif

#ifndef MAX
# define MAX(a,b) (((a)>(b))?(a):(b))
#endif

#ifndef __P
#if defined(__STDC__) || defined(__cplusplus)
#define __P(protos)     protos          /* full-blown ANSI C */
#else   /* !(__STDC__ || __cplusplus) */
#define __P(protos)     ()              /* traditional C preprocessor */
#endif
#endif

/*
 *   Externals.
 */

/* from main.c */
extern char *config;
extern char *ident;
extern char *pidfile;
extern char *pwdfile;
extern char *localpwd;
extern char *bindtodevice;
extern int max_child;
extern int cur_child;
#define MAX_AUTH_METH  10
extern int method_num;
extern char method_tab[];
extern int bind_restrict;
extern int same_interface;
#ifdef HAVE_LIBWRAP
extern int use_tcpwrap;
#endif /* HAVE_LIBWRAP */
extern int fg;		/* foreground operation */
extern int inetd_mode;  /* inetd mode */
extern int verbosity;   /* logging verbosity */

/* from init.c */
extern char **str_serv_sock;
extern int *serv_sock;
extern int serv_sock_ind;
extern int maxsock;
extern fd_set allsock;
extern int sig_queue[];

/* from readconf.c */
extern ROUTE_INFO *proxy_tbl;
extern int num_routes;

/* from relay.c */
extern int resolv_client;
extern u_long   idle_timeout; 

/* from util.c */
extern int forcesyslog;
extern int be_quiet;

/* from socks.c */

/* from auth-pwd.c */

/*
 *   external functions
 */

/* init.c */
extern int serv_init __P((char *));
extern void close_all_serv __P((void));
extern int queue_init __P((void));

/* main.c */

/* readconf.c */
extern int readconf __P((FILE *));
extern int getpasswd __P((bin_addr *, u_int16_t, struct user_pass *, char *));
extern int checklocalpwd __P((char *, char *, char *));

/* relay.c */
extern void relay __P((SOCKS_STATE *));

/* socks.c */
int wait_for_read __P((int, long));
ssize_t timerd_read __P((int, u_char *, size_t, int, int));
ssize_t timerd_write __P((int, u_char *, size_t, int));
extern int proto_socks __P((SOCKS_STATE *));

/* get-bind.c */
int get_bind_addr __P((bin_addr *, struct addrinfo *));

/* util.c */
extern void msg_out __P((int, const char *, ...));
extern int addr_comp __P((bin_addr *, bin_addr *, int));
extern void set_blocking __P((int));
extern int settimer __P((int));
extern void timeout __P((int));
extern void do_sigchld __P((int));
extern void do_sighup __P((int));
extern void do_sigterm __P((int));
extern void reapchild __P((void));
extern void cleanup __P((void));
extern void reload __P((void));
extern sigfunc_t setsignal __P((int, sigfunc_t));
extern int blocksignal __P((int));
extern int releasesignal __P((int));
extern void proclist_add __P((pid_t));
extern void proclist_drop __P((pid_t));
extern char *base64_encode __P((const char *));

/* auth-pwd.c */
extern int auth_pwd_server __P((int));
extern int auth_pwd_client __P((int, bin_addr *, u_int16_t));
