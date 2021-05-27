#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <xtables.h>
#include <limits.h> /* INT_MAX in ip_tables.h */
#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/netfilter/nf_nat.h>

enum {
	O_TO_PORTS = 0,
	O_RANDOM,
	O_RANDOM_FULLY,
};

static void MASQUERADE_help(void)
{
	printf(
"MASQUERADE target options:\n"
" --to-ports <port>[-<port>]\n"
"				Port (range) to map to.\n"
" --random\n"
"				Randomize source port.\n"
" --random-fully\n"
"				Fully randomize source port.\n");
}

static const struct xt_option_entry MASQUERADE_opts[] = {
	{.name = "to-ports", .id = O_TO_PORTS, .type = XTTYPE_STRING},
	{.name = "random", .id = O_RANDOM, .type = XTTYPE_NONE},
	{.name = "random-fully", .id = O_RANDOM_FULLY, .type = XTTYPE_NONE},
	XTOPT_TABLEEND,
};

static void MASQUERADE_init(struct xt_entry_target *t)
{
	struct nf_nat_ipv4_multi_range_compat *mr = (struct nf_nat_ipv4_multi_range_compat *)t->data;

	/* Actually, it's 0, but it's ignored at the moment. */
	mr->rangesize = 1;
}

/* Parses ports */
static void
parse_ports(const char *arg, struct nf_nat_ipv4_multi_range_compat *mr)
{
	char *end;
	unsigned int port, maxport;

	mr->range[0].flags |= NF_NAT_RANGE_PROTO_SPECIFIED;

	if (!xtables_strtoui(arg, &end, &port, 0, UINT16_MAX))
		xtables_param_act(XTF_BAD_VALUE, "MASQUERADE", "--to-ports", arg);

	switch (*end) {
	case '\0':
		mr->range[0].min.tcp.port
			= mr->range[0].max.tcp.port
			= htons(port);
		return;
	case '-':
		if (!xtables_strtoui(end + 1, NULL, &maxport, 0, UINT16_MAX))
			break;

		if (maxport < port)
			break;

		mr->range[0].min.tcp.port = htons(port);
		mr->range[0].max.tcp.port = htons(maxport);
		return;
	default:
		break;
	}
	xtables_param_act(XTF_BAD_VALUE, "MASQUERADE", "--to-ports", arg);
}

static void MASQUERADE_parse(struct xt_option_call *cb)
{
	const struct ipt_entry *entry = cb->xt_entry;
	int portok;
	struct nf_nat_ipv4_multi_range_compat *mr = cb->data;

	if (entry->ip.proto == IPPROTO_TCP
	    || entry->ip.proto == IPPROTO_UDP
	    || entry->ip.proto == IPPROTO_SCTP
	    || entry->ip.proto == IPPROTO_DCCP
	    || entry->ip.proto == IPPROTO_ICMP)
		portok = 1;
	else
		portok = 0;

	xtables_option_parse(cb);
	switch (cb->entry->id) {
	case O_TO_PORTS:
		if (!portok)
			xtables_error(PARAMETER_PROBLEM,
				   "Need TCP, UDP, SCTP or DCCP with port specification");
		parse_ports(cb->arg, mr);
		break;
	case O_RANDOM:
		mr->range[0].flags |=  NF_NAT_RANGE_PROTO_RANDOM;
		break;
	case O_RANDOM_FULLY:
		mr->range[0].flags |=  NF_NAT_RANGE_PROTO_RANDOM_FULLY;
		break;
	}
}

static void
MASQUERADE_print(const void *ip, const struct xt_entry_target *target,
                 int numeric)
{
	const struct nf_nat_ipv4_multi_range_compat *mr = (const void *)target->data;
	const struct nf_nat_ipv4_range *r = &mr->range[0];

	if (r->flags & NF_NAT_RANGE_PROTO_SPECIFIED) {
		printf(" masq ports: ");
		printf("%hu", ntohs(r->min.tcp.port));
		if (r->max.tcp.port != r->min.tcp.port)
			printf("-%hu", ntohs(r->max.tcp.port));
	}

	if (r->flags & NF_NAT_RANGE_PROTO_RANDOM)
		printf(" random");

	if (r->flags & NF_NAT_RANGE_PROTO_RANDOM_FULLY)
		printf(" random-fully");
}

static void
MASQUERADE_save(const void *ip, const struct xt_entry_target *target)
{
	const struct nf_nat_ipv4_multi_range_compat *mr = (const void *)target->data;
	const struct nf_nat_ipv4_range *r = &mr->range[0];

	if (r->flags & NF_NAT_RANGE_PROTO_SPECIFIED) {
		printf(" --to-ports %hu", ntohs(r->min.tcp.port));
		if (r->max.tcp.port != r->min.tcp.port)
			printf("-%hu", ntohs(r->max.tcp.port));
	}

	if (r->flags & NF_NAT_RANGE_PROTO_RANDOM)
		printf(" --random");

	if (r->flags & NF_NAT_RANGE_PROTO_RANDOM_FULLY)
		printf(" --random-fully");
}

static int MASQUERADE_xlate(struct xt_xlate *xl,
			    const struct xt_xlate_tg_params *params)
{
	const struct nf_nat_ipv4_multi_range_compat *mr =
		(const void *)params->target->data;
	const struct nf_nat_ipv4_range *r = &mr->range[0];

	xt_xlate_add(xl, "masquerade");

	if (r->flags & NF_NAT_RANGE_PROTO_SPECIFIED) {
		xt_xlate_add(xl, " to :%hu", ntohs(r->min.tcp.port));
		if (r->max.tcp.port != r->min.tcp.port)
			xt_xlate_add(xl, "-%hu", ntohs(r->max.tcp.port));
        }

	xt_xlate_add(xl, " ");
	if (r->flags & NF_NAT_RANGE_PROTO_RANDOM)
		xt_xlate_add(xl, "random ");

	return 1;
}

static struct xtables_target masquerade_tg_reg = {
	.name		= "MASQUERADE",
	.version	= XTABLES_VERSION,
	.family		= NFPROTO_IPV4,
	.size		= XT_ALIGN(sizeof(struct nf_nat_ipv4_multi_range_compat)),
	.userspacesize	= XT_ALIGN(sizeof(struct nf_nat_ipv4_multi_range_compat)),
	.help		= MASQUERADE_help,
	.init		= MASQUERADE_init,
	.x6_parse	= MASQUERADE_parse,
	.print		= MASQUERADE_print,
	.save		= MASQUERADE_save,
	.x6_options	= MASQUERADE_opts,
	.xlate		= MASQUERADE_xlate,
};

void _init(void)
{
	xtables_register_target(&masquerade_tg_reg);
}
