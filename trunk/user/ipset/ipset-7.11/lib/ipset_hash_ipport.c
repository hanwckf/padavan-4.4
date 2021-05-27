/* Copyright 2007-2010 Jozsef Kadlecsik (kadlec@netfilter.org)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <libipset/data.h>			/* IPSET_OPT_* */
#include <libipset/parse.h>			/* parser functions */
#include <libipset/print.h>			/* printing functions */
#include <libipset/ipset.h>			/* ipset_port_usage */
#include <libipset/types.h>			/* prototypes */

/* SCTP and UDPLITE support */
static struct ipset_type ipset_hash_ipport1 = {
	.name = "hash:ip,port",
	.alias = { "ipporthash", NULL },
	.revision = 1,
	.family = NFPROTO_IPSET_IPV46,
	.dimension = IPSET_DIM_TWO,
	.elem = {
		[IPSET_DIM_ONE - 1] = {
			.parse = ipset_parse_ip4_single6,
			.print = ipset_print_ip,
			.opt = IPSET_OPT_IP
		},
		[IPSET_DIM_TWO - 1] = {
			.parse = ipset_parse_proto_port,
			.print = ipset_print_proto_port,
			.opt = IPSET_OPT_PORT
		},
	},
	.cmd = {
		[IPSET_CREATE] = {
			.args = {
				IPSET_ARG_FAMILY,
				/* Aliases */
				IPSET_ARG_INET,
				IPSET_ARG_INET6,
				IPSET_ARG_HASHSIZE,
				IPSET_ARG_MAXELEM,
				IPSET_ARG_TIMEOUT,
				/* Ignored options: backward compatibilty */
				IPSET_ARG_PROBES,
				IPSET_ARG_RESIZE,
				IPSET_ARG_IGNORED_FROM,
				IPSET_ARG_IGNORED_TO,
				IPSET_ARG_IGNORED_NETWORK,
				IPSET_ARG_NONE,
			},
			.need = 0,
			.full = 0,
			.help = "",
		},
		[IPSET_ADD] = {
			.args = {
				IPSET_ARG_TIMEOUT,
				IPSET_ARG_NONE,
			},
			.need = IPSET_FLAG(IPSET_OPT_IP)
				| IPSET_FLAG(IPSET_OPT_PROTO)
				| IPSET_FLAG(IPSET_OPT_PORT),
			.full = IPSET_FLAG(IPSET_OPT_IP)
				| IPSET_FLAG(IPSET_OPT_IP_TO)
				| IPSET_FLAG(IPSET_OPT_PROTO)
				| IPSET_FLAG(IPSET_OPT_PORT)
				| IPSET_FLAG(IPSET_OPT_PORT_TO),
			.help = "IP,[PROTO:]PORT",
		},
		[IPSET_DEL] = {
			.args = {
				IPSET_ARG_NONE,
			},
			.need = IPSET_FLAG(IPSET_OPT_IP)
				| IPSET_FLAG(IPSET_OPT_PROTO)
				| IPSET_FLAG(IPSET_OPT_PORT),
			.full = IPSET_FLAG(IPSET_OPT_IP)
				| IPSET_FLAG(IPSET_OPT_IP_TO)
				| IPSET_FLAG(IPSET_OPT_PROTO)
				| IPSET_FLAG(IPSET_OPT_PORT)
				| IPSET_FLAG(IPSET_OPT_PORT_TO),
			.help = "IP,[PROTO:]PORT",
		},
		[IPSET_TEST] = {
			.args = {
				IPSET_ARG_NONE,
			},
			.need = IPSET_FLAG(IPSET_OPT_IP)
				| IPSET_FLAG(IPSET_OPT_PROTO)
				| IPSET_FLAG(IPSET_OPT_PORT),
			.full = IPSET_FLAG(IPSET_OPT_IP)
				| IPSET_FLAG(IPSET_OPT_PROTO)
				| IPSET_FLAG(IPSET_OPT_PORT),
			.help = "IP,[PROTO:]PORT",
		},
	},
	.usage = "where depending on the INET family\n"
		 "      IP is a valid IPv4 or IPv6 address (or hostname).\n"
		 "      Adding/deleting multiple elements in IP/CIDR or FROM-TO form\n"
		 "      is supported for IPv4.\n"
		 "      Adding/deleting multiple elements with TCP/SCTP/UDP/UDPLITE\n"
		 "      port range is supported both for IPv4 and IPv6.",
	.usagefn = ipset_port_usage,
	.description = "SCTP and UDPLITE support",
};

/* counters support */
static struct ipset_type ipset_hash_ipport2 = {
	.name = "hash:ip,port",
	.alias = { "ipporthash", NULL },
	.revision = 2,
	.family = NFPROTO_IPSET_IPV46,
	.dimension = IPSET_DIM_TWO,
	.elem = {
		[IPSET_DIM_ONE - 1] = {
			.parse = ipset_parse_ip4_single6,
			.print = ipset_print_ip,
			.opt = IPSET_OPT_IP
		},
		[IPSET_DIM_TWO - 1] = {
			.parse = ipset_parse_proto_port,
			.print = ipset_print_proto_port,
			.opt = IPSET_OPT_PORT
		},
	},
	.cmd = {
		[IPSET_CREATE] = {
			.args = {
				IPSET_ARG_FAMILY,
				/* Aliases */
				IPSET_ARG_INET,
				IPSET_ARG_INET6,
				IPSET_ARG_HASHSIZE,
				IPSET_ARG_MAXELEM,
				IPSET_ARG_TIMEOUT,
				IPSET_ARG_COUNTERS,
				/* Ignored options: backward compatibilty */
				IPSET_ARG_PROBES,
				IPSET_ARG_RESIZE,
				IPSET_ARG_IGNORED_FROM,
				IPSET_ARG_IGNORED_TO,
				IPSET_ARG_IGNORED_NETWORK,
				IPSET_ARG_NONE,
			},
			.need = 0,
			.full = 0,
			.help = "",
		},
		[IPSET_ADD] = {
			.args = {
				IPSET_ARG_TIMEOUT,
				IPSET_ARG_PACKETS,
				IPSET_ARG_BYTES,
				IPSET_ARG_NONE,
			},
			.need = IPSET_FLAG(IPSET_OPT_IP)
				| IPSET_FLAG(IPSET_OPT_PROTO)
				| IPSET_FLAG(IPSET_OPT_PORT),
			.full = IPSET_FLAG(IPSET_OPT_IP)
				| IPSET_FLAG(IPSET_OPT_IP_TO)
				| IPSET_FLAG(IPSET_OPT_PROTO)
				| IPSET_FLAG(IPSET_OPT_PORT)
				| IPSET_FLAG(IPSET_OPT_PORT_TO),
			.help = "IP,[PROTO:]PORT",
		},
		[IPSET_DEL] = {
			.args = {
				IPSET_ARG_NONE,
			},
			.need = IPSET_FLAG(IPSET_OPT_IP)
				| IPSET_FLAG(IPSET_OPT_PROTO)
				| IPSET_FLAG(IPSET_OPT_PORT),
			.full = IPSET_FLAG(IPSET_OPT_IP)
				| IPSET_FLAG(IPSET_OPT_IP_TO)
				| IPSET_FLAG(IPSET_OPT_PROTO)
				| IPSET_FLAG(IPSET_OPT_PORT)
				| IPSET_FLAG(IPSET_OPT_PORT_TO),
			.help = "IP,[PROTO:]PORT",
		},
		[IPSET_TEST] = {
			.args = {
				IPSET_ARG_NONE,
			},
			.need = IPSET_FLAG(IPSET_OPT_IP)
				| IPSET_FLAG(IPSET_OPT_PROTO)
				| IPSET_FLAG(IPSET_OPT_PORT),
			.full = IPSET_FLAG(IPSET_OPT_IP)
				| IPSET_FLAG(IPSET_OPT_PROTO)
				| IPSET_FLAG(IPSET_OPT_PORT),
			.help = "IP,[PROTO:]PORT",
		},
	},
	.usage = "where depending on the INET family\n"
		 "      IP is a valid IPv4 or IPv6 address (or hostname).\n"
		 "      Adding/deleting multiple elements in IP/CIDR or FROM-TO form\n"
		 "      is supported for IPv4.\n"
		 "      Adding/deleting multiple elements with TCP/SCTP/UDP/UDPLITE\n"
		 "      port range is supported both for IPv4 and IPv6.",
	.usagefn = ipset_port_usage,
	.description = "counters support",
};

/* comment support */
static struct ipset_type ipset_hash_ipport3 = {
	.name = "hash:ip,port",
	.alias = { "ipporthash", NULL },
	.revision = 3,
	.family = NFPROTO_IPSET_IPV46,
	.dimension = IPSET_DIM_TWO,
	.elem = {
		[IPSET_DIM_ONE - 1] = {
			.parse = ipset_parse_ip4_single6,
			.print = ipset_print_ip,
			.opt = IPSET_OPT_IP
		},
		[IPSET_DIM_TWO - 1] = {
			.parse = ipset_parse_proto_port,
			.print = ipset_print_proto_port,
			.opt = IPSET_OPT_PORT
		},
	},
	.cmd = {
		[IPSET_CREATE] = {
			.args = {
				IPSET_ARG_FAMILY,
				/* Aliases */
				IPSET_ARG_INET,
				IPSET_ARG_INET6,
				IPSET_ARG_HASHSIZE,
				IPSET_ARG_MAXELEM,
				IPSET_ARG_TIMEOUT,
				IPSET_ARG_COUNTERS,
				IPSET_ARG_COMMENT,
				/* Ignored options: backward compatibilty */
				IPSET_ARG_PROBES,
				IPSET_ARG_RESIZE,
				IPSET_ARG_IGNORED_FROM,
				IPSET_ARG_IGNORED_TO,
				IPSET_ARG_IGNORED_NETWORK,
				IPSET_ARG_NONE,
			},
			.need = 0,
			.full = 0,
			.help = "",
		},
		[IPSET_ADD] = {
			.args = {
				IPSET_ARG_TIMEOUT,
				IPSET_ARG_PACKETS,
				IPSET_ARG_BYTES,
				IPSET_ARG_ADT_COMMENT,
				IPSET_ARG_NONE,
			},
			.need = IPSET_FLAG(IPSET_OPT_IP)
				| IPSET_FLAG(IPSET_OPT_PROTO)
				| IPSET_FLAG(IPSET_OPT_PORT),
			.full = IPSET_FLAG(IPSET_OPT_IP)
				| IPSET_FLAG(IPSET_OPT_IP_TO)
				| IPSET_FLAG(IPSET_OPT_PROTO)
				| IPSET_FLAG(IPSET_OPT_PORT)
				| IPSET_FLAG(IPSET_OPT_PORT_TO),
			.help = "IP,[PROTO:]PORT",
		},
		[IPSET_DEL] = {
			.args = {
				IPSET_ARG_NONE,
			},
			.need = IPSET_FLAG(IPSET_OPT_IP)
				| IPSET_FLAG(IPSET_OPT_PROTO)
				| IPSET_FLAG(IPSET_OPT_PORT),
			.full = IPSET_FLAG(IPSET_OPT_IP)
				| IPSET_FLAG(IPSET_OPT_IP_TO)
				| IPSET_FLAG(IPSET_OPT_PROTO)
				| IPSET_FLAG(IPSET_OPT_PORT)
				| IPSET_FLAG(IPSET_OPT_PORT_TO),
			.help = "IP,[PROTO:]PORT",
		},
		[IPSET_TEST] = {
			.args = {
				IPSET_ARG_NONE,
			},
			.need = IPSET_FLAG(IPSET_OPT_IP)
				| IPSET_FLAG(IPSET_OPT_PROTO)
				| IPSET_FLAG(IPSET_OPT_PORT),
			.full = IPSET_FLAG(IPSET_OPT_IP)
				| IPSET_FLAG(IPSET_OPT_PROTO)
				| IPSET_FLAG(IPSET_OPT_PORT),
			.help = "IP,[PROTO:]PORT",
		},
	},
	.usage = "where depending on the INET family\n"
		 "      IP is a valid IPv4 or IPv6 address (or hostname).\n"
		 "      Adding/deleting multiple elements in IP/CIDR or FROM-TO form\n"
		 "      is supported for IPv4.\n"
		 "      Adding/deleting multiple elements with TCP/SCTP/UDP/UDPLITE\n"
		 "      port range is supported both for IPv4 and IPv6.",
	.usagefn = ipset_port_usage,
	.description = "comment support",
};

/* forceadd support */
static struct ipset_type ipset_hash_ipport4 = {
	.name = "hash:ip,port",
	.alias = { "ipporthash", NULL },
	.revision = 4,
	.family = NFPROTO_IPSET_IPV46,
	.dimension = IPSET_DIM_TWO,
	.elem = {
		[IPSET_DIM_ONE - 1] = {
			.parse = ipset_parse_ip4_single6,
			.print = ipset_print_ip,
			.opt = IPSET_OPT_IP
		},
		[IPSET_DIM_TWO - 1] = {
			.parse = ipset_parse_proto_port,
			.print = ipset_print_proto_port,
			.opt = IPSET_OPT_PORT
		},
	},
	.cmd = {
		[IPSET_CREATE] = {
			.args = {
				IPSET_ARG_FAMILY,
				/* Aliases */
				IPSET_ARG_INET,
				IPSET_ARG_INET6,
				IPSET_ARG_HASHSIZE,
				IPSET_ARG_MAXELEM,
				IPSET_ARG_TIMEOUT,
				IPSET_ARG_COUNTERS,
				IPSET_ARG_COMMENT,
				IPSET_ARG_FORCEADD,
				/* Ignored options: backward compatibilty */
				IPSET_ARG_PROBES,
				IPSET_ARG_RESIZE,
				IPSET_ARG_IGNORED_FROM,
				IPSET_ARG_IGNORED_TO,
				IPSET_ARG_IGNORED_NETWORK,
				IPSET_ARG_NONE,
			},
			.need = 0,
			.full = 0,
			.help = "",
		},
		[IPSET_ADD] = {
			.args = {
				IPSET_ARG_TIMEOUT,
				IPSET_ARG_PACKETS,
				IPSET_ARG_BYTES,
				IPSET_ARG_ADT_COMMENT,
				IPSET_ARG_NONE,
			},
			.need = IPSET_FLAG(IPSET_OPT_IP)
				| IPSET_FLAG(IPSET_OPT_PROTO)
				| IPSET_FLAG(IPSET_OPT_PORT),
			.full = IPSET_FLAG(IPSET_OPT_IP)
				| IPSET_FLAG(IPSET_OPT_IP_TO)
				| IPSET_FLAG(IPSET_OPT_PROTO)
				| IPSET_FLAG(IPSET_OPT_PORT)
				| IPSET_FLAG(IPSET_OPT_PORT_TO),
			.help = "IP,[PROTO:]PORT",
		},
		[IPSET_DEL] = {
			.args = {
				IPSET_ARG_NONE,
			},
			.need = IPSET_FLAG(IPSET_OPT_IP)
				| IPSET_FLAG(IPSET_OPT_PROTO)
				| IPSET_FLAG(IPSET_OPT_PORT),
			.full = IPSET_FLAG(IPSET_OPT_IP)
				| IPSET_FLAG(IPSET_OPT_IP_TO)
				| IPSET_FLAG(IPSET_OPT_PROTO)
				| IPSET_FLAG(IPSET_OPT_PORT)
				| IPSET_FLAG(IPSET_OPT_PORT_TO),
			.help = "IP,[PROTO:]PORT",
		},
		[IPSET_TEST] = {
			.args = {
				IPSET_ARG_NONE,
			},
			.need = IPSET_FLAG(IPSET_OPT_IP)
				| IPSET_FLAG(IPSET_OPT_PROTO)
				| IPSET_FLAG(IPSET_OPT_PORT),
			.full = IPSET_FLAG(IPSET_OPT_IP)
				| IPSET_FLAG(IPSET_OPT_PROTO)
				| IPSET_FLAG(IPSET_OPT_PORT),
			.help = "IP,[PROTO:]PORT",
		},
	},
	.usage = "where depending on the INET family\n"
		 "      IP is a valid IPv4 or IPv6 address (or hostname).\n"
		 "      Adding/deleting multiple elements in IP/CIDR or FROM-TO form\n"
		 "      is supported for IPv4.\n"
		 "      Adding/deleting multiple elements with TCP/SCTP/UDP/UDPLITE\n"
		 "      port range is supported both for IPv4 and IPv6.",
	.usagefn = ipset_port_usage,
	.description = "forceadd support",
};

/* skbinfo support */
static struct ipset_type ipset_hash_ipport5 = {
	.name = "hash:ip,port",
	.alias = { "ipporthash", NULL },
	.revision = 5,
	.family = NFPROTO_IPSET_IPV46,
	.dimension = IPSET_DIM_TWO,
	.elem = {
		[IPSET_DIM_ONE - 1] = {
			.parse = ipset_parse_ip4_single6,
			.print = ipset_print_ip,
			.opt = IPSET_OPT_IP
		},
		[IPSET_DIM_TWO - 1] = {
			.parse = ipset_parse_proto_port,
			.print = ipset_print_proto_port,
			.opt = IPSET_OPT_PORT
		},
	},
	.cmd = {
		[IPSET_CREATE] = {
			.args = {
				IPSET_ARG_FAMILY,
				/* Aliases */
				IPSET_ARG_INET,
				IPSET_ARG_INET6,
				IPSET_ARG_HASHSIZE,
				IPSET_ARG_MAXELEM,
				IPSET_ARG_TIMEOUT,
				IPSET_ARG_COUNTERS,
				IPSET_ARG_COMMENT,
				IPSET_ARG_FORCEADD,
				IPSET_ARG_SKBINFO,
				/* Ignored options: backward compatibilty */
				IPSET_ARG_PROBES,
				IPSET_ARG_RESIZE,
				IPSET_ARG_IGNORED_FROM,
				IPSET_ARG_IGNORED_TO,
				IPSET_ARG_IGNORED_NETWORK,
				IPSET_ARG_NONE,
			},
			.need = 0,
			.full = 0,
			.help = "",
		},
		[IPSET_ADD] = {
			.args = {
				IPSET_ARG_TIMEOUT,
				IPSET_ARG_PACKETS,
				IPSET_ARG_BYTES,
				IPSET_ARG_ADT_COMMENT,
				IPSET_ARG_SKBMARK,
				IPSET_ARG_SKBPRIO,
				IPSET_ARG_SKBQUEUE,
				IPSET_ARG_NONE,
			},
			.need = IPSET_FLAG(IPSET_OPT_IP)
				| IPSET_FLAG(IPSET_OPT_PROTO)
				| IPSET_FLAG(IPSET_OPT_PORT),
			.full = IPSET_FLAG(IPSET_OPT_IP)
				| IPSET_FLAG(IPSET_OPT_IP_TO)
				| IPSET_FLAG(IPSET_OPT_PROTO)
				| IPSET_FLAG(IPSET_OPT_PORT)
				| IPSET_FLAG(IPSET_OPT_PORT_TO),
			.help = "IP,[PROTO:]PORT",
		},
		[IPSET_DEL] = {
			.args = {
				IPSET_ARG_NONE,
			},
			.need = IPSET_FLAG(IPSET_OPT_IP)
				| IPSET_FLAG(IPSET_OPT_PROTO)
				| IPSET_FLAG(IPSET_OPT_PORT),
			.full = IPSET_FLAG(IPSET_OPT_IP)
				| IPSET_FLAG(IPSET_OPT_IP_TO)
				| IPSET_FLAG(IPSET_OPT_PROTO)
				| IPSET_FLAG(IPSET_OPT_PORT)
				| IPSET_FLAG(IPSET_OPT_PORT_TO),
			.help = "IP,[PROTO:]PORT",
		},
		[IPSET_TEST] = {
			.args = {
				IPSET_ARG_NONE,
			},
			.need = IPSET_FLAG(IPSET_OPT_IP)
				| IPSET_FLAG(IPSET_OPT_PROTO)
				| IPSET_FLAG(IPSET_OPT_PORT),
			.full = IPSET_FLAG(IPSET_OPT_IP)
				| IPSET_FLAG(IPSET_OPT_PROTO)
				| IPSET_FLAG(IPSET_OPT_PORT),
			.help = "IP,[PROTO:]PORT",
		},
	},
	.usage = "where depending on the INET family\n"
		 "      IP is a valid IPv4 or IPv6 address (or hostname).\n"
		 "      Adding/deleting multiple elements in IP/CIDR or FROM-TO form\n"
		 "      is supported for IPv4.\n"
		 "      Adding/deleting multiple elements with TCP/SCTP/UDP/UDPLITE\n"
		 "      port range is supported both for IPv4 and IPv6.",
	.usagefn = ipset_port_usage,
	.description = "skbinfo support",
};

/* bucketsize support */
static struct ipset_type ipset_hash_ipport6 = {
	.name = "hash:ip,port",
	.alias = { "ipporthash", NULL },
	.revision = 6,
	.family = NFPROTO_IPSET_IPV46,
	.dimension = IPSET_DIM_TWO,
	.elem = {
		[IPSET_DIM_ONE - 1] = {
			.parse = ipset_parse_ip4_single6,
			.print = ipset_print_ip,
			.opt = IPSET_OPT_IP
		},
		[IPSET_DIM_TWO - 1] = {
			.parse = ipset_parse_proto_port,
			.print = ipset_print_proto_port,
			.opt = IPSET_OPT_PORT
		},
	},
	.cmd = {
		[IPSET_CREATE] = {
			.args = {
				IPSET_ARG_FAMILY,
				/* Aliases */
				IPSET_ARG_INET,
				IPSET_ARG_INET6,
				IPSET_ARG_HASHSIZE,
				IPSET_ARG_MAXELEM,
				IPSET_ARG_TIMEOUT,
				IPSET_ARG_COUNTERS,
				IPSET_ARG_COMMENT,
				IPSET_ARG_FORCEADD,
				IPSET_ARG_SKBINFO,
				IPSET_ARG_BUCKETSIZE,
				IPSET_ARG_INITVAL,
				/* Ignored options: backward compatibilty */
				IPSET_ARG_PROBES,
				IPSET_ARG_RESIZE,
				IPSET_ARG_IGNORED_FROM,
				IPSET_ARG_IGNORED_TO,
				IPSET_ARG_IGNORED_NETWORK,
				IPSET_ARG_NONE,
			},
			.need = 0,
			.full = 0,
			.help = "",
		},
		[IPSET_ADD] = {
			.args = {
				IPSET_ARG_TIMEOUT,
				IPSET_ARG_PACKETS,
				IPSET_ARG_BYTES,
				IPSET_ARG_ADT_COMMENT,
				IPSET_ARG_SKBMARK,
				IPSET_ARG_SKBPRIO,
				IPSET_ARG_SKBQUEUE,
				IPSET_ARG_NONE,
			},
			.need = IPSET_FLAG(IPSET_OPT_IP)
				| IPSET_FLAG(IPSET_OPT_PROTO)
				| IPSET_FLAG(IPSET_OPT_PORT),
			.full = IPSET_FLAG(IPSET_OPT_IP)
				| IPSET_FLAG(IPSET_OPT_IP_TO)
				| IPSET_FLAG(IPSET_OPT_PROTO)
				| IPSET_FLAG(IPSET_OPT_PORT)
				| IPSET_FLAG(IPSET_OPT_PORT_TO),
			.help = "IP,[PROTO:]PORT",
		},
		[IPSET_DEL] = {
			.args = {
				IPSET_ARG_NONE,
			},
			.need = IPSET_FLAG(IPSET_OPT_IP)
				| IPSET_FLAG(IPSET_OPT_PROTO)
				| IPSET_FLAG(IPSET_OPT_PORT),
			.full = IPSET_FLAG(IPSET_OPT_IP)
				| IPSET_FLAG(IPSET_OPT_IP_TO)
				| IPSET_FLAG(IPSET_OPT_PROTO)
				| IPSET_FLAG(IPSET_OPT_PORT)
				| IPSET_FLAG(IPSET_OPT_PORT_TO),
			.help = "IP,[PROTO:]PORT",
		},
		[IPSET_TEST] = {
			.args = {
				IPSET_ARG_NONE,
			},
			.need = IPSET_FLAG(IPSET_OPT_IP)
				| IPSET_FLAG(IPSET_OPT_PROTO)
				| IPSET_FLAG(IPSET_OPT_PORT),
			.full = IPSET_FLAG(IPSET_OPT_IP)
				| IPSET_FLAG(IPSET_OPT_PROTO)
				| IPSET_FLAG(IPSET_OPT_PORT),
			.help = "IP,[PROTO:]PORT",
		},
	},
	.usage = "where depending on the INET family\n"
		 "      IP is a valid IPv4 or IPv6 address (or hostname).\n"
		 "      Adding/deleting multiple elements in IP/CIDR or FROM-TO form\n"
		 "      is supported for IPv4.\n"
		 "      Adding/deleting multiple elements with TCP/SCTP/UDP/UDPLITE\n"
		 "      port range is supported both for IPv4 and IPv6.",
	.usagefn = ipset_port_usage,
	.description = "bucketsize, initval support",
};

void _init(void);
void _init(void)
{
	ipset_type_add(&ipset_hash_ipport1);
	ipset_type_add(&ipset_hash_ipport2);
	ipset_type_add(&ipset_hash_ipport3);
	ipset_type_add(&ipset_hash_ipport4);
	ipset_type_add(&ipset_hash_ipport5);
	ipset_type_add(&ipset_hash_ipport6);
}
