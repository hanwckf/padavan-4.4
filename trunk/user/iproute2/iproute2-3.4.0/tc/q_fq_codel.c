/*
 * Fair Queue Codel
 *
 *  Copyright (C) 2012 Eric Dumazet <edumazet@google.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The names of the authors may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * Alternatively, provided that this notice is retained in full, this
 * software may be distributed under the terms of the GNU General
 * Public License ("GPL") version 2, in which case the provisions of the
 * GPL apply INSTEAD OF those given above.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include "utils.h"
#include "tc_util.h"

static void explain(void)
{
	fprintf(stderr, "Usage: ... fq_codel [ limit PACKETS ] [ flows NUMBER ]\n");
	fprintf(stderr, "                    [ target TIME] [ interval TIME ]\n");
	fprintf(stderr, "                    [ quantum BYTES ] [ [no]ecn ]\n");
}

static int fq_codel_parse_opt(struct qdisc_util *qu, int argc, char **argv,
			      struct nlmsghdr *n)
{
	unsigned limit = 0;
	unsigned flows = 0;
	unsigned target = 0;
	unsigned interval = 0;
	unsigned quantum = 0;
	int ecn = -1;
	struct rtattr *tail;

	while (argc > 0) {
		if (strcmp(*argv, "limit") == 0) {
			NEXT_ARG();
			if (get_unsigned(&limit, *argv, 0)) {
				fprintf(stderr, "Illegal \"limit\"\n");
				return -1;
			}
		} else if (strcmp(*argv, "flows") == 0) {
			NEXT_ARG();
			if (get_unsigned(&flows, *argv, 0)) {
				fprintf(stderr, "Illegal \"flows\"\n");
				return -1;
			}
		} else if (strcmp(*argv, "quantum") == 0) {
			NEXT_ARG();
			if (get_unsigned(&quantum, *argv, 0)) {
				fprintf(stderr, "Illegal \"quantum\"\n");
				return -1;
			}
		} else if (strcmp(*argv, "target") == 0) {
			NEXT_ARG();
			if (get_time(&target, *argv)) {
				fprintf(stderr, "Illegal \"target\"\n");
				return -1;
			}
		} else if (strcmp(*argv, "interval") == 0) {
			NEXT_ARG();
			if (get_time(&interval, *argv)) {
				fprintf(stderr, "Illegal \"interval\"\n");
				return -1;
			}
		} else if (strcmp(*argv, "ecn") == 0) {
			ecn = 1;
		} else if (strcmp(*argv, "noecn") == 0) {
			ecn = 0;
		} else if (strcmp(*argv, "help") == 0) {
			explain();
			return -1;
		} else {
			fprintf(stderr, "What is \"%s\"?\n", *argv);
			explain();
			return -1;
		}
		argc--; argv++;
	}

	tail = NLMSG_TAIL(n);
	addattr_l(n, 1024, TCA_OPTIONS, NULL, 0);
	if (limit)
		addattr_l(n, 1024, TCA_FQ_CODEL_LIMIT, &limit, sizeof(limit));
	if (flows)
		addattr_l(n, 1024, TCA_FQ_CODEL_FLOWS, &flows, sizeof(flows));
	if (quantum)
		addattr_l(n, 1024, TCA_FQ_CODEL_QUANTUM, &quantum, sizeof(quantum));
	if (interval)
		addattr_l(n, 1024, TCA_FQ_CODEL_INTERVAL, &interval, sizeof(interval));
	if (target)
		addattr_l(n, 1024, TCA_FQ_CODEL_TARGET, &target, sizeof(target));
	if (ecn != -1)
		addattr_l(n, 1024, TCA_FQ_CODEL_ECN, &ecn, sizeof(ecn));
	tail->rta_len = (void *) NLMSG_TAIL(n) - (void *) tail;
	return 0;
}

static int fq_codel_print_opt(struct qdisc_util *qu, FILE *f, struct rtattr *opt)
{
	struct rtattr *tb[TCA_FQ_CODEL_MAX + 1];
	unsigned limit;
	unsigned flows;
	unsigned interval;
	unsigned target;
	unsigned ecn;
	unsigned quantum;
	SPRINT_BUF(b1);

	if (opt == NULL)
		return 0;

	parse_rtattr_nested(tb, TCA_FQ_CODEL_MAX, opt);

	if (tb[TCA_FQ_CODEL_LIMIT] &&
	    RTA_PAYLOAD(tb[TCA_FQ_CODEL_LIMIT]) >= sizeof(__u32)) {
		limit = rta_getattr_u32(tb[TCA_FQ_CODEL_LIMIT]);
		fprintf(f, "limit %up ", limit);
	}
	if (tb[TCA_FQ_CODEL_FLOWS] &&
	    RTA_PAYLOAD(tb[TCA_FQ_CODEL_FLOWS]) >= sizeof(__u32)) {
		flows = rta_getattr_u32(tb[TCA_FQ_CODEL_FLOWS]);
		fprintf(f, "flows %u ", flows);
	}
	if (tb[TCA_FQ_CODEL_QUANTUM] &&
	    RTA_PAYLOAD(tb[TCA_FQ_CODEL_QUANTUM]) >= sizeof(__u32)) {
		quantum = rta_getattr_u32(tb[TCA_FQ_CODEL_QUANTUM]);
		fprintf(f, "quantum %u ", quantum);
	}
	if (tb[TCA_FQ_CODEL_TARGET] &&
	    RTA_PAYLOAD(tb[TCA_FQ_CODEL_TARGET]) >= sizeof(__u32)) {
		target = rta_getattr_u32(tb[TCA_FQ_CODEL_TARGET]);
		fprintf(f, "target %s ", sprint_time(target, b1));
	}
	if (tb[TCA_FQ_CODEL_INTERVAL] &&
	    RTA_PAYLOAD(tb[TCA_FQ_CODEL_INTERVAL]) >= sizeof(__u32)) {
		interval = rta_getattr_u32(tb[TCA_FQ_CODEL_INTERVAL]);
		fprintf(f, "interval %s ", sprint_time(interval, b1));
	}
	if (tb[TCA_FQ_CODEL_ECN] &&
	    RTA_PAYLOAD(tb[TCA_FQ_CODEL_ECN]) >= sizeof(__u32)) {
		ecn = rta_getattr_u32(tb[TCA_FQ_CODEL_ECN]);
		if (ecn)
			fprintf(f, "ecn ");
	}

	return 0;
}

static int fq_codel_print_xstats(struct qdisc_util *qu, FILE *f,
				 struct rtattr *xstats)
{
	struct tc_fq_codel_xstats *st;
	SPRINT_BUF(b1);

	if (xstats == NULL)
		return 0;

	if (RTA_PAYLOAD(xstats) < sizeof(*st))
		return -1;

	st = RTA_DATA(xstats);
	if (st->type == TCA_FQ_CODEL_XSTATS_QDISC) {
		fprintf(f, "  maxpacket %u drop_overlimit %u new_flow_count %u ecn_mark %u",
			st->qdisc_stats.maxpacket,
			st->qdisc_stats.drop_overlimit,
			st->qdisc_stats.new_flow_count,
			st->qdisc_stats.ecn_mark);
		fprintf(f, "\n  new_flows_len %u old_flows_len %u",
			st->qdisc_stats.new_flows_len,
			st->qdisc_stats.old_flows_len);
	}
	if (st->type == TCA_FQ_CODEL_XSTATS_CLASS) {
		fprintf(f, "  deficit %d count %u lastcount %u ldelay %s",
			st->class_stats.deficit,
			st->class_stats.count,
			st->class_stats.lastcount,
			sprint_time(st->class_stats.ldelay, b1));
		if (st->class_stats.dropping) {
			fprintf(f, " dropping");
			if (st->class_stats.drop_next < 0)
				fprintf(f, " drop_next -%s",
					sprint_time(-st->class_stats.drop_next, b1));
			else
				fprintf(f, " drop_next %s",
					sprint_time(st->class_stats.drop_next, b1));
		}
	}
	return 0;

}

struct qdisc_util fq_codel_qdisc_util = {
	.id		= "fq_codel",
	.parse_qopt	= fq_codel_parse_opt,
	.print_qopt	= fq_codel_print_opt,
	.print_xstats	= fq_codel_print_xstats,
};
