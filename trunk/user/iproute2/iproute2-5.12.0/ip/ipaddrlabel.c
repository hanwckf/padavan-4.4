/*
 * ipaddrlabel.c	"ip addrlabel"
 *
 * Copyright (C)2007 USAGI/WIDE Project
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
 *
 *
 * Based on iprule.c.
 *
 * Authors:	YOSHIFUJI Hideaki <yoshfuji@linux-ipv6.org>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <string.h>
#include <linux/types.h>
#include <linux/if_addrlabel.h>

#include "rt_names.h"
#include "utils.h"
#include "ip_common.h"
#include "json_print.h"

#define IFAL_RTA(r)	((struct rtattr *)(((char *)(r)) + NLMSG_ALIGN(sizeof(struct ifaddrlblmsg))))
#define IFAL_PAYLOAD(n)	NLMSG_PAYLOAD(n, sizeof(struct ifaddrlblmsg))

extern struct rtnl_handle rth;

static void usage(void) __attribute__((noreturn));

static void usage(void)
{
	fprintf(stderr,
		"Usage: ip addrlabel { add | del } prefix PREFIX [ dev DEV ] [ label LABEL ]\n"
		"       ip addrlabel [ list | flush | help ]\n");
	exit(-1);
}

int print_addrlabel(struct nlmsghdr *n, void *arg)
{
	struct ifaddrlblmsg *ifal = NLMSG_DATA(n);
	int len = n->nlmsg_len;
	struct rtattr *tb[IFAL_MAX+1];

	if (n->nlmsg_type != RTM_NEWADDRLABEL && n->nlmsg_type != RTM_DELADDRLABEL)
		return 0;

	len -= NLMSG_LENGTH(sizeof(*ifal));
	if (len < 0)
		return -1;

	parse_rtattr(tb, IFAL_MAX, IFAL_RTA(ifal), len);

	open_json_object(NULL);
	if (n->nlmsg_type == RTM_DELADDRLABEL)
		print_bool(PRINT_ANY, "deleted", "Deleted ", true);

	if (tb[IFAL_ADDRESS]) {
		const char *host
			= format_host_rta(ifal->ifal_family,
					  tb[IFAL_ADDRESS]);

		print_string(PRINT_FP, NULL, "prefix ", NULL);
		print_color_string(PRINT_ANY,
				   ifa_family_color(ifal->ifal_family),
				   "address", "%s", host);

		print_uint(PRINT_ANY, "prefixlen", "/%u ",
			   ifal->ifal_prefixlen);
	}

	if (ifal->ifal_index) {
		print_string(PRINT_FP, NULL, "dev ", NULL);
		print_color_string(PRINT_ANY, COLOR_IFNAME,
				   "ifname", "%s ",
				   ll_index_to_name(ifal->ifal_index));
	}

	if (tb[IFAL_LABEL] && RTA_PAYLOAD(tb[IFAL_LABEL]) == sizeof(uint32_t)) {
		uint32_t label = rta_getattr_u32(tb[IFAL_LABEL]);

		print_uint(PRINT_ANY,
			   "label", "label %u ", label);
	}
	print_string(PRINT_FP, NULL, "\n", "");
	close_json_object();

	return 0;
}

static int ipaddrlabel_list(int argc, char **argv)
{
	int af = preferred_family;

	if (af == AF_UNSPEC)
		af = AF_INET6;

	if (argc > 0) {
		fprintf(stderr, "\"ip addrlabel show\" does not take any arguments.\n");
		return -1;
	}

	if (rtnl_addrlbldump_req(&rth, af) < 0) {
		perror("Cannot send dump request");
		return 1;
	}

	new_json_obj(json);
	if (rtnl_dump_filter(&rth, print_addrlabel, stdout) < 0) {
		fprintf(stderr, "Dump terminated\n");
		return 1;
	}
	delete_json_obj();

	return 0;
}


static int ipaddrlabel_modify(int cmd, int argc, char **argv)
{
	struct {
		struct nlmsghdr	n;
		struct ifaddrlblmsg	ifal;
		char			buf[1024];
	} req = {
		.n.nlmsg_type = cmd,
		.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrlblmsg)),
		.n.nlmsg_flags = NLM_F_REQUEST,
		.ifal.ifal_family = preferred_family,
	};

	inet_prefix prefix = {};
	uint32_t label = 0xffffffffUL;
	char *p = NULL;
	char *l = NULL;

	if (cmd == RTM_NEWADDRLABEL) {
		req.n.nlmsg_flags |= NLM_F_CREATE|NLM_F_EXCL;
	}

	while (argc > 0) {
		if (strcmp(*argv, "prefix") == 0) {
			NEXT_ARG();
			p = *argv;
			get_prefix(&prefix, *argv, preferred_family);
		} else if (strcmp(*argv, "dev") == 0) {
			NEXT_ARG();
			if ((req.ifal.ifal_index = ll_name_to_index(*argv)) == 0)
				invarg("dev is invalid\n", *argv);
		} else if (strcmp(*argv, "label") == 0) {
			NEXT_ARG();
			l = *argv;
			if (get_u32(&label, *argv, 0) || label == 0xffffffffUL)
				invarg("label is invalid\n", *argv);
		}
		argc--;
		argv++;
	}
	if (p == NULL) {
		fprintf(stderr, "Not enough information: \"prefix\" argument is required.\n");
		return -1;
	}
	if (l == NULL) {
		fprintf(stderr, "Not enough information: \"label\" argument is required.\n");
		return -1;
	}
	addattr32(&req.n, sizeof(req), IFAL_LABEL, label);
	addattr_l(&req.n, sizeof(req), IFAL_ADDRESS, &prefix.data, prefix.bytelen);
	req.ifal.ifal_prefixlen = prefix.bitlen;

	if (req.ifal.ifal_family == AF_UNSPEC)
		req.ifal.ifal_family = AF_INET6;

	if (rtnl_talk(&rth, &req.n, NULL) < 0)
		return -2;

	return 0;
}


static int flush_addrlabel(struct nlmsghdr *n, void *arg)
{
	struct rtnl_handle rth2;
	struct rtmsg *r = NLMSG_DATA(n);
	int len = n->nlmsg_len;
	struct rtattr *tb[IFAL_MAX+1];

	len -= NLMSG_LENGTH(sizeof(*r));
	if (len < 0)
		return -1;

	parse_rtattr(tb, IFAL_MAX, RTM_RTA(r), len);

	if (tb[IFAL_ADDRESS]) {
		n->nlmsg_type = RTM_DELADDRLABEL;
		n->nlmsg_flags = NLM_F_REQUEST;

		if (rtnl_open(&rth2, 0) < 0)
			return -1;

		if (rtnl_talk(&rth2, n, NULL) < 0)
			return -2;

		rtnl_close(&rth2);
	}

	return 0;
}

static int ipaddrlabel_flush(int argc, char **argv)
{
	int af = preferred_family;

	if (af == AF_UNSPEC)
		af = AF_INET6;

	if (argc > 0) {
		fprintf(stderr, "\"ip addrlabel flush\" does not allow extra arguments\n");
		return -1;
	}

	if (rtnl_addrlbldump_req(&rth, af) < 0) {
		perror("Cannot send dump request");
		return -1;
	}

	if (rtnl_dump_filter(&rth, flush_addrlabel, NULL) < 0) {
		fprintf(stderr, "Flush terminated\n");
		return -1;
	}

	return 0;
}

int do_ipaddrlabel(int argc, char **argv)
{
	if (argc < 1) {
		return ipaddrlabel_list(0, NULL);
	} else if (matches(argv[0], "list") == 0 ||
		   matches(argv[0], "lst") == 0 ||
		   matches(argv[0], "show") == 0) {
		return ipaddrlabel_list(argc-1, argv+1);
	} else if (matches(argv[0], "add") == 0) {
		return ipaddrlabel_modify(RTM_NEWADDRLABEL, argc-1, argv+1);
	} else if (matches(argv[0], "delete") == 0) {
		return ipaddrlabel_modify(RTM_DELADDRLABEL, argc-1, argv+1);
	} else if (matches(argv[0], "flush") == 0) {
		return ipaddrlabel_flush(argc-1, argv+1);
	} else if (matches(argv[0], "help") == 0)
		usage();

	fprintf(stderr, "Command \"%s\" is unknown, try \"ip addrlabel help\".\n", *argv);
	exit(-1);
}
