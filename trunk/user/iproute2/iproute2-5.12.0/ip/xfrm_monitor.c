/* $USAGI: $ */

/*
 * Copyright (C)2005 USAGI/WIDE Project
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
 * based on ipmonitor.c
 */
/*
 * Authors:
 *	Masahide NAKAMURA @USAGI
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>

#include "utils.h"
#include "xfrm.h"
#include "ip_common.h"

static void usage(void) __attribute__((noreturn));
static int listen_all_nsid;
static bool nokeys;

static void usage(void)
{
	fprintf(stderr,
		"Usage: ip xfrm monitor [ nokeys ] [ all-nsid ] [ all | OBJECTS | help ]\n"
		"OBJECTS := { acquire | expire | SA | aevent | policy | report }\n");
	exit(-1);
}

static int xfrm_acquire_print(struct nlmsghdr *n, void *arg)
{
	FILE *fp = (FILE *)arg;
	struct xfrm_user_acquire *xacq = NLMSG_DATA(n);
	int len = n->nlmsg_len;
	struct rtattr *tb[XFRMA_MAX+1];
	__u16 family;

	len -= NLMSG_LENGTH(sizeof(*xacq));
	if (len < 0) {
		fprintf(stderr, "BUG: wrong nlmsg len %d\n", len);
		return -1;
	}

	parse_rtattr(tb, XFRMA_MAX, XFRMACQ_RTA(xacq), len);

	family = xacq->sel.family;
	if (family == AF_UNSPEC)
		family = xacq->policy.sel.family;
	if (family == AF_UNSPEC)
		family = preferred_family;

	fprintf(fp, "acquire ");

	fprintf(fp, "proto %s ", strxf_xfrmproto(xacq->id.proto));
	if (show_stats > 0 || xacq->id.spi) {
		__u32 spi = ntohl(xacq->id.spi);

		fprintf(fp, "spi 0x%08x", spi);
		if (show_stats > 0)
			fprintf(fp, "(%u)", spi);
		fprintf(fp, " ");
	}
	fprintf(fp, "%s", _SL_);

	xfrm_selector_print(&xacq->sel, family, fp, "  sel ");

	xfrm_policy_info_print(&xacq->policy, tb, fp, "    ", "  policy ");

	if (show_stats > 0)
		fprintf(fp, "  seq 0x%08u ", xacq->seq);
	if (show_stats > 0) {
		fprintf(fp, "%s-mask %s ",
			strxf_algotype(XFRMA_ALG_CRYPT),
			strxf_mask32(xacq->ealgos));
		fprintf(fp, "%s-mask %s ",
			strxf_algotype(XFRMA_ALG_AUTH),
			strxf_mask32(xacq->aalgos));
		fprintf(fp, "%s-mask %s",
			strxf_algotype(XFRMA_ALG_COMP),
			strxf_mask32(xacq->calgos));
	}
	fprintf(fp, "%s", _SL_);

	if (oneline)
		fprintf(fp, "\n");
	fflush(fp);

	return 0;
}

static int xfrm_state_flush_print(struct nlmsghdr *n, void *arg)
{
	FILE *fp = (FILE *)arg;
	struct xfrm_usersa_flush *xsf = NLMSG_DATA(n);
	int len = n->nlmsg_len;
	const char *str;

	len -= NLMSG_SPACE(sizeof(*xsf));
	if (len < 0) {
		fprintf(stderr, "BUG: wrong nlmsg len %d\n", len);
		return -1;
	}

	fprintf(fp, "Flushed state ");

	str = strxf_xfrmproto(xsf->proto);
	if (str)
		fprintf(fp, "proto %s", str);
	else
		fprintf(fp, "proto %u", xsf->proto);
	fprintf(fp, "%s", _SL_);

	if (oneline)
		fprintf(fp, "\n");
	fflush(fp);

	return 0;
}

static int xfrm_policy_flush_print(struct nlmsghdr *n, void *arg)
{
	struct rtattr *tb[XFRMA_MAX+1];
	FILE *fp = (FILE *)arg;
	int len = n->nlmsg_len;

	len -= NLMSG_SPACE(0);
	if (len < 0) {
		fprintf(stderr, "BUG: wrong nlmsg len %d\n", len);
		return -1;
	}

	fprintf(fp, "Flushed policy ");

	parse_rtattr(tb, XFRMA_MAX, NLMSG_DATA(n), len);

	if (tb[XFRMA_POLICY_TYPE]) {
		struct xfrm_userpolicy_type *upt;

		fprintf(fp, "ptype ");

		if (RTA_PAYLOAD(tb[XFRMA_POLICY_TYPE]) < sizeof(*upt))
			fprintf(fp, "(ERROR truncated)");

		upt = RTA_DATA(tb[XFRMA_POLICY_TYPE]);
		fprintf(fp, "%s ", strxf_ptype(upt->type));
	}

	fprintf(fp, "%s", _SL_);

	if (oneline)
		fprintf(fp, "\n");
	fflush(fp);

	return 0;
}

static int xfrm_report_print(struct nlmsghdr *n, void *arg)
{
	FILE *fp = (FILE *)arg;
	struct xfrm_user_report *xrep = NLMSG_DATA(n);
	int len = n->nlmsg_len;
	struct rtattr *tb[XFRMA_MAX+1];
	__u16 family;

	len -= NLMSG_LENGTH(sizeof(*xrep));
	if (len < 0) {
		fprintf(stderr, "BUG: wrong nlmsg len %d\n", len);
		return -1;
	}

	family = xrep->sel.family;
	if (family == AF_UNSPEC)
		family = preferred_family;

	fprintf(fp, "report ");

	fprintf(fp, "proto %s ", strxf_xfrmproto(xrep->proto));
	fprintf(fp, "%s", _SL_);

	xfrm_selector_print(&xrep->sel, family, fp, "  sel ");

	parse_rtattr(tb, XFRMA_MAX, XFRMREP_RTA(xrep), len);

	xfrm_xfrma_print(tb, family, fp, "  ", nokeys);

	if (oneline)
		fprintf(fp, "\n");

	return 0;
}

static void xfrm_ae_flags_print(__u32 flags, void *arg)
{
	FILE *fp = (FILE *)arg;

	fprintf(fp, " (0x%x) ", flags);
	if (!flags)
		return;
	if (flags & XFRM_AE_CR)
		fprintf(fp, " replay update ");
	if (flags & XFRM_AE_CE)
		fprintf(fp, " timer expired ");
	if (flags & XFRM_AE_CU)
		fprintf(fp, " policy updated ");

}

static void xfrm_usersa_print(const struct xfrm_usersa_id *sa_id, __u32 reqid, FILE *fp)
{
	fprintf(fp, "dst %s ",
		rt_addr_n2a(sa_id->family, sizeof(sa_id->daddr), &sa_id->daddr));

	fprintf(fp, " reqid 0x%x", reqid);

	fprintf(fp, " protocol %s ", strxf_proto(sa_id->proto));
	fprintf(fp, " SPI 0x%x", ntohl(sa_id->spi));
}

static int xfrm_ae_print(struct nlmsghdr *n, void *arg)
{
	FILE *fp = (FILE *)arg;
	struct xfrm_aevent_id *id = NLMSG_DATA(n);

	fprintf(fp, "Async event ");
	xfrm_ae_flags_print(id->flags, arg);
	fprintf(fp, "\n\t");
	fprintf(fp, "src %s ", rt_addr_n2a(id->sa_id.family,
					   sizeof(id->saddr), &id->saddr));

	xfrm_usersa_print(&id->sa_id, id->reqid, fp);

	fprintf(fp, "\n");
	fflush(fp);

	return 0;
}

static void xfrm_print_addr(FILE *fp, int family, xfrm_address_t *a)
{
	fprintf(fp, "%s", rt_addr_n2a(family, sizeof(*a), a));
}

static int xfrm_mapping_print(struct nlmsghdr *n, void *arg)
{
	FILE *fp = (FILE *)arg;
	struct xfrm_user_mapping *map = NLMSG_DATA(n);

	fprintf(fp, "Mapping change ");
	xfrm_print_addr(fp, map->id.family, &map->old_saddr);

	fprintf(fp, ":%d -> ", ntohs(map->old_sport));
	xfrm_print_addr(fp, map->id.family, &map->new_saddr);
	fprintf(fp, ":%d\n\t", ntohs(map->new_sport));

	xfrm_usersa_print(&map->id, map->reqid, fp);

	fprintf(fp, "\n");
	fflush(fp);
	return 0;
}

static int xfrm_accept_msg(struct rtnl_ctrl_data *ctrl,
			   struct nlmsghdr *n, void *arg)
{
	FILE *fp = (FILE *)arg;

	if (timestamp)
		print_timestamp(fp);

	if (listen_all_nsid) {
		if (ctrl == NULL || ctrl->nsid < 0)
			fprintf(fp, "[nsid current]");
		else
			fprintf(fp, "[nsid %d]", ctrl->nsid);
	}

	switch (n->nlmsg_type) {
	case XFRM_MSG_NEWSA:
	case XFRM_MSG_DELSA:
	case XFRM_MSG_UPDSA:
	case XFRM_MSG_EXPIRE:
		xfrm_state_print(n, arg);
		return 0;
	case XFRM_MSG_NEWPOLICY:
	case XFRM_MSG_DELPOLICY:
	case XFRM_MSG_UPDPOLICY:
	case XFRM_MSG_POLEXPIRE:
		xfrm_policy_print(n, arg);
		return 0;
	case XFRM_MSG_ACQUIRE:
		xfrm_acquire_print(n, arg);
		return 0;
	case XFRM_MSG_FLUSHSA:
		xfrm_state_flush_print(n, arg);
		return 0;
	case XFRM_MSG_FLUSHPOLICY:
		xfrm_policy_flush_print(n, arg);
		return 0;
	case XFRM_MSG_REPORT:
		xfrm_report_print(n, arg);
		return 0;
	case XFRM_MSG_NEWAE:
		xfrm_ae_print(n, arg);
		return 0;
	case XFRM_MSG_MAPPING:
		xfrm_mapping_print(n, arg);
		return 0;
	default:
		break;
	}

	if (n->nlmsg_type != NLMSG_ERROR && n->nlmsg_type != NLMSG_NOOP &&
	    n->nlmsg_type != NLMSG_DONE) {
		fprintf(fp, "Unknown message: %08d 0x%08x 0x%08x\n",
			n->nlmsg_len, n->nlmsg_type, n->nlmsg_flags);
	}
	return 0;
}

extern struct rtnl_handle rth;

int do_xfrm_monitor(int argc, char **argv)
{
	char *file = NULL;
	unsigned int groups = ~((unsigned)0); /* XXX */
	int lacquire = 0;
	int lexpire = 0;
	int laevent = 0;
	int lpolicy = 0;
	int lsa = 0;
	int lreport = 0;

	rtnl_close(&rth);

	while (argc > 0) {
		if (matches(*argv, "file") == 0) {
			NEXT_ARG();
			file = *argv;
		} else if (strcmp(*argv, "nokeys") == 0) {
			nokeys = true;
		} else if (strcmp(*argv, "all") == 0) {
			/* fall out */
		} else if (matches(*argv, "all-nsid") == 0) {
			listen_all_nsid = 1;
		} else if (matches(*argv, "acquire") == 0) {
			lacquire = 1;
			groups = 0;
		} else if (matches(*argv, "expire") == 0) {
			lexpire = 1;
			groups = 0;
		} else if (matches(*argv, "SA") == 0) {
			lsa = 1;
			groups = 0;
		} else if (matches(*argv, "aevent") == 0) {
			laevent = 1;
			groups = 0;
		} else if (matches(*argv, "policy") == 0) {
			lpolicy = 1;
			groups = 0;
		} else if (matches(*argv, "report") == 0) {
			lreport = 1;
			groups = 0;
		} else if (matches(*argv, "help") == 0) {
			usage();
		} else {
			fprintf(stderr, "Argument \"%s\" is unknown, try \"ip xfrm monitor help\".\n", *argv);
			exit(-1);
		}
		argc--;	argv++;
	}

	if (lacquire)
		groups |= nl_mgrp(XFRMNLGRP_ACQUIRE);
	if (lexpire)
		groups |= nl_mgrp(XFRMNLGRP_EXPIRE);
	if (lsa)
		groups |= nl_mgrp(XFRMNLGRP_SA);
	if (lpolicy)
		groups |= nl_mgrp(XFRMNLGRP_POLICY);
	if (laevent)
		groups |= nl_mgrp(XFRMNLGRP_AEVENTS);
	if (lreport)
		groups |= nl_mgrp(XFRMNLGRP_REPORT);

	if (file) {
		FILE *fp;
		int err;

		fp = fopen(file, "r");
		if (fp == NULL) {
			perror("Cannot fopen");
			exit(-1);
		}
		err = rtnl_from_file(fp, xfrm_accept_msg, stdout);
		fclose(fp);
		return err;
	}

	if (rtnl_open_byproto(&rth, groups, NETLINK_XFRM) < 0)
		exit(1);
	if (listen_all_nsid && rtnl_listen_all_nsid(&rth) < 0)
		exit(1);

	if (rtnl_listen(&rth, xfrm_accept_msg, (void *)stdout) < 0)
		exit(2);

	return 0;
}
