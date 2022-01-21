/*
  relay.c:
  $Id: relay.c,v 1.21 2017/08/16 13:00:01 bulkstream Exp $

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

#define TIMEOUTSEC   30

typedef struct {
  int		from, to;
  size_t	nr, nw;
  ssize_t	nread, nwritten;
  int		flags;		/* flag OOB, ... */
  struct sockaddr *ss;		/* sockaddr used for recv / sendto */
  socklen_t	len;		/* sockaddr len used for recv / sendto */
  char		buf[BUFSIZE];	/* data */
  int		top;		/* top position of udp data */
  int		dir;		/* udp relay direction */
} rlyinfo;

enum { UP, DOWN };		/* udp relay direction */

int resolv_client;

/* proto types */
void readn	 __P((rlyinfo *));
void writen	 __P((rlyinfo *));
ssize_t forward	 __P((rlyinfo *));
ssize_t forward_udp __P((rlyinfo *, UDP_ATTR *, int));
int decode_socks_udp __P((UDP_ATTR *, u_char *));
void relay_tcp __P((SOCKS_STATE *));
void relay_udp __P((SOCKS_STATE *));
int log_transfer __P((SOCK_INFO *, LOGINFO *));

void readn(rlyinfo *ri)
{
  ri->nread = 0;
  ri->nread = recvfrom(ri->from, ri->buf+ri->top, ri->nr, ri->flags,
		       ri->ss, &ri->len);
  if (ri->nread < 0) {
    msg_out(warn, "read: %m");
  }
}

void writen(rlyinfo *ri)
{
  ri->nwritten = 0;
  ri->nwritten = sendto(ri->to, ri->buf+ri->top, ri->nw, ri->flags,
			ri->ss, ri->len);
  if (ri->nwritten <= 0) {
    msg_out(warn, "write: %m");
  }
}

ssize_t forward(rlyinfo *ri)
{
  settimer(TIMEOUTSEC);
  readn(ri);
  if (ri->nread > 0) {
    ri->nw = ri->nread;
    writen(ri);
  }
  settimer(0);
  if (ri->nread == 0)
    return(0);           /* EOF */
  if (ri->nread < 0)
    return(-1);
  return(ri->nwritten);
}

ssize_t forward_udp(rlyinfo *ri, UDP_ATTR *udp, int method)
{
  settimer(TIMEOUTSEC);
  if (method == DIRECT) {
    switch (ri->dir) {
    case UP:
      ri->top = 0;
      ri->len = SS_LEN;
      readn(ri);
      if (ri->nread > 0) {
	/* (check and) save down-ward sockaddr */
	memcpy(&udp->si.prc.addr, ri->ss, ri->len);
	udp->si.prc.len = ri->len;

	/* decode socks udp header and set it to up-side sockaddr */
	if (decode_socks_udp(udp, (u_char *)ri->buf) < 0)
	  return(-1);
	/* shift buf top pointer by udp header length */
	ri->top = udp->sv.len;
	/* open upward socket unless opened yet */
	/* XXXX little bit ambiguous ?? */
	if (udp->u < 0) {
	  if ((udp->u = socket(udp->si.prs.addr.sa.sa_family,
				     SOCK_DGRAM, IPPROTO_IP)) < 0)
	    return(-1);
	  ri->to = udp->u;
	}
	/* set destination(up-ward) sockaddr */
	memcpy(ri->ss, &udp->si.prs.addr, udp->si.prs.len);
	ri->len = udp->si.prs.len;

	/* set write data len */
	if (ri->nread - udp->sv.len < 0)
	  return(-1);
	ri->nw = ri->nread - udp->sv.len;
      }
      break;
    case DOWN:
      if (udp->sv.len <= 0)
	return(-1);
      /* shift buf top pointer by udp header length */
      ri->top = udp->sv.len;
      ri->len = SS_LEN;
      readn(ri);
      if(ri->nread > 0) {
	/* (check and) save up-ward sockaddr */
	memcpy(&udp->si.prs.addr, ri->ss, ri->len);
	udp->si.prs.len = ri->len;

	/* prepend socks udp header to buffer */
	memcpy(ri->buf, udp->sv.data, udp->sv.len);
	/* set destination(down-ward) sockaddr */
	memcpy(ri->ss, &udp->si.prc.addr, udp->si.prc.len);
	ri->len = udp->si.prc.len;

	/* reset buf top */
	ri->top = 0;
	/* set write data len */
	ri->nw = ri->nread + udp->sv.len;
      }
      break;
    }
    writen(ri);
  } else {
    /* PROXY just relay */
    /* XXXXX  not yet */
  }
  settimer(0);
  if (ri->nread == 0)
    /* none the EOF case of UDP */
    return(0);
  if (ri->nread < 0)
    return(-1);
  return(ri->nwritten);
}

/*
  decode_socks_udp:
	decode socks udp header.
 */
int decode_socks_udp(UDP_ATTR *udp, u_char *buf)
{
  int	len;
  struct addrinfo hints, *res;
  char host[256];

#define _addr_  udp->si.prs.addr

  len = 0;
  udp->sv.len = 0;
  if (buf[2] != 0x00) {
    /* we do not support fragment */
    return(-1);
  }

  switch (buf[3]) { /* address type */
  case S5ATIPV4:
#ifdef HAVE_SOCKADDR_SA_LEN
    _addr_.v4.sin_len = V4_LEN;
#endif
    _addr_.v4.sin_family = AF_INET;
    memcpy(&_addr_.v4.sin_addr, buf+4, sizeof(struct in_addr));
    udp->si.prs.len = V4_LEN;
    _addr_.v4.sin_port = htons(buf[8] * 0x100 + buf[9]);
    udp->sv.len = 10;
    break;

  case S5ATIPV6:
#ifdef HAVE_SOCKADDR_SA_LEN
    _addr_.v6.sin6_len = V6_LEN;
#endif
    _addr_.v6.sin6_family = AF_INET6;
    memcpy(&(_addr_.v6.sin6_addr), buf+4, sizeof(struct in6_addr));
    udp->si.prs.len = V6_LEN;
    _addr_.v6.sin6_scope_id = 0;
    _addr_.v6.sin6_port = htons(buf[20] * 0x100 + buf[21]);
    udp->sv.len = 22;
    break;

  case S5ATFQDN:
    len = buf[4] & 0xFF;   /* length */
    if (len < 0 || len > 255) {  /* will not occur */
      return(-1);
    }
    memcpy(host, buf+5, len);
    host[len] = 0x00;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo(host, 0, &hints, &res) > 0 ) {
      /* error */
      return(-1);
    }
    memcpy(&udp->si.prs.addr, res->ai_addr, res->ai_addrlen);
    udp->si.prs.len = res->ai_addrlen;
    switch (res->ai_family) {
    case AF_INET:
      _addr_.v4.sin_port = htons(buf[5+len] * 0x100 + buf[5+len+1]);
      break;
    case AF_INET6:
      _addr_.v6.sin6_port = htons(buf[5+len] * 0x100 + buf[5+len+1]);
      break;
    default:
      /* error */
      break;
    }
    udp->sv.len = 5+len+2;
    freeaddrinfo(res);
    break;
  default:
    /* error */
    return(-1);
  }
  /* save socks udp header */
  memcpy(udp->sv.data, buf, udp->sv.len);
  return(0);
#undef _addr_
}

/*
 *  relay: switch relay tcp/udp->
*/
void relay(SOCKS_STATE *state)
{
  if (state->sr.req == S5REQ_UDPA)
    relay_udp(state);
  else
    relay_tcp(state);
  if (state->prx != NULL) {
    free(state->prx);
    state->prx = NULL;
  }
}

#ifndef MAX
# define MAX(a,b)  (((a)>(b))?(a):(b))
#endif

u_long idle_timeout = IDLE_TIMEOUT;

void relay_tcp(SOCKS_STATE *state)
{
  fd_set   rfds, xfds;
  int      nfds, sfd;
  struct   timeval tv;
  struct   timezone tz;
  ssize_t  wc;
  rlyinfo  ri;
  int      done;
  u_long   max_count = idle_timeout;
  u_long   timeout_count;
  LOGINFO	li;

  memset(&ri, 0, sizeof(ri));
  memset(&li, 0, sizeof(li));
  ri.ss = (struct sockaddr *)NULL;
  ri.len = 0;
  ri.nr = BUFSIZE;

  nfds = MAX(state->r, state->s);
  setsignal(SIGALRM, timeout);
  gettimeofday(&li.start, &tz);
  li.bc = li.upl = li.dnl = 0;
  ri.flags = 0; timeout_count = 0;
  for (;;) {
    FD_ZERO(&rfds);
    FD_SET(state->s, &rfds); FD_SET(state->r, &rfds);
    if (ri.flags == 0) {
      FD_ZERO(&xfds);
      FD_SET(state->s, &xfds); FD_SET(state->r, &xfds);
    }
    done = 0;
    /* idle timeout related setting. */
    tv.tv_sec = 60; tv.tv_usec = 0;   /* unit = 1 minute. */
    tz.tz_minuteswest = 0; tz.tz_dsttime = 0;
    sfd = select(nfds+1, &rfds, 0, &xfds, &tv);
    if (sfd > 0) {
      if (FD_ISSET(state->r, &rfds)) {
	ri.from = state->r; ri.to = state->s; ri.flags = 0;
	if ((wc = forward(&ri)) <= 0)
	  done++;
	else
	  li.bc += wc; li.dnl += wc;

	FD_CLR(state->r, &rfds);
      }
      if (FD_ISSET(state->r, &xfds)) {
	ri.from = state->r; ri.to = state->s; ri.flags = MSG_OOB;
	if ((wc = forward(&ri)) <= 0)
	  done++;
	else
	  li.bc += wc; li.dnl += wc;
	FD_CLR(state->r, &xfds);
      }
      if (FD_ISSET(state->s, &rfds)) {
	ri.from = state->s; ri.to = state->r; ri.flags = 0;
	if ((wc = forward(&ri)) <= 0)
	  done++;
	else
	  li.bc += wc; li.upl += wc;
	FD_CLR(state->s, &rfds);
      }
      if (FD_ISSET(state->s, &xfds)) {
	ri.from = state->s; ri.to = state->r; ri.flags = MSG_OOB;
	if ((wc = forward(&ri)) <= 0)
	  done++;
	else
	  li.bc += wc; li.upl += wc;
	FD_CLR(state->s, &xfds);
      }
      if (done > 0)
	break;
    } else if (sfd < 0) {
      if (errno != EINTR)
	break;
    } else { /* sfd == 0 */
      if (max_count != 0) {
	timeout_count++;
	if (timeout_count > max_count)
	  break;
      }
    }
  }
  gettimeofday(&li.end, &tz);
  log_transfer(state->si, &li);

  close(state->r);
  close(state->s);
}

void relay_udp(SOCKS_STATE *state)
{
  fd_set   rfds;
  int      nfds, sfd;
  struct   timeval tv;
  struct   timezone tz;
  ssize_t  wc;
  rlyinfo  ri;
  int      done;
  u_long   max_count = idle_timeout;
  u_long   timeout_count;
  struct sockaddr_storage ss;
  LOGINFO	li;

  memset(&ri, 0, sizeof(ri));
  memset(&li, 0, sizeof(li));
  ri.ss = (struct sockaddr *)&ss;
  ri.flags = 0;
  ri.nr = BUFSIZE-sizeof(UDPH);

  setsignal(SIGALRM, timeout);
  gettimeofday(&li.start, &tz);
  li.bc = li.upl = li.dnl = 0;
  timeout_count = 0;
  for (;;) {
    FD_ZERO(&rfds);
    FD_SET(state->s, &rfds); FD_SET(state->sr.udp->d, &rfds);
    nfds = MAX(state->s, state->sr.udp->d);
    if (state->r >= 0) {
      FD_SET(state->r, &rfds);
      nfds = MAX(nfds, state->r);
    }
    if (state->sr.udp->u >= 0) {
      FD_SET(state->sr.udp->u, &rfds);
      nfds = MAX(nfds, state->sr.udp->u);
    }

    done = 0;
    /* idle timeout related setting. */
    tv.tv_sec = 60; tv.tv_usec = 0;   /* unit = 1 minute. */
    tz.tz_minuteswest = 0; tz.tz_dsttime = 0;
    sfd = select(nfds+1, &rfds, 0, 0, &tv);
    if (sfd > 0) {
      /* UDP channels */
      /* in case of UDP, wc == 0 does not mean EOF (??) */
      if (FD_ISSET(state->sr.udp->d, &rfds)) {
	ri.from = state->sr.udp->d; ri.to = state->sr.udp->u;
	ri.dir = UP;
	if ((wc = forward_udp(&ri, state->sr.udp, state->hops)) < 0)
	  done++;
	else
	  li.bc += wc; li.upl += wc;
	FD_CLR(state->sr.udp->d, &rfds);
      }
      if (state->sr.udp->u >= 0 && FD_ISSET(state->sr.udp->u, &rfds)) {
	ri.from = state->sr.udp->u; ri.to = state->sr.udp->d;
	ri.dir = DOWN;
	if ((wc = forward_udp(&ri, state->sr.udp, state->hops)) < 0)
	  done++;
	else
	  li.bc += wc; li.dnl += wc;
	FD_CLR(state->sr.udp->d, &rfds);
      }
      /* packets on TCP channel may indicate
	 termination of UDP assoc.
      */
      if (FD_ISSET(state->s, &rfds)) {
	ri.from = state->s; ri.to = state->r; ri.flags = 0;
	if ((wc = forward(&ri)) <= 0)
	  done++;
	FD_CLR(state->s, &rfds);
      }
      if (FD_ISSET(state->r >= 0 && state->r, &rfds)) {
	ri.from = state->r; ri.to = state->s; ri.flags = 0;
	if ((wc = forward(&ri)) <= 0)
	  done++;
	FD_CLR(state->r, &rfds);
      }
      if (done > 0)
	break;
    } else if (sfd < 0) {
      if (errno != EINTR)
	break;
    } else { /* sfd == 0 */
      if (max_count != 0) {
	timeout_count++;
	if (timeout_count > max_count)
	  break;
      }
    }
  }

  gettimeofday(&li.end, &tz);
  /* getsockname for logging */
  state->sr.udp->si.myc.len = SS_LEN;
  getsockname(state->sr.udp->d,
	      &state->sr.udp->si.myc.addr.sa,
	      (socklen_t *)&state->sr.udp->si.myc.len);
  if (state->sr.udp->u >= 0) {
    state->sr.udp->si.mys.len = SS_LEN;
    getsockname(state->sr.udp->u,
		&state->sr.udp->si.mys.addr.sa,
		(socklen_t *)&state->sr.udp->si.mys.len);
  }

  log_transfer(&state->sr.udp->si, &li);

  close(state->s);
  if (state->r >= 0)
    close(state->r);
  close(state->sr.udp->d);
  if (state->sr.udp->u >= 0)
    close(state->sr.udp->u);
  if (state->sr.udp != NULL)
    free(state->sr.udp);
}

int log_transfer(SOCK_INFO *si, LOGINFO *li)
{

  char    myc_ip[NI_MAXHOST], mys_ip[NI_MAXHOST];
  char    prc_ip[NI_MAXHOST], prs_ip[NI_MAXHOST];
  char    myc_port[NI_MAXSERV], mys_port[NI_MAXSERV];
  char    prc_port[NI_MAXSERV], prs_port[NI_MAXSERV];
  struct timeval elp;

  memcpy(&elp, &li->end, sizeof(struct timeval));
  if (elp.tv_usec < li->start.tv_usec) {
    elp.tv_sec--; elp.tv_usec += 1000000;
  }
  elp.tv_sec  -= li->start.tv_sec;
  elp.tv_usec -= li->start.tv_usec;

  *myc_ip = *mys_ip = *prc_ip = *prs_ip = '\0';
  *myc_port = *mys_port = *prc_port = *prs_port = '\0';

  getnameinfo(&si->myc.addr.sa, si->myc.len,
	      myc_ip, sizeof(myc_ip),
	      myc_port, sizeof(myc_port),
	      NI_NUMERICHOST|NI_NUMERICSERV);
  getnameinfo(&si->mys.addr.sa, si->mys.len,
	      mys_ip, sizeof(mys_ip),
	      mys_port, sizeof(mys_port),
	      NI_NUMERICHOST|NI_NUMERICSERV);
  getnameinfo(&si->prc.addr.sa, si->prc.len,
	      prc_ip, sizeof(prc_ip),
	      prc_port, sizeof(prc_port),
	      NI_NUMERICHOST|NI_NUMERICSERV);
  getnameinfo(&si->prs.addr.sa, si->prs.len,
	      prs_ip, sizeof(prs_ip),
	      prs_port, sizeof(prs_port),
	      NI_NUMERICHOST|NI_NUMERICSERV);
  
  msg_out(norm, "%s:%s-%s:%s/%s:%s-%s:%s %u(%u/%u) %u.%06u",
	  prc_ip, prc_port, myc_ip, myc_port,
	  mys_ip, mys_port, prs_ip, prs_port,
	  li->bc, li->upl, li->dnl,
	  elp.tv_sec, elp.tv_usec);

  return(0);
}
