/*
 * tc.c		"tc" utility frontend.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Authors:	Alexey Kuznetsov, <kuznet@ms2.inr.ac.ru>
 *
 * Fixes:
 *
 * Petri Mattila <petri@prihateam.fi> 990308: wrong memset's resulted in faults
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

#include "version.h"
#include "utils.h"
#include "tc_util.h"
#include "tc_common.h"
#include "namespace.h"
#include "rt_names.h"
#include "bpf_util.h"

int show_stats;
int show_details;
int show_raw;
int show_graph;
int timestamp;

int batch_mode;
int use_iec;
int force;
bool use_names;
int json;
int color;
int oneline;
int brief;

static char *conf_file;

struct rtnl_handle rth;

static void *BODY;	/* cached handle dlopen(NULL) */
static struct qdisc_util *qdisc_list;
static struct filter_util *filter_list;

static int print_noqopt(struct qdisc_util *qu, FILE *f,
			struct rtattr *opt)
{
	if (opt && RTA_PAYLOAD(opt))
		fprintf(f, "[Unknown qdisc, optlen=%u] ",
			(unsigned int) RTA_PAYLOAD(opt));
	return 0;
}

static int parse_noqopt(struct qdisc_util *qu, int argc, char **argv,
			struct nlmsghdr *n, const char *dev)
{
	if (argc) {
		fprintf(stderr,
			"Unknown qdisc \"%s\", hence option \"%s\" is unparsable\n",
			qu->id, *argv);
		return -1;
	}
	return 0;
}

static int print_nofopt(struct filter_util *qu, FILE *f, struct rtattr *opt, __u32 fhandle)
{
	if (opt && RTA_PAYLOAD(opt))
		fprintf(f, "fh %08x [Unknown filter, optlen=%u] ",
			fhandle, (unsigned int) RTA_PAYLOAD(opt));
	else if (fhandle)
		fprintf(f, "fh %08x ", fhandle);
	return 0;
}

static int parse_nofopt(struct filter_util *qu, char *fhandle,
			int argc, char **argv, struct nlmsghdr *n)
{
	__u32 handle;

	if (argc) {
		fprintf(stderr,
			"Unknown filter \"%s\", hence option \"%s\" is unparsable\n",
			qu->id, *argv);
		return -1;
	}
	if (fhandle) {
		struct tcmsg *t = NLMSG_DATA(n);

		if (get_u32(&handle, fhandle, 16)) {
			fprintf(stderr, "Unparsable filter ID \"%s\"\n", fhandle);
			return -1;
		}
		t->tcm_handle = handle;
	}
	return 0;
}

struct qdisc_util *get_qdisc_kind(const char *str)
{
	void *dlh;
	char buf[256];
	struct qdisc_util *q;

	for (q = qdisc_list; q; q = q->next)
		if (strcmp(q->id, str) == 0)
			return q;

	snprintf(buf, sizeof(buf), "%s/q_%s.so", get_tc_lib(), str);
	dlh = dlopen(buf, RTLD_LAZY);
	if (!dlh) {
		/* look in current binary, only open once */
		dlh = BODY;
		if (dlh == NULL) {
			dlh = BODY = dlopen(NULL, RTLD_LAZY);
			if (dlh == NULL)
				goto noexist;
		}
	}

	snprintf(buf, sizeof(buf), "%s_qdisc_util", str);
	q = dlsym(dlh, buf);
	if (q == NULL)
		goto noexist;

reg:
	q->next = qdisc_list;
	qdisc_list = q;
	return q;

noexist:
	q = calloc(1, sizeof(*q));
	if (q) {
		q->id = strdup(str);
		q->parse_qopt = parse_noqopt;
		q->print_qopt = print_noqopt;
		goto reg;
	}
	return q;
}


struct filter_util *get_filter_kind(const char *str)
{
	void *dlh;
	char buf[256];
	struct filter_util *q;

	for (q = filter_list; q; q = q->next)
		if (strcmp(q->id, str) == 0)
			return q;

	snprintf(buf, sizeof(buf), "%s/f_%s.so", get_tc_lib(), str);
	dlh = dlopen(buf, RTLD_LAZY);
	if (dlh == NULL) {
		dlh = BODY;
		if (dlh == NULL) {
			dlh = BODY = dlopen(NULL, RTLD_LAZY);
			if (dlh == NULL)
				goto noexist;
		}
	}

	snprintf(buf, sizeof(buf), "%s_filter_util", str);
	q = dlsym(dlh, buf);
	if (q == NULL)
		goto noexist;

reg:
	q->next = filter_list;
	filter_list = q;
	return q;
noexist:
	q = calloc(1, sizeof(*q));
	if (q) {
		strncpy(q->id, str, 15);
		q->parse_fopt = parse_nofopt;
		q->print_fopt = print_nofopt;
		goto reg;
	}
	return q;
}

static void usage(void)
{
	fprintf(stderr,
		"Usage:	tc [ OPTIONS ] OBJECT { COMMAND | help }\n"
		"	tc [-force] -batch filename\n"
		"where  OBJECT := { qdisc | class | filter | chain |\n"
		"		    action | monitor | exec }\n"
		"       OPTIONS := { -V[ersion] | -s[tatistics] | -d[etails] | -r[aw] |\n"
		"		    -o[neline] | -j[son] | -p[retty] | -c[olor]\n"
		"		    -b[atch] [filename] | -n[etns] name | -N[umeric] |\n"
		"		     -nm | -nam[es] | { -cf | -conf } path\n"
		"		     -br[ief] }\n");
}

static int do_cmd(int argc, char **argv)
{
	if (matches(*argv, "qdisc") == 0)
		return do_qdisc(argc-1, argv+1);
	if (matches(*argv, "class") == 0)
		return do_class(argc-1, argv+1);
	if (matches(*argv, "filter") == 0)
		return do_filter(argc-1, argv+1);
	if (matches(*argv, "chain") == 0)
		return do_chain(argc-1, argv+1);
	if (matches(*argv, "actions") == 0)
		return do_action(argc-1, argv+1);
	if (matches(*argv, "monitor") == 0)
		return do_tcmonitor(argc-1, argv+1);
	if (matches(*argv, "exec") == 0)
		return do_exec(argc-1, argv+1);
	if (matches(*argv, "help") == 0) {
		usage();
		return 0;
	}

	fprintf(stderr, "Object \"%s\" is unknown, try \"tc help\".\n",
		*argv);
	return -1;
}

static int tc_batch_cmd(int argc, char *argv[], void *data)
{
	return do_cmd(argc, argv);
}

static int batch(const char *name)
{
	int ret;

	batch_mode = 1;
	tc_core_init();

	if (rtnl_open(&rth, 0) < 0) {
		fprintf(stderr, "Cannot open rtnetlink\n");
		return -1;
	}

	ret = do_batch(name, force, tc_batch_cmd, NULL);

	rtnl_close(&rth);
	return ret;
}


int main(int argc, char **argv)
{
	const char *libbpf_version;
	char *batch_file = NULL;
	int ret;

	while (argc > 1) {
		if (argv[1][0] != '-')
			break;
		if (matches(argv[1], "-stats") == 0 ||
			 matches(argv[1], "-statistics") == 0) {
			++show_stats;
		} else if (matches(argv[1], "-details") == 0) {
			++show_details;
		} else if (matches(argv[1], "-raw") == 0) {
			++show_raw;
		} else if (matches(argv[1], "-pretty") == 0) {
			++pretty;
		} else if (matches(argv[1], "-graph") == 0) {
			show_graph = 1;
		} else if (matches(argv[1], "-Version") == 0) {
			printf("tc utility, iproute2-%s", version);
			libbpf_version = get_libbpf_version();
			if (libbpf_version)
				printf(", libbpf %s", libbpf_version);
			printf("\n");
			return 0;
		} else if (matches(argv[1], "-iec") == 0) {
			++use_iec;
		} else if (matches(argv[1], "-help") == 0) {
			usage();
			return 0;
		} else if (matches(argv[1], "-force") == 0) {
			++force;
		} else if (matches(argv[1], "-batch") == 0) {
			argc--;	argv++;
			if (argc <= 1)
				usage();
			batch_file = argv[1];
		} else if (matches(argv[1], "-netns") == 0) {
			NEXT_ARG();
			if (netns_switch(argv[1]))
				return -1;
		} else if (matches(argv[1], "-Numeric") == 0) {
			++numeric;
		} else if (matches(argv[1], "-names") == 0 ||
				matches(argv[1], "-nm") == 0) {
			use_names = true;
		} else if (matches(argv[1], "-cf") == 0 ||
				matches(argv[1], "-conf") == 0) {
			NEXT_ARG();
			conf_file = argv[1];
		} else if (matches_color(argv[1], &color)) {
		} else if (matches(argv[1], "-timestamp") == 0) {
			timestamp++;
		} else if (matches(argv[1], "-tshort") == 0) {
			++timestamp;
			++timestamp_short;
		} else if (matches(argv[1], "-json") == 0) {
			++json;
		} else if (matches(argv[1], "-oneline") == 0) {
			++oneline;
		}else if (matches(argv[1], "-brief") == 0) {
			++brief;
		} else {
			fprintf(stderr,
				"Option \"%s\" is unknown, try \"tc -help\".\n",
				argv[1]);
			return -1;
		}
		argc--;	argv++;
	}

	_SL_ = oneline ? "\\" : "\n";

	check_enable_color(color, json);

	if (batch_file)
		return batch(batch_file);

	if (argc <= 1) {
		usage();
		return 0;
	}

	tc_core_init();
	if (rtnl_open(&rth, 0) < 0) {
		fprintf(stderr, "Cannot open rtnetlink\n");
		exit(1);
	}

	if (use_names && cls_names_init(conf_file)) {
		ret = -1;
		goto Exit;
	}

	ret = do_cmd(argc-1, argv+1);
Exit:
	rtnl_close(&rth);

	if (use_names)
		cls_names_uninit();

	return ret;
}
