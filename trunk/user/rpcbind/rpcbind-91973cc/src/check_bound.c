/*	$NetBSD: check_bound.c,v 1.2 2000/06/22 08:09:26 fvdl Exp $	*/
/*	$FreeBSD: src/usr.sbin/rpcbind/check_bound.c,v 1.4 2002/10/07 02:56:59 alfred Exp $ */

/*
 * Copyright (c) 2009, Sun Microsystems, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of Sun Microsystems, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Copyright (c) 1986 - 1991 by Sun Microsystems, Inc.
 */

/* #ident	"@(#)check_bound.c	1.15	93/07/05 SMI" */

#if 0
#ifndef lint
static	char sccsid[] = "@(#)check_bound.c 1.11 89/04/21 Copyr 1989 Sun Micro";
#endif
#endif

/*
 * check_bound.c
 * Checks to see whether the program is still bound to the
 * claimed address and returns the univeral merged address
 *
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <rpc/rpc.h>
#include <stdio.h>
#include <netconfig.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "rpcbind.h"

struct fdlist {
	int fd;
	struct netconfig *nconf;
	struct fdlist *next;
	int check_binding;
};

static struct fdlist *fdhead;	/* Link list of the check fd's */
static struct fdlist *fdtail;
static char *nullstring = "";

static bool_t check_bound(struct fdlist *, char *uaddr);

/*
 * Returns 1 if the given address is bound for the given addr & transport
 * For all error cases, we assume that the address is bound
 * Returns 0 for success.
 */
static bool_t
check_bound(struct fdlist *fdl, char *uaddr)
{
	int fd;
	struct netbuf *na;
	int ans;

	if (fdl->check_binding == FALSE)
		return (TRUE);

	na = uaddr2taddr(fdl->nconf, uaddr);
	if (!na)
		return (TRUE); /* punt, should never happen */

	fd = __rpc_nconf2fd(fdl->nconf);
	if (fd < 0) {
		free(na->buf);
		free(na);
		return (TRUE);
	}

	ans = bind(fd, (struct sockaddr *)na->buf, na->len);

	close(fd);
	free(na->buf);
	free(na);

	return (ans == 0 ? FALSE : TRUE);
}

int
add_bndlist(struct netconfig *nconf, struct netbuf *baddr /*__unused*/)
{
	struct fdlist *fdl;
	struct netconfig *newnconf;

	newnconf = getnetconfigent(nconf->nc_netid);
	if (newnconf == NULL)
		return (-1);
	fdl = malloc(sizeof (struct fdlist));
	if (fdl == NULL) {
		freenetconfigent(newnconf);
		syslog(LOG_ERR, "no memory!");
		return (-1);
	}
	fdl->nconf = newnconf;
	fdl->next = NULL;
	if (fdhead == NULL) {
		fdhead = fdl;
		fdtail = fdl;
	} else {
		fdtail->next = fdl;
		fdtail = fdl;
	}
	/* XXX no bound checking for now */
	fdl->check_binding = FALSE;

	return 0;
}

bool_t
is_bound(char *netid, char *uaddr)
{
	struct fdlist *fdl;

	for (fdl = fdhead; fdl; fdl = fdl->next)
		if (strcmp(fdl->nconf->nc_netid, netid) == 0)
			break;
	if (fdl == NULL)
		return (TRUE);
	return (check_bound(fdl, uaddr));
}

/*
 * Returns NULL if there was some system error.
 * Returns "" if the address was not bound, i.e the server crashed.
 * Returns the merged address otherwise.
 */
char *
mergeaddr(SVCXPRT *xprt, char *netid, char *uaddr, char *saddr)
{
	struct fdlist *fdl;
	char *c_uaddr, *s_uaddr, *m_uaddr, *allocated_uaddr = NULL;

	for (fdl = fdhead; fdl; fdl = fdl->next)
		if (strcmp(fdl->nconf->nc_netid, netid) == 0)
			break;
	if (fdl == NULL)
		return (NULL);
	if (check_bound(fdl, uaddr) == FALSE)
		/* that server died */
		return (nullstring);
	/*
	 * If saddr is not NULL, the remote client may have included the
	 * address by which it contacted us.  Use that for the "client" uaddr,
	 * otherwise use the info from the SVCXPRT.
	 */
	if (saddr != NULL) {
		c_uaddr = saddr;
	} else {
		c_uaddr = taddr2uaddr(fdl->nconf, svc_getrpccaller(xprt));
		if (c_uaddr == NULL) {
			syslog(LOG_ERR, "taddr2uaddr failed for %s",
				fdl->nconf->nc_netid);
			return (NULL);
		}
		allocated_uaddr = c_uaddr;
	}

#ifdef ND_DEBUG
	if (debugging) {
		if (saddr == NULL) {
			fprintf(stderr, "mergeaddr: client uaddr = %s\n",
			    c_uaddr);
		} else {
			fprintf(stderr, "mergeaddr: contact uaddr = %s\n",
			    c_uaddr);
		}
	}
#endif
	s_uaddr = uaddr;
	/*
	 * This is all we should need for IP 4 and 6
	 */
	m_uaddr = addrmerge(svc_getrpccaller(xprt), s_uaddr, c_uaddr, netid);
#ifdef ND_DEBUG
	if (debugging)
		fprintf(stderr, "mergeaddr: uaddr = %s, merged uaddr = %s\n",
				uaddr, m_uaddr);
#endif
	if (allocated_uaddr != NULL)
		free(allocated_uaddr);
	return (m_uaddr);
}

/*
 * Returns a netconf structure from its internal list.  This
 * structure should not be freed.
 */
struct netconfig *
rpcbind_get_conf(char *netid)
{
	struct fdlist *fdl;

	for (fdl = fdhead; fdl; fdl = fdl->next)
		if (strcmp(fdl->nconf->nc_netid, netid) == 0)
			break;
	if (fdl == NULL)
		return (NULL);
	return (fdl->nconf);
}
