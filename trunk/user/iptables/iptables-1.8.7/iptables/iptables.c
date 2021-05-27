/* Code to take an iptables-style command line and do it. */

/*
 * Author: Paul.Russell@rustcorp.com.au and mneuling@radlogic.com.au
 *
 * (C) 2000-2002 by the netfilter coreteam <coreteam@netfilter.org>:
 * 		    Paul 'Rusty' Russell <rusty@rustcorp.com.au>
 * 		    Marc Boucher <marc+nf@mbsi.ca>
 * 		    James Morris <jmorris@intercode.com.au>
 * 		    Harald Welte <laforge@gnumonks.org>
 * 		    Jozsef Kadlecsik <kadlec@blackhole.kfki.hu>
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include "config.h"
#include <getopt.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <limits.h>
#include <unistd.h>
#include <iptables.h>
#include <xtables.h>
#include <fcntl.h>
#include "xshared.h"

static const char unsupported_rev[] = " [unsupported revision]";

static struct option original_opts[] = {
	{.name = "append",        .has_arg = 1, .val = 'A'},
	{.name = "delete",        .has_arg = 1, .val = 'D'},
	{.name = "check",         .has_arg = 1, .val = 'C'},
	{.name = "insert",        .has_arg = 1, .val = 'I'},
	{.name = "replace",       .has_arg = 1, .val = 'R'},
	{.name = "list",          .has_arg = 2, .val = 'L'},
	{.name = "list-rules",    .has_arg = 2, .val = 'S'},
	{.name = "flush",         .has_arg = 2, .val = 'F'},
	{.name = "zero",          .has_arg = 2, .val = 'Z'},
	{.name = "new-chain",     .has_arg = 1, .val = 'N'},
	{.name = "delete-chain",  .has_arg = 2, .val = 'X'},
	{.name = "rename-chain",  .has_arg = 1, .val = 'E'},
	{.name = "policy",        .has_arg = 1, .val = 'P'},
	{.name = "source",        .has_arg = 1, .val = 's'},
	{.name = "destination",   .has_arg = 1, .val = 'd'},
	{.name = "src",           .has_arg = 1, .val = 's'}, /* synonym */
	{.name = "dst",           .has_arg = 1, .val = 'd'}, /* synonym */
	{.name = "protocol",      .has_arg = 1, .val = 'p'},
	{.name = "in-interface",  .has_arg = 1, .val = 'i'},
	{.name = "jump",          .has_arg = 1, .val = 'j'},
	{.name = "table",         .has_arg = 1, .val = 't'},
	{.name = "match",         .has_arg = 1, .val = 'm'},
	{.name = "numeric",       .has_arg = 0, .val = 'n'},
	{.name = "out-interface", .has_arg = 1, .val = 'o'},
	{.name = "verbose",       .has_arg = 0, .val = 'v'},
	{.name = "wait",          .has_arg = 2, .val = 'w'},
	{.name = "wait-interval", .has_arg = 2, .val = 'W'},
	{.name = "exact",         .has_arg = 0, .val = 'x'},
	{.name = "fragments",     .has_arg = 0, .val = 'f'},
	{.name = "version",       .has_arg = 0, .val = 'V'},
	{.name = "help",          .has_arg = 2, .val = 'h'},
	{.name = "line-numbers",  .has_arg = 0, .val = '0'},
	{.name = "modprobe",      .has_arg = 1, .val = 'M'},
	{.name = "set-counters",  .has_arg = 1, .val = 'c'},
	{.name = "goto",          .has_arg = 1, .val = 'g'},
	{.name = "ipv4",          .has_arg = 0, .val = '4'},
	{.name = "ipv6",          .has_arg = 0, .val = '6'},
	{NULL},
};

void iptables_exit_error(enum xtables_exittype status, const char *msg, ...) __attribute__((noreturn, format(printf,2,3)));

struct xtables_globals iptables_globals = {
	.option_offset = 0,
	.program_version = PACKAGE_VERSION,
	.orig_opts = original_opts,
	.exit_err = iptables_exit_error,
	.compat_rev = xtables_compatible_revision,
};

static const int inverse_for_options[NUMBER_OF_OPT] =
{
/* -n */ 0,
/* -s */ IPT_INV_SRCIP,
/* -d */ IPT_INV_DSTIP,
/* -p */ XT_INV_PROTO,
/* -j */ 0,
/* -v */ 0,
/* -x */ 0,
/* -i */ IPT_INV_VIA_IN,
/* -o */ IPT_INV_VIA_OUT,
/*--line*/ 0,
/* -c */ 0,
/* -f */ IPT_INV_FRAG,
};

#define opts iptables_globals.opts
#define prog_name iptables_globals.program_name
#define prog_vers iptables_globals.program_version

static void __attribute__((noreturn))
exit_tryhelp(int status)
{
	if (line != -1)
		fprintf(stderr, "Error occurred at line: %d\n", line);
	fprintf(stderr, "Try `%s -h' or '%s --help' for more information.\n",
			prog_name, prog_name);
	xtables_free_opts(1);
	exit(status);
}

static void
exit_printhelp(const struct xtables_rule_match *matches)
{
	printf("%s v%s\n\n"
"Usage: %s -[ACD] chain rule-specification [options]\n"
"       %s -I chain [rulenum] rule-specification [options]\n"
"       %s -R chain rulenum rule-specification [options]\n"
"       %s -D chain rulenum [options]\n"
"       %s -[LS] [chain [rulenum]] [options]\n"
"       %s -[FZ] [chain] [options]\n"
"       %s -[NX] chain\n"
"       %s -E old-chain-name new-chain-name\n"
"       %s -P chain target [options]\n"
"       %s -h (print this help information)\n\n",
	       prog_name, prog_vers, prog_name, prog_name,
	       prog_name, prog_name, prog_name, prog_name,
	       prog_name, prog_name, prog_name, prog_name);

	printf(
"Commands:\n"
"Either long or short options are allowed.\n"
"  --append  -A chain		Append to chain\n"
"  --check   -C chain		Check for the existence of a rule\n"
"  --delete  -D chain		Delete matching rule from chain\n"
"  --delete  -D chain rulenum\n"
"				Delete rule rulenum (1 = first) from chain\n"
"  --insert  -I chain [rulenum]\n"
"				Insert in chain as rulenum (default 1=first)\n"
"  --replace -R chain rulenum\n"
"				Replace rule rulenum (1 = first) in chain\n"
"  --list    -L [chain [rulenum]]\n"
"				List the rules in a chain or all chains\n"
"  --list-rules -S [chain [rulenum]]\n"
"				Print the rules in a chain or all chains\n"
"  --flush   -F [chain]		Delete all rules in  chain or all chains\n"
"  --zero    -Z [chain [rulenum]]\n"
"				Zero counters in chain or all chains\n"
"  --new     -N chain		Create a new user-defined chain\n"
"  --delete-chain\n"
"            -X [chain]		Delete a user-defined chain\n"
"  --policy  -P chain target\n"
"				Change policy on chain to target\n"
"  --rename-chain\n"
"            -E old-chain new-chain\n"
"				Change chain name, (moving any references)\n"

"Options:\n"
"    --ipv4	-4		Nothing (line is ignored by ip6tables-restore)\n"
"    --ipv6	-6		Error (line is ignored by iptables-restore)\n"
"[!] --protocol	-p proto	protocol: by number or name, eg. `tcp'\n"
"[!] --source	-s address[/mask][...]\n"
"				source specification\n"
"[!] --destination -d address[/mask][...]\n"
"				destination specification\n"
"[!] --in-interface -i input name[+]\n"
"				network interface name ([+] for wildcard)\n"
" --jump	-j target\n"
"				target for rule (may load target extension)\n"
#ifdef IPT_F_GOTO
"  --goto      -g chain\n"
"                              jump to chain with no return\n"
#endif
"  --match	-m match\n"
"				extended match (may load extension)\n"
"  --numeric	-n		numeric output of addresses and ports\n"
"[!] --out-interface -o output name[+]\n"
"				network interface name ([+] for wildcard)\n"
"  --table	-t table	table to manipulate (default: `filter')\n"
"  --verbose	-v		verbose mode\n"
"  --wait	-w [seconds]	maximum wait to acquire xtables lock before give up\n"
"  --wait-interval -W [usecs]	wait time to try to acquire xtables lock\n"
"				default is 1 second\n"
"  --line-numbers		print line numbers when listing\n"
"  --exact	-x		expand numbers (display exact values)\n"
"[!] --fragment	-f		match second or further fragments only\n"
"  --modprobe=<command>		try to insert modules using this command\n"
"  --set-counters PKTS BYTES	set the counter during insert/append\n"
"[!] --version	-V		print package version.\n");

	print_extension_helps(xtables_targets, matches);
	exit(0);
}

void
iptables_exit_error(enum xtables_exittype status, const char *msg, ...)
{
	va_list args;

	va_start(args, msg);
	fprintf(stderr, "%s v%s (legacy): ", prog_name, prog_vers);
	vfprintf(stderr, msg, args);
	va_end(args);
	fprintf(stderr, "\n");
	if (status == PARAMETER_PROBLEM)
		exit_tryhelp(status);
	if (status == VERSION_PROBLEM)
		fprintf(stderr,
			"Perhaps iptables or your kernel needs to be upgraded.\n");
	/* On error paths, make sure that we don't leak memory */
	xtables_free_opts(1);
	exit(status);
}

/*
 *	All functions starting with "parse" should succeed, otherwise
 *	the program fails.
 *	Most routines return pointers to static data that may change
 *	between calls to the same or other routines with a few exceptions:
 *	"host_to_addr", "parse_hostnetwork", and "parse_hostnetworkmask"
 *	return global static data.
*/

/* Christophe Burki wants `-p 6' to imply `-m tcp'.  */

static void
parse_chain(const char *chainname)
{
	const char *ptr;

	if (strlen(chainname) >= XT_EXTENSION_MAXNAMELEN)
		xtables_error(PARAMETER_PROBLEM,
			   "chain name `%s' too long (must be under %u chars)",
			   chainname, XT_EXTENSION_MAXNAMELEN);

	if (*chainname == '-' || *chainname == '!')
		xtables_error(PARAMETER_PROBLEM,
			   "chain name not allowed to start "
			   "with `%c'\n", *chainname);

	if (xtables_find_target(chainname, XTF_TRY_LOAD))
		xtables_error(PARAMETER_PROBLEM,
			   "chain name may not clash "
			   "with target name\n");

	for (ptr = chainname; *ptr; ptr++)
		if (isspace(*ptr))
			xtables_error(PARAMETER_PROBLEM,
				   "Invalid chain name `%s'", chainname);
}

static void
set_option(unsigned int *options, unsigned int option, uint8_t *invflg,
	   int invert)
{
	if (*options & option)
		xtables_error(PARAMETER_PROBLEM, "multiple -%c flags not allowed",
			   opt2char(option));
	*options |= option;

	if (invert) {
		unsigned int i;
		for (i = 0; 1 << i != option; i++);

		if (!inverse_for_options[i])
			xtables_error(PARAMETER_PROBLEM,
				   "cannot have ! before -%c",
				   opt2char(option));
		*invflg |= inverse_for_options[i];
	}
}

static void
print_header(unsigned int format, const char *chain, struct xtc_handle *handle)
{
	struct xt_counters counters;
	const char *pol = iptc_get_policy(chain, &counters, handle);
	printf("Chain %s", chain);
	if (pol) {
		printf(" (policy %s", pol);
		if (!(format & FMT_NOCOUNTS)) {
			fputc(' ', stdout);
			xtables_print_num(counters.pcnt, (format|FMT_NOTABLE));
			fputs("packets, ", stdout);
			xtables_print_num(counters.bcnt, (format|FMT_NOTABLE));
			fputs("bytes", stdout);
		}
		printf(")\n");
	} else {
		unsigned int refs;
		if (!iptc_get_references(&refs, chain, handle))
			printf(" (ERROR obtaining refs)\n");
		else
			printf(" (%u references)\n", refs);
	}

	if (format & FMT_LINENUMBERS)
		printf(FMT("%-4s ", "%s "), "num");
	if (!(format & FMT_NOCOUNTS)) {
		if (format & FMT_KILOMEGAGIGA) {
			printf(FMT("%5s ","%s "), "pkts");
			printf(FMT("%5s ","%s "), "bytes");
		} else {
			printf(FMT("%8s ","%s "), "pkts");
			printf(FMT("%10s ","%s "), "bytes");
		}
	}
	if (!(format & FMT_NOTARGET))
		printf(FMT("%-9s ","%s "), "target");
	fputs(" prot ", stdout);
	if (format & FMT_OPTIONS)
		fputs("opt", stdout);
	if (format & FMT_VIA) {
		printf(FMT(" %-6s ","%s "), "in");
		printf(FMT("%-6s ","%s "), "out");
	}
	printf(FMT(" %-19s ","%s "), "source");
	printf(FMT(" %-19s "," %s "), "destination");
	printf("\n");
}


static int
print_match(const struct xt_entry_match *m,
	    const struct ipt_ip *ip,
	    int numeric)
{
	const char *name = m->u.user.name;
	const int revision = m->u.user.revision;
	struct xtables_match *match, *mt;

	match = xtables_find_match(name, XTF_TRY_LOAD, NULL);
	if (match) {
		mt = xtables_find_match_revision(name, XTF_TRY_LOAD,
						 match, revision);
		if (mt && mt->print)
			mt->print(ip, m, numeric);
		else if (match->print)
			printf("%s%s ", match->name, unsupported_rev);
		else
			printf("%s ", match->name);
	} else {
		if (name[0])
			printf("UNKNOWN match `%s' ", name);
	}
	/* Don't stop iterating. */
	return 0;
}

/* e is called `fw' here for historical reasons */
static void
print_firewall(const struct ipt_entry *fw,
	       const char *targname,
	       unsigned int num,
	       unsigned int format,
	       struct xtc_handle *const handle)
{
	struct xtables_target *target, *tg;
	const struct xt_entry_target *t;
	uint8_t flags;

	if (!iptc_is_chain(targname, handle))
		target = xtables_find_target(targname, XTF_TRY_LOAD);
	else
		target = xtables_find_target(XT_STANDARD_TARGET,
		         XTF_LOAD_MUST_SUCCEED);

	t = ipt_get_target((struct ipt_entry *)fw);
	flags = fw->ip.flags;

	if (format & FMT_LINENUMBERS)
		printf(FMT("%-4u ", "%u "), num);

	if (!(format & FMT_NOCOUNTS)) {
		xtables_print_num(fw->counters.pcnt, format);
		xtables_print_num(fw->counters.bcnt, format);
	}

	if (!(format & FMT_NOTARGET))
		printf(FMT("%-9s ", "%s "), targname);

	fputc(fw->ip.invflags & XT_INV_PROTO ? '!' : ' ', stdout);
	{
		const char *pname = proto_to_name(fw->ip.proto, format&FMT_NUMERIC);
		if (pname)
			printf(FMT("%-5s", "%s "), pname);
		else
			printf(FMT("%-5hu", "%hu "), fw->ip.proto);
	}

	if (format & FMT_OPTIONS) {
		if (format & FMT_NOTABLE)
			fputs("opt ", stdout);
		fputc(fw->ip.invflags & IPT_INV_FRAG ? '!' : '-', stdout);
		fputc(flags & IPT_F_FRAG ? 'f' : '-', stdout);
		fputc(' ', stdout);
	}

	print_ifaces(fw->ip.iniface, fw->ip.outiface, fw->ip.invflags, format);

	print_ipv4_addresses(fw, format);

	if (format & FMT_NOTABLE)
		fputs("  ", stdout);

#ifdef IPT_F_GOTO
	if(fw->ip.flags & IPT_F_GOTO)
		printf("[goto] ");
#endif

	IPT_MATCH_ITERATE(fw, print_match, &fw->ip, format & FMT_NUMERIC);

	if (target) {
		const int revision = t->u.user.revision;

		tg = xtables_find_target_revision(targname, XTF_TRY_LOAD,
						  target, revision);
		if (tg && tg->print)
			/* Print the target information. */
			tg->print(&fw->ip, t, format & FMT_NUMERIC);
		else if (target->print)
			printf(" %s%s", target->name, unsupported_rev);
	} else if (t->u.target_size != sizeof(*t))
		printf("[%u bytes of unknown target data] ",
		       (unsigned int)(t->u.target_size - sizeof(*t)));

	if (!(format & FMT_NONEWLINE))
		fputc('\n', stdout);
}

static void
print_firewall_line(const struct ipt_entry *fw,
		    struct xtc_handle *const h)
{
	struct xt_entry_target *t;

	t = ipt_get_target((struct ipt_entry *)fw);
	print_firewall(fw, t->u.user.name, 0, FMT_PRINT_RULE, h);
}

static int
append_entry(const xt_chainlabel chain,
	     struct ipt_entry *fw,
	     unsigned int nsaddrs,
	     const struct in_addr saddrs[],
	     const struct in_addr smasks[],
	     unsigned int ndaddrs,
	     const struct in_addr daddrs[],
	     const struct in_addr dmasks[],
	     int verbose,
	     struct xtc_handle *handle)
{
	unsigned int i, j;
	int ret = 1;

	for (i = 0; i < nsaddrs; i++) {
		fw->ip.src.s_addr = saddrs[i].s_addr;
		fw->ip.smsk.s_addr = smasks[i].s_addr;
		for (j = 0; j < ndaddrs; j++) {
			fw->ip.dst.s_addr = daddrs[j].s_addr;
			fw->ip.dmsk.s_addr = dmasks[j].s_addr;
			if (verbose)
				print_firewall_line(fw, handle);
			ret &= iptc_append_entry(chain, fw, handle);
		}
	}

	return ret;
}

static int
replace_entry(const xt_chainlabel chain,
	      struct ipt_entry *fw,
	      unsigned int rulenum,
	      const struct in_addr *saddr, const struct in_addr *smask,
	      const struct in_addr *daddr, const struct in_addr *dmask,
	      int verbose,
	      struct xtc_handle *handle)
{
	fw->ip.src.s_addr = saddr->s_addr;
	fw->ip.dst.s_addr = daddr->s_addr;
	fw->ip.smsk.s_addr = smask->s_addr;
	fw->ip.dmsk.s_addr = dmask->s_addr;

	if (verbose)
		print_firewall_line(fw, handle);
	return iptc_replace_entry(chain, fw, rulenum, handle);
}

static int
insert_entry(const xt_chainlabel chain,
	     struct ipt_entry *fw,
	     unsigned int rulenum,
	     unsigned int nsaddrs,
	     const struct in_addr saddrs[],
	     const struct in_addr smasks[],
	     unsigned int ndaddrs,
	     const struct in_addr daddrs[],
	     const struct in_addr dmasks[],
	     int verbose,
	     struct xtc_handle *handle)
{
	unsigned int i, j;
	int ret = 1;

	for (i = 0; i < nsaddrs; i++) {
		fw->ip.src.s_addr = saddrs[i].s_addr;
		fw->ip.smsk.s_addr = smasks[i].s_addr;
		for (j = 0; j < ndaddrs; j++) {
			fw->ip.dst.s_addr = daddrs[j].s_addr;
			fw->ip.dmsk.s_addr = dmasks[j].s_addr;
			if (verbose)
				print_firewall_line(fw, handle);
			ret &= iptc_insert_entry(chain, fw, rulenum, handle);
		}
	}

	return ret;
}

static unsigned char *
make_delete_mask(const struct xtables_rule_match *matches,
		 const struct xtables_target *target)
{
	/* Establish mask for comparison */
	unsigned int size;
	const struct xtables_rule_match *matchp;
	unsigned char *mask, *mptr;

	size = sizeof(struct ipt_entry);
	for (matchp = matches; matchp; matchp = matchp->next)
		size += XT_ALIGN(sizeof(struct xt_entry_match)) + matchp->match->size;

	mask = xtables_calloc(1, size
			 + XT_ALIGN(sizeof(struct xt_entry_target))
			 + target->size);

	memset(mask, 0xFF, sizeof(struct ipt_entry));
	mptr = mask + sizeof(struct ipt_entry);

	for (matchp = matches; matchp; matchp = matchp->next) {
		memset(mptr, 0xFF,
		       XT_ALIGN(sizeof(struct xt_entry_match))
		       + matchp->match->userspacesize);
		mptr += XT_ALIGN(sizeof(struct xt_entry_match)) + matchp->match->size;
	}

	memset(mptr, 0xFF,
	       XT_ALIGN(sizeof(struct xt_entry_target))
	       + target->userspacesize);

	return mask;
}

static int
delete_entry(const xt_chainlabel chain,
	     struct ipt_entry *fw,
	     unsigned int nsaddrs,
	     const struct in_addr saddrs[],
	     const struct in_addr smasks[],
	     unsigned int ndaddrs,
	     const struct in_addr daddrs[],
	     const struct in_addr dmasks[],
	     int verbose,
	     struct xtc_handle *handle,
	     struct xtables_rule_match *matches,
	     const struct xtables_target *target)
{
	unsigned int i, j;
	int ret = 1;
	unsigned char *mask;

	mask = make_delete_mask(matches, target);
	for (i = 0; i < nsaddrs; i++) {
		fw->ip.src.s_addr = saddrs[i].s_addr;
		fw->ip.smsk.s_addr = smasks[i].s_addr;
		for (j = 0; j < ndaddrs; j++) {
			fw->ip.dst.s_addr = daddrs[j].s_addr;
			fw->ip.dmsk.s_addr = dmasks[j].s_addr;
			if (verbose)
				print_firewall_line(fw, handle);
			ret &= iptc_delete_entry(chain, fw, mask, handle);
		}
	}
	free(mask);

	return ret;
}

static int
check_entry(const xt_chainlabel chain, struct ipt_entry *fw,
	    unsigned int nsaddrs, const struct in_addr *saddrs,
	    const struct in_addr *smasks, unsigned int ndaddrs,
	    const struct in_addr *daddrs, const struct in_addr *dmasks,
	    bool verbose, struct xtc_handle *handle,
	    struct xtables_rule_match *matches,
	    const struct xtables_target *target)
{
	unsigned int i, j;
	int ret = 1;
	unsigned char *mask;

	mask = make_delete_mask(matches, target);
	for (i = 0; i < nsaddrs; i++) {
		fw->ip.src.s_addr = saddrs[i].s_addr;
		fw->ip.smsk.s_addr = smasks[i].s_addr;
		for (j = 0; j < ndaddrs; j++) {
			fw->ip.dst.s_addr = daddrs[j].s_addr;
			fw->ip.dmsk.s_addr = dmasks[j].s_addr;
			if (verbose)
				print_firewall_line(fw, handle);
			ret &= iptc_check_entry(chain, fw, mask, handle);
		}
	}

	free(mask);
	return ret;
}

int
for_each_chain4(int (*fn)(const xt_chainlabel, int, struct xtc_handle *),
	       int verbose, int builtinstoo, struct xtc_handle *handle)
{
        int ret = 1;
	const char *chain;
	char *chains;
	unsigned int i, chaincount = 0;

	chain = iptc_first_chain(handle);
	while (chain) {
		chaincount++;
		chain = iptc_next_chain(handle);
        }

	chains = xtables_malloc(sizeof(xt_chainlabel) * chaincount);
	i = 0;
	chain = iptc_first_chain(handle);
	while (chain) {
		strcpy(chains + i*sizeof(xt_chainlabel), chain);
		i++;
		chain = iptc_next_chain(handle);
        }

	for (i = 0; i < chaincount; i++) {
		if (!builtinstoo
		    && iptc_builtin(chains + i*sizeof(xt_chainlabel),
				    handle) == 1)
			continue;
	        ret &= fn(chains + i*sizeof(xt_chainlabel), verbose, handle);
	}

	free(chains);
        return ret;
}

int
flush_entries4(const xt_chainlabel chain, int verbose,
	      struct xtc_handle *handle)
{
	if (!chain)
		return for_each_chain4(flush_entries4, verbose, 1, handle);

	if (verbose)
		fprintf(stdout, "Flushing chain `%s'\n", chain);
	return iptc_flush_entries(chain, handle);
}

static int
zero_entries(const xt_chainlabel chain, int verbose,
	     struct xtc_handle *handle)
{
	if (!chain)
		return for_each_chain4(zero_entries, verbose, 1, handle);

	if (verbose)
		fprintf(stdout, "Zeroing chain `%s'\n", chain);
	return iptc_zero_entries(chain, handle);
}

int
delete_chain4(const xt_chainlabel chain, int verbose,
	     struct xtc_handle *handle)
{
	if (!chain)
		return for_each_chain4(delete_chain4, verbose, 0, handle);

	if (verbose)
		fprintf(stdout, "Deleting chain `%s'\n", chain);
	return iptc_delete_chain(chain, handle);
}

static int
list_entries(const xt_chainlabel chain, int rulenum, int verbose, int numeric,
	     int expanded, int linenumbers, struct xtc_handle *handle)
{
	int found = 0;
	unsigned int format;
	const char *this;

	format = FMT_OPTIONS;
	if (!verbose)
		format |= FMT_NOCOUNTS;
	else
		format |= FMT_VIA;

	if (numeric)
		format |= FMT_NUMERIC;

	if (!expanded)
		format |= FMT_KILOMEGAGIGA;

	if (linenumbers)
		format |= FMT_LINENUMBERS;

	for (this = iptc_first_chain(handle);
	     this;
	     this = iptc_next_chain(handle)) {
		const struct ipt_entry *i;
		unsigned int num;

		if (chain && strcmp(chain, this) != 0)
			continue;

		if (found) printf("\n");

		if (!rulenum)
			print_header(format, this, handle);
		i = iptc_first_rule(this, handle);

		num = 0;
		while (i) {
			num++;
			if (!rulenum || num == rulenum)
				print_firewall(i,
					       iptc_get_target(i, handle),
					       num,
					       format,
					       handle);
			i = iptc_next_rule(i, handle);
		}
		found = 1;
	}

	errno = ENOENT;
	return found;
}

static void print_proto(uint16_t proto, int invert)
{
	if (proto) {
		unsigned int i;
		const char *invertstr = invert ? " !" : "";

		const struct protoent *pent = getprotobynumber(proto);
		if (pent) {
			printf("%s -p %s", invertstr, pent->p_name);
			return;
		}

		for (i = 0; xtables_chain_protos[i].name != NULL; ++i)
			if (xtables_chain_protos[i].num == proto) {
				printf("%s -p %s",
				       invertstr, xtables_chain_protos[i].name);
				return;
			}

		printf("%s -p %u", invertstr, proto);
	}
}

#define IP_PARTS_NATIVE(n)			\
(unsigned int)((n)>>24)&0xFF,			\
(unsigned int)((n)>>16)&0xFF,			\
(unsigned int)((n)>>8)&0xFF,			\
(unsigned int)((n)&0xFF)

#define IP_PARTS(n) IP_PARTS_NATIVE(ntohl(n))

/* This assumes that mask is contiguous, and byte-bounded. */
static void
print_iface(char letter, const char *iface, const unsigned char *mask,
	    int invert)
{
	unsigned int i;

	if (mask[0] == 0)
		return;

	printf("%s -%c ", invert ? " !" : "", letter);

	for (i = 0; i < IFNAMSIZ; i++) {
		if (mask[i] != 0) {
			if (iface[i] != '\0')
				printf("%c", iface[i]);
		} else {
			/* we can access iface[i-1] here, because
			 * a few lines above we make sure that mask[0] != 0 */
			if (iface[i-1] != '\0')
				printf("+");
			break;
		}
	}
}

static int print_match_save(const struct xt_entry_match *e,
			const struct ipt_ip *ip)
{
	const char *name = e->u.user.name;
	const int revision = e->u.user.revision;
	struct xtables_match *match, *mt, *mt2;

	match = xtables_find_match(name, XTF_TRY_LOAD, NULL);
	if (match) {
		mt = mt2 = xtables_find_match_revision(name, XTF_TRY_LOAD,
						       match, revision);
		if (!mt2)
			mt2 = match;
		printf(" -m %s", mt2->alias ? mt2->alias(e) : name);

		/* some matches don't provide a save function */
		if (mt && mt->save)
			mt->save(ip, e);
		else if (match->save)
			printf(unsupported_rev);
	} else {
		if (e->u.match_size) {
			fprintf(stderr,
				"Can't find library for match `%s'\n",
				name);
			exit(1);
		}
	}
	return 0;
}

/* Print a given ip including mask if necessary. */
static void print_ip(const char *prefix, uint32_t ip,
		     uint32_t mask, int invert)
{
	uint32_t bits, hmask = ntohl(mask);
	int i;

	if (!mask && !ip && !invert)
		return;

	printf("%s %s %u.%u.%u.%u",
		invert ? " !" : "",
		prefix,
		IP_PARTS(ip));

	if (mask == 0xFFFFFFFFU) {
		printf("/32");
		return;
	}

	i    = 32;
	bits = 0xFFFFFFFEU;
	while (--i >= 0 && hmask != bits)
		bits <<= 1;
	if (i >= 0)
		printf("/%u", i);
	else
		printf("/%u.%u.%u.%u", IP_PARTS(mask));
}

/* We want this to be readable, so only print out necessary fields.
 * Because that's the kind of world I want to live in.
 */
void print_rule4(const struct ipt_entry *e,
		struct xtc_handle *h, const char *chain, int counters)
{
	const struct xt_entry_target *t;
	const char *target_name;

	/* print counters for iptables-save */
	if (counters > 0)
		printf("[%llu:%llu] ", (unsigned long long)e->counters.pcnt, (unsigned long long)e->counters.bcnt);

	/* print chain name */
	printf("-A %s", chain);

	/* Print IP part. */
	print_ip("-s", e->ip.src.s_addr,e->ip.smsk.s_addr,
			e->ip.invflags & IPT_INV_SRCIP);

	print_ip("-d", e->ip.dst.s_addr, e->ip.dmsk.s_addr,
			e->ip.invflags & IPT_INV_DSTIP);

	print_iface('i', e->ip.iniface, e->ip.iniface_mask,
		    e->ip.invflags & IPT_INV_VIA_IN);

	print_iface('o', e->ip.outiface, e->ip.outiface_mask,
		    e->ip.invflags & IPT_INV_VIA_OUT);

	print_proto(e->ip.proto, e->ip.invflags & XT_INV_PROTO);

	if (e->ip.flags & IPT_F_FRAG)
		printf("%s -f",
		       e->ip.invflags & IPT_INV_FRAG ? " !" : "");

	/* Print matchinfo part */
	if (e->target_offset)
		IPT_MATCH_ITERATE(e, print_match_save, &e->ip);

	/* print counters for iptables -R */
	if (counters < 0)
		printf(" -c %llu %llu", (unsigned long long)e->counters.pcnt, (unsigned long long)e->counters.bcnt);

	/* Print target name and targinfo part */
	target_name = iptc_get_target(e, h);
	t = ipt_get_target((struct ipt_entry *)e);
	if (t->u.user.name[0]) {
		const char *name = t->u.user.name;
		const int revision = t->u.user.revision;
		struct xtables_target *target, *tg, *tg2;

		target = xtables_find_target(name, XTF_TRY_LOAD);
		if (!target) {
			fprintf(stderr, "Can't find library for target `%s'\n",
				name);
			exit(1);
		}

		tg = tg2 = xtables_find_target_revision(name, XTF_TRY_LOAD,
							target, revision);
		if (!tg2)
			tg2 = target;
		printf(" -j %s", tg2->alias ? tg2->alias(t) : target_name);

		if (tg && tg->save)
			tg->save(&e->ip, t);
		else if (target->save)
			printf(unsupported_rev);
		else {
			/* If the target size is greater than xt_entry_target
			 * there is something to be saved, we just don't know
			 * how to print it */
			if (t->u.target_size !=
			    sizeof(struct xt_entry_target)) {
				fprintf(stderr, "Target `%s' is missing "
						"save function\n",
					name);
				exit(1);
			}
		}
	} else if (target_name && (*target_name != '\0'))
#ifdef IPT_F_GOTO
		printf(" -%c %s", e->ip.flags & IPT_F_GOTO ? 'g' : 'j', target_name);
#else
		printf(" -j %s", target_name);
#endif

	printf("\n");
}

static int
list_rules(const xt_chainlabel chain, int rulenum, int counters,
	     struct xtc_handle *handle)
{
	const char *this = NULL;
	int found = 0;

	if (counters)
	    counters = -1;		/* iptables -c format */

	/* Dump out chain names first,
	 * thereby preventing dependency conflicts */
	if (!rulenum) for (this = iptc_first_chain(handle);
	     this;
	     this = iptc_next_chain(handle)) {
		if (chain && strcmp(this, chain) != 0)
			continue;

		if (iptc_builtin(this, handle)) {
			struct xt_counters count;
			printf("-P %s %s", this, iptc_get_policy(this, &count, handle));
			if (counters)
			    printf(" -c %llu %llu", (unsigned long long)count.pcnt, (unsigned long long)count.bcnt);
			printf("\n");
		} else {
			printf("-N %s\n", this);
		}
	}

	for (this = iptc_first_chain(handle);
	     this;
	     this = iptc_next_chain(handle)) {
		const struct ipt_entry *e;
		int num = 0;

		if (chain && strcmp(this, chain) != 0)
			continue;

		/* Dump out rules */
		e = iptc_first_rule(this, handle);
		while(e) {
			num++;
			if (!rulenum || num == rulenum)
			    print_rule4(e, handle, this, counters);
			e = iptc_next_rule(e, handle);
		}
		found = 1;
	}

	errno = ENOENT;
	return found;
}

static struct ipt_entry *
generate_entry(const struct ipt_entry *fw,
	       struct xtables_rule_match *matches,
	       struct xt_entry_target *target)
{
	unsigned int size;
	struct xtables_rule_match *matchp;
	struct ipt_entry *e;

	size = sizeof(struct ipt_entry);
	for (matchp = matches; matchp; matchp = matchp->next)
		size += matchp->match->m->u.match_size;

	e = xtables_malloc(size + target->u.target_size);
	*e = *fw;
	e->target_offset = size;
	e->next_offset = size + target->u.target_size;

	size = 0;
	for (matchp = matches; matchp; matchp = matchp->next) {
		memcpy(e->elems + size, matchp->match->m, matchp->match->m->u.match_size);
		size += matchp->match->m->u.match_size;
	}
	memcpy(e->elems + size, target, target->u.target_size);

	return e;
}

int do_command4(int argc, char *argv[], char **table,
		struct xtc_handle **handle, bool restore)
{
	struct iptables_command_state cs = {
		.jumpto	= "",
		.argv	= argv,
	};
	struct ipt_entry *e = NULL;
	unsigned int nsaddrs = 0, ndaddrs = 0;
	struct in_addr *saddrs = NULL, *smasks = NULL;
	struct in_addr *daddrs = NULL, *dmasks = NULL;
	struct timeval wait_interval = {
		.tv_sec = 1,
	};
	bool wait_interval_set = false;
	int verbose = 0;
	int wait = 0;
	const char *chain = NULL;
	const char *shostnetworkmask = NULL, *dhostnetworkmask = NULL;
	const char *policy = NULL, *newname = NULL;
	unsigned int rulenum = 0, command = 0;
	const char *pcnt = NULL, *bcnt = NULL;
	int ret = 1;
	struct xtables_match *m;
	struct xtables_rule_match *matchp;
	struct xtables_target *t;
	unsigned long long cnt;
	bool table_set = false;

	/* re-set optind to 0 in case do_command4 gets called
	 * a second time */
	optind = 0;

	/* clear mflags in case do_command4 gets called a second time
	 * (we clear the global list of all matches for security)*/
	for (m = xtables_matches; m; m = m->next)
		m->mflags = 0;

	for (t = xtables_targets; t; t = t->next) {
		t->tflags = 0;
		t->used = 0;
	}

	/* Suppress error messages: we may add new options if we
           demand-load a protocol. */
	opterr = 0;
	opts = xt_params->orig_opts;
	while ((cs.c = getopt_long(argc, argv,
	   "-:A:C:D:R:I:L::S::M:F::Z::N:X::E:P:Vh::o:p:s:d:j:i:fbvw::W::nt:m:xc:g:46",
					   opts, NULL)) != -1) {
		switch (cs.c) {
			/*
			 * Command selection
			 */
		case 'A':
			add_command(&command, CMD_APPEND, CMD_NONE,
				    cs.invert);
			chain = optarg;
			break;

		case 'C':
			add_command(&command, CMD_CHECK, CMD_NONE,
				    cs.invert);
			chain = optarg;
			break;

		case 'D':
			add_command(&command, CMD_DELETE, CMD_NONE,
				    cs.invert);
			chain = optarg;
			if (xs_has_arg(argc, argv)) {
				rulenum = parse_rulenumber(argv[optind++]);
				command = CMD_DELETE_NUM;
			}
			break;

		case 'R':
			add_command(&command, CMD_REPLACE, CMD_NONE,
				    cs.invert);
			chain = optarg;
			if (xs_has_arg(argc, argv))
				rulenum = parse_rulenumber(argv[optind++]);
			else
				xtables_error(PARAMETER_PROBLEM,
					   "-%c requires a rule number",
					   cmd2char(CMD_REPLACE));
			break;

		case 'I':
			add_command(&command, CMD_INSERT, CMD_NONE,
				    cs.invert);
			chain = optarg;
			if (xs_has_arg(argc, argv))
				rulenum = parse_rulenumber(argv[optind++]);
			else rulenum = 1;
			break;

		case 'L':
			add_command(&command, CMD_LIST,
				    CMD_ZERO | CMD_ZERO_NUM, cs.invert);
			if (optarg) chain = optarg;
			else if (xs_has_arg(argc, argv))
				chain = argv[optind++];
			if (xs_has_arg(argc, argv))
				rulenum = parse_rulenumber(argv[optind++]);
			break;

		case 'S':
			add_command(&command, CMD_LIST_RULES,
				    CMD_ZERO|CMD_ZERO_NUM, cs.invert);
			if (optarg) chain = optarg;
			else if (xs_has_arg(argc, argv))
				chain = argv[optind++];
			if (xs_has_arg(argc, argv))
				rulenum = parse_rulenumber(argv[optind++]);
			break;

		case 'F':
			add_command(&command, CMD_FLUSH, CMD_NONE,
				    cs.invert);
			if (optarg) chain = optarg;
			else if (xs_has_arg(argc, argv))
				chain = argv[optind++];
			break;

		case 'Z':
			add_command(&command, CMD_ZERO, CMD_LIST|CMD_LIST_RULES,
				    cs.invert);
			if (optarg) chain = optarg;
			else if (xs_has_arg(argc, argv))
				chain = argv[optind++];
			if (xs_has_arg(argc, argv)) {
				rulenum = parse_rulenumber(argv[optind++]);
				command = CMD_ZERO_NUM;
			}
			break;

		case 'N':
			parse_chain(optarg);
			add_command(&command, CMD_NEW_CHAIN, CMD_NONE,
				    cs.invert);
			chain = optarg;
			break;

		case 'X':
			add_command(&command, CMD_DELETE_CHAIN, CMD_NONE,
				    cs.invert);
			if (optarg) chain = optarg;
			else if (xs_has_arg(argc, argv))
				chain = argv[optind++];
			break;

		case 'E':
			add_command(&command, CMD_RENAME_CHAIN, CMD_NONE,
				    cs.invert);
			chain = optarg;
			if (xs_has_arg(argc, argv))
				newname = argv[optind++];
			else
				xtables_error(PARAMETER_PROBLEM,
					   "-%c requires old-chain-name and "
					   "new-chain-name",
					    cmd2char(CMD_RENAME_CHAIN));
			break;

		case 'P':
			add_command(&command, CMD_SET_POLICY, CMD_NONE,
				    cs.invert);
			chain = optarg;
			if (xs_has_arg(argc, argv))
				policy = argv[optind++];
			else
				xtables_error(PARAMETER_PROBLEM,
					   "-%c requires a chain and a policy",
					   cmd2char(CMD_SET_POLICY));
			break;

		case 'h':
			if (!optarg)
				optarg = argv[optind];

			/* iptables -p icmp -h */
			if (!cs.matches && cs.protocol)
				xtables_find_match(cs.protocol,
					XTF_TRY_LOAD, &cs.matches);

			exit_printhelp(cs.matches);

			/*
			 * Option selection
			 */
		case 'p':
			set_option(&cs.options, OPT_PROTOCOL, &cs.fw.ip.invflags,
				   cs.invert);

			/* Canonicalize into lower case */
			for (cs.protocol = optarg; *cs.protocol; cs.protocol++)
				*cs.protocol = tolower(*cs.protocol);

			cs.protocol = optarg;
			cs.fw.ip.proto = xtables_parse_protocol(cs.protocol);

			if (cs.fw.ip.proto == 0
			    && (cs.fw.ip.invflags & XT_INV_PROTO))
				xtables_error(PARAMETER_PROBLEM,
					   "rule would never match protocol");
			break;

		case 's':
			set_option(&cs.options, OPT_SOURCE, &cs.fw.ip.invflags,
				   cs.invert);
			shostnetworkmask = optarg;
			break;

		case 'd':
			set_option(&cs.options, OPT_DESTINATION, &cs.fw.ip.invflags,
				   cs.invert);
			dhostnetworkmask = optarg;
			break;

#ifdef IPT_F_GOTO
		case 'g':
			set_option(&cs.options, OPT_JUMP, &cs.fw.ip.invflags,
				   cs.invert);
			cs.fw.ip.flags |= IPT_F_GOTO;
			cs.jumpto = xt_parse_target(optarg);
			break;
#endif

		case 'j':
			set_option(&cs.options, OPT_JUMP, &cs.fw.ip.invflags,
				   cs.invert);
			command_jump(&cs, optarg);
			break;


		case 'i':
			if (*optarg == '\0')
				xtables_error(PARAMETER_PROBLEM,
					"Empty interface is likely to be "
					"undesired");
			set_option(&cs.options, OPT_VIANAMEIN, &cs.fw.ip.invflags,
				   cs.invert);
			xtables_parse_interface(optarg,
					cs.fw.ip.iniface,
					cs.fw.ip.iniface_mask);
			break;

		case 'o':
			if (*optarg == '\0')
				xtables_error(PARAMETER_PROBLEM,
					"Empty interface is likely to be "
					"undesired");
			set_option(&cs.options, OPT_VIANAMEOUT, &cs.fw.ip.invflags,
				   cs.invert);
			xtables_parse_interface(optarg,
					cs.fw.ip.outiface,
					cs.fw.ip.outiface_mask);
			break;

		case 'f':
			set_option(&cs.options, OPT_FRAGMENT, &cs.fw.ip.invflags,
				   cs.invert);
			cs.fw.ip.flags |= IPT_F_FRAG;
			break;

		case 'v':
			if (!verbose)
				set_option(&cs.options, OPT_VERBOSE,
					   &cs.fw.ip.invflags, cs.invert);
			verbose++;
			break;

		case 'w':
			if (restore) {
				xtables_error(PARAMETER_PROBLEM,
					      "You cannot use `-w' from "
					      "iptables-restore");
			}
			wait = parse_wait_time(argc, argv);
			break;

		case 'W':
			if (restore) {
				xtables_error(PARAMETER_PROBLEM,
					      "You cannot use `-W' from "
					      "iptables-restore");
			}
			parse_wait_interval(argc, argv, &wait_interval);
			wait_interval_set = true;
			break;

		case 'm':
			command_match(&cs);
			break;

		case 'n':
			set_option(&cs.options, OPT_NUMERIC, &cs.fw.ip.invflags,
				   cs.invert);
			break;

		case 't':
			if (cs.invert)
				xtables_error(PARAMETER_PROBLEM,
					   "unexpected ! flag before --table");
			if (restore && table_set)
				xtables_error(PARAMETER_PROBLEM,
					      "The -t option (seen in line %u) cannot be used in %s.\n",
					      line, xt_params->program_name);
			*table = optarg;
			table_set = true;
			break;

		case 'x':
			set_option(&cs.options, OPT_EXPANDED, &cs.fw.ip.invflags,
				   cs.invert);
			break;

		case 'V':
			if (cs.invert)
				printf("Not %s ;-)\n", prog_vers);
			else
				printf("%s v%s (legacy)\n",
				       prog_name, prog_vers);
			exit(0);

		case '0':
			set_option(&cs.options, OPT_LINENUMBERS, &cs.fw.ip.invflags,
				   cs.invert);
			break;

		case 'M':
			xtables_modprobe_program = optarg;
			break;

		case 'c':

			set_option(&cs.options, OPT_COUNTERS, &cs.fw.ip.invflags,
				   cs.invert);
			pcnt = optarg;
			bcnt = strchr(pcnt + 1, ',');
			if (bcnt)
			    bcnt++;
			if (!bcnt && xs_has_arg(argc, argv))
				bcnt = argv[optind++];
			if (!bcnt)
				xtables_error(PARAMETER_PROBLEM,
					"-%c requires packet and byte counter",
					opt2char(OPT_COUNTERS));

			if (sscanf(pcnt, "%llu", &cnt) != 1)
				xtables_error(PARAMETER_PROBLEM,
					"-%c packet counter not numeric",
					opt2char(OPT_COUNTERS));
			cs.fw.counters.pcnt = cnt;

			if (sscanf(bcnt, "%llu", &cnt) != 1)
				xtables_error(PARAMETER_PROBLEM,
					"-%c byte counter not numeric",
					opt2char(OPT_COUNTERS));
			cs.fw.counters.bcnt = cnt;
			break;

		case '4':
			/* This is indeed the IPv4 iptables */
			break;

		case '6':
			/* This is not the IPv6 ip6tables */
			if (line != -1)
				return 1; /* success: line ignored */
			fprintf(stderr, "This is the IPv4 version of iptables.\n");
			exit_tryhelp(2);

		case 1: /* non option */
			if (optarg[0] == '!' && optarg[1] == '\0') {
				if (cs.invert)
					xtables_error(PARAMETER_PROBLEM,
						   "multiple consecutive ! not"
						   " allowed");
				cs.invert = true;
				optarg[0] = '\0';
				continue;
			}
			fprintf(stderr, "Bad argument `%s'\n", optarg);
			exit_tryhelp(2);

		default:
			if (command_default(&cs, &iptables_globals) == 1)
				/* cf. ip6tables.c */
				continue;
			break;
		}
		cs.invert = false;
	}

	if (!wait && wait_interval_set)
		xtables_error(PARAMETER_PROBLEM,
			      "--wait-interval only makes sense with --wait\n");

	if (strcmp(*table, "nat") == 0 &&
	    ((policy != NULL && strcmp(policy, "DROP") == 0) ||
	    (cs.jumpto != NULL && strcmp(cs.jumpto, "DROP") == 0)))
		xtables_error(PARAMETER_PROBLEM,
			"\nThe \"nat\" table is not intended for filtering, "
		        "the use of DROP is therefore inhibited.\n\n");

	for (matchp = cs.matches; matchp; matchp = matchp->next)
		xtables_option_mfcall(matchp->match);
	if (cs.target != NULL)
		xtables_option_tfcall(cs.target);

	/* Fix me: must put inverse options checking here --MN */

	if (optind < argc)
		xtables_error(PARAMETER_PROBLEM,
			   "unknown arguments found on commandline");
	if (!command)
		xtables_error(PARAMETER_PROBLEM, "no command specified");
	if (cs.invert)
		xtables_error(PARAMETER_PROBLEM,
			   "nothing appropriate following !");

	if (command & (CMD_REPLACE | CMD_INSERT | CMD_DELETE | CMD_APPEND | CMD_CHECK)) {
		if (!(cs.options & OPT_DESTINATION))
			dhostnetworkmask = "0.0.0.0/0";
		if (!(cs.options & OPT_SOURCE))
			shostnetworkmask = "0.0.0.0/0";
	}

	if (shostnetworkmask)
		xtables_ipparse_multiple(shostnetworkmask, &saddrs,
					 &smasks, &nsaddrs);

	if (dhostnetworkmask)
		xtables_ipparse_multiple(dhostnetworkmask, &daddrs,
					 &dmasks, &ndaddrs);

	if ((nsaddrs > 1 || ndaddrs > 1) &&
	    (cs.fw.ip.invflags & (IPT_INV_SRCIP | IPT_INV_DSTIP)))
		xtables_error(PARAMETER_PROBLEM, "! not allowed with multiple"
			   " source or destination IP addresses");

	if (command == CMD_REPLACE && (nsaddrs != 1 || ndaddrs != 1))
		xtables_error(PARAMETER_PROBLEM, "Replacement rule does not "
			   "specify a unique address");

	generic_opt_check(command, cs.options);

	/* Attempt to acquire the xtables lock */
	if (!restore)
		xtables_lock_or_exit(wait, &wait_interval);

	/* only allocate handle if we weren't called with a handle */
	if (!*handle)
		*handle = iptc_init(*table);

	/* try to insmod the module if iptc_init failed */
	if (!*handle && xtables_load_ko(xtables_modprobe_program, false) != -1)
		*handle = iptc_init(*table);

	if (!*handle)
		xtables_error(VERSION_PROBLEM,
			   "can't initialize iptables table `%s': %s",
			   *table, iptc_strerror(errno));

	if (command == CMD_APPEND
	    || command == CMD_DELETE
	    || command == CMD_CHECK
	    || command == CMD_INSERT
	    || command == CMD_REPLACE) {
		if (strcmp(chain, "PREROUTING") == 0
		    || strcmp(chain, "INPUT") == 0) {
			/* -o not valid with incoming packets. */
			if (cs.options & OPT_VIANAMEOUT)
				xtables_error(PARAMETER_PROBLEM,
					   "Can't use -%c with %s\n",
					   opt2char(OPT_VIANAMEOUT),
					   chain);
		}

		if (strcmp(chain, "POSTROUTING") == 0
		    || strcmp(chain, "OUTPUT") == 0) {
			/* -i not valid with outgoing packets */
			if (cs.options & OPT_VIANAMEIN)
				xtables_error(PARAMETER_PROBLEM,
					   "Can't use -%c with %s\n",
					   opt2char(OPT_VIANAMEIN),
					   chain);
		}

		if (cs.target && iptc_is_chain(cs.jumpto, *handle)) {
			fprintf(stderr,
				"Warning: using chain %s, not extension\n",
				cs.jumpto);

			if (cs.target->t)
				free(cs.target->t);

			cs.target = NULL;
		}

		/* If they didn't specify a target, or it's a chain
		   name, use standard. */
		if (!cs.target
		    && (strlen(cs.jumpto) == 0
			|| iptc_is_chain(cs.jumpto, *handle))) {
			size_t size;

			cs.target = xtables_find_target(XT_STANDARD_TARGET,
					 XTF_LOAD_MUST_SUCCEED);

			size = sizeof(struct xt_entry_target)
				+ cs.target->size;
			cs.target->t = xtables_calloc(1, size);
			cs.target->t->u.target_size = size;
			strcpy(cs.target->t->u.user.name, cs.jumpto);
			if (!iptc_is_chain(cs.jumpto, *handle))
				cs.target->t->u.user.revision = cs.target->revision;
			xs_init_target(cs.target);
		}

		if (!cs.target) {
			/* It is no chain, and we can't load a plugin.
			 * We cannot know if the plugin is corrupt, non
			 * existent OR if the user just misspelled a
			 * chain.
			 */
#ifdef IPT_F_GOTO
			if (cs.fw.ip.flags & IPT_F_GOTO)
				xtables_error(PARAMETER_PROBLEM,
					   "goto '%s' is not a chain\n",
					   cs.jumpto);
#endif
			xtables_find_target(cs.jumpto, XTF_LOAD_MUST_SUCCEED);
		} else {
			e = generate_entry(&cs.fw, cs.matches, cs.target->t);
			free(cs.target->t);
		}
	}

	switch (command) {
	case CMD_APPEND:
		ret = append_entry(chain, e,
				   nsaddrs, saddrs, smasks,
				   ndaddrs, daddrs, dmasks,
				   cs.options&OPT_VERBOSE,
				   *handle);
		break;
	case CMD_DELETE:
		ret = delete_entry(chain, e,
				   nsaddrs, saddrs, smasks,
				   ndaddrs, daddrs, dmasks,
				   cs.options&OPT_VERBOSE,
				   *handle, cs.matches, cs.target);
		break;
	case CMD_DELETE_NUM:
		ret = iptc_delete_num_entry(chain, rulenum - 1, *handle);
		break;
	case CMD_CHECK:
		ret = check_entry(chain, e,
				   nsaddrs, saddrs, smasks,
				   ndaddrs, daddrs, dmasks,
				   cs.options&OPT_VERBOSE,
				   *handle, cs.matches, cs.target);
		break;
	case CMD_REPLACE:
		ret = replace_entry(chain, e, rulenum - 1,
				    saddrs, smasks, daddrs, dmasks,
				    cs.options&OPT_VERBOSE, *handle);
		break;
	case CMD_INSERT:
		ret = insert_entry(chain, e, rulenum - 1,
				   nsaddrs, saddrs, smasks,
				   ndaddrs, daddrs, dmasks,
				   cs.options&OPT_VERBOSE,
				   *handle);
		break;
	case CMD_FLUSH:
		ret = flush_entries4(chain, cs.options&OPT_VERBOSE, *handle);
		break;
	case CMD_ZERO:
		ret = zero_entries(chain, cs.options&OPT_VERBOSE, *handle);
		break;
	case CMD_ZERO_NUM:
		ret = iptc_zero_counter(chain, rulenum, *handle);
		break;
	case CMD_LIST:
	case CMD_LIST|CMD_ZERO:
	case CMD_LIST|CMD_ZERO_NUM:
		ret = list_entries(chain,
				   rulenum,
				   cs.options&OPT_VERBOSE,
				   cs.options&OPT_NUMERIC,
				   cs.options&OPT_EXPANDED,
				   cs.options&OPT_LINENUMBERS,
				   *handle);
		if (ret && (command & CMD_ZERO))
			ret = zero_entries(chain,
					   cs.options&OPT_VERBOSE, *handle);
		if (ret && (command & CMD_ZERO_NUM))
			ret = iptc_zero_counter(chain, rulenum, *handle);
		break;
	case CMD_LIST_RULES:
	case CMD_LIST_RULES|CMD_ZERO:
	case CMD_LIST_RULES|CMD_ZERO_NUM:
		ret = list_rules(chain,
				   rulenum,
				   cs.options&OPT_VERBOSE,
				   *handle);
		if (ret && (command & CMD_ZERO))
			ret = zero_entries(chain,
					   cs.options&OPT_VERBOSE, *handle);
		if (ret && (command & CMD_ZERO_NUM))
			ret = iptc_zero_counter(chain, rulenum, *handle);
		break;
	case CMD_NEW_CHAIN:
		ret = iptc_create_chain(chain, *handle);
		break;
	case CMD_DELETE_CHAIN:
		ret = delete_chain4(chain, cs.options&OPT_VERBOSE, *handle);
		break;
	case CMD_RENAME_CHAIN:
		ret = iptc_rename_chain(chain, newname,	*handle);
		break;
	case CMD_SET_POLICY:
		ret = iptc_set_policy(chain, policy, cs.options&OPT_COUNTERS ? &cs.fw.counters : NULL, *handle);
		break;
	default:
		/* We should never reach this... */
		exit_tryhelp(2);
	}

	if (verbose > 1)
		dump_entries(*handle);

	xtables_rule_matches_free(&cs.matches);

	if (e != NULL) {
		free(e);
		e = NULL;
	}

	free(saddrs);
	free(smasks);
	free(daddrs);
	free(dmasks);
	xtables_free_opts(1);

	return ret;
}
