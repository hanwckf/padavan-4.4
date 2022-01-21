/*
  socks.c:
  $Id: socks.c,v 1.30 2017/09/01 07:04:50 bulkstream Exp $

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

#include "srelay.h"

#define TIMEOUTSEC    30

#define GEN_ERR_REP(s, v) \
    switch ((v)) { \
    case 0x04:\
      socks_rep((s), (v), S4EGENERAL, 0);\
      close((s));\
      break;\
    case 0x05:\
      socks_rep((s), (v), S5EGENERAL, 0);\
      close((s));\
      break;\
    case -1:\
      break;\
    default:\
      break;\
    }\


#define POSITIVE_REP(s, v, a) \
    switch ((v)) { \
    case 0x04:\
      error = socks_rep((s), (v), S4AGRANTED, (a));\
      break;\
    case 0x05:\
      error = socks_rep((s), (v), S5AGRANTED, (a));\
      break;\
    case -1:\
      error = 0;\
      break;\
    default:\
      error = -1;\
      break;\
    }\

/* ss: pointer of struct sockaddr_storage */
#define SET_SOCK_PORT(ss, port) \
  switch (((struct sockaddr *)(ss))->sa_family) {\
  case AF_INET:\
    ((struct sockaddr_in*)(ss))->sin_port = (port);\
    break;\
  case AF_INET6:\
    ((struct sockaddr_in6*)(ss))->sin6_port = (port);\
    break;\
  }\

struct host_info {
  char    host[NI_MAXHOST];
  char    port[NI_MAXSERV];
};

struct req_host_info {
  struct  host_info dest;
  struct  host_info proxy;
};

/* prototypes */
int proto_socks4 __P((SOCKS_STATE *));
int proto_socks5 __P((SOCKS_STATE *));
int socks_direct_conn __P((SOCKS_STATE *));
int proxy_connect __P((SOCKS_STATE *));
int build_socks_request __P((SOCKS_STATE *, u_char *, int));
int connect_to_socks __P((SOCKS_STATE *));
int socks_proxy_reply __P((int, SOCKS_STATE *));
int socks_rep __P((int , int , int , SockAddr *));
int build_socks_reply __P((int, int, SockAddr *, u_char *));
int s5auth_s __P((int));
int s5auth_s_rep __P((int, int));
int s5auth_c __P((int, bin_addr *, u_int16_t));
int connect_to_http __P((SOCKS_STATE *));
int forward_connect __P((SOCKS_STATE *));
int bind_sock __P((int, SOCKS_STATE *, struct addrinfo *));
int do_bind __P((int, struct addrinfo *, u_int16_t));
#ifdef SO_BINDTODEVICE
static int do_bindtodevice __P((int, char *));
#endif

int read_until_delim __P((int, char *, size_t, int));
int get_line __P((int, char *, size_t));
int get_str __P((int, char *, size_t));
int lookup_tbl __P((SOCKS_STATE *));
int resolv_host __P((bin_addr *, u_int16_t, struct host_info *));
int log_request __P((SOCKS_STATE *));


/*
  proto_socks:
               handle socks protocol.
*/
int proto_socks(SOCKS_STATE *state)
{
  u_char buf[128];
  int r;
  int on = 1;
  size_t len;

  state->r = -1; /* forward socket not connected. */

  r = timerd_read(state->s, buf, 1, TIMEOUTSEC, 0);
  if ( r < 1 ) {  /* EOF or error */
    close(state->s);
    return(-1);
  }

  state->sr.ver = buf[0];
  switch (state->sr.ver) {
  case 4:
    if (method_num > 0) {
      /* this implies this server is working in V5 mode */
      /* dummy read for flush socket buf */
      r = timerd_read(state->s, buf, sizeof(buf), TIMEOUTSEC, 0);
      GEN_ERR_REP(state->s, 4);
      msg_out(warn, "V4 request is not accepted.");
      r = -1;
    } else {
      r = proto_socks4(state);
    }
    break;
  case 5:
    if ((r = s5auth_s(state->s)) == 0) {
      r = proto_socks5(state);
    }
    break;
  default:
    r = -1;
  }

  if (r >= 0) {
    lookup_tbl(state);
    log_request(state);


    if ( (state->hops == DIRECT && state->sr.req == S5REQ_CONN)
	 || state->hops > 0 ) {
      r = forward_connect(state);
      if (r < 0) {
	GEN_ERR_REP(state->s, state->sr.ver);
	return(-1);
      }
    }

    if (state->hops > 0) {
      for (state->cur = state->hops - 1; state->cur >= 0; state->cur--) {
	r = proxy_connect(state);
      }

    } else {
      r = socks_direct_conn(state);
    }

  }

  if (r >= 0) {
    if (state->r >= 0) {
      /* state->sr.req != UDPA or proxy_connect */
      setsockopt(state->r, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof on);
#if defined(FREEBSD) || defined(MACOSX)
      setsockopt(state->r, SOL_SOCKET, SO_REUSEPORT, (char *)&on, sizeof on);
#endif
      setsockopt(state->r, IPPROTO_TCP, TCP_NODELAY, (char *)&on, sizeof on);

      /* get upstream-side socket/peer name */
      len = SS_LEN;
      getsockname(state->r, &state->si->mys.addr.sa, (socklen_t *)&len);
      state->si->mys.len = len;
      len = SS_LEN;
      getpeername(state->r, &state->si->prs.addr.sa, (socklen_t *)&len);
      state->si->prs.len = len;
    }

    return(0);   /* 0: OK */
  }
  /* error */
  if (state->r >= 0) {
    close(state->r);
  }
  state->r = -1;
  return(-1);  /* error */
}

/*  socks4 protocol functions */
/*
  proto_socks4:
           handle socks v4/v4a protocol.
	   get socks v4/v4a request from client.
*/
int proto_socks4(SOCKS_STATE *state)
{
  u_char  buf[512];
  int     r;

  r = timerd_read(state->s, buf+1, 1+2+4, TIMEOUTSEC, 0);
  if (r < 1+2+4) {    /* cannot read request */
    GEN_ERR_REP(state->s, 4);
    return(-1);
  }

  state->sr.req = buf[1];

  /* check if request has socks4-a domain name format */
  if ( buf[4] == 0 && buf[5] == 0 &&
       buf[6] == 0 && buf[7] != 0 ) {
    state->sr.dest.atype = S4ATFQDN;
  } else {
    state->sr.dest.atype = S4ATIPV4;
  }

  /* port */
  state->sr.port = buf[2] * 0x100 + buf[3];
  /* IP */
  memcpy(state->sr.dest.v4_addr, &buf[4], 4);
  
  /* rest of the req could be
          username '\0'
      or,
          username '\0' hostname '\0'
  */

  /* read client user name in request */
  r = get_str(state->s, (char *)buf, sizeof(buf));

  if (r < 0 || r > 255) {
    /* invalid username length */
    GEN_ERR_REP(state->s, 4);
    return(-1);
  }

  state->sr.u_len = r;
  memcpy(state->sr.user, buf, r);

  if ( state->sr.dest.atype == S4ATFQDN ) {
    /* request is socks4-A specific */
    r = get_str(state->s, (char *)buf, sizeof(buf));

    if ( r > 0 && r <= 256 ) {   /* r should be 1 <= r <= 256 */
      state->sr.dest.len_fqdn = r;
      memcpy(state->sr.dest.fqdn, buf, r);
    } else {
      /* read error or something */
      GEN_ERR_REP(state->s, 4);
      return(-1);
    }
  }
  return(0);
}


/* socks5 protocol functions */
/*
  proto_socks5:
           handle socks v5 protocol.
	   get socks v5 request from client.
*/
int proto_socks5(SOCKS_STATE *state)
{
  u_char    buf[512];
  int     r, len;

  /* read first 4 bytes of request. */
  r = timerd_read(state->s, buf, 4, TIMEOUTSEC, 0);

  if ( r < 4 ) {
    /* cannot read client request */
    close(state->s);
    return(-1);
  }

  if ( buf[0] != 0x05 ) {
    /* wrong version request */
    GEN_ERR_REP(state->s, 5);
    return(-1);
  }

  state->sr.req = buf[1];
  state->sr.dest.atype = buf[3];  /* address type field */

  switch(state->sr.dest.atype) {
  case S5ATIPV4:  /* IPv4 address */
    r = timerd_read(state->s, buf+4, 4+2, TIMEOUTSEC, 0);
    if (r < 4+2) {     /* cannot read request (why?) */
      GEN_ERR_REP(state->s, 5);
      return(-1);
    }
    memcpy(state->sr.dest.v4_addr, &buf[4], sizeof(struct in_addr));
    state->sr.port = buf[8] * 0x100 + buf[9];
    break;

  case S5ATIPV6:
    r = timerd_read(state->s, buf+4, 16+2, TIMEOUTSEC, 0);
    if (r < 16+2) {     /* cannot read request (why?) */
      GEN_ERR_REP(state->s, 5);
      return(-1);
    }
    memcpy(state->sr.dest.v6_addr, &buf[4], sizeof(struct in6_addr));
    state->sr.port = buf[20] * 0x100 + buf[21];
    break;

  case S5ATFQDN:  /* string or FQDN */
    r = timerd_read(state->s, buf+4, 1, TIMEOUTSEC, 0);
    if (r < 1) {     /* cannot read request (why?) */
      GEN_ERR_REP(state->s, 5);
      return(-1);
    }

    if ((len = buf[4]) < 0 || len > 255) {
      /* invalid length */
      socks_rep(state->s, 5, S5EINVADDR, 0);
      close(state->s);
      return(-1);
    }
    r = timerd_read(state->s, buf+5, len+2, TIMEOUTSEC, 0);
    if ( r < len+2 ) {  /* cannot read request (why?) */
      GEN_ERR_REP(state->s, 5);
      return(-1);
    }
    memcpy(state->sr.dest.fqdn, &buf[5], len);
    state->sr.dest.len_fqdn = len;
    state->sr.port = buf[5+len] * 0x100 + buf[5+len+1];
    break;

  default:
    /* unsupported address */
    socks_rep(state->s, 5, S5EUSATYPE, 0);
    close(state->s);
    return(-1);
  }
  return(0);
}


/*
  socks_direct_conn:
      behave as socks server to connect/bind.
 */
int socks_direct_conn(SOCKS_STATE *state)
{
  int	cs, acs;
  int	len;
  struct addrinfo ba;
  SockAddr ss;
  int	error = 0;
  int	save_errno = 0;

  /* process direct connect/bind to destination */
  cs = acs = -1;

  /* process by_command request */
  switch (state->sr.req) {   /* request */
  case S5REQ_CONN:
    len = SS_LEN;
    if (getsockname(state->r, &ss.sa, (socklen_t *)&len) < 0) {
      save_errno = errno;
      close(state->r);
      state->r = -1;
    }
    if (save_errno != 0) {
      /* any socket error */
      switch (state->sr.ver) {
      case 0x04:
	GEN_ERR_REP(state->s, 4);
	break;
      case 0x05:
	switch(save_errno) {
	case ENETUNREACH:  socks_rep(state->s, 5, S5ENETURCH, 0); break;
	case ECONNREFUSED: socks_rep(state->s, 5, S5ECREFUSE, 0); break;
#ifndef _POSIX_SOURCE
	case EHOSTUNREACH: socks_rep(state->s, 5, S5EHOSURCH, 0); break;
#endif
	case ETIMEDOUT:    socks_rep(state->s, 5, S5ETTLEXPR, 0); break; /* ??? */
	default:           socks_rep(state->s, 5, S5EGENERAL, 0); break;
	}
	break;
      default:
	break;
      }
      close(state->s);
      return(-1);
    }
    break;

  case S5REQ_BIND:
    memset(&ba, 0, sizeof(ba));
    memset(&ss.ss, 0, SS_LEN);
    ba.ai_addr = &ss.sa;
    ba.ai_addrlen = SS_LEN;
    /* just one address can be stored */
    error = get_bind_addr(&state->sr.dest, &ba);
    if (error) {
      GEN_ERR_REP(state->s, state->sr.ver);
      return(-1);
    }
    acs = socket(ba.ai_family, ba.ai_socktype, ba.ai_protocol);
    if (acs < 0) {
      /* socket error */
      GEN_ERR_REP(state->s, state->sr.ver);
      return(-1);
    }

#ifdef SO_BINDTODEVICE
    if (bindtodevice && do_bindtodevice(acs, bindtodevice) < 0) {
      GEN_ERR_REP(state->s, state->sr.ver);
      close(acs);
      return(-1);
    }
#endif

    if (bind_sock(acs, state, &ba) != 0) {
      GEN_ERR_REP(state->s, state->sr.ver);
      return(-1);
    }

    listen(acs, 64);
    /* get my socket name again to acquire an
       actual listen port number */
    len = SS_LEN;
    if (getsockname(acs, &ss.sa, (socklen_t *)&len) == -1) {
      /* getsockname failed */
      GEN_ERR_REP(state->s, state->sr.ver);
      close(acs);
      return(-1);
    }
    memcpy(&state->si->mys.addr.ss, &ss, len);
    state->si->mys.len = len;

    /* first reply for bind request */
    POSITIVE_REP(state->s, state->sr.ver, &ss);
    if ( error < 0 ) {
      /* could not reply */
      close(state->s);
      close(acs);
      return(-1);
    }
    if (wait_for_read(acs, TIMEOUTSEC) <= 0) {
      GEN_ERR_REP(state->s, state->sr.ver);
      close(acs);
      return(-1);
    }

    len = SS_LEN;
    if ((cs = accept(acs, &ss.sa, (socklen_t *)&len)) < 0) {
      GEN_ERR_REP(state->s, state->sr.ver);
      close(acs);
      return(-1);
    }
    close(acs); /* accept socket is not needed
		   any more, for current socks spec. */
    state->r = cs;   /* set forwarding socket */
    /* sock name is in ss */
    /* TODO:
     *  we must check ss against state->sr.dest here for security reason
     */
    /* XXXXX */
    break;

  case S5REQ_UDPA:
    /* on UDP assoc of DIRECT method, state->r will not used */
    /* Allocate UDP_ATTR structure */
    state->sr.udp = (UDP_ATTR *)malloc(sizeof(UDP_ATTR));
    if (state->sr.udp == NULL) {
      	GEN_ERR_REP(state->s, state->sr.ver);
	close(state->s);
	return(-1);
    }
    memset(state->sr.udp, 0, sizeof(UDP_ATTR));
    /* save the socket name of requester's TCP socket into UDP_ATTR
     * for checking lator.
     */
    memcpy(&state->sr.udp->si.prc.addr, &state->si->prc.addr, SS_LEN);
    SET_SOCK_PORT(&state->sr.udp->si.prc.addr, 0);
    if (state->sr.port != 0)
      SET_SOCK_PORT(&state->sr.udp->si.prc.addr, htons(state->sr.port));

    /* initialize UDP transport sockets */
    state->sr.udp->d = state->sr.udp->u = -1;
    /* create UDP socket on the same I/F as the request was reached */
    memcpy(&ss.ss, &state->si->myc.addr.ss, SS_LEN);
    if ((cs = socket(ss.sa.sa_family, SOCK_DGRAM, IPPROTO_IP)) >= 0) {
      SET_SOCK_PORT(&ss.ss, 0);
      if (bind(cs, &ss.sa, state->si->myc.len) < 0) {
	/* bind error */
	close(cs);
	GEN_ERR_REP(state->s, state->sr.ver);
	close(state->s);
	return(-1);
      }
      /* XXXXXXXX */
      /* get my socket name to acquire an actual bound port */
      len = SS_LEN;
      if (getsockname(cs, &ss.sa, (socklen_t *)&len) == -1) {
	/* getsockname failed */
	GEN_ERR_REP(state->s, state->sr.ver);
	close(cs);
	return(-1);
      }
      /* ss is actual socket name here for usig positive reply */
      /* set downward soket */
      state->sr.udp->d = cs;
    } else {
      GEN_ERR_REP(state->s, state->sr.ver);
      return(-1);
    }
    break;

  default:
    /* unsupported request */
    GEN_ERR_REP(state->s, state->sr.ver);
    close(state->s);
    return(-1);
  }

  POSITIVE_REP(state->s, state->sr.ver, &ss);
  if ( error < 0 ) {
    /* could not reply */
    close(state->s);
    close(cs);
    return(-1);
  }
  return(0);
}

/*   proxy socks functions  */
/*
  proxy_connect:
	   connect to next hop socks/ server.
           indirect connection to destination.
*/
int proxy_connect(SOCKS_STATE *state)
{
  int     r = 0;

  switch (state->prx[state->cur].pproto) {
  case HTTP:
    r = connect_to_http(state);
    break;
  case SOCKS:
  case SOCKSv4:
  case SOCKSv5:
  default:
    r = connect_to_socks(state);
    break;
  }
  if (r < 0 && state->cur == 0) {
    GEN_ERR_REP(state->s, state->sr.ver);
    return(-1);
  }
  return(0);

}

int connect_to_socks(SOCKS_STATE *state)
{
  int     r, len = 0;
  u_char  buf[640];
  int     cur = state->cur;
  int     pproto = state->prx[cur].pproto;

  if (state->r < 0 && state->cur == 0) {
    GEN_ERR_REP(state->s, state->sr.ver);
    return(-1);
  }

  /* process proxy request to next hop socks */
  /* first try socks5 server */
  if (pproto == SOCKS || pproto == SOCKSv5) {
    if ((len = build_socks_request(state, buf, 5)) > 0) {
      if (s5auth_c(state->r, &state->prx[cur].proxy, state->prx[cur].pport) == 0) {
	/* socks5 auth nego to next hop success */
	r = timerd_write(state->r, buf, len, TIMEOUTSEC);
	if ( r == len ) {
	  /* send request success */
	  r = socks_proxy_reply(5, state);
	  if (r == 0) {
	    return(r);
	  }
	}
      }
    }
  }

  /* if an error, second try socks4 server */
  if (pproto == SOCKS || pproto == SOCKSv4) {
    if ((len = build_socks_request(state, buf, 4)) > 0) {
      r = timerd_write(state->r, buf, len, TIMEOUTSEC);
      if ( r == len ) {
	/* send request success */
	r = socks_proxy_reply(4, state);
	if (r == 0) {
	  return(r);
	}
      }
    }
  }

  /* still be an error, give it up. */
  if (state->cur == 0) {
    GEN_ERR_REP(state->s, state->sr.ver);
  }
  return(-1);
}

/*
  build_socks_request:
      build socks request on buf
      return buf length
 */
int build_socks_request(SOCKS_STATE *state, u_char *buf, int ver)
{
  /* buf must be at least 640 bytes long */
  int       r, len = 0;
  char      *user;
  bin_addr  dest;
  u_int16_t port;
  int       req;

  if (state->cur > 0) {
    port = state->prx[state->cur-1].pport;
    req = S5REQ_CONN;  /* only CONNECT among relays */
    memcpy(&dest, &state->prx[state->cur-1].proxy, sizeof(bin_addr));
  } else {
    port = state->sr.port;
    req = state->sr.req;
    memcpy(&dest, &state->sr.dest, sizeof(bin_addr));
  }

  switch (ver) {   /* next hop socks server version */
  case 0x04:
    /* build v4 request */
    buf[0] = 0x04;
    buf[1] = req;
    if ( state->sr.u_len == 0 ) {
      user = S4DEFUSR;
      r = strlen(user);
    } else {
      user = state->sr.user;
      r = state->sr.u_len;
    }
    if (r < 0 || r > 255) {
      return(-1);
    }
    buf[2] = (port / 256);
    buf[3] = (port % 256);
    memcpy(&buf[8], user, r);
    len = 8+r;
    buf[len++] = 0x00;
    switch (dest.atype) {
    case S4ATIPV4:
      memcpy(&buf[4], dest.v4_addr, sizeof(struct in_addr));
      break;
    case S4ATFQDN:
      buf[4] = buf[5] = buf[6] = 0; buf[7] = 1;
      r = dest.len_fqdn;
      if (r <= 0 || r > 255) {
	return(-1);
      }
      memcpy(&buf[len], dest.fqdn, r);
      len += r;
      buf[len++] = 0x00;
      break;
    default:
      return(-1);
    }
    break;

  case 0x05:
    /* build v5 request */
    buf[0] = 0x05;
    buf[1] = req;
    buf[2] = 0;
    buf[3] = dest.atype;
    switch (dest.atype) {
    case S5ATIPV4:
      memcpy(&buf[4], dest.v4_addr, 4);
      buf[8] = (port / 256);
      buf[9] = (port % 256);
      len = 10;
      break;
    case S5ATIPV6:
      memcpy(&buf[4], dest.v6_addr, 16);
      buf[20] = (port / 256);
      buf[21] = (port % 256);
      len = 22;
      break;
    case S5ATFQDN:
      len = dest.len_fqdn;
      buf[4] = len;
      memcpy(&buf[5], dest.fqdn, len);
      buf[5+len]   = (port / 256);
      buf[5+len+1] = (port % 256);
      len = 5+len+2;
      break;
    default:
      return(-1);
    }
    break;
  default:
    return(-1);   /* unknown version */
  }
  return(len); /* OK */
}


/*
  socks_proxy_reply:
       v: server socks version.
       read server response and
       write converted reply to client if needed.
       note: state->sr.ver == -1 means DEEP indirect proxy.
*/
int socks_proxy_reply(int v, SOCKS_STATE *state)
{
  int	r, c, len;
  u_char buf[512];
  struct addrinfo hints, *res, *res0;
  int	error;
  int	found = 0;
  SockAddr ss;

  switch (state->sr.req) {
  case S5REQ_CONN:
    c = 1;
    break;

  case S5REQ_BIND:
    c = 2;
    break;

  default:   /* i don't know what to do */
    c = 1;
    break;
  }

  while (c-- > 0) {
    memset(&ss, 0, SS_LEN);
    /* read server reply */
    r = timerd_read(state->r, buf, sizeof buf, TIMEOUTSEC, 0);

    if (state->cur > 0) {  /* special case */
      if ((v == 5 && buf[1] == S5AGRANTED)
	  || (v == 4 && buf[1] == S4AGRANTED)) {
	return(0);
      }
      return(-1);
    }

    switch (v) { /* server socks version */

    case 4: /* server v:4 */
      if ( r < 8 ) {  /* from v4 spec, r should be 8 */
	/* cannot read server reply */
	r = -1;
	break;
      }
      switch (state->sr.ver) {  /* client ver */
      case 4: /* same version */
	r = timerd_write(state->s, buf, r, TIMEOUTSEC);
	break;

      case 5:
	if ( buf[1] == S4AGRANTED ) {
	  /* translate reply v4->v5 */
	  ss.v4.sin_family = AF_INET;
	  memcpy(&(ss.v4.sin_addr), &buf[4], 4);
	  memcpy(&(ss.v4.sin_port), &buf[2], 2);
	  r = socks_rep(state->s, 5, S5AGRANTED, &ss);
	} else {
	  r = -1;
	}
	break;
      default:
	r = -1;
	break;
      }
      break;

    case 5: /* server v:5 */
      if ( r < 7 ) {   /* should be 10 or more */
	/* cannot read server reply */
	r = -1;
	break;
      }
      switch (state->sr.ver) { /* client ver */
      case 4:
	/* translate reply v5->v4 */
	if ( buf[1] == S5AGRANTED ) {
	  switch (buf[3]) {   /* address type */
	  case S5ATIPV4:
	    ss.v4.sin_family = AF_INET;
	    memcpy(&(ss.v4.sin_addr), &buf[4], 4);
	    memcpy(&(ss.v4.sin_port), &buf[8], 2);
	    break;
	  case S5ATIPV6:
	    /* basically v4 cannot handle IPv6 address */
	    /* make fake IPv4 to forcing reply */
	    ss.v4.sin_family = AF_INET;
	    memcpy(&(ss.v4.sin_addr), &buf[16], 4);
	    memcpy(&(ss.v4.sin_port), &buf[20], 2);
	    break;
	  case S5ATFQDN:
	  default:
	    ss.v4.sin_family = AF_INET;
	    len = buf[4] & 0xff;
	    memcpy(&(ss.v4.sin_port), &buf[5+len], 2);
	    buf[5+len] = '\0';
	    memset(&hints, 0, sizeof(hints));
	    hints.ai_socktype = SOCK_STREAM;
	    hints.ai_family = AF_INET;
	    error = getaddrinfo((char *)&buf[5], NULL, &hints, &res0);
	    if (!error) {
	      for (res = res0; res; res = res->ai_next) {
		if (res->ai_socktype != AF_INET)
		  continue;
		memcpy(&(ss.v4.sin_addr),
		       &(((struct sockaddr_in *)res->ai_addr)->sin_addr),
		       sizeof(struct in_addr));
		found++; break;
	      }
	      freeaddrinfo(res0);
	    }
	    if (!found) {
	      /* set fake ip */
	      memset(&(ss.v4.sin_addr), 0, 4);
	    }
	    break;
	  }
	  r = socks_rep(state->s, 4, S4AGRANTED, &ss);
	} else {
	  r = -1;
	}
	break;
      case 5: /* same version */
	r = timerd_write(state->s, buf, r, TIMEOUTSEC);
	break;
      default:
	r = -1;
	break;
      }
      break;

    default:
      /* parameter error */
      r = -1;
      break;
    }

    if (r < 0) {
      return(r);
    }
  }
  return(0);
}


int socks_rep(int s, int ver, int code, SockAddr *addr)
{
  u_char     buf[512];
  int        len = 0, r;

  memset(buf, 0, sizeof(buf));
  len = build_socks_reply(ver, code, addr, buf);

  if (len > 0)
    r = timerd_write(s, buf, len, TIMEOUTSEC);
  else
    r = -1;

  if (r < len)
    return -1;

  return 0;
}

int build_socks_reply(int ver, int code, SockAddr *addr, u_char *buf)
{

  int len = 0;

  switch (ver) {
  case 0x04:
    switch (code) {
    case S4AGRANTED:
      buf[0] = 0;
      buf[1] = code;   /* succeeded */
      if (addr) {
	memcpy(&buf[2], &addr->v4.sin_port, 2);
	memcpy(&buf[4], &addr->v4.sin_addr, 4);
      }
      len = 8;
      break;

    default:  /* error cases */
      buf[0] = ver;
      buf[1] = code;   /* error code */
      len = 8;
      break;
    }
    break;

  case 0x05:
    switch (code) {
    case S5AGRANTED:
      buf[0] = ver;
      buf[1] = code;   /* succeeded */
      buf[2] = 0;
      if (addr) {
	switch (addr->sa.sa_family) {
	case AF_INET:
	  buf[3] = S5ATIPV4;
	  memcpy(&buf[4], &addr->v4.sin_addr, 4);
	  memcpy(&buf[8], &addr->v4.sin_port, 2);
	  len = 4+4+2;
	  break;
	case AF_INET6:
	  buf[3] = S5ATIPV6;
	  memcpy(&buf[4], &addr->v6.sin6_addr, 16);
	  memcpy(&buf[20], &addr->v6.sin6_port, 2);
	  len = 4+16+2;
	  break;
	default:
	  buf[1] = S5EUSATYPE;
	  buf[3] = S5ATIPV4;
	  len = 4+4+2;
	  break;
	}
      }
      break;

    default:  /* error cases */
      buf[0] = ver;
      buf[1] = code & 0xff;   /* error code */
      buf[2] = 0;
      buf[3] = S5ATIPV4;  /* addr type fixed to IPv4 */
      len = 10;
      break;
    }
    break;

  default:
    /* unsupported socks version */
    len = 0;
    break;
  }
  return(len);
}

/*
  socks5 auth negotiation as server.
*/
int s5auth_s(int s)
{
  u_char buf[512];
  int r, i, j, len;
  int method=0, done=0;

  /* auth method negotiation */
  r = timerd_read(s, buf+1, 1, TIMEOUTSEC, 0);
  if ( r < 1 ) {
    /* cannot read */
    s5auth_s_rep(s, S5ANOTACC);
    return(-1);
  }

  len = buf[1];
  if ( len < 0 || len > 255 ) {
    /* invalid number of methods */
    s5auth_s_rep(s, S5ANOTACC);
    return(-1);
  }

  r = timerd_read(s, buf, len, TIMEOUTSEC, 0);
  if (method_num == 0) {
    for (i = 0; i < r; i++) {
      if (buf[i] == S5ANOAUTH) {
	method = S5ANOAUTH;
	done = 1;
	break;
      }
    }
  } else {
    for (i = 0; i < method_num; i++) {
      for (j = 0; j < r; j++) {
	if (buf[j] == method_tab[i]){
	  method = method_tab[i];
	  done = 1;
	  break;
	}
      }
      if (done)
	break;
    }
  }
  if (!done) {
    /* no suitable method found */
    method = S5ANOTACC;
  }

  if (s5auth_s_rep(s, method) < 0)
    return(-1);

  switch (method) {
  case S5ANOAUTH:
    /* heh, do nothing */
    break;
  case S5AUSRPAS:
    if (auth_pwd_server(s) == 0) {
      break;
    } else {
      close(s);
      return(-1);
    }
  default:
    /* other methods are unknown or not implemented */
    close(s);
    return(-1);
  }
  return(0);
}

/*
  Auth method negotiation reply
*/
int s5auth_s_rep(int s, int method)
{
  u_char buf[2];
  int r;

  /* reply to client */
  buf[0] = 0x05;   /* socks version */
  buf[1] = method & 0xff;   /* authentication method */
  r = timerd_write(s, buf, 2, TIMEOUTSEC);
  if (r < 2) {
    /* write error */
    close(s);
    return(-1);
  }
  return(0);
}

/*
  socks5 auth negotiation as client.
*/
int s5auth_c(int s, bin_addr *proxy, u_int16_t pport)
{
  u_char buf[512];
  int r, num=0;

  /* auth method negotiation */
  buf[0] = 0x05;
  buf[1] = 1;           /* number of methods.*/
  buf[2] = S5ANOAUTH;   /* no authentication */
  num = 3;

  if (getpasswd(proxy, pport, NULL, pwdfile) == 0 ) { /* pwdfile(global) */
    buf[1] = 2;
    buf[3] = S5AUSRPAS;   /* username/passwd authentication */
    num++;
  }
  r = timerd_write(s, buf, num, TIMEOUTSEC);
  if ( r < num ) {
    /* cannot write */
    close(s);
    return(-1);
  }

  r = timerd_read(s, buf, 2, TIMEOUTSEC, 0);
  if ( r < 2 ) {
    /* cannot read */
    close(s);
    return(-1);
  }
  if (buf[0] == 0x05 && buf[1] == 0) {
    /* no auth method is accepted */
    return(0);
  }
  if (buf[0] == 0x05 && buf[1] == 2) {
    /* do username/passwd authentication */
    return(auth_pwd_client(s, proxy, pport));
  }
  /* auth negotiation failed */
  return(-1);
}

int connect_to_http(SOCKS_STATE *state)
{
  struct host_info hinfo;
  bin_addr   dest;
  u_int16_t  port;
  char   buf[2048];
  int    c, r, len;
  int    error;
  SockAddr ss;
  struct user_pass up;
  char   userpass[1024];
  char   authhead[1024];
  char   *b64;
  char *p;
  
  p = buf;

  if (state->cur > 0) {
    memcpy(&dest, &state->prx[state->cur-1].proxy, sizeof(bin_addr));
    port = state->prx[state->cur-1].pport;
  } else {
    memcpy(&dest, &state->sr.dest, sizeof(bin_addr));
    port = state->sr.port;
  }

  resolv_host(&dest, port, &hinfo);

  snprintf(buf, sizeof(buf), "CONNECT %s:%s HTTP/1.1\r\n"
	   "Host: %s:%s\r\n",
	   hinfo.host, hinfo.port, hinfo.host, hinfo.port);
  if (getpasswd(&state->prx[state->cur].proxy, state->prx[state->cur].pport, &up, pwdfile) == 0
      && (strlen(buf) + (up.ulen+up.plen+2)*4/3+27 < sizeof(buf))) {
    /* http/proxy auth */
    strcpy(authhead, "Proxy-Authorization: basic ");
    snprintf(userpass, sizeof(userpass), "%s:%s", up.user, up.pass);
    b64 = base64_encode(userpass);
    strncat(authhead, b64, strlen(b64));
    free(b64);
    strncat(buf, authhead, strlen(authhead));
    strncat(buf, "\r\n", 2);
  }
  strncat(buf, "\r\n", 2);
  /* debug */
  msg_out(norm, ">>HTTP CONN %s:%s", hinfo.host, hinfo.port);

  len = strlen(buf);
  r = timerd_write(state->r, (u_char *)buf, len, TIMEOUTSEC);

  if ( r == len ) {
    /* get resp */
    r = get_line(state->r, buf, sizeof(buf));
    if (r >= 12) {
      while (r>0 && ((c=*p) != ' ' && c != '\t'))
	p++; r--;
      while (r>0 && ((c=*p) == ' ' || c == '\t'))
	p++; r--;
      if (strncmp(p, "200", 3) == 0) {
	/* redirection not supported */
	do { /* skip resp headers */
	  r = get_line(state->r, buf, sizeof(buf));
	  if (r <= 0 && state->cur == 0) {
	    GEN_ERR_REP(state->s, state->sr.ver);
	    return(-1);
	  }
	} while (strcmp(buf, "\r\n") != 0);
	len = SS_LEN;
	getsockname(state->r, &ss.sa, (socklen_t *)&len);
	/* is this required ?? */
	if (state->cur == 0) {
	  POSITIVE_REP(state->s, state->sr.ver, &ss);
	  return(0);
	}
      }
    }
  }
  if (state->cur == 0) {
    GEN_ERR_REP(state->s, state->sr.ver);
  }
  return(-1);
}

/*
  forward_connect:
      just resolve host and connect to her.
      return ok(0)/error(-1);
 */

int forward_connect(SOCKS_STATE *state)
{
  int    cs = -1;
  struct host_info dest;
  struct addrinfo hints, *res, *res0;
  SockAddr ss;
  int    error = 0;

  if (state->hops > 0) {
    error = resolv_host(&state->prx[state->hops-1].proxy,
			state->prx[state->hops-1].pport, &dest);
  } else {
    error = resolv_host(&state->sr.dest, state->sr.port, &dest);
  }

  /* string addr => addrinfo */
  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_STREAM;
  if (same_interface) {
    memcpy(&ss.ss, &state->si->myc.addr.ss, state->si->myc.len);
    hints.ai_family = ss.sa.sa_family;
    /* XXXXX
      this may cause getaddrinfo returns same address
      family info as clients connected. Is this correct ?
    */
  }
  error = getaddrinfo(dest.host, dest.port, &hints, &res0);
  if (error) { /* getaddrinfo returns error>0 when error */
    return -1;
  }

  for (res = res0; res; res = res->ai_next) {
    error = 0;
    cs = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if ( cs < 0 ) {
      /* socket error */
      error = errno;
      continue;
    }

#ifdef SO_BINDTODEVICE
    /*
      bindtodevice may case error at bind call if device
      config has conflicts with same_interface option ???
     */
    if (bindtodevice && do_bindtodevice(cs, bindtodevice) < 0) {
      error = errno;
      close(cs);
      continue;
    }
#endif

    if (same_interface) {
      /* bind the outgoing socket to the same interface
	 as the inbound client */
      SET_SOCK_PORT(&ss.ss, 0);
      if (bind(cs, &ss.sa, state->si->myc.len) <0) {
	/* bind error */
	error = errno;
	close(cs);
	continue;
      }
    }

    if (connect(cs, res->ai_addr, res->ai_addrlen) == 0) {
      error = 0;
      break;
    } else {
      /* connect fail */
      error = errno;
      close(cs);
      continue;
    }
  }
  freeaddrinfo(res0);

  msg_out(norm, "Forward connect to %s:%s rc=%d",
	  dest.host, dest.port, error);
  state->r = cs;
  if (cs >= 0 && error == 0)
    return(0);
  else
    return(-1);
}

int bind_sock(int s, SOCKS_STATE *state, struct addrinfo *ai)
{
  /*
    BIND port selection priority.
    1. requested port. (assuming dest->sin_port as requested port)
    2. clients src port.
    3. free port.
  */
  SockAddr	ss;
  u_int16_t  port;
  size_t     len;

  /* try requested port */
  if (do_bind(s, ai, state->sr.port) == 0)
    return 0;

  /* try same port as client's */
  len = SS_LEN;
  memset(&ss, 0, len);
  if (getpeername(state->s, &ss.sa, (socklen_t *)&len) != 0)
    port = 0;
  else {
    switch (ss.sa.sa_family) {
    case AF_INET:
      port = ntohs(ss.v4.sin_port);
      break;
    case AF_INET6:
      port = ntohs(ss.v6.sin6_port);
      break;
    default:
      port = 0;
    }
  }
  if (do_bind(s, ai, port) == 0)
    return 0;

  /*  bind free port */
  return(do_bind(s, ai, 0));
}

int do_bind(int s, struct addrinfo *ai, u_int16_t p)
{
  u_int16_t port = p;  /* Host Byte Order */
  int       r;

  if ( bind_restrict && port < IPPORT_RESERVEDSTART)
    port = 0;

  switch (ai->ai_family) {
  case AF_INET:
    ((struct sockaddr_in *)ai->ai_addr)->sin_port = htons(port);
    break;
  case AF_INET6:
    ((struct sockaddr_in6 *)ai->ai_addr)->sin6_port = htons(port);
    break;
  default:
    /* unsupported */
    return(-1);
  }

#ifdef IPV6_V6ONLY
  {
    int    on = 1;
    if (ai->ai_family == AF_INET6 &&
	setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on)) < 0)
      return -1;
  }
#endif

  if (port > 0 && port < IPPORT_RESERVED)
    setuid(0);
  r = bind(s, ai->ai_addr, ai->ai_addrlen);
  setreuid(PROCUID, -1);
  return(r);
}

#ifdef SO_BINDTODEVICE
/*
  do_bindtodevice:
          bind socket to named device.
 */
#include <net/if.h>
static int do_bindtodevice(int cs, char *dev)
{
  int rc;
  struct ifreq interface;

  strncpy(interface.ifr_name, dev, IFNAMSIZ);
  setuid(0);
  rc = setsockopt(cs, SOL_SOCKET, SO_BINDTODEVICE,
                  (char *)&interface, sizeof(interface));
  setreuid(PROCUID, -1);
  if (rc < 0)
    msg_out(crit, "setsockopt SO_BINDTODEVICE(%s) failed: %d", dev, errno);
  return(rc);
}
#endif

/*
  wait_for_read:
          wait for readable status.
	  descriptor 's' must be in blocking i/o mode.
 */
int wait_for_read(int s, long sec)
{
  fd_set fds;
  int n, nfd;
  struct timeval tv;

  tv.tv_sec = sec;
  tv.tv_usec = 0;

  nfd = s;
  FD_ZERO(&fds); FD_SET(s, &fds);
  n = select(nfd+1, &fds, 0, 0, &tv);
  switch (n) {
  case -1:            /* error */
    return(-1);
  case 0:             /* timed out */
    return(0);
  default:            /* ok */
    if (FD_ISSET(s, &fds))
      return(s);
    else
      return(-1);
  }
}

ssize_t timerd_read(int s, u_char *buf, size_t len, int sec, int flags)
{
  ssize_t r = -1;
  settimer(sec);
  r = recvfrom(s, buf, len, flags, 0, 0);
  settimer(0);
  return(r);
}

ssize_t timerd_write(int s, u_char *buf, size_t len, int sec)
{
  ssize_t r = -1;
  settimer(sec);
  r = sendto(s, buf, len, 0, 0, 0);
  settimer(0);
  return(r);
}

int get_line(int s, char *buf, size_t len)
{
  return read_until_delim(s, buf, len, 012);
}

int get_str(int s, char *buf, size_t len)
{
  int r = read_until_delim(s, buf, len, 0);
  if (r > 0)
    r--;
  return(r); 
}

int read_until_delim(int s, char *buf, size_t len, int delim)
{
  int     r = 0;
  char   *p = buf;
  int     ret;

  while ( len > 1 ) { /* guard the buf */
    ret = timerd_read(s, (u_char *)p, 1, TIMEOUTSEC, 0);
    if (ret < 0) {
      if (errno == EINTR) {  /* not thread safe ?? */
	continue;
      }
      r = -1;
      break;
    } else if ( ret == 0 ) { /* EOF */
      len = 0; /* to exit loop */
    } else {
      len--;
      if (*p == delim) {
	len = 0; /* to exit loop */
      }
      r++; p++;
    }
  }
  *p = '\0';
  return(r);

}

int lookup_tbl(SOCKS_STATE *state)
{
  int    i, match, error;
  struct addrinfo hints, *res, *res0;
  char   name[NI_MAXHOST];
  bin_addr addr;
  struct sockaddr_in  *sa;
  struct sockaddr_in6 *sa6;

  match = 0;
  for (i=0; i < num_routes; i++) {
    /* check IP PROTO */
    if ( (state->sr.req == S5REQ_UDPA && proxy_tbl[i].proto == TCP)
	 || (state->sr.req != S5REQ_UDPA && proxy_tbl[i].proto == UDP))
      continue;
    /* check destination port */
    if ( state->sr.port < proxy_tbl[i].port_l
	 || state->sr.port > proxy_tbl[i].port_h)
      continue;

    if (addr_comp(&(state->sr.dest), &(proxy_tbl[i].dest),
		  proxy_tbl[i].mask) == 0) {
      match++;
      break;
    }
  }

  if ( !match && state->sr.dest.atype == S5ATFQDN ) {
    /* fqdn 2nd stage: try resolve and lookup */

    strncpy(name, (char *)state->sr.dest.fqdn, state->sr.dest.len_fqdn);
    name[state->sr.dest.len_fqdn] = '\0';
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    error = getaddrinfo(name, NULL, &hints, &res0);

    if ( !error ) {
      for (res = res0; res; res = res->ai_next) {
	for (i = 0; i < num_routes; i++) {
	  /* check IP PROTO */
	  if ( (state->sr.req == S5REQ_UDPA && proxy_tbl[i].proto == TCP)
	       || (state->sr.req != S5REQ_UDPA && proxy_tbl[i].proto == UDP))
	    continue;
	  /* check destination port */
	  if ( state->sr.port < proxy_tbl[i].port_l
	       || state->sr.port > proxy_tbl[i].port_h)
	    continue;

	  memset(&addr, 0, sizeof(addr));
	  switch (res->ai_family) {
	  case AF_INET:
	    addr.atype = S5ATIPV4;
	    sa = (struct sockaddr_in *)res->ai_addr;
	    memcpy(addr.v4_addr,
		   &sa->sin_addr, sizeof(struct in_addr));
	    break;
	  case AF_INET6:
	    addr.atype = S5ATIPV6;
	    sa6 = (struct sockaddr_in6 *)res->ai_addr;
	    memcpy(addr.v6_addr,
		   &sa6->sin6_addr, sizeof(struct in6_addr));
	    addr.v6_scope = sa6->sin6_scope_id;
	    break;
	  default:
	    addr.atype = -1;
	    break;
	  }
	  if ( addr.atype != proxy_tbl[i].dest.atype )
	    continue;
	  if (addr_comp(&addr, &(proxy_tbl[i].dest),
			proxy_tbl[i].mask) == 0)
	    match++;
	  break;
	}
	if ( match )
	  break;
      }
      freeaddrinfo(res0);
    }
  }

  state->hops = 0;
  state->cur = 0;
  state->prx = NULL;

  if (match && proxy_tbl[i].hops > 0
      && (state->prx = malloc(sizeof(PROXY_INFO)*proxy_tbl[i].hops)) != NULL ) {
    memcpy(state->prx, proxy_tbl[i].prx, sizeof(PROXY_INFO)*proxy_tbl[i].hops);
    state->hops = proxy_tbl[i].hops;
    state->cur = state->hops-1;
  }
  return(0);
}

/*
  resolv_host:
       convert binary addr into string replesentation host_info
 */
int resolv_host(bin_addr *addr, u_int16_t port, struct host_info *info)
{
  SockAddr ss;
  int     error = 0;
  int     len = 0;

  memset(&ss.ss, 0, SS_LEN);
  switch (addr->atype) {
  case S5ATIPV4:
    len = V4_LEN;
#ifdef HAVE_SOCKADDR_SA_LEN
    ss.v4.sin_len = len;
#endif
    ss.v4.sin_family = AF_INET;
    memcpy(&(ss.v4.sin_addr), addr->v4_addr, sizeof(struct in_addr));
    ss.v4.sin_port = htons(port);
    break;
  case S5ATIPV6:
    len = V6_LEN;
#ifdef HAVE_SOCKADDR_SA_LEN
    ss.v6.sin6_len = len;
#endif
    ss.v6.sin6_family = AF_INET6;
    memcpy(&(ss.v6.sin6_addr), addr->v6_addr, sizeof(struct in6_addr));
    ss.v6.sin6_scope_id = addr->v6_scope;
    ss.v6.sin6_port = htons(port);
    break;
  case S5ATFQDN:
    len = V4_LEN;
#ifdef HAVE_SOCKADDR_SA_LEN
    ss.v4.sin_len = len;
#endif
    ss.v4.sin_family = AF_INET;
    ss.v4.sin_port = htons(port);
    break;
  default:
    break;
  }
  if (addr->atype == S5ATIPV4 || addr->atype == S5ATIPV6) {
    error = getnameinfo(&ss.sa, len,
			info->host, sizeof(info->host),
			info->port, sizeof(info->port),
			NI_NUMERICHOST | NI_NUMERICSERV);
  } else if (addr->atype == S5ATFQDN) {
    error = getnameinfo(&ss.sa, len,
			NULL, 0,
			info->port, sizeof(info->port),
			NI_NUMERICSERV);
    strncpy(info->host, (char *)addr->fqdn, addr->len_fqdn);
    info->host[addr->len_fqdn] = '\0';
  } else {
    strcpy(info->host, "?");
    strcpy(info->port, "?");
    error++;
  }
  return(error);
}

/*
  log_request:
*/
int log_request(SOCKS_STATE *state)
{
  struct  host_info client, dest, proxy;
  int     error = 0;
  char    user[256];
  char    *ats[] =  {"ipv4", "fqdn", "ipv6", "?"};
  char    *reqs[] = {"CON", "BND", "UDP", "?"};
  int     atmap[] = {3, 0, 3, 1, 2};
  int     reqmap[] = {3, 0, 1, 2};
  int     direct = 0;

  if (state->hops > 0) {
    error += resolv_host(&state->prx[state->hops-1].proxy,
			 state->prx[state->hops-1].pport,
			 &proxy);
  } else {
    /* proxy_XX is N/A */
    strcpy(proxy.host, "-");
    strcpy(proxy.port, "-");
    direct = 1;
  }
  error += resolv_host(&state->sr.dest, state->sr.port, &dest);

  error += getnameinfo((struct sockaddr *)&state->si->prc.addr, state->si->prc.len,
			client.host, sizeof(client.host),
			client.port, sizeof(client.port),
			NI_NUMERICHOST | NI_NUMERICSERV);
 
  strncpy(user, state->sr.user, state->sr.u_len);
  user[state->sr.u_len] = '\0';

  msg_out(norm, "%s:%s %d-%s %s:%s(%s) %s %s%s:%s.",
		client.host, client.port,
		state->sr.ver, reqs[reqmap[state->sr.req]],
	        dest.host, dest.port,
		ats[atmap[state->sr.dest.atype]],
	        user,
		direct ? "direct" : "relay=",
	        proxy.host, proxy.port );
  return(error);
}
