/* SPDX-License-Identifier: GPL-2.0 */
/*
 * q_dsmark.c		Differentiated Services field marking.
 *
 * Hacked 1998,1999 by Werner Almesberger, EPFL ICA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include "utils.h"
#include "tc_util.h"


static void explain(void)
{
	fprintf(stderr,"Usage: dsmark indices INDICES [ default_index DEFAULT_INDEX ] [ set_tc_index ]\n");
}


static int dsmark_parse_opt(struct qdisc_util *qu, int argc, char **argv,
	struct nlmsghdr *n, const char *dev)
{
	struct rtattr *tail;
	__u16 ind;
	char *end;
	int dflt, set_tc_index;

	ind = set_tc_index = 0;
	dflt = -1;
	while (argc > 0) {
		if (!strcmp(*argv, "indices")) {
			NEXT_ARG();
			ind = strtoul(*argv, &end, 0);
			if (*end) {
				explain();
				return -1;
			}
		} else if (!strcmp(*argv,"default_index") || !strcmp(*argv,
		    "default")) {
			NEXT_ARG();
			dflt = strtoul(*argv, &end, 0);
			if (*end) {
				explain();
				return -1;
			}
		} else if (!strcmp(*argv,"set_tc_index")) {
			set_tc_index = 1;
		} else {
			explain();
			return -1;
		}
		argc--;
		argv++;
	}
	if (!ind) {
		explain();
		return -1;
	}
	tail = addattr_nest(n, 1024, TCA_OPTIONS);
	addattr_l(n, 1024, TCA_DSMARK_INDICES, &ind, sizeof(ind));
	if (dflt != -1) {
	    __u16 tmp = dflt;

	    addattr_l(n, 1024, TCA_DSMARK_DEFAULT_INDEX, &tmp, sizeof(tmp));
	}
	if (set_tc_index)
		addattr_l(n, 1024, TCA_DSMARK_SET_TC_INDEX, NULL, 0);
	addattr_nest_end(n, tail);
	return 0;
}


static void explain_class(void)
{
	fprintf(stderr, "Usage: ... dsmark [ mask MASK ] [ value VALUE ]\n");
}


static int dsmark_parse_class_opt(struct qdisc_util *qu, int argc, char **argv,
	struct nlmsghdr *n, const char *dev)
{
	struct rtattr *tail;
	__u8 tmp;
	char *end;

	tail = addattr_nest(n, 1024, TCA_OPTIONS);
	while (argc > 0) {
		if (!strcmp(*argv, "mask")) {
			NEXT_ARG();
			tmp = strtoul(*argv, &end, 0);
			if (*end) {
				explain_class();
				return -1;
			}
			addattr_l(n, 1024, TCA_DSMARK_MASK, &tmp, 1);
		} else if (!strcmp(*argv,"value")) {
			NEXT_ARG();
			tmp = strtoul(*argv, &end, 0);
			if (*end) {
				explain_class();
				return -1;
			}
			addattr_l(n, 1024, TCA_DSMARK_VALUE, &tmp, 1);
		} else {
			explain_class();
			return -1;
		}
		argc--;
		argv++;
	}
	addattr_nest_end(n, tail);
	return 0;
}



static int dsmark_print_opt(struct qdisc_util *qu, FILE *f, struct rtattr *opt)
{
	struct rtattr *tb[TCA_DSMARK_MAX+1];

	if (!opt) return 0;
	parse_rtattr(tb, TCA_DSMARK_MAX, RTA_DATA(opt), RTA_PAYLOAD(opt));
	if (tb[TCA_DSMARK_MASK]) {
		if (!RTA_PAYLOAD(tb[TCA_DSMARK_MASK]))
			fprintf(stderr, "dsmark: empty mask\n");
		else fprintf(f, "mask 0x%02x ",
			    rta_getattr_u8(tb[TCA_DSMARK_MASK]));
	}
	if (tb[TCA_DSMARK_VALUE]) {
		if (!RTA_PAYLOAD(tb[TCA_DSMARK_VALUE]))
			fprintf(stderr, "dsmark: empty value\n");
		else fprintf(f, "value 0x%02x ",
			    rta_getattr_u8(tb[TCA_DSMARK_VALUE]));
	}
	if (tb[TCA_DSMARK_INDICES]) {
		if (RTA_PAYLOAD(tb[TCA_DSMARK_INDICES]) < sizeof(__u16))
			fprintf(stderr, "dsmark: indices too short\n");
		else fprintf(f, "indices 0x%04x ",
			    rta_getattr_u16(tb[TCA_DSMARK_INDICES]));
	}
	if (tb[TCA_DSMARK_DEFAULT_INDEX]) {
		if (RTA_PAYLOAD(tb[TCA_DSMARK_DEFAULT_INDEX]) < sizeof(__u16))
			fprintf(stderr, "dsmark: default_index too short\n");
		else fprintf(f, "default_index 0x%04x ",
			    rta_getattr_u16(tb[TCA_DSMARK_DEFAULT_INDEX]));
	}
	if (tb[TCA_DSMARK_SET_TC_INDEX]) fprintf(f, "set_tc_index ");
	return 0;
}


struct qdisc_util dsmark_qdisc_util = {
	.id		= "dsmark",
	.parse_qopt	= dsmark_parse_opt,
	.print_qopt	= dsmark_print_opt,
	.parse_copt	= dsmark_parse_class_opt,
	.print_copt	= dsmark_print_opt,
};
