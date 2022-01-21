/*
  readconf.c:
  $Id: readconf.c,v 1.19 2017/09/01 07:04:50 bulkstream Exp $

Copyright (C) 2001-2018 Tomo.M (author).
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

#include "srelay.h"

/* prototypes */
int setport __P((u_int16_t *, char *));
void add_proxy __P((PROXY_INFO *, PROXY_INFO **, int *));
void add_entry __P((ROUTE_INFO *, ROUTE_INFO **, int *));
void parse_err __P((int, int, char *));
int dot_to_masklen __P((char *));
int str_to_addr __P((char *, bin_addr *));

#define MAXLINE  1024
#define SP        040
#define HT        011
#define NL        012
#define VT        013
#define NP        014
#define CR        015

#define SPACES(c) (c == SP || c == HT || c == VT || c == NP)
#define DELIMS(c) (c == '\0' || c == '#' || c == ';' || c == NL || c == CR)

#define PORT_MIN  0
#define PORT_MAX  65535

ROUTE_INFO  *proxy_tbl;    /* proxy routing table */
int    num_routes;         /* number of proxy routes */

/*
  config format:
        #   comment line
	# dest_ip[/mask]          port-low-port-hi  next-proxy  [porxy-port]
	192.168.1.2/255.255.255.0   1-100           172.16.5.1  1080
	172.17.5.0/24               901             102.100.2.1 11080
	172.17.8.0/16               any
	0.0.0.0/0.0.0.0             0-32767         10.1.1.2    1080

  note:
        port-low, port-hi includes specified ports.
	port numbers must be port-low <= port-hi.
	separator of port-low and port-hi is '-'. no space chars.
	port-low = NULL (-port-hi) means 0 to port-hi.
	port-hi=NULL (port-low-) means port-low to 65535.
	           ... so, single '-' means 0 to 65535 (?).
	special port 'any' means 0-65535
	no next-proxy means "direct" connect to destination.

        destination port followed by /T, /U limits to relay TCP, UDP
	respecive.

*/

int readconf(FILE *fp)
{
  char		*p, *q, *r, *tok, *last;
  int		len;
  int		n = 0;
  char		*any = "any";
  char		buf[MAXLINE];
  ROUTE_INFO	tmp;
  ROUTE_INFO	*new_proxy_tbl = NULL;
  PROXY_INFO    prx;
  int		new_num_routes = 0;
  int           i, px;

  while (fp && fgets(buf, MAXLINE-1, fp) != NULL) {
    p = strtok_r(buf, " \t,\r\n", &last);
    if (p == NULL || *p == '#' || *p == ';') {
      /* comment line or something */
      continue;
    }
    memset(&tmp, 0, sizeof(ROUTE_INFO));
    n++;
    /* hops = 0: no next hop */
    tmp.hops = 0;

    /* destination */
    tok = p;
    q = strchr(tok, '/');
    /* check whether dest has address mask */
    if (q != NULL) {
      *q++ = '\0';  /* delimit */
      tmp.mask = 0;
      len = strlen(q);
      if ( len > 0 ) {
	if ((r = strchr(q, '.')) != NULL) { /* may be dotted decimal */
	  if ((tmp.mask = dot_to_masklen(q)) < 0) {
	      parse_err(warn, n, "parse_addr error.");
	      continue;
	  }
	} else {
	  tmp.mask = atoi(q);
	  if ( errno == ERANGE ) {
	    parse_err(warn, n, "parse mask length.");
	    continue;
	  }
	}
      }
    }

    /* set destination to tmp.dest */
    if (str_to_addr(tok, &tmp.dest) != 0) {
      parse_err(warn, n, "parse_addr error.");
      continue;
    }

    p = strtok_r(NULL, " \t,;#\r\n", &last);
    if (p == NULL) {
      parse_err(warn, n, "dest port missing or invalid, ignore this line.");
      continue;
    }

    /* relay IP PROTO default */
    tmp.proto = ANY;

    /* dest port */
    tok = p;
    if ((q = strchr(tok, '/')) != NULL ) {
      *q++ = '\0';  /* delimit */
      len = strlen(q);
      if ( len > 0 ) {
	switch((int)*q) {
	case 'T':
	case 't':
	  tmp.proto = TCP;
	  break;
	case 'U':
	case 'u':
	  tmp.proto = UDP;
	  break;
	default:
	  tmp.proto = ANY;
	  break;
	}
      }
    }
    if ((q = strchr(tok, '-')) != NULL ) {
      if (tok == q) {           /* special case '-port-hi' */
	tmp.port_l = PORT_MIN;
      } else {
	*q = '\0';
	if (setport(&(tmp.port_l), tok) < 0) {
	  continue;
	}
      }
      if (*++q == '\0') {       /* special case 'port-low-' */
	tmp.port_h = PORT_MAX;
      } else {
	if (setport(&(tmp.port_h), q) < 0) {
	  continue;
	}
      }
    } else if ((strncasecmp(tok, any, strlen(any))) == 0) {
      tmp.port_l = PORT_MIN;
      tmp.port_h = PORT_MAX;
    } else {     /* may be single port */
      if (setport(&(tmp.port_l), tok) < 0) {
	continue;
      }
      tmp.port_h = tmp.port_l;
    }
    if (tmp.port_l > tmp.port_h) {
      parse_err(warn, n, "dest port range is invalid.");
      continue;
    }

    p = strtok_r(NULL, " \t,;#\r\n", &last);
    if (p == NULL) {        /* no proxy entry */
      add_entry(&tmp, &new_proxy_tbl, &new_num_routes);
      continue;
    }

    /* ================================ */
    /* proxy */
    px = 0;
    tmp.hops = 0;
    tmp.prx = NULL;
  Proxy_Loop:
    memset(&prx, 0, sizeof(PROXY_INFO));
    tok = p;
    if (str_to_addr(tok, &prx.proxy) != 0) {
      parse_err(warn, n, "proxy address parse error.");
      continue;
    }

    /* proxy proto */
    prx.pproto = SOCKS;        /* defaults to socks proxy */

    /* proxy port */
    p = strtok_r(NULL, " \t,;#\r\n", &last);
    if (p == NULL) { /* proxy-port is ommited */
      prx.pport = SOCKS_PORT;     /* defaults to socks port */
      add_proxy(&prx, &tmp.prx, &tmp.hops);
      add_entry(&tmp, &new_proxy_tbl, &new_num_routes);
      /* remaining data is ignored */
      continue;

    } else {
      tok = p;
      q = strchr(tok, '/');
      /* check whether port has optional proto */
      if (q != NULL) {
	*q++ = '\0';  /* delimit */
	len = strlen(q);
	if (len > 0) {
	  switch((int)*q) {
	  case 'H':
	  case 'h':
	    prx.pproto = HTTP;
	    break;
	  case '4':
	    prx.pproto = SOCKSv4;
	    break;
	  case '5':
	    prx.pproto = SOCKSv5;
	    break;
	  case 'S':
	  case 's':
	  default:
	    prx.pproto = SOCKS; /* try v5->v4 */
	    break;
	  }
	}
      }
      if (setport(&(prx.pport), tok) < 0) {
	continue;
      }
    }
    add_proxy(&prx, &tmp.prx, &tmp.hops);
    p = strtok_r(NULL, " \t,;#\r\n", &last);
    if (p == NULL) {
      add_entry(&tmp, &new_proxy_tbl, &new_num_routes);
      continue;
    } else {
      goto Proxy_Loop;
    }

  }

  if ( new_num_routes <= 0 ) { /* no valid entries */
    parse_err(warn, n, "no valid entries found. using default.");
  }

  if (new_proxy_tbl != NULL) {
    if (proxy_tbl != NULL) { /* may holds previous table */
      for (i = 0; i < num_routes; i++) {
	if (proxy_tbl[i].hops > 0 && proxy_tbl[i].prx != NULL) {
	  free(proxy_tbl[i].prx);
	}
      }
      free(proxy_tbl);
    }
    proxy_tbl  = new_proxy_tbl;
    num_routes = new_num_routes;
  }
  return(0);
}

int setport(u_int16_t *to, char *str) {
  int	tport;

  tport = atoi(str);
  if ( errno == ERANGE
       || tport < PORT_MIN
       || tport > PORT_MAX) {
    parse_err(warn, -1, "parse port number.");
    return -1;
  }
  *to = tport;
  return 0;
}

void add_proxy(PROXY_INFO *add, PROXY_INFO **to, int *hops)
{
  PROXY_INFO *p = *to;

  p = realloc(p, sizeof(PROXY_INFO)*((*hops)+1));
  if (p != NULL) {
    memcpy(&p[(*hops)], add, sizeof(PROXY_INFO));
    *to = p;
    (*hops)++;
  }
}

void add_entry(ROUTE_INFO *route, ROUTE_INFO **tbl, int *ind)
{
  ROUTE_INFO *t = *tbl;

  if (*ind >= MAX_ROUTE) {
    /* error in add_entry */
    return;
  }
  t = realloc(t, sizeof(ROUTE_INFO)*((*ind)+1));
  if (t != NULL) {
    memcpy(&t[(*ind)], route, sizeof(ROUTE_INFO));
    *tbl = t;
    (*ind)++;
  }
}

void parse_err(int sev, int line, char *msg)
{
  msg_out(sev, "%s: line %d: %s\n", CONFIG, line, msg);
}

int str_to_addr(char *addr, bin_addr *dest)
{
  char     *q;
  int	   len;
  struct addrinfo hints, *res0, *res;
  int      error;
  struct sockaddr_in   *sa;
  struct sockaddr_in6  *sa6;

  /* check address type */
  /* IPv4 10.1.2.3
     IPv6 [2001:dead::beef]
     FQDN www.auuuu.com
  */
  if ( strchr(addr, ':') != NULL && (q = strchr(addr, '[')) != NULL ) {
    addr++;
    if ( (q = strchr(addr, ']')) != NULL )
      *q = '\0';
    dest->atype = S5ATIPV6;
  } else {
    dest->atype = S5ATIPV4;
    for (q = addr; q < addr+strlen(addr); q++) {
      if (strchr("0123456789.", (int)(*q)) == NULL) {
	dest->atype = S5ATFQDN;
	break;
      }
    }
  }

  error = 0;
  /* copy address to structure */
  switch (dest->atype) {
  case S5ATFQDN:
    if ((len = strlen(addr)) > 0 && len < 256) {
      dest->len_fqdn = len;
      strncpy((char *)dest->fqdn, addr, len);
    } else {
      error++;
    }
    break;

  case S5ATIPV4:
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICHOST;
    error = getaddrinfo(addr, NULL, &hints, &res0);
    if (!error) {
      int done = 0;
      for (res = res0; res; res = res->ai_next) {
	if (res->ai_family != AF_INET)
	  continue;
	sa = (struct sockaddr_in *)res->ai_addr;
	memcpy(dest->v4_addr, &sa->sin_addr, sizeof(struct in_addr));
	done = 1;
	break;
      }
      if (!done)
	error++;
      freeaddrinfo(res0);
    }
    break;

  case S5ATIPV6:
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICHOST;
    error = getaddrinfo(addr, NULL, &hints, &res0);
    if (!error) {
      int done = 0;
      for (res = res0; res; res = res->ai_next) {
	if (res->ai_family != AF_INET6)
	  continue;
	sa6 = (struct sockaddr_in6 *)res->ai_addr;
	memcpy(dest->v6_addr, &sa6->sin6_addr, sizeof(struct in6_addr));
	dest->v6_scope = sa6->sin6_scope_id;
	done = 1;
	break;
      }
      if (!done)
	error++;
      freeaddrinfo(res0);
    }
    break;
  default:
    error++;
    break;
  }
  return error;
}

int dot_to_masklen(char *addr)
{
  /* Address family dependant */

  struct addrinfo  hints, *res;
  int    i, error;
  u_int32_t xx;
  struct sockaddr_in *sin;

  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_family = AF_INET;
  hints.ai_flags = AI_NUMERICHOST;
  error = getaddrinfo(addr, NULL, &hints, &res);
  if (error) {
    return -1;
  }
  if (res->ai_family != AF_INET) {  /*** !!! ***/
    freeaddrinfo(res);
    return -1;
  }

  sin = (struct sockaddr_in *)res->ai_addr;
  xx = ntohl(sin->sin_addr.s_addr) & 0xffffffff;
  for (i=32; i>0; i--) {
    if ( xx & 1 )
      break;
    xx >>= 1;
  }
  freeaddrinfo(res);
  return i;
}


/*
  getpasswd:
    read from pwdfile, search proxy and set user and pass.
	it is little bit dangerous, that this routine will
	over-writes arguemts 'struct user_pass' contents.
    if user_pass * is null, just check existence of
    valid proxy entry.
    TODO: proxy entry should include proxy-port.
    File format:
    # comment
    # proxy-host-ip/name   user    passwd
    10.0.1.117             tomo    hogerata
    mxs001.c-wind.com      bob     foobar
*/
int getpasswd(bin_addr *proxy, u_int16_t pport, struct user_pass *up, char *path_pwdfile)
{
  FILE     *fp;
  char     buf[MAXLINE];
  char     *p, *q, *tok, *host, *last;
  u_int16_t  port = 0;
  int      len, done = 0;
  bin_addr addr;

  if (path_pwdfile == NULL) {
    return(-1);
  }

  setuid(0);
  fp = fopen(path_pwdfile, "r");
  setreuid(-1, PROCUID);

  if ( fp == NULL ) {
    DEBUG(2, "getpasswd(): cannot open pwdfile(%s)", path_pwdfile);
    return(-1);
  }

  while (fgets(buf, MAXLINE-1, fp) != NULL) {
    p = strtok_r(buf, " \t,\r\n", &last);
    if (p == NULL || *p == '#' || *p == ';') {
      /* comment line or something */
      continue;
    }
    memset(&addr, 0, sizeof(addr));
    /* proxy host ip/name entry */
    tok = p;
    q = strchr(tok, '/');
    /* check whether host entry has port number */
    if (q != NULL) {
      *q++ = '\0'; /* delimit */
      len = strlen(q);
      if (len > 0) {
	port = atoi(q);
	if ( port > 0 && port != pport) {
	  /* there is a port and is not matched pport */
	  continue;
	}
      }
    }
    len = strlen(tok);
    if (str_to_addr(tok, &addr) != 0)  /* error */
      continue;

    host = tok;
    if (addr_comp(proxy, &addr, 0) != 0) {
      /* address not match */
      continue;
    }
 
    p = strtok_r(NULL, " \t,;#\r\n", &last);
    if (p == NULL) {
      /* insufficient fields, ignore this line */
      continue;
    }

    tok = p; len = strlen(tok);
    if (len >= USER_PASS_MAX) {
      /* invalid length, ignore this line */
      continue;
    }
  
    if (up != NULL) {
      memset(up, 0, sizeof(struct user_pass));
    strncpy(up->user, tok, len);
    up->user[len] = '\0';
    up->ulen = len;
    }

    p = strtok_r(NULL, " \t,;#\r\n", &last);
    if (p == NULL) {
      /* insufficient fields, ignore this line */
      continue;
    }

    tok = p; len = strlen(tok);
    if (len >= USER_PASS_MAX) {
      /* invalid length, ignore this line */
      continue;
    }

    if (up != NULL) {
    strncpy(up->pass, tok, len);
    up->pass[len] = '\0';
    up->plen = len;
    }
    /* OK, that's done */
    done++;
    break;
  }

  fclose(fp);
  if (done) {
    DEBUG(2, "getpasswd(): matched dest host found(%s)", host);
    return(0);
  }

  /* matching entry not found or error */
  return(-1);

}

int checklocalpwd(char *user, char *pass, char *path_localpwd)
{
  FILE     *fp;
  char     buf[MAXLINE];
  char     *p, *tok;
  int      matched = 0;

  if (path_localpwd == NULL
      || (fp = fopen(path_localpwd, "r")) == NULL) {
    DEBUG(2, "checklocalpwd(): cannot open localpwd(%s)", path_localpwd);
    return(-1);
  }

  while (fgets(buf, MAXLINE-1, fp) != NULL) {
    p = tok = buf;
    if (*p == '#' || *p == ';')
      continue;    /* ignore comment line */

    if ((p = strchr(tok, ':')) == NULL)
      continue;    /* ignore invalid record */

    *p++ = '\0';
    if (strncmp(user, tok, strlen(user)) != 0)
      continue;

    tok = p;
    if ((p = strpbrk(tok, " \t\r\n")) != NULL)
      *p = '\0';

    if (strcmp(tok, crypt(pass, tok)) == 0) {
      matched++;
      break;
    }
  }

  fclose(fp);
  if (matched)
    return(0);
  /* matching entry not found or error */
  return(-1);

}

#if 0
/* how to do with #if 1 */
/*
  ./configure
  make readconf.o util.o socks.o
  gcc -pthread -lcrypt -o readconf readconf.o util.o socks.o
  ./readconf conf
*/
/* dummy */
char *pidfile;
int cur_child;
int sig_queue[2];
int threading;
pthread_t main_thread;
char *config;
int fg;
int verbosity = 0;
int bind_restrict = 1;
int method_num;
char method_tab[1];
char *pwdfile;
int same_interface = 0;
char *bindtodevice = NULL;
/* dummy */

struct host_info {
  char    host[NI_MAXHOST];
  char    port[NI_MAXSERV];
};

int auth_pwd_client (int a, bin_addr *b, u_int16_t c) { return(0); }
int auth_pwd_server (int a) { return(0); }
int get_bind_addr (bin_addr *a, struct addrinfo *b) { return(0); }

extern int resolv_host (bin_addr *, u_int16_t, struct host_info *);

void dump_entry()
{
  int    i, j;
  struct host_info dest;

  for (i=0; i < num_routes; i++) {
    fprintf(stdout, "--- %d ---\n", i);
    fprintf(stdout, "atype: %d\n", proxy_tbl[i].dest.atype);

    resolv_host(&proxy_tbl[i].dest, 0, &dest);
    fprintf(stdout, "dest: %s\n", dest.host);

    fprintf(stdout, "mask: %d\n", proxy_tbl[i].mask);
    fprintf(stdout, "port_l: %u\n", proxy_tbl[i].port_l);
    fprintf(stdout, "port_h: %u\n", proxy_tbl[i].port_h);

    fprintf(stdout, "hops: %d\n", proxy_tbl[i].hops);

    for (j=0; j<proxy_tbl[i].hops; j++) {
      resolv_host(&proxy_tbl[i].prx[j].proxy, 0, &dest);
      fprintf(stdout, "proxy[%d]: %s\n", j, dest.host);
      fprintf(stdout, "pport[%d]: %u\n", j, proxy_tbl[i].prx[j].pport);
      fprintf(stdout, "pproto[%d]: %d\n", j, proxy_tbl[i].prx[j].pproto);
    }
  }
}

/*
void checkpwd(bin_addr *proxy, struct user_pass *up)
{
  if (getasswd(proxy, up) == 0) {
    fprintf(stdout, "%s %s\n", up->user, up->pass);
  }

}
*/

int main(int argc, char **argv) {

  FILE *fp;

  /*
  bin_addr proxy;
  struct user_pass up;
  char buf[128];
  */

  if (argc < 2) {
    fprintf(stderr, "need args\n");
    return(1);
  }

  if ( (fp = fopen(argv[1], "r")) == NULL ) {
    fprintf(stderr, "can't open %s\n", argv[1]);
    return(1);
  }
  printf("proxy_tbl: %p\n", proxy_tbl);

  readconf(fp);
  fclose(fp);

  printf("proxy_tbl: %p\n", proxy_tbl);

  if (num_routes > 0)
    dump_entry();
  return(0);

  /*  
  int r = str_to_addr("ab7.def.com", &proxy);
  printf("%d %s\n", r, inet_ntop(AF_INET, proxy.v4_addr, buf, sizeof buf));
  checkpwd(&proxy, &up);
  */

}
#endif
