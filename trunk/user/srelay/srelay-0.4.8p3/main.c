/*
  main.c:
  $Id: main.c,v 1.29 2017/09/06 13:35:01 bulkstream Exp $

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

#include <sys/stat.h>
#include "srelay.h"

/* prototypes */
void show_version  __P((void));
void usage	   __P((void));
int serv_loop	   __P((void));
int validate_access __P((CL_INFO *));

char *config = CONFIG;
char *ident = "srelay";
char *pidfile = PIDFILE;
char *pwdfile = NULL;
char *localpwd = NULL;
char *bindtodevice = NULL;
pid_t master_pid;

#if USE_THREAD
pthread_t main_thread;
int max_thread;
int threading;
#endif

int same_interface = 0;

#ifdef HAVE_LIBWRAP
int use_tcpwrap = 0;
# include <tcpd.h>
# ifdef LINUX
int    allow_severity = LOG_AUTH|LOG_INFO;
int    deny_severity  = LOG_AUTH|LOG_NOTICE;
# endif /* LINUX */
extern int hosts_ctl __P((char *, char *, char *, char *));
#endif /* HAVE_LIBWRAP */

int max_child;
int cur_child;

int fg;        /* foreground operation */
int inetd_mode = 0;  /* inetd mode */
int bind_restrict = 1; /* socks bind port is restricted */

/* authentication method priority table */
int method_num;
char method_tab[MAX_AUTH_METH+1];

int verbosity = 0;

void show_version()
{
  fprintf(stderr, "%s\n", version);
}

void usage()
{
  show_version();
  fprintf(stderr, "usage: %s [options]\n",
	  ident);
  fprintf(stderr, "options:\n"
	  "\t-c file\tconfig file\n"
	  "\t-i i/f\tlisten interface IP[:PORT]\n"
#ifdef SO_BINDTODEVICE
	  "\t-J i/f\toutbound interface name\n"
#endif
	  "\t-m num\tmax child/thread\n"
	  "\t-o min\tidle timeout minutes\n"
	  "\t-p file\tpid file\n"
	  "\t-a np\tauth methods n: no, p:pass\n"
	  "\t-u file\tsrelay password file\n"
	  "\t-U file\tsrelay local user base file\n"
	  "\t-f\trun into foreground\n"
	  "\t-r\tresolve client name in log\n"
	  "\t-s\tforce logging to syslog\n"
	  "\t-t\tdisable threading\n"
	  "\t-b\tavoid BIND port restriction\n"
	  "\t-g\tuse the same interface for outbound as inbound\n"
#ifdef HAVE_LIBWRAP
	  "\t-w\tuse tcp_wrapper access control\n"
#endif /* HAVE_LIBWRAP */
	  "\t-I\tinetd mode\n"
	  "\t-q\twill be quiet\n"
	  "\t-v\tincrease verbosity\n"
	  "\t-V\tshow version and exit\n"
	  "\t-h\tshow this help and exit\n");
  exit(1);
}

void signal_setup()
{

  setsignal(SIGHUP, reload);
  if (!isatty(fileno(stdin))) {
    setsignal(SIGINT, SIG_IGN);
  }
  setsignal(SIGQUIT, SIG_IGN);
  setsignal(SIGILL, SIG_IGN);
  setsignal(SIGTRAP, SIG_IGN);
  setsignal(SIGABRT, SIG_IGN);
#ifdef SIGEMT
  setsignal(SIGEMT, SIG_IGN);
#endif
  setsignal(SIGFPE, SIG_IGN);
  setsignal(SIGBUS, SIG_IGN);
  setsignal(SIGSEGV, SIG_IGN);
  setsignal(SIGSYS, SIG_IGN);
  setsignal(SIGPIPE, SIG_IGN);
  setsignal(SIGALRM, SIG_IGN);
  setsignal(SIGTERM, cleanup);
  setsignal(SIGUSR1, SIG_IGN);
  setsignal(SIGUSR2, SIG_IGN);
#ifdef SIGPOLL
  setsignal(SIGPOLL, SIG_IGN);
#endif
  setsignal(SIGVTALRM, SIG_IGN);
  setsignal(SIGPROF, SIG_IGN);
  setsignal(SIGXCPU, SIG_IGN);
  setsignal(SIGXFSZ, SIG_IGN);

}

int validate_access(CL_INFO *client)
{
  int stat = 0;
#ifdef HAVE_LIBWRAP
  int i;

  if ( use_tcpwrap ) {
    /* proc ident pattern */
    stat = hosts_ctl(ident, client->name, client->addr, STRING_UNKNOWN);
    /* IP.PORT pattern */
    for (i = 0; i < serv_sock_ind; i++) {
      if (str_serv_sock[i] != NULL && str_serv_sock[i][0] != 0) {
	stat |= hosts_ctl(str_serv_sock[i],
			  client->name, client->addr, STRING_UNKNOWN);
      }
    }
  } else {
#endif /* HAVE_LIBWRAP */
    stat = 1;  /* allow access un-conditionaly */
#ifdef HAVE_LIBWRAP
  }
#endif /* HAVE_LIBWRAP */

  if (stat < 1) {
    msg_out(warn, "%s[%s] access denied.", client->name, client->addr);
  }

  return stat;
}

#ifdef USE_THREAD
pthread_mutex_t mutex_select;
#endif

int serv_loop()
{

  SOCKS_STATE	state;
  SOCK_INFO	si;
  CL_INFO	client;

  int    cs;
  fd_set readable;
  int    i, n, len;
  int    error;
  pid_t  pid;

  memset(&state, 0, sizeof(state));
  memset(&si, 0, sizeof(si));
  memset(&client, 0, sizeof(client));
  state.si = &si;

#ifdef USE_THREAD
  if (threading) {
    blocksignal(SIGHUP);
  }
#endif

  for (;;) {
    readable = allsock;

    MUTEX_LOCK(mutex_select);
    n = select(maxsock+1, &readable, 0, 0, 0);
    if (n <= 0) {
      if (n < 0 && errno != EINTR) {
        msg_out(warn, "select: %m");
      }
      MUTEX_UNLOCK(mutex_select);
      continue;
    }

#ifdef USE_THREAD
    if ( ! threading ) {
#endif
      /* handle any queued signal flags */
      if (FD_ISSET(sig_queue[0], &readable)) {
        if (ioctl(sig_queue[0], FIONREAD, &i) != 0) {
          msg_out(crit, "ioctl: %m");
          exit(-1);
        }
        while (--i >= 0) {
          char c;
          if (read(sig_queue[0], &c, 1) != 1) {
            msg_out(crit, "read: %m");
            exit(-1);
          }
          switch(c) {
          case 'H': /* sighup */
            reload();
            break;
          case 'C': /* sigchld */
            reapchild();
            break;
          case 'T': /* sigterm */
            cleanup();
            break;
          default:
            break;
          }
        }
      }
#ifdef USE_THREAD
    }
#endif

    for ( i = 0; i < serv_sock_ind; i++ ) {
      if (FD_ISSET(serv_sock[i], &readable)) {
	n--;
	break;
      }
    }
    if ( n < 0 || i >= serv_sock_ind ) {
      MUTEX_UNLOCK(mutex_select);
      continue;
    }

    len = SS_LEN;
    cs = accept(serv_sock[i], &si.prc.addr.sa, (socklen_t *)&len);
    si.prc.len = len;
    if (cs < 0) {
      if (errno == EINTR
#ifdef SOLARIS
	  || errno == EPROTO
#endif
	  || errno == EWOULDBLOCK
	  || errno == ECONNABORTED) {
	; /* ignore */
      } else {
	/* real accept error */
	msg_out(warn, "accept: %m");
      }
      MUTEX_UNLOCK(mutex_select);
      continue;
    }
    MUTEX_UNLOCK(mutex_select);

#ifdef USE_THREAD
    if ( !threading ) {
#endif
      if (max_child > 0 && cur_child >= max_child) {
	msg_out(warn, "child: cur %d; exeedeing max(%d)",
		          cur_child, max_child);
	close(cs);
	continue;
      }
#ifdef USE_THREAD
    }
#endif

    /* get downstream-side socket name */
    len = SS_LEN;
    getsockname(cs, &si.myc.addr.sa, (socklen_t *)&len);
    si.myc.len = len;

    error = getnameinfo(&si.prc.addr.sa, si.prc.len,
			client.addr, sizeof(client.addr),
			NULL, 0,
			NI_NUMERICHOST);
    if (resolv_client) {
      error = getnameinfo(&si.prc.addr.sa, si.prc.len,
			  client.name, sizeof(client.name),
			  NULL, 0, 0);
      msg_out(norm, "%s[%s] connected", client.name, client.addr);
    } else {
      msg_out(norm, "%s connected", client.addr);
      strncpy(client.name, client.addr, sizeof(client.name));
    }

    i = validate_access(&client);
    if (i < 1) {
      /* access denied */
      close(cs);
      continue;
    }

    set_blocking(cs);
    state.s = cs;

#ifdef USE_THREAD
    if (!threading ) {
#endif
      blocksignal(SIGHUP);
      blocksignal(SIGCHLD);
      pid = fork();
      switch (pid) {
      case -1:  /* fork child failed */
	break;
      case 0:   /* i am child */
	for ( i = 0; i < serv_sock_ind; i++ ) {
	  close(serv_sock[i]);
	}
	setsignal(SIGCHLD, SIG_DFL);
        setsignal(SIGHUP, SIG_DFL);
        releasesignal(SIGCHLD);
        releasesignal(SIGHUP);
	error = proto_socks(&state);
	if ( error == -1 ) {
	  close(state.s);  /* may already be closed */
	  exit(1);
	}
	relay(&state);
	exit(0);
      default: /* may be parent */
	proclist_add(pid);
	break;
      }
      close(state.s);
      releasesignal(SIGHUP);
      releasesignal(SIGCHLD);
#ifdef USE_THREAD
    } else {
      error = proto_socks(&state);
      if ( error == -1 ) {
	close(state.s);  /* may already be closed */
	/* udp may be dynamically allocated */
	if (state.sr.udp != NULL)
	  free(state.sr.udp);
	if (state.prx != NULL) {
	  free(state.prx);
	  state.prx = NULL;
	}
	continue;
      }
      relay(&state);
    }
#endif
  }
}

int inetd_service(int cs)
{
  SOCKS_STATE	state;
  SOCK_INFO	si;
  CL_INFO	client;
  int    len;
  int    error;
 
  memset(&state, 0, sizeof(state));
  memset(&si, 0, sizeof(si));
  state.si = &si;

  signal_setup();

  /* get downstream-side socket name */
  len = SS_LEN;
  getsockname(cs, &si.myc.addr.sa, (socklen_t *)&len);
  si.myc.len = len;

  /* get downstream-side peer name */
  len = SS_LEN;
  getpeername(cs, &si.prc.addr.sa, (socklen_t *)&len);
  si.prc.len = len;

  error = getnameinfo(&si.prc.addr.sa, si.prc.len,
		      client.addr, sizeof(client.addr),
		      NULL, 0,
		      NI_NUMERICHOST);
  if (resolv_client) {
    error = getnameinfo(&si.prc.addr.sa, si.prc.len,
			client.name, sizeof(client.name),
			NULL, 0, 0);
    msg_out(norm, "%s[%s] connected", client.name, client.addr);
  } else {
    msg_out(norm, "%s connected", client.addr);
    strncpy(client.name, client.addr, sizeof(client.name));
  }

  set_blocking(cs);
  state.s = cs;

  error = proto_socks(&state);
  if (error == -1) {
    msg_out(warn, "socks proto error");
    close(state.s);
    if (state.sr.udp != NULL)
      free(state.sr.udp);
    return(-1);
  }
  /* start relaying */
  relay(&state);
  close(cs);
  return(0);
}

int main(int ac, char **av)
{
  int     ch, i=0;
  char    *p;
  pid_t   pid;
  FILE    *fp;
  uid_t   uid;
#ifdef USE_THREAD
  pthread_t tid;
  pthread_attr_t attr;
  struct rlimit rl;
  rlim_t max_fd = (rlim_t)MAX_FD;
  rlim_t save_fd = 0;
#endif

#ifdef USE_THREAD
  threading = 1;
  max_thread = MAX_THREAD;
#endif

  max_child = MAX_CHILD;
  cur_child = 0;

  /* create service socket table (malloc) */
  if (serv_init(NULL) < 0) {
    msg_out(crit, "cannot malloc: %m\n");
    exit(-1);
  }

  proxy_tbl = NULL;
  num_routes = 0;

  method_num = 0;
  method_tab[0] = '\0';

  uid = getuid();

  openlog(ident, LOG_PID | LOG_NDELAY, SYSLOGFAC);

  while((ch = getopt(ac, av, "a:c:i:J:m:o:p:u:U:frstbwgIqvVh?")) != -1) {
    switch (ch) {
    case 'a':
      if (optarg != NULL) {
	if (strchr(optarg, 'n') != NULL) {
	  /* no-auth resets any other options */
	  method_num = 0;  /* reset auth methods */
	    break;
	    }

	for (p=optarg, i=0; method_num < MAX_AUTH_METH && i < MAX_AUTH_METH; p++, i++) {
	  if (*p == '\0')
	    break;
	  switch (*p) {
	  case 'p':
	    if (memchr(method_tab, 'p', method_num) == NULL) {
	      method_tab[method_num] = S5AUSRPAS;
	    method_num++;
	    }
	    break;
	  default:
	    /* un-supported/invalid method */
	    break;
	  }
	}
      }
      break;

    case 'b':
      bind_restrict = 0;
      break;

    case 'c':
      if (optarg != NULL) {
        config = strdup(optarg);
      }
      break;

    case 'u':
      if (optarg != NULL) {
        pwdfile = strdup(optarg);
      }
      break;

    case 'U':
      if (optarg != NULL) {
        localpwd = strdup(optarg);
      }
      break;

    case 'i':
      if (optarg != NULL) {
	if (serv_init(optarg) < 0) {
	  msg_out(warn, "cannot init server socket(-i %s): %m\n", optarg);
	  break;
	}
      }
      break;

#ifdef SO_BINDTODEVICE
    case 'J':
      if (optarg != NULL) {
	bindtodevice = strdup(optarg);
      }
      break;
#endif

    case 'o':
      if (optarg != NULL) {
	idle_timeout = atol(optarg);
      }
      break;

    case 'p':
      if (optarg != NULL) {
	pidfile = strdup(optarg);
      }
      break;

    case 'm':
      if (optarg != NULL) {
#ifdef USE_THREAD
	max_thread = atoi(optarg);
#endif
	max_child = atoi(optarg);
      }
      break;

    case 't':
#ifdef USE_THREAD
      threading = 0;    /* threading disabled. */
#endif
      break;

    case 'g':
      same_interface = 1;
      break;

    case 'f':
      fg = 1;
      break;

    case 'r':
      resolv_client = 1;
      break;

    case 's':
      forcesyslog = 1;
      break;

    case 'w':
#ifdef HAVE_LIBWRAP
      use_tcpwrap = 1;
#endif /* HAVE_LIBWRAP */
      break;

    case 'I':
      inetd_mode = 1;
      break;

    case 'q':
      be_quiet = 1;
      break;

    case 'v':
      verbosity++;
      break;

    case 'V':
      show_version();
      exit(1);

    case 'h':
    case '?':
    default:
      usage();
    }
  }

  ac -= optind;
  av += optind;

  if ((fp = fopen(config, "r")) != NULL) {
    if (readconf(fp) != 0) {
      /* readconf error */
      exit(1);
    }
    fclose(fp);
  }

  if ( method_num > 0
       && uid != 0
       && memchr(method_tab, 'p', method_num) != NULL
       && localpwd == NULL) {
    /* process does not started by root */
    msg_out(warn, "uid == %d (!=0),"
	    "user/pass auth may not work.\n", uid);
  }

  if (inetd_mode) {
    /* close all server socket if opened */
    close_all_serv();
    /* assuming that STDIN_FILENO handles bi-directional
     */
    exit(inetd_service(STDIN_FILENO));
    /* not reached */
  }

  if (serv_sock_ind == 0) {   /* no valid ifs yet */
    if (serv_init(":") < 0) { /* use default */
      /* fatal */
      msg_out(crit, "cannot open server socket\n");
      exit(1);
    }
  }

#ifdef USE_THREAD
  if ( ! threading ) {
#endif
    if (queue_init() != 0) {
      msg_out(crit, "cannot init signal queue\n");
      exit(1);
    }
#ifdef USE_THREAD
  }
#endif

  /* try changing working directory */
  if ( chdir(WORKDIR0) != 0 )
    if ( chdir(WORKDIR1) != 0 )
      msg_out(norm, "giving up chdir to workdir");

  if (!fg) {
    /* force stdin/out/err allocate to /dev/null */
    fclose(stdin);
    fp = fopen("/dev/null", "w+");
    if (fileno(fp) != STDIN_FILENO) {
      msg_out(crit, "fopen: %m");
      exit(1);
    }
    if (dup2(STDIN_FILENO, STDOUT_FILENO) == -1) {
      msg_out(crit, "dup2-1: %m");
      exit(1);
    }
    if (dup2(STDIN_FILENO, STDERR_FILENO) == -1) {
      msg_out(crit, "dup2-2: %m");
      exit(1);
    }

    switch(fork()) {
    case -1:
      msg_out(crit, "fork: %m");
      exit(1);
    case 0:
      /* child */
      pid = setsid();
      if (pid == -1) {
	msg_out(crit, "setsid: %m");
	exit(1);
      }
      break;
    default:
      /* parent */
      exit(0);
    }
  }

  master_pid = getpid();
  umask(S_IWGRP|S_IWOTH);
  if ((fp = fopen(pidfile, "w")) != NULL) {
    fprintf(fp, "%u\n", (unsigned)master_pid);
    fchown(fileno(fp), PROCUID, PROCGID);
    fclose(fp);
  } else {
    msg_out(warn, "cannot open pidfile %s", pidfile);
  }

  signal_setup();

#ifdef USE_THREAD
  if ( threading ) {
    if (max_thread <= 0 || max_thread > THREAD_LIMIT) {
      max_thread = THREAD_LIMIT;
    }
    /* resource limit is problem in threadig (e.g. Solaris:=64)*/
    memset((caddr_t)&rl, 0, sizeof rl);
    if (getrlimit(RLIMIT_NOFILE, &rl) != 0)
      msg_out(warn, "getrlimit: %m");
    else
      save_fd = rl.rlim_cur;
    if (rl.rlim_cur < (rlim_t)max_fd)
      rl.rlim_cur = max_fd;        /* willing to fix to max_fd */
    if ( rl.rlim_cur != save_fd )  /* if rlim_cur is changed   */
      if (setrlimit(RLIMIT_NOFILE, &rl) != 0)
        msg_out(warn, "cannot set rlimit(max_fd)");

    setregid(-1, PROCGID);
    setreuid(-1, PROCUID);

    pthread_mutex_init(&mutex_select, NULL);
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    msg_out(norm, "Starting: MAX_TH(%d)", max_thread);
    for (i=0; i<max_thread; i++) {
      if (pthread_create(&tid, &attr,
			 (void *)&serv_loop, (void *)NULL) != 0)
        exit(1);
    }
    main_thread = pthread_self();   /* store main thread ID */
    for (;;) {
      pause();
    }
  } else {
#endif
    setsignal(SIGCHLD, reapchild);
    setregid(-1, PROCGID);
    setreuid(-1, PROCUID);
    msg_out(norm, "Starting: MAX_CH(%d)", max_child);
    serv_loop();
#ifdef USE_THREAD
  }
#endif
  return(0);
}
