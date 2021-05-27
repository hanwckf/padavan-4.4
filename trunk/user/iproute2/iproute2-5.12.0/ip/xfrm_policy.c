/* $USAGI: $ */

/*
 * Copyright (C)2004 USAGI/WIDE Project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses>.
 */
/*
 * based on iproute.c
 */
/*
 * Authors:
 *	Masahide NAKAMURA @USAGI
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <linux/netlink.h>
#include "utils.h"
#include "xfrm.h"
#include "ip_common.h"

/* #define NLMSG_DELETEALL_BUF_SIZE (4096-512) */
#define NLMSG_DELETEALL_BUF_SIZE 8192

/*
 * Receiving buffer defines:
 * nlmsg
 *   data = struct xfrm_userpolicy_info
 *   rtattr
 *     data = struct xfrm_user_tmpl[]
 */
#define NLMSG_BUF_SIZE 4096
#define RTA_BUF_SIZE 2048
#define XFRM_TMPLS_BUF_SIZE 1024
#define CTX_BUF_SIZE 256

static void usage(void) __attribute__((noreturn));

static void usage(void)
{
	fprintf(stderr,
		"Usage: ip xfrm policy { add | update } SELECTOR dir DIR [ ctx CTX ]\n"
		"	[ mark MARK [ mask MASK ] ] [ index INDEX ] [ ptype PTYPE ]\n"
		"	[ action ACTION ] [ priority PRIORITY ] [ flag FLAG-LIST ]\n"
		"	[ if_id IF_ID ] [ LIMIT-LIST ] [ TMPL-LIST ]\n"
		"Usage: ip xfrm policy { delete | get } { SELECTOR | index INDEX } dir DIR\n"
		"	[ ctx CTX ] [ mark MARK [ mask MASK ] ] [ ptype PTYPE ]\n"
		"	[ if_id IF_ID ]\n"
		"Usage: ip xfrm policy { deleteall | list } [ nosock ] [ SELECTOR ] [ dir DIR ]\n"
		"	[ index INDEX ] [ ptype PTYPE ] [ action ACTION ] [ priority PRIORITY ]\n"
		"	[ flag FLAG-LIST ]\n"
		"Usage: ip xfrm policy flush [ ptype PTYPE ]\n"
		"Usage: ip xfrm policy count\n"
		"Usage: ip xfrm policy set [ hthresh4 LBITS RBITS ] [ hthresh6 LBITS RBITS ]\n"
		"SELECTOR := [ src ADDR[/PLEN] ] [ dst ADDR[/PLEN] ] [ dev DEV ] [ UPSPEC ]\n"
		"UPSPEC := proto { { tcp | udp | sctp | dccp } [ sport PORT ] [ dport PORT ] |\n"
		"                  { icmp | ipv6-icmp | mobility-header } [ type NUMBER ] [ code NUMBER ] |\n"
		"                  gre [ key { DOTTED-QUAD | NUMBER } ] | PROTO }\n"
		"DIR := in | out | fwd\n"
		"PTYPE := main | sub\n"
		"ACTION := allow | block\n"
		"FLAG-LIST := [ FLAG-LIST ] FLAG\n"
		"FLAG := localok | icmp\n"
		"LIMIT-LIST := [ LIMIT-LIST ] limit LIMIT\n"
		"LIMIT := { time-soft | time-hard | time-use-soft | time-use-hard } SECONDS |\n"
		"         { byte-soft | byte-hard } SIZE | { packet-soft | packet-hard } COUNT\n"
		"TMPL-LIST := [ TMPL-LIST ] tmpl TMPL\n"
		"TMPL := ID [ mode MODE ] [ reqid REQID ] [ level LEVEL ]\n"
		"ID := [ src ADDR ] [ dst ADDR ] [ proto XFRM-PROTO ] [ spi SPI ]\n"
		"XFRM-PROTO := ");
	fprintf(stderr,
		"%s | %s | %s | %s | %s\n",
		strxf_xfrmproto(IPPROTO_ESP),
		strxf_xfrmproto(IPPROTO_AH),
		strxf_xfrmproto(IPPROTO_COMP),
		strxf_xfrmproto(IPPROTO_ROUTING),
		strxf_xfrmproto(IPPROTO_DSTOPTS));
	fprintf(stderr,
		"MODE := transport | tunnel | beet | ro | in_trigger\n"
		"LEVEL := required | use\n");

	exit(-1);
}

static int xfrm_policy_dir_parse(__u8 *dir, int *argcp, char ***argvp)
{
	int argc = *argcp;
	char **argv = *argvp;

	if (strcmp(*argv, "in") == 0)
		*dir = XFRM_POLICY_IN;
	else if (strcmp(*argv, "out") == 0)
		*dir = XFRM_POLICY_OUT;
	else if (strcmp(*argv, "fwd") == 0)
		*dir = XFRM_POLICY_FWD;
	else
		invarg("DIR value is invalid", *argv);

	*argcp = argc;
	*argvp = argv;

	return 0;
}

static int xfrm_policy_ptype_parse(__u8 *ptype, int *argcp, char ***argvp)
{
	int argc = *argcp;
	char **argv = *argvp;

	if (strcmp(*argv, "main") == 0)
		*ptype = XFRM_POLICY_TYPE_MAIN;
	else if (strcmp(*argv, "sub") == 0)
		*ptype = XFRM_POLICY_TYPE_SUB;
	else
		invarg("PTYPE value is invalid", *argv);

	*argcp = argc;
	*argvp = argv;

	return 0;
}

static int xfrm_policy_flag_parse(__u8 *flags, int *argcp, char ***argvp)
{
	int argc = *argcp;
	char **argv = *argvp;
	int len = strlen(*argv);

	if (len > 2 && strncmp(*argv, "0x", 2) == 0) {
		__u8 val = 0;

		if (get_u8(&val, *argv, 16))
			invarg("FLAG value is invalid", *argv);
		*flags = val;
	} else {
		while (1) {
			if (strcmp(*argv, "localok") == 0)
				*flags |= XFRM_POLICY_LOCALOK;
			else if (strcmp(*argv, "icmp") == 0)
				*flags |= XFRM_POLICY_ICMP;
			else {
				PREV_ARG(); /* back track */
				break;
			}

			if (!NEXT_ARG_OK())
				break;
			NEXT_ARG();
		}
	}

	*argcp = argc;
	*argvp = argv;

	return 0;
}

static int xfrm_tmpl_parse(struct xfrm_user_tmpl *tmpl,
			   int *argcp, char ***argvp)
{
	int argc = *argcp;
	char **argv = *argvp;
	char *idp = NULL;

	while (1) {
		if (strcmp(*argv, "mode") == 0) {
			NEXT_ARG();
			xfrm_mode_parse(&tmpl->mode,  &argc, &argv);
		} else if (strcmp(*argv, "reqid") == 0) {
			NEXT_ARG();
			xfrm_reqid_parse(&tmpl->reqid, &argc, &argv);
		} else if (strcmp(*argv, "level") == 0) {
			NEXT_ARG();

			if (strcmp(*argv, "required") == 0)
				tmpl->optional = 0;
			else if (strcmp(*argv, "use") == 0)
				tmpl->optional = 1;
			else
				invarg("LEVEL value is invalid\n", *argv);

		} else {
			if (idp) {
				PREV_ARG(); /* back track */
				break;
			}
			idp = *argv;
			preferred_family = AF_UNSPEC;
			xfrm_id_parse(&tmpl->saddr, &tmpl->id, &tmpl->family,
				      0, &argc, &argv);
			preferred_family = tmpl->family;
		}

		if (!NEXT_ARG_OK())
			break;

		NEXT_ARG();
	}
	if (argc == *argcp)
		missarg("TMPL");

	*argcp = argc;
	*argvp = argv;

	return 0;
}

int xfrm_sctx_parse(char *ctxstr, char *s,
			   struct xfrm_user_sec_ctx *sctx)
{
	int slen;

	slen = strlen(s) + 1;

	sctx->exttype = XFRMA_SEC_CTX;
	sctx->ctx_doi = 1;
	sctx->ctx_alg = 1;
	sctx->ctx_len = slen;
	sctx->len = sizeof(struct xfrm_user_sec_ctx) + slen;
	memcpy(ctxstr, s, slen);

	return 0;
}

static int xfrm_policy_modify(int cmd, unsigned int flags, int argc, char **argv)
{
	struct rtnl_handle rth;
	struct {
		struct nlmsghdr			n;
		struct xfrm_userpolicy_info	xpinfo;
		char				buf[RTA_BUF_SIZE];
	} req = {
		.n.nlmsg_len = NLMSG_LENGTH(sizeof(req.xpinfo)),
		.n.nlmsg_flags = NLM_F_REQUEST | flags,
		.n.nlmsg_type = cmd,
		.xpinfo.sel.family = preferred_family,
		.xpinfo.lft.soft_byte_limit = XFRM_INF,
		.xpinfo.lft.hard_byte_limit = XFRM_INF,
		.xpinfo.lft.soft_packet_limit = XFRM_INF,
		.xpinfo.lft.hard_packet_limit = XFRM_INF,
	};
	char *dirp = NULL;
	char *selp = NULL;
	char *ptypep = NULL;
	char *sctxp = NULL;
	struct xfrm_userpolicy_type upt = {};
	char tmpls_buf[XFRM_TMPLS_BUF_SIZE] = {};
	int tmpls_len = 0;
	struct xfrm_mark mark = {0, 0};
	struct {
		struct xfrm_user_sec_ctx sctx;
		char	str[CTX_BUF_SIZE];
	} ctx = {};
	bool is_if_id_set = false;
	__u32 if_id = 0;

	while (argc > 0) {
		if (strcmp(*argv, "dir") == 0) {
			if (dirp)
				duparg("dir", *argv);
			dirp = *argv;

			NEXT_ARG();
			xfrm_policy_dir_parse(&req.xpinfo.dir, &argc, &argv);
		} else if (strcmp(*argv, "ctx") == 0) {
			char *context;

			if (sctxp)
				duparg("ctx", *argv);
			sctxp = *argv;
			NEXT_ARG();
			context = *argv;
			xfrm_sctx_parse((char *)&ctx.str, context, &ctx.sctx);
		} else if (strcmp(*argv, "mark") == 0) {
			xfrm_parse_mark(&mark, &argc, &argv);
		} else if (strcmp(*argv, "index") == 0) {
			NEXT_ARG();
			if (get_u32(&req.xpinfo.index, *argv, 0))
				invarg("INDEX value is invalid", *argv);
		} else if (strcmp(*argv, "ptype") == 0) {
			if (ptypep)
				duparg("ptype", *argv);
			ptypep = *argv;

			NEXT_ARG();
			xfrm_policy_ptype_parse(&upt.type, &argc, &argv);
		} else if (strcmp(*argv, "action") == 0) {
			NEXT_ARG();
			if (strcmp(*argv, "allow") == 0)
				req.xpinfo.action = XFRM_POLICY_ALLOW;
			else if (strcmp(*argv, "block") == 0)
				req.xpinfo.action = XFRM_POLICY_BLOCK;
			else
				invarg("ACTION value is invalid\n", *argv);
		} else if (strcmp(*argv, "priority") == 0) {
			NEXT_ARG();
			if (get_u32(&req.xpinfo.priority, *argv, 0))
				invarg("PRIORITY value is invalid", *argv);
		} else if (strcmp(*argv, "flag") == 0) {
			NEXT_ARG();
			xfrm_policy_flag_parse(&req.xpinfo.flags, &argc,
					       &argv);
		} else if (strcmp(*argv, "limit") == 0) {
			NEXT_ARG();
			xfrm_lifetime_cfg_parse(&req.xpinfo.lft, &argc, &argv);
		} else if (strcmp(*argv, "tmpl") == 0) {
			struct xfrm_user_tmpl *tmpl;

			if (tmpls_len + sizeof(*tmpl) > sizeof(tmpls_buf)) {
				fprintf(stderr, "Too many tmpls: buffer overflow\n");
				exit(1);
			}
			tmpl = (struct xfrm_user_tmpl *)((char *)tmpls_buf + tmpls_len);

			tmpl->family = preferred_family;
			tmpl->aalgos = (~(__u32)0);
			tmpl->ealgos = (~(__u32)0);
			tmpl->calgos = (~(__u32)0);

			NEXT_ARG();
			xfrm_tmpl_parse(tmpl, &argc, &argv);

			tmpls_len += sizeof(*tmpl);
		} else if (strcmp(*argv, "if_id") == 0) {
			NEXT_ARG();
			if (get_u32(&if_id, *argv, 0))
				invarg("IF_ID value is invalid", *argv);
			is_if_id_set = true;
		} else {
			if (selp)
				duparg("unknown", *argv);
			selp = *argv;

			xfrm_selector_parse(&req.xpinfo.sel, &argc, &argv);
			if (preferred_family == AF_UNSPEC)
				preferred_family = req.xpinfo.sel.family;
		}

		argc--; argv++;
	}

	if (!dirp) {
		fprintf(stderr, "Not enough information: DIR is required.\n");
		exit(1);
	}

	if (ptypep) {
		addattr_l(&req.n, sizeof(req), XFRMA_POLICY_TYPE,
			  (void *)&upt, sizeof(upt));
	}

	if (tmpls_len > 0) {
		addattr_l(&req.n, sizeof(req), XFRMA_TMPL,
			  (void *)tmpls_buf, tmpls_len);
	}

	if (mark.m) {
		int r = addattr_l(&req.n, sizeof(req.buf), XFRMA_MARK,
				  (void *)&mark, sizeof(mark));
		if (r < 0) {
			fprintf(stderr, "%s: XFRMA_MARK failed\n", __func__);
			exit(1);
		}
	}

	if (sctxp) {
		addattr_l(&req.n, sizeof(req), XFRMA_SEC_CTX,
			  (void *)&ctx, ctx.sctx.len);
	}

	if (is_if_id_set)
		addattr32(&req.n, sizeof(req.buf), XFRMA_IF_ID, if_id);

	if (rtnl_open_byproto(&rth, 0, NETLINK_XFRM) < 0)
		exit(1);

	if (req.xpinfo.sel.family == AF_UNSPEC)
		req.xpinfo.sel.family = AF_INET;

	if (rtnl_talk(&rth, &req.n, NULL) < 0)
		exit(2);

	rtnl_close(&rth);

	return 0;
}

static int xfrm_policy_filter_match(struct xfrm_userpolicy_info *xpinfo,
				    __u8 ptype)
{
	if (!filter.use)
		return 1;

	if (filter.xpinfo.sel.family != AF_UNSPEC &&
	    filter.xpinfo.sel.family != xpinfo->sel.family)
		return 0;

	if ((xpinfo->dir^filter.xpinfo.dir)&filter.dir_mask)
		return 0;

	if (filter.filter_socket && (xpinfo->dir >= XFRM_POLICY_MAX))
		return 0;

	if ((ptype^filter.ptype)&filter.ptype_mask)
		return 0;

	if (filter.sel_src_mask) {
		if (xfrm_addr_match(&xpinfo->sel.saddr, &filter.xpinfo.sel.saddr,
				    filter.sel_src_mask))
			return 0;
	}

	if (filter.sel_dst_mask) {
		if (xfrm_addr_match(&xpinfo->sel.daddr, &filter.xpinfo.sel.daddr,
				    filter.sel_dst_mask))
			return 0;
	}

	if ((xpinfo->sel.ifindex^filter.xpinfo.sel.ifindex)&filter.sel_dev_mask)
		return 0;

	if ((xpinfo->sel.proto^filter.xpinfo.sel.proto)&filter.upspec_proto_mask)
		return 0;

	if (filter.upspec_sport_mask) {
		if ((xpinfo->sel.sport^filter.xpinfo.sel.sport)&filter.upspec_sport_mask)
			return 0;
	}

	if (filter.upspec_dport_mask) {
		if ((xpinfo->sel.dport^filter.xpinfo.sel.dport)&filter.upspec_dport_mask)
			return 0;
	}

	if ((xpinfo->index^filter.xpinfo.index)&filter.index_mask)
		return 0;

	if ((xpinfo->action^filter.xpinfo.action)&filter.action_mask)
		return 0;

	if ((xpinfo->priority^filter.xpinfo.priority)&filter.priority_mask)
		return 0;

	if (filter.policy_flags_mask)
		if ((xpinfo->flags & filter.xpinfo.flags) == 0)
			return 0;

	return 1;
}

int xfrm_policy_print(struct nlmsghdr *n, void *arg)
{
	struct rtattr *tb[XFRMA_MAX+1];
	struct rtattr *rta;
	struct xfrm_userpolicy_info *xpinfo = NULL;
	struct xfrm_user_polexpire *xpexp = NULL;
	struct xfrm_userpolicy_id *xpid = NULL;
	__u8 ptype = XFRM_POLICY_TYPE_MAIN;
	FILE *fp = (FILE *)arg;
	int len = n->nlmsg_len;

	if (n->nlmsg_type != XFRM_MSG_NEWPOLICY &&
	    n->nlmsg_type != XFRM_MSG_DELPOLICY &&
	    n->nlmsg_type != XFRM_MSG_UPDPOLICY &&
	    n->nlmsg_type != XFRM_MSG_POLEXPIRE) {
		fprintf(stderr, "Not a policy: %08x %08x %08x\n",
			n->nlmsg_len, n->nlmsg_type, n->nlmsg_flags);
		return 0;
	}

	if (n->nlmsg_type == XFRM_MSG_DELPOLICY)  {
		xpid = NLMSG_DATA(n);
		len -= NLMSG_SPACE(sizeof(*xpid));
	} else if (n->nlmsg_type == XFRM_MSG_POLEXPIRE) {
		xpexp = NLMSG_DATA(n);
		xpinfo = &xpexp->pol;
		len -= NLMSG_SPACE(sizeof(*xpexp));
	} else {
		xpexp = NULL;
		xpinfo = NLMSG_DATA(n);
		len -= NLMSG_SPACE(sizeof(*xpinfo));
	}

	if (len < 0) {
		fprintf(stderr, "BUG: wrong nlmsg len %d\n", len);
		return -1;
	}

	if (n->nlmsg_type == XFRM_MSG_DELPOLICY)
		rta = XFRMPID_RTA(xpid);
	else if (n->nlmsg_type == XFRM_MSG_POLEXPIRE)
		rta = XFRMPEXP_RTA(xpexp);
	else
		rta = XFRMP_RTA(xpinfo);

	parse_rtattr(tb, XFRMA_MAX, rta, len);

	if (tb[XFRMA_POLICY_TYPE]) {
		struct xfrm_userpolicy_type *upt;

		if (RTA_PAYLOAD(tb[XFRMA_POLICY_TYPE]) < sizeof(*upt)) {
			fprintf(stderr, "too short XFRMA_POLICY_TYPE len\n");
			return -1;
		}
		upt = RTA_DATA(tb[XFRMA_POLICY_TYPE]);
		ptype = upt->type;
	}

	if (xpinfo && !xfrm_policy_filter_match(xpinfo, ptype))
		return 0;

	if (n->nlmsg_type == XFRM_MSG_DELPOLICY)
		fprintf(fp, "Deleted ");
	else if (n->nlmsg_type == XFRM_MSG_UPDPOLICY)
		fprintf(fp, "Updated ");
	else if (n->nlmsg_type == XFRM_MSG_POLEXPIRE)
		fprintf(fp, "Expired ");

	if (n->nlmsg_type == XFRM_MSG_DELPOLICY) {
		/* xfrm_policy_id_print(); */
		if (!tb[XFRMA_POLICY]) {
			fprintf(stderr, "Buggy XFRM_MSG_DELPOLICY: no XFRMA_POLICY\n");
			return -1;
		}
		if (RTA_PAYLOAD(tb[XFRMA_POLICY]) < sizeof(*xpinfo)) {
			fprintf(stderr, "Buggy XFRM_MSG_DELPOLICY: too short XFRMA_POLICY len\n");
			return -1;
		}
		xpinfo = RTA_DATA(tb[XFRMA_POLICY]);
	}

	xfrm_policy_info_print(xpinfo, tb, fp, NULL, NULL);

	if (n->nlmsg_type == XFRM_MSG_POLEXPIRE) {
		fprintf(fp, "\t");
		fprintf(fp, "hard %u", xpexp->hard);
		fprintf(fp, "%s", _SL_);
	}

	if (oneline)
		fprintf(fp, "\n");
	fflush(fp);

	return 0;
}

static int xfrm_policy_get_or_delete(int argc, char **argv, int delete,
				     struct nlmsghdr **answer)
{
	struct rtnl_handle rth;
	struct {
		struct nlmsghdr			n;
		struct xfrm_userpolicy_id	xpid;
		char				buf[RTA_BUF_SIZE];
	} req = {
		.n.nlmsg_len = NLMSG_LENGTH(sizeof(req.xpid)),
		.n.nlmsg_flags = NLM_F_REQUEST,
		.n.nlmsg_type = delete ? XFRM_MSG_DELPOLICY
				       : XFRM_MSG_GETPOLICY,
	};
	char *dirp = NULL;
	char *selp = NULL;
	char *indexp = NULL;
	char *ptypep = NULL;
	char *sctxp = NULL;
	struct xfrm_userpolicy_type upt = {};
	struct xfrm_mark mark = {0, 0};
	struct {
		struct xfrm_user_sec_ctx sctx;
		char    str[CTX_BUF_SIZE];
	} ctx = {};
	bool is_if_id_set = false;
	__u32 if_id = 0;

	while (argc > 0) {
		if (strcmp(*argv, "dir") == 0) {
			if (dirp)
				duparg("dir", *argv);
			dirp = *argv;

			NEXT_ARG();
			xfrm_policy_dir_parse(&req.xpid.dir, &argc, &argv);

		} else if (strcmp(*argv, "ctx") == 0) {
			char *context;

			if (sctxp)
				duparg("ctx", *argv);
			sctxp = *argv;
			NEXT_ARG();
			context = *argv;
			xfrm_sctx_parse((char *)&ctx.str, context, &ctx.sctx);
		} else if (strcmp(*argv, "mark") == 0) {
			xfrm_parse_mark(&mark, &argc, &argv);
		} else if (strcmp(*argv, "index") == 0) {
			if (indexp)
				duparg("index", *argv);
			indexp = *argv;

			NEXT_ARG();
			if (get_u32(&req.xpid.index, *argv, 0))
				invarg("INDEX value is invalid", *argv);

		} else if (strcmp(*argv, "ptype") == 0) {
			if (ptypep)
				duparg("ptype", *argv);
			ptypep = *argv;

			NEXT_ARG();
			xfrm_policy_ptype_parse(&upt.type, &argc, &argv);
		} else if (strcmp(*argv, "if_id") == 0) {
			NEXT_ARG();
			if (get_u32(&if_id, *argv, 0))
				invarg("IF_ID value is invalid", *argv);
			is_if_id_set = true;
		} else {
			if (selp)
				invarg("unknown", *argv);
			selp = *argv;

			xfrm_selector_parse(&req.xpid.sel, &argc, &argv);
			if (preferred_family == AF_UNSPEC)
				preferred_family = req.xpid.sel.family;

		}

		argc--; argv++;
	}

	if (!dirp) {
		fprintf(stderr, "Not enough information: DIR is required.\n");
		exit(1);
	}
	if (ptypep) {
		addattr_l(&req.n, sizeof(req), XFRMA_POLICY_TYPE,
			  (void *)&upt, sizeof(upt));
	}
	if (!selp && !indexp) {
		fprintf(stderr, "Not enough information: either SELECTOR or INDEX is required.\n");
		exit(1);
	}
	if (selp && indexp)
		duparg2("SELECTOR", "INDEX");

	if (rtnl_open_byproto(&rth, 0, NETLINK_XFRM) < 0)
		exit(1);

	if (req.xpid.sel.family == AF_UNSPEC)
		req.xpid.sel.family = AF_INET;

	if (mark.m & mark.v) {
		int r = addattr_l(&req.n, sizeof(req.buf), XFRMA_MARK,
				  (void *)&mark, sizeof(mark));
		if (r < 0) {
			fprintf(stderr, "%s: XFRMA_MARK failed\n", __func__);
			exit(1);
		}
	}

	if (sctxp) {
		addattr_l(&req.n, sizeof(req), XFRMA_SEC_CTX,
			  (void *)&ctx, ctx.sctx.len);
	}

	if (is_if_id_set)
		addattr32(&req.n, sizeof(req.buf), XFRMA_IF_ID, if_id);

	if (rtnl_talk(&rth, &req.n, answer) < 0)
		exit(2);

	rtnl_close(&rth);

	return 0;
}

static int xfrm_policy_delete(int argc, char **argv)
{
	return xfrm_policy_get_or_delete(argc, argv, 1, NULL);
}

static int xfrm_policy_get(int argc, char **argv)
{
	struct nlmsghdr *n = NULL;

	xfrm_policy_get_or_delete(argc, argv, 0, &n);

	if (xfrm_policy_print(n, (void *)stdout) < 0) {
		fprintf(stderr, "An error :-)\n");
		exit(1);
	}

	free(n);
	return 0;
}

/*
 * With an existing policy of nlmsg, make new nlmsg for deleting the policy
 * and store it to buffer.
 */
static int xfrm_policy_keep(struct nlmsghdr *n, void *arg)
{
	struct xfrm_buffer *xb = (struct xfrm_buffer *)arg;
	struct rtnl_handle *rth = xb->rth;
	struct xfrm_userpolicy_info *xpinfo = NLMSG_DATA(n);
	int len = n->nlmsg_len;
	struct rtattr *tb[XFRMA_MAX+1];
	__u8 ptype = XFRM_POLICY_TYPE_MAIN;
	struct nlmsghdr *new_n;
	struct xfrm_userpolicy_id *xpid;

	if (n->nlmsg_type != XFRM_MSG_NEWPOLICY) {
		fprintf(stderr, "Not a policy: %08x %08x %08x\n",
			n->nlmsg_len, n->nlmsg_type, n->nlmsg_flags);
		return 0;
	}

	len -= NLMSG_LENGTH(sizeof(*xpinfo));
	if (len < 0) {
		fprintf(stderr, "BUG: wrong nlmsg len %d\n", len);
		return -1;
	}

	parse_rtattr(tb, XFRMA_MAX, XFRMP_RTA(xpinfo), len);

	if (tb[XFRMA_POLICY_TYPE]) {
		struct xfrm_userpolicy_type *upt;

		if (RTA_PAYLOAD(tb[XFRMA_POLICY_TYPE]) < sizeof(*upt)) {
			fprintf(stderr, "too short XFRMA_POLICY_TYPE len\n");
			return -1;
		}
		upt = RTA_DATA(tb[XFRMA_POLICY_TYPE]);
		ptype = upt->type;
	}

	if (!xfrm_policy_filter_match(xpinfo, ptype))
		return 0;

	/* can't delete socket policies */
	if (xpinfo->dir >= XFRM_POLICY_MAX)
		return 0;

	if (xb->offset + NLMSG_LENGTH(sizeof(*xpid)) > xb->size)
		return 0;

	new_n = (struct nlmsghdr *)(xb->buf + xb->offset);
	new_n->nlmsg_len = NLMSG_LENGTH(sizeof(*xpid));
	new_n->nlmsg_flags = NLM_F_REQUEST;
	new_n->nlmsg_type = XFRM_MSG_DELPOLICY;
	new_n->nlmsg_seq = ++rth->seq;

	xpid = NLMSG_DATA(new_n);
	memcpy(&xpid->sel, &xpinfo->sel, sizeof(xpid->sel));
	xpid->dir = xpinfo->dir;
	xpid->index = xpinfo->index;

	if (tb[XFRMA_MARK]) {
		int r = addattr_l(new_n, xb->size, XFRMA_MARK,
				(void *)RTA_DATA(tb[XFRMA_MARK]), tb[XFRMA_MARK]->rta_len);
		if (r < 0) {
			fprintf(stderr, "%s: XFRMA_MARK failed\n", __func__);
			exit(1);
		}
	}

	if (tb[XFRMA_IF_ID]) {
		addattr32(new_n, xb->size, XFRMA_IF_ID,
			  rta_getattr_u32(tb[XFRMA_IF_ID]));
	}

	xb->offset += new_n->nlmsg_len;
	xb->nlmsg_count++;

	return 0;
}

static int xfrm_policy_list_or_deleteall(int argc, char **argv, int deleteall)
{
	char *selp = NULL;
	struct rtnl_handle rth;

	if (argc > 0 || preferred_family != AF_UNSPEC)
		filter.use = 1;
	filter.xpinfo.sel.family = preferred_family;

	while (argc > 0) {
		if (strcmp(*argv, "dir") == 0) {
			NEXT_ARG();
			xfrm_policy_dir_parse(&filter.xpinfo.dir, &argc, &argv);

			filter.dir_mask = XFRM_FILTER_MASK_FULL;

		} else if (strcmp(*argv, "index") == 0) {
			NEXT_ARG();
			if (get_u32(&filter.xpinfo.index, *argv, 0))
				invarg("INDEX value is invalid", *argv);

			filter.index_mask = XFRM_FILTER_MASK_FULL;

		} else if (strcmp(*argv, "ptype") == 0) {
			NEXT_ARG();
			xfrm_policy_ptype_parse(&filter.ptype, &argc, &argv);

			filter.ptype_mask = XFRM_FILTER_MASK_FULL;

		} else if (strcmp(*argv, "action") == 0) {
			NEXT_ARG();
			if (strcmp(*argv, "allow") == 0)
				filter.xpinfo.action = XFRM_POLICY_ALLOW;
			else if (strcmp(*argv, "block") == 0)
				filter.xpinfo.action = XFRM_POLICY_BLOCK;
			else
				invarg("ACTION value is invalid\n", *argv);

			filter.action_mask = XFRM_FILTER_MASK_FULL;

		} else if (strcmp(*argv, "priority") == 0) {
			NEXT_ARG();
			if (get_u32(&filter.xpinfo.priority, *argv, 0))
				invarg("PRIORITY value is invalid", *argv);

			filter.priority_mask = XFRM_FILTER_MASK_FULL;

		} else if (strcmp(*argv, "flag") == 0) {
			NEXT_ARG();
			xfrm_policy_flag_parse(&filter.xpinfo.flags, &argc,
					       &argv);

			filter.policy_flags_mask = XFRM_FILTER_MASK_FULL;

		} else if (strcmp(*argv, "nosock") == 0) {
			/* filter all socket-based policies */
			filter.filter_socket = 1;
		} else {
			if (selp)
				invarg("unknown", *argv);
			selp = *argv;

			xfrm_selector_parse(&filter.xpinfo.sel, &argc, &argv);
			if (preferred_family == AF_UNSPEC)
				preferred_family = filter.xpinfo.sel.family;

		}

		argc--; argv++;
	}

	if (rtnl_open_byproto(&rth, 0, NETLINK_XFRM) < 0)
		exit(1);

	if (deleteall) {
		struct xfrm_buffer xb;
		char buf[NLMSG_DELETEALL_BUF_SIZE];
		int i;

		xb.buf = buf;
		xb.size = sizeof(buf);
		xb.rth = &rth;

		for (i = 0; ; i++) {
			struct {
				struct nlmsghdr n;
				char buf[NLMSG_BUF_SIZE];
			} req = {
				.n.nlmsg_len = NLMSG_HDRLEN,
				.n.nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST,
				.n.nlmsg_type = XFRM_MSG_GETPOLICY,
				.n.nlmsg_seq = rth.dump = ++rth.seq,
			};

			xb.offset = 0;
			xb.nlmsg_count = 0;

			if (show_stats > 1)
				fprintf(stderr, "Delete-all round = %d\n", i);

			if (rtnl_send(&rth, (void *)&req, req.n.nlmsg_len) < 0) {
				perror("Cannot send dump request");
				exit(1);
			}

			if (rtnl_dump_filter(&rth, xfrm_policy_keep, &xb) < 0) {
				fprintf(stderr, "Delete-all terminated\n");
				exit(1);
			}
			if (xb.nlmsg_count == 0) {
				if (show_stats > 1)
					fprintf(stderr, "Delete-all completed\n");
				break;
			}

			if (rtnl_send_check(&rth, xb.buf, xb.offset) < 0) {
				perror("Failed to send delete-all request");
				exit(1);
			}
			if (show_stats > 1)
				fprintf(stderr, "Delete-all nlmsg count = %d\n", xb.nlmsg_count);

			xb.offset = 0;
			xb.nlmsg_count = 0;
		}
	} else {
		struct {
			struct nlmsghdr n;
			char buf[NLMSG_BUF_SIZE];
		} req = {
			.n.nlmsg_len = NLMSG_HDRLEN,
			.n.nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST,
			.n.nlmsg_type = XFRM_MSG_GETPOLICY,
			.n.nlmsg_seq = rth.dump = ++rth.seq,
		};

		if (rtnl_send(&rth, (void *)&req, req.n.nlmsg_len) < 0) {
			perror("Cannot send dump request");
			exit(1);
		}

		if (rtnl_dump_filter(&rth, xfrm_policy_print, stdout) < 0) {
			fprintf(stderr, "Dump terminated\n");
			exit(1);
		}
	}

	rtnl_close(&rth);

	exit(0);
}

static int print_spdinfo(struct nlmsghdr *n, void *arg)
{
	FILE *fp = (FILE *)arg;
	__u32 *f = NLMSG_DATA(n);
	struct rtattr *tb[XFRMA_SPD_MAX+1];
	struct rtattr *rta;

	int len = n->nlmsg_len;

	len -= NLMSG_LENGTH(sizeof(__u32));
	if (len < 0) {
		fprintf(stderr, "SPDinfo: Wrong len %d\n", len);
		return -1;
	}

	rta = XFRMSAPD_RTA(f);
	parse_rtattr(tb, XFRMA_SPD_MAX, rta, len);

	fprintf(fp, "\t SPD");
	if (tb[XFRMA_SPD_INFO]) {
		struct xfrmu_spdinfo *si;

		if (RTA_PAYLOAD(tb[XFRMA_SPD_INFO]) < sizeof(*si)) {
			fprintf(stderr, "SPDinfo: Wrong len %d\n", len);
			return -1;
		}
		si = RTA_DATA(tb[XFRMA_SPD_INFO]);
		fprintf(fp, " IN  %d", si->incnt);
		fprintf(fp, " OUT %d", si->outcnt);
		fprintf(fp, " FWD %d", si->fwdcnt);

		if (show_stats) {
			fprintf(fp, " (Sock:");
			fprintf(fp, " IN %d", si->inscnt);
			fprintf(fp, " OUT %d", si->outscnt);
			fprintf(fp, " FWD %d", si->fwdscnt);
			fprintf(fp, ")");
		}

		fprintf(fp, "%s", _SL_);
	}
	if (show_stats > 1) {
		struct xfrmu_spdhinfo *sh;

		if (tb[XFRMA_SPD_HINFO]) {
			if (RTA_PAYLOAD(tb[XFRMA_SPD_HINFO]) < sizeof(*sh)) {
				fprintf(stderr, "SPDinfo: Wrong len %d\n", len);
				return -1;
			}
			sh = RTA_DATA(tb[XFRMA_SPD_HINFO]);
			fprintf(fp, "\t SPD buckets:");
			fprintf(fp, " count %d", sh->spdhcnt);
			fprintf(fp, " Max %d", sh->spdhmcnt);
			fprintf(fp, "%s", _SL_);
		}
		if (tb[XFRMA_SPD_IPV4_HTHRESH]) {
			struct xfrmu_spdhthresh *th;

			if (RTA_PAYLOAD(tb[XFRMA_SPD_IPV4_HTHRESH]) < sizeof(*th)) {
				fprintf(stderr, "SPDinfo: Wrong len %d\n", len);
				return -1;
			}
			th = RTA_DATA(tb[XFRMA_SPD_IPV4_HTHRESH]);
			fprintf(fp, "\t SPD IPv4 thresholds:");
			fprintf(fp, " local %d", th->lbits);
			fprintf(fp, " remote %d", th->rbits);
			fprintf(fp, "%s", _SL_);

		}
		if (tb[XFRMA_SPD_IPV6_HTHRESH]) {
			struct xfrmu_spdhthresh *th;

			if (RTA_PAYLOAD(tb[XFRMA_SPD_IPV6_HTHRESH]) < sizeof(*th)) {
				fprintf(stderr, "SPDinfo: Wrong len %d\n", len);
				return -1;
			}
			th = RTA_DATA(tb[XFRMA_SPD_IPV6_HTHRESH]);
			fprintf(fp, "\t SPD IPv6 thresholds:");
			fprintf(fp, " local %d", th->lbits);
			fprintf(fp, " remote %d", th->rbits);
			fprintf(fp, "%s", _SL_);
		}
	}

	if (oneline)
		fprintf(fp, "\n");

	return 0;
}

static int xfrm_spd_setinfo(int argc, char **argv)
{
	struct rtnl_handle rth;
	struct {
		struct nlmsghdr			n;
		__u32				flags;
		char				buf[RTA_BUF_SIZE];
	} req = {
		.n.nlmsg_len = NLMSG_LENGTH(sizeof(__u32)),
		.n.nlmsg_flags = NLM_F_REQUEST,
		.n.nlmsg_type = XFRM_MSG_NEWSPDINFO,
		.flags = 0XFFFFFFFF,
	};

	char *thr4 = NULL;
	char *thr6 = NULL;

	while (argc > 0) {
		if (strcmp(*argv, "hthresh4") == 0) {
			struct xfrmu_spdhthresh thr;

			if (thr4)
				duparg("hthresh4", *argv);
			thr4 = *argv;
			NEXT_ARG();
			if (get_u8(&thr.lbits, *argv, 0) || thr.lbits > 32)
				invarg("hthresh4 LBITS value is invalid", *argv);
			NEXT_ARG();
			if (get_u8(&thr.rbits, *argv, 0) || thr.rbits > 32)
				invarg("hthresh4 RBITS value is invalid", *argv);

			addattr_l(&req.n, sizeof(req), XFRMA_SPD_IPV4_HTHRESH,
				  (void *)&thr, sizeof(thr));
		} else if (strcmp(*argv, "hthresh6") == 0) {
			struct xfrmu_spdhthresh thr;

			if (thr6)
				duparg("hthresh6", *argv);
			thr6 = *argv;
			NEXT_ARG();
			if (get_u8(&thr.lbits, *argv, 0) || thr.lbits > 128)
				invarg("hthresh6 LBITS value is invalid", *argv);
			NEXT_ARG();
			if (get_u8(&thr.rbits, *argv, 0) || thr.rbits > 128)
				invarg("hthresh6 RBITS value is invalid", *argv);

			addattr_l(&req.n, sizeof(req), XFRMA_SPD_IPV6_HTHRESH,
				  (void *)&thr, sizeof(thr));
		} else {
			invarg("unknown", *argv);
		}

		argc--; argv++;
	}

	if (rtnl_open_byproto(&rth, 0, NETLINK_XFRM) < 0)
		exit(1);

	if (rtnl_talk(&rth, &req.n, NULL) < 0)
		exit(2);

	rtnl_close(&rth);

	return 0;
}

static int xfrm_spd_getinfo(int argc, char **argv)
{
	struct rtnl_handle rth;
	struct {
		struct nlmsghdr			n;
		__u32				flags;
	} req = {
		.n.nlmsg_len = NLMSG_LENGTH(sizeof(__u32)),
		.n.nlmsg_flags = NLM_F_REQUEST,
		.n.nlmsg_type = XFRM_MSG_GETSPDINFO,
		.flags = 0XFFFFFFFF,
	};
	struct nlmsghdr *answer;

	if (rtnl_open_byproto(&rth, 0, NETLINK_XFRM) < 0)
		exit(1);

	if (rtnl_talk(&rth, &req.n, &answer) < 0)
		exit(2);

	print_spdinfo(answer, (void *)stdout);

	free(answer);
	rtnl_close(&rth);

	return 0;
}

static int xfrm_policy_flush(int argc, char **argv)
{
	struct rtnl_handle rth;
	struct {
		struct nlmsghdr	n;
		char		buf[RTA_BUF_SIZE];
	} req = {
		.n.nlmsg_len = NLMSG_LENGTH(0), /* nlmsg data is nothing */
		.n.nlmsg_flags = NLM_F_REQUEST,
		.n.nlmsg_type = XFRM_MSG_FLUSHPOLICY,
	};
	char *ptypep = NULL;
	struct xfrm_userpolicy_type upt = {};

	while (argc > 0) {
		if (strcmp(*argv, "ptype") == 0) {
			if (ptypep)
				duparg("ptype", *argv);
			ptypep = *argv;

			NEXT_ARG();
			xfrm_policy_ptype_parse(&upt.type, &argc, &argv);
		} else
			invarg("unknown", *argv);

		argc--; argv++;
	}

	if (ptypep) {
		addattr_l(&req.n, sizeof(req), XFRMA_POLICY_TYPE,
			  (void *)&upt, sizeof(upt));
	}

	if (rtnl_open_byproto(&rth, 0, NETLINK_XFRM) < 0)
		exit(1);

	if (show_stats > 1)
		fprintf(stderr, "Flush policy\n");

	if (rtnl_talk(&rth, &req.n, NULL) < 0)
		exit(2);

	rtnl_close(&rth);

	return 0;
}

int do_xfrm_policy(int argc, char **argv)
{
	if (argc < 1)
		return xfrm_policy_list_or_deleteall(0, NULL, 0);

	if (matches(*argv, "add") == 0)
		return xfrm_policy_modify(XFRM_MSG_NEWPOLICY, 0,
					  argc-1, argv+1);
	if (matches(*argv, "update") == 0)
		return xfrm_policy_modify(XFRM_MSG_UPDPOLICY, 0,
					  argc-1, argv+1);
	if (matches(*argv, "delete") == 0)
		return xfrm_policy_delete(argc-1, argv+1);
	if (matches(*argv, "deleteall") == 0 || matches(*argv, "delall") == 0)
		return xfrm_policy_list_or_deleteall(argc-1, argv+1, 1);
	if (matches(*argv, "list") == 0 || matches(*argv, "show") == 0
	    || matches(*argv, "lst") == 0)
		return xfrm_policy_list_or_deleteall(argc-1, argv+1, 0);
	if (matches(*argv, "get") == 0)
		return xfrm_policy_get(argc-1, argv+1);
	if (matches(*argv, "flush") == 0)
		return xfrm_policy_flush(argc-1, argv+1);
	if (matches(*argv, "count") == 0)
		return xfrm_spd_getinfo(argc, argv);
	if (matches(*argv, "set") == 0)
		return xfrm_spd_setinfo(argc-1, argv+1);
	if (matches(*argv, "help") == 0)
		usage();
	fprintf(stderr, "Command \"%s\" is unknown, try \"ip xfrm policy help\".\n", *argv);
	exit(-1);
}
