/*
  auth-pwd.c
  $Id: auth-pwd.c,v 1.16 2017/09/01 05:59:24 bulkstream Exp $

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
#if defined(FREEBSD) || defined(MACOSX)
#include <pwd.h>
#elif defined(LINUX) || defined(SOLARIS)
#include <shadow.h>
#include <crypt.h>
#endif

#define TIMEOUTSEC    30

/* proto types */
int checkpasswd(char *, char *);

int auth_pwd_server(int s)
{
  u_char buf[512];
  int  r, len;
  char user[256];
  char pass[256];
  struct sockaddr_storage client;
  char client_ip[NI_MAXHOST];
  int  code = 0;

  r = timerd_read(s, buf, sizeof(buf), TIMEOUTSEC, MSG_PEEK);
  if ( r < 2 ) {
    return(-1);
  }
  if (buf[0] != 0x01) { /* current username/password auth version */
    /* error in version */
    return(-1);
  }
  len = buf[1];
  if (len < 1 || len > 255) {
    /* invalid username len */
    return(-1);
  }
  /* read username */
  r = timerd_read(s, buf, 2+len, TIMEOUTSEC, 0);
  if (r < 2+len) {
    /* read error */
    return(-1);
  }
  strncpy(user, (char *)&buf[2], len);
  user[len] = '\0';

  /* get passwd */
  r = timerd_read(s, buf, sizeof(buf), TIMEOUTSEC, MSG_PEEK);
  if ( r < 1 ) {
    return(-1);
  }
  len = buf[0];
  if (len < 1 || len > 255) {
    /* invalid password len */
    return(-1);
  }
  /* read passwd */
  r = timerd_read(s, buf, 1+len, TIMEOUTSEC, 0);
  if (r < 1+len) {
    /* read error */
    return(-1);
  }
  strncpy(pass, (char *)&buf[1], len);
  pass[len] = '\0';

  /* logging */
  len = sizeof(struct sockaddr_storage);
  if (getpeername(s, (struct sockaddr *)&client, (socklen_t *)&len) == 0
      && getnameinfo((struct sockaddr *)&client, len,
		     client_ip, sizeof(client_ip),
		     NULL, 0, NI_NUMERICHOST) == 0) {
    ;; /* OK */
  } else {
    client_ip[0] = '\0';
  }

  /* do authentication */
  if (localpwd != NULL) { /* localpwd(global) */
    r = checklocalpwd(user, pass, localpwd);
  } else {
    r = checkpasswd(user, pass);
  }

  msg_out(norm, "%s 5-U/P_AUTH for %s using %s.", client_ip,
	  user, (localpwd != NULL) ? "Local pwd" : "System pwd");
  msg_out(norm, "%s 5-U/P_AUTH for %s %s.", client_ip,
	  user, r == 0 ? "accepted" : "denied");

  /* erase uname and passwd storage */
  memset(user, 0, sizeof(user));
  memset(pass, 0, sizeof(pass));

  code = ( r == 0 ? 0 : -1 );

  /* reply to client */
  buf[0] = 0x01;  /* sub negotiation version */
  buf[1] = code & 0xff;  /* grant or not */
  r = timerd_write(s, buf, 2, TIMEOUTSEC);
  if (r < 2) {
    /* write error */
    return(-1);
  }
  return(code);   /* access granted or not */
}

int auth_pwd_client(int s, bin_addr *proxy, u_int16_t pport)
{
  u_char buf[640];
  int  r, ret, done;
  struct user_pass up;

  ret = -1; done = 0;
  /* get username/password */
  r = getpasswd(proxy, pport, &up, pwdfile); /* pwdfile(global) */
  if ( r == 0 ) { /* getpasswd gets match */
    if ( up.ulen >= 1 && up.ulen <= 255
	 && up.plen >= 1 && up.plen <= 255 ) {
      /* build auth data */
      buf[0] = 0x01;
      buf[1] = up.ulen & 0xff;
      memcpy(&buf[2], up.user, up.ulen);
      buf[2+up.ulen] = up.plen & 0xff;
      memcpy(&buf[2+up.ulen+1], up.pass, up.plen);
      done++;
    }
  }
  
  if (! done) {
    /* build fake auth data */
    /* little bit BAD idea */
    buf[0] = 0x01;
    buf[1] = 0x01;
    buf[2] = ' ';
    buf[3] = 0x01;
    buf[4] = ' ';
    up.ulen = up.plen = 1;
  }

  r = timerd_write(s, buf, 3+up.ulen+up.plen, TIMEOUTSEC);
  if (r < 3+up.ulen+up.plen) {
    /* cannot write */
    goto err_ret;
  }

  /* get server reply */
  r = timerd_read(s, buf, 2, TIMEOUTSEC, 0);
  if (r < 2) {
    /* cannot read */
    goto err_ret;
  }
  if (buf[0] == 0x01 && buf[1] == 0) {
    /* username/passwd auth succeded */
    ret = 0;
  }
 err_ret:

  /* erase uname and passwd storage */
  memset(&up, 0, sizeof(struct user_pass));
  return(ret);
}

int checkpasswd(char *user, char *pass)
{

#if defined(FREEBSD) || defined(MACOSX)
  struct passwd pwbuf, *pwbufp = &pwbuf;
#elif defined(LINUX) || defined(SOLARIS)
  struct spwd spbuf, *spbufp = &spbuf;
#endif

  char   buf[2048];
  int matched = 0;

  if (user == NULL) {
    /* user must be specified */
    return(-1);
  }

  setuid(0);
#if defined(FREEBSD) || defined(MACOSX)
  getpwnam_r(user, &pwbuf, buf, sizeof(buf), &pwbufp);
#elif defined(LINUX)
  getspnam_r(user, &spbuf, buf, sizeof(buf), &spbufp);
#elif defined(SOLARIS)
  spbufp = getspnam_r(user, &spbuf, buf, sizeof buf);
#endif
  setreuid(-1, PROCUID);


#if defined(FREEBSD) || defined(MACOSX)
  if (pwbufp == NULL) {
    /* error in getpwnam */
    return(-1);
  }
  if (pwbufp->pw_passwd == NULL && pass == NULL) {
    /* null password matched */
    return(0);
  }
  if (*pwbufp->pw_passwd) {
    if (strcmp(pwbufp->pw_passwd, crypt(pass, pwbufp->pw_passwd)) == 0) {
      matched = 1;
    }
  }

#elif defined(LINUX) || defined(SOLARIS)
  if (spbufp == NULL) {
    /* error in getpwnam */
    return(-1);
  }
  if (spbufp->sp_pwdp == NULL && pass == NULL) {
    /* null password matched */
    return(0);
  }
  if (*spbufp->sp_pwdp) {
    if (strcmp(spbufp->sp_pwdp, crypt(pass, spbufp->sp_pwdp)) == 0) {
      matched = 1;
    }
  }
#endif

  memset(buf, 0, sizeof(buf));

  
  if (matched) {
    return(0);
  } else {
    return(-1);
  }
}
