/*
 * q_rr.c		RR.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Authors:	PJ Waskiewicz, <peter.p.waskiewicz.jr@intel.com>
 * Original Authors:	Alexey Kuznetsov, <kuznet@ms2.inr.ac.ru> (from PRIO)
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
	fprintf(stderr, "Usage: ... rr bands NUMBER priomap P1 P2... [multiqueue]\n");
}


static int rr_parse_opt(struct qdisc_util *qu, int argc, char **argv, struct nlmsghdr *n, const char *dev)
{
	int pmap_mode = 0;
	int idx = 0;
	struct tc_prio_qopt opt = {3, { 1, 2, 2, 2, 1, 2, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1 } };
	struct rtattr *nest;
	unsigned char mq = 0;

	while (argc > 0) {
		if (strcmp(*argv, "bands") == 0) {
			if (pmap_mode)
				explain();
			NEXT_ARG();
			if (get_integer(&opt.bands, *argv, 10)) {
				fprintf(stderr, "Illegal \"bands\"\n");
				return -1;
			}
		} else if (strcmp(*argv, "priomap") == 0) {
			if (pmap_mode) {
				fprintf(stderr, "Error: duplicate priomap\n");
				return -1;
			}
			pmap_mode = 1;
		} else if (strcmp(*argv, "help") == 0) {
			explain();
			return -1;
		} else if (strcmp(*argv, "multiqueue") == 0) {
			mq = 1;
		} else {
			unsigned int band;

			if (!pmap_mode) {
				fprintf(stderr, "What is \"%s\"?\n", *argv);
				explain();
				return -1;
			}
			if (get_unsigned(&band, *argv, 10)) {
				fprintf(stderr, "Illegal \"priomap\" element\n");
				return -1;
			}
			if (band > opt.bands) {
				fprintf(stderr, "\"priomap\" element is out of bands\n");
				return -1;
			}
			if (idx > TC_PRIO_MAX) {
				fprintf(stderr, "\"priomap\" index > TC_RR_MAX=%u\n", TC_PRIO_MAX);
				return -1;
			}
			opt.priomap[idx++] = band;
		}
		argc--; argv++;
	}

	nest = addattr_nest_compat(n, 1024, TCA_OPTIONS, &opt, sizeof(opt));
	if (mq)
		addattr_l(n, 1024, TCA_PRIO_MQ, NULL, 0);
	addattr_nest_compat_end(n, nest);
	return 0;
}

static int rr_print_opt(struct qdisc_util *qu, FILE *f, struct rtattr *opt)
{
	int i;
	struct tc_prio_qopt *qopt;
	struct rtattr *tb[TCA_PRIO_MAX + 1];

	if (opt == NULL)
		return 0;

	if (parse_rtattr_nested_compat(tb, TCA_PRIO_MAX, opt, qopt,
						sizeof(*qopt)))
		return -1;

	fprintf(f, "bands %u priomap ", qopt->bands);
	for (i = 0; i <= TC_PRIO_MAX; i++)
		fprintf(f, " %d", qopt->priomap[i]);

	if (tb[TCA_PRIO_MQ])
		fprintf(f, " multiqueue: %s ",
			rta_getattr_u8(tb[TCA_PRIO_MQ]) ? "on" : "off");

	return 0;
}

struct qdisc_util rr_qdisc_util = {
	.id		= "rr",
	.parse_qopt	= rr_parse_opt,
	.print_qopt	= rr_print_opt,
};
