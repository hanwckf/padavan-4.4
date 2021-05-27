/*	$NetBSD: pmap_svc.c,v 1.2 2000/10/20 11:49:40 fvdl Exp $	*/
/*	$FreeBSD: src/usr.sbin/rpcbind/pmap_svc.c,v 1.4 2002/10/07 02:56:59 alfred Exp $ */

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
 * Copyright (c) 1984 - 1991 by Sun Microsystems, Inc.
 */

/* #ident	"@(#)pmap_svc.c	1.14	93/07/05 SMI" */

#if 0
#ifndef lint
static	char sccsid[] = "@(#)pmap_svc.c 1.23 89/04/05 Copyr 1984 Sun Micro";
#endif
#endif

/*
 * pmap_svc.c
 * The server procedure for the version 2 portmaper.
 * All the portmapper related interface from the portmap side.
 */

#ifdef PORTMAP
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <rpc/rpc.h>
#include <rpc/pmap_prot.h>
#include <rpc/rpcb_prot.h>
#ifdef RPCBIND_DEBUG
#include <syslog.h>
#include <stdlib.h>
#endif
#include "rpcbind.h"
#include "xlog.h"
#include <rpc/svc_soc.h> /* svc_getcaller routine definition */
static struct pmaplist *find_service_pmap(rpcprog_t, rpcvers_t,
					       rpcprot_t);
static bool_t pmapproc_change(struct svc_req *, SVCXPRT *, u_long);
static bool_t pmapproc_getport(struct svc_req *, SVCXPRT *);
static bool_t pmapproc_dump(struct svc_req *, SVCXPRT *);

/*
 * Called for all the version 2 inquiries.
 */
void
pmap_service(struct svc_req *rqstp, SVCXPRT *xprt)
{
	rpcbs_procinfo(RPCBVERS_2_STAT, rqstp->rq_proc);
	switch (rqstp->rq_proc) {
	case PMAPPROC_NULL:
		/*
		 * Null proc call
		 */
#ifdef RPCBIND_DEBUG
		if (debugging)
			xlog(LOG_DEBUG, "PMAPPROC_NULL\n");
#endif
		check_access(xprt, rqstp->rq_proc, 0, PMAPVERS);
		if ((!svc_sendreply(xprt, (xdrproc_t) xdr_void, NULL)) &&
			debugging) {
			if (doabort) {
				rpcbind_abort();
			}
		}
		break;

	case PMAPPROC_SET:
		/*
		 * Set a program, version to port mapping
		 */
		pmapproc_change(rqstp, xprt, rqstp->rq_proc);
		break;

	case PMAPPROC_UNSET:
		/*
		 * Remove a program, version to port mapping.
		 */
		pmapproc_change(rqstp, xprt, rqstp->rq_proc);
		break;

	case PMAPPROC_GETPORT:
		/*
		 * Lookup the mapping for a program, version and return its
		 * port number.
		 */
		pmapproc_getport(rqstp, xprt);
		break;

	case PMAPPROC_DUMP:
		/*
		 * Return the current set of mapped program, version
		 */
#ifdef RPCBIND_DEBUG
		if (debugging)
			xlog(LOG_DEBUG, "PMAPPROC_DUMP\n");
#endif
		pmapproc_dump(rqstp, xprt);
		break;

	case PMAPPROC_CALLIT:
		/*
		 * Calls a procedure on the local machine. If the requested
		 * procedure is not registered this procedure does not return
		 * error information!!
		 * This procedure is only supported on rpc/udp and calls via
		 * rpc/udp. It passes null authentication parameters.
		 */
		rpcbproc_callit_com(rqstp, xprt, PMAPPROC_CALLIT, PMAPVERS);
		break;

	default:
		svcerr_noproc(xprt);
		break;
	}
}

/*
 * returns the item with the given program, version number. If that version
 * number is not found, it returns the item with that program number, so that
 * the port number is now returned to the caller. The caller when makes a
 * call to this program, version number, the call will fail and it will
 * return with PROGVERS_MISMATCH. The user can then determine the highest
 * and the lowest version number for this program using clnt_geterr() and
 * use those program version numbers.
 */
static struct pmaplist *
find_service_pmap(rpcprog_t prog, rpcvers_t vers, rpcprot_t prot)
{
	register struct pmaplist *hit = NULL;
	register struct pmaplist *pml;

	for (pml = list_pml; pml != NULL; pml = pml->pml_next) {
		if ((pml->pml_map.pm_prog != prog) ||
			(pml->pml_map.pm_prot != prot))
			continue;
		hit = pml;
		if (pml->pml_map.pm_vers == vers)
			break;
	}
	return (hit);
}

static bool_t
pmapproc_change(struct svc_req *rqstp /*__unused*/, SVCXPRT *xprt, unsigned long op)
{
	struct pmap reg;
	RPCB rpcbreg;
	long ans;
	uid_t uid;
	char uidbuf[32];
	int rc = TRUE;

	/*
	 * Can't use getpwnam here. We might end up calling ourselves
	 * and looping.
	 */
	if (__rpc_get_local_uid(xprt, &uid) < 0) {
		rpcbreg.r_owner = "unknown";
		if (is_localroot(svc_getrpccaller(xprt)))
			rpcbreg.r_owner = "superuser";
	} else if (uid == 0)
		rpcbreg.r_owner = "superuser";
	else {
		/* r_owner will be strdup-ed later */
		snprintf(uidbuf, sizeof uidbuf, "%d", uid);
		rpcbreg.r_owner = uidbuf;
	}

	if (!svc_getargs(xprt, (xdrproc_t) xdr_pmap, (char *)&reg)) {
		svcerr_decode(xprt);
		rc = FALSE;
		goto done;
	}
#ifdef RPCBIND_DEBUG
	if (debugging)
	  xlog(LOG_DEBUG, "%s request for (%lu, %lu) : ",
		  op == PMAPPROC_SET ? "PMAP_SET" : "PMAP_UNSET",
		  reg.pm_prog, reg.pm_vers);
#endif

	if (!check_access(xprt, op, reg.pm_prog, PMAPVERS)) {
		svcerr_weakauth(xprt);
		rc = (FALSE);
		goto done;
	}

	rpcbreg.r_prog = reg.pm_prog;
	rpcbreg.r_vers = reg.pm_vers;

	if (op == PMAPPROC_SET) {
		char buf[32];

		rpcbreg.r_netid = pmap_ipprot2netid(reg.pm_prot);
		if (rpcbreg.r_netid == NULL) {
			ans = FALSE;
			goto done_change;
		}
		if (!memcmp(rpcbreg.r_netid, "udp6", 4) ||
		    !memcmp(rpcbreg.r_netid, "tcp6", 4)) {
			snprintf(buf, sizeof buf, "::.%d.%d",
				(int)((reg.pm_port >> 8) & 0xff),
				(int)(reg.pm_port & 0xff));
		} else {
			snprintf(buf, sizeof buf, "0.0.0.0.%d.%d",
				(int)((reg.pm_port >> 8) & 0xff),
				(int)(reg.pm_port & 0xff));
		}
		rpcbreg.r_addr = buf;
		ans = map_set(&rpcbreg, rpcbreg.r_owner);
	} else if (op == PMAPPROC_UNSET) {
		bool_t ans1, ans2;
		rpcbreg.r_addr = NULL;
		rpcbreg.r_netid = tcptrans;
		ans1 = map_unset(&rpcbreg, rpcbreg.r_owner);
		rpcbreg.r_netid = udptrans;
		ans2 = map_unset(&rpcbreg, rpcbreg.r_owner);
		ans = ans1 || ans2;
	} else {
		ans = FALSE;
	}
done_change:
	if ((!svc_sendreply(xprt, (xdrproc_t) xdr_long, (caddr_t) &ans)) &&
	    debugging) {
		xlog(L_ERROR, "portmap: svc_sendreply failed!\n");
		if (doabort) {
			rpcbind_abort();
		}
	}
#ifdef RPCBIND_DEBUG
	if (debugging)
		xlog(LOG_DEBUG, "%s\n", ans == TRUE ? "succeeded" : "failed");
#endif
	if (op == PMAPPROC_SET)
		rpcbs_set(RPCBVERS_2_STAT, ans);
	else
		rpcbs_unset(RPCBVERS_2_STAT, ans);
done:
	if (!svc_freeargs(xprt, (xdrproc_t) xdr_pmap, (char *)&reg)) {
#ifdef RPCBIND_DEBUG
		if (debugging) {
			(void) xlog(LOG_DEBUG, "unable to free arguments\n");
			if (doabort) {
				rpcbind_abort();
			}
		}
#endif
	}
	return (rc);
}

/* ARGSUSED */
static bool_t
pmapproc_getport(struct svc_req *rqstp /*__unused*/, SVCXPRT *xprt)
{
	struct pmap reg;
	long lport;
	int port = 0;
	struct pmaplist *fnd;
#ifdef RPCBIND_DEBUG
	char *uaddr;
#endif
	int rc = TRUE;

	if (!svc_getargs(xprt, (xdrproc_t) xdr_pmap, (char *)&reg)) {
		svcerr_decode(xprt);
		rc = FALSE;
		goto done;
	}

	if (!check_access(xprt, PMAPPROC_GETPORT, reg.pm_prog, PMAPVERS)) {
		svcerr_weakauth(xprt);
		rc = FALSE;
		goto done;
	}

#ifdef RPCBIND_DEBUG
	if (debugging) {
		uaddr =  taddr2uaddr(rpcbind_get_conf(xprt->xp_netid),
			    svc_getrpccaller(xprt));
		xlog(LOG_DEBUG, "PMAP_GETPORT req for (%lu, %lu, %s) from %s :",
			reg.pm_prog, reg.pm_vers,
			pmap_ipprot2netid(reg.pm_prot)?: "<invalid>",
			uaddr);
		free(uaddr);
	}
#endif
	fnd = find_service_pmap(reg.pm_prog, reg.pm_vers, reg.pm_prot);
	if (fnd) {
		char serveuaddr[32];
		char *netid;

		netid = pmap_ipprot2netid(reg.pm_prot);
		if (netid != NULL) {
			snprintf(serveuaddr, sizeof serveuaddr,
				"0.0.0.0.%ld.%ld",
				(fnd->pml_map.pm_port >> 8) & 0xff,
				(fnd->pml_map.pm_port) & 0xff);

			if (is_bound(netid, serveuaddr)) {
				port = fnd->pml_map.pm_port;
			} else { /* this service is dead; delete it */
				delete_prog(reg.pm_prog);
			}
		}
	}

	lport = port;
	if ((!svc_sendreply(xprt, (xdrproc_t) xdr_long, (caddr_t)&lport)) &&
			debugging) {
		xlog(L_ERROR, "portmap: svc_sendreply failed!\n");
		if (doabort) {
			rpcbind_abort();
		}
	}
#ifdef RPCBIND_DEBUG
	if (debugging)
		xlog(LOG_DEBUG, "port = %d\n", port);
#endif
	rpcbs_getaddr(RPCBVERS_2_STAT, reg.pm_prog, reg.pm_vers,
		pmap_ipprot2netid(reg.pm_prot) ?: "<unknown>",
		port ? udptrans : "");

done:
	if (!svc_freeargs(xprt, (xdrproc_t) xdr_pmap, (char *)&reg)) {
#ifdef RPCBIND_DEBUG
		if (debugging) {
			(void) xlog(LOG_DEBUG, "unable to free arguments\n");
			if (doabort) {
				rpcbind_abort();
			}
		}
#endif
	}
	return (rc);
}

/* ARGSUSED */
static bool_t
pmapproc_dump(struct svc_req *rqstp /*__unused*/, SVCXPRT *xprt)
{
	int rc = TRUE;

	if (!svc_getargs(xprt, (xdrproc_t)xdr_void, NULL)) {
		svcerr_decode(xprt);
		rc = FALSE;
		goto done;
	}

	if (!check_access(xprt, PMAPPROC_DUMP, 0, PMAPVERS)) {
		svcerr_weakauth(xprt);
		rc = FALSE;
		goto done;
	}
	
	if ((!svc_sendreply(xprt, (xdrproc_t) xdr_pmaplist_ptr,
			(caddr_t)&list_pml)) && debugging) {
		xlog(L_ERROR, "portmap: svc_sendreply failed!\n");
		if (doabort) {
			rpcbind_abort();
		}
	}

done:
	if (!svc_freeargs(xprt, (xdrproc_t) xdr_void, (char *)NULL)) {
#ifdef RPCBIND_DEBUG
		if (debugging) {
			(void) xlog(LOG_DEBUG, "unable to free arguments\n");
			if (doabort) {
				rpcbind_abort();
			}
		}
#endif
	}
	return (rc);
}

int pmap_netid2ipprot(const char *netid)
{
	if (!netid)
		return 0;
	if (strcmp(netid, tcptrans) == 0)
		return IPPROTO_TCP;
	if (strcmp(netid, udptrans) == 0)
		return IPPROTO_UDP;
	return 0;
}

char *pmap_ipprot2netid(unsigned long proto)
{
	if (proto == IPPROTO_UDP)
		return udptrans;
	if (proto == IPPROTO_TCP)
		return tcptrans;
	return NULL;
}

#endif /* PORTMAP */
