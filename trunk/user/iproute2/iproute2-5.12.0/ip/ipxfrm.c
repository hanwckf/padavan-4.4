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
 * based on ip.c, iproute.c
 */
/*
 * Authors:
 *	Masahide NAKAMURA @USAGI
 */

#include <alloca.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <netdb.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/udp.h>

#include "utils.h"
#include "xfrm.h"
#include "ip_common.h"

#define STRBUF_SIZE	(128)

struct xfrm_filter filter;

static void usage(void) __attribute__((noreturn));

static void usage(void)
{
	fprintf(stderr,
		"Usage: ip xfrm XFRM-OBJECT { COMMAND | help }\n"
		"where  XFRM-OBJECT := state | policy | monitor\n");
	exit(-1);
}

/* This is based on utils.c(inet_addr_match) */
int xfrm_addr_match(xfrm_address_t *x1, xfrm_address_t *x2, int bits)
{
	__u32 *a1 = (__u32 *)x1;
	__u32 *a2 = (__u32 *)x2;
	int words = bits >> 0x05;

	bits &= 0x1f;

	if (words)
		if (memcmp(a1, a2, words << 2))
			return -1;

	if (bits) {
		__u32 w1, w2;
		__u32 mask;

		w1 = a1[words];
		w2 = a2[words];

		mask = htonl((0xffffffff) << (0x20 - bits));

		if ((w1 ^ w2) & mask)
			return 1;
	}

	return 0;
}

int xfrm_xfrmproto_is_ipsec(__u8 proto)
{
	return (proto ==  IPPROTO_ESP ||
		proto ==  IPPROTO_AH  ||
		proto ==  IPPROTO_COMP);
}

int xfrm_xfrmproto_is_ro(__u8 proto)
{
	return (proto ==  IPPROTO_ROUTING ||
		proto ==  IPPROTO_DSTOPTS);
}

struct typeent {
	const char *t_name;
	int t_type;
};

static const struct typeent xfrmproto_types[] = {
	{ "esp", IPPROTO_ESP }, { "ah", IPPROTO_AH }, { "comp", IPPROTO_COMP },
	{ "route2", IPPROTO_ROUTING }, { "hao", IPPROTO_DSTOPTS },
	{ "ipsec-any", IPSEC_PROTO_ANY },
	{ NULL, -1 }
};

int xfrm_xfrmproto_getbyname(char *name)
{
	int i;

	for (i = 0; ; i++) {
		const struct typeent *t = &xfrmproto_types[i];

		if (!t->t_name || t->t_type == -1)
			break;

		if (strcmp(t->t_name, name) == 0)
			return t->t_type;
	}

	return -1;
}

const char *strxf_xfrmproto(__u8 proto)
{
	static char str[16];
	int i;

	for (i = 0; ; i++) {
		const struct typeent *t = &xfrmproto_types[i];

		if (!t->t_name || t->t_type == -1)
			break;

		if (t->t_type == proto)
			return t->t_name;
	}

	sprintf(str, "%u", proto);
	return str;
}

static const struct typeent algo_types[] = {
	{ "enc", XFRMA_ALG_CRYPT }, { "auth", XFRMA_ALG_AUTH },
	{ "comp", XFRMA_ALG_COMP }, { "aead", XFRMA_ALG_AEAD },
	{ "auth-trunc", XFRMA_ALG_AUTH_TRUNC },
	{ NULL, -1 }
};

int xfrm_algotype_getbyname(char *name)
{
	int i;

	for (i = 0; ; i++) {
		const struct typeent *t = &algo_types[i];

		if (!t->t_name || t->t_type == -1)
			break;

		if (strcmp(t->t_name, name) == 0)
			return t->t_type;
	}

	return -1;
}

const char *strxf_algotype(int type)
{
	static char str[32];
	int i;

	for (i = 0; ; i++) {
		const struct typeent *t = &algo_types[i];

		if (!t->t_name || t->t_type == -1)
			break;

		if (t->t_type == type)
			return t->t_name;
	}

	sprintf(str, "%d", type);
	return str;
}

static const char *strxf_mask8(__u8 mask)
{
	static char str[16];
	const int sn = sizeof(mask) * 8 - 1;
	__u8 b;
	int i = 0;

	for (b = (1 << sn); b > 0; b >>= 1)
		str[i++] = ((b & mask) ? '1' : '0');
	str[i] = '\0';

	return str;
}

const char *strxf_mask32(__u32 mask)
{
	static char str[16];

	sprintf(str, "%.8x", mask);

	return str;
}

static const char *strxf_share(__u8 share)
{
	static char str[32];

	switch (share) {
	case XFRM_SHARE_ANY:
		strcpy(str, "any");
		break;
	case XFRM_SHARE_SESSION:
		strcpy(str, "session");
		break;
	case XFRM_SHARE_USER:
		strcpy(str, "user");
		break;
	case XFRM_SHARE_UNIQUE:
		strcpy(str, "unique");
		break;
	default:
		sprintf(str, "%u", share);
		break;
	}

	return str;
}

const char *strxf_proto(__u8 proto)
{
	static char buf[32];
	struct protoent *pp;
	const char *p;

	pp = getprotobynumber(proto);
	if (pp)
		p = pp->p_name;
	else {
		sprintf(buf, "%u", proto);
		p = buf;
	}

	return p;
}

const char *strxf_ptype(__u8 ptype)
{
	static char str[16];

	switch (ptype) {
	case XFRM_POLICY_TYPE_MAIN:
		strcpy(str, "main");
		break;
	case XFRM_POLICY_TYPE_SUB:
		strcpy(str, "sub");
		break;
	default:
		sprintf(str, "%u", ptype);
		break;
	}

	return str;
}

static void xfrm_id_info_print(xfrm_address_t *saddr, struct xfrm_id *id,
			__u8 mode, __u32 reqid, __u16 family, int force_spi,
			FILE *fp, const char *prefix, const char *title)
{
	if (title)
		fputs(title, fp);

	fprintf(fp, "src %s ", rt_addr_n2a(family, sizeof(*saddr), saddr));
	fprintf(fp, "dst %s", rt_addr_n2a(family, sizeof(id->daddr), &id->daddr));
	fprintf(fp, "%s", _SL_);

	if (prefix)
		fputs(prefix, fp);
	fprintf(fp, "\t");

	fprintf(fp, "proto %s ", strxf_xfrmproto(id->proto));

	if (show_stats > 0 || force_spi || id->spi) {
		__u32 spi = ntohl(id->spi);

		fprintf(fp, "spi 0x%08x", spi);
		if (show_stats > 0)
			fprintf(fp, "(%u)", spi);
		fprintf(fp, " ");
	}

	fprintf(fp, "reqid %u", reqid);
	if (show_stats > 0)
		fprintf(fp, "(0x%08x)", reqid);
	fprintf(fp, " ");

	fprintf(fp, "mode ");
	switch (mode) {
	case XFRM_MODE_TRANSPORT:
		fprintf(fp, "transport");
		break;
	case XFRM_MODE_TUNNEL:
		fprintf(fp, "tunnel");
		break;
	case XFRM_MODE_ROUTEOPTIMIZATION:
		fprintf(fp, "ro");
		break;
	case XFRM_MODE_IN_TRIGGER:
		fprintf(fp, "in_trigger");
		break;
	case XFRM_MODE_BEET:
		fprintf(fp, "beet");
		break;
	default:
		fprintf(fp, "%u", mode);
		break;
	}
	fprintf(fp, "%s", _SL_);
}

static const char *strxf_limit(__u64 limit)
{
	static char str[32];

	if (limit == XFRM_INF)
		strcpy(str, "(INF)");
	else
		sprintf(str, "%llu", (unsigned long long) limit);

	return str;
}

static void xfrm_stats_print(struct xfrm_stats *s, FILE *fp,
			     const char *prefix)
{
	if (prefix)
		fputs(prefix, fp);
	fprintf(fp, "stats:%s", _SL_);

	if (prefix)
		fputs(prefix, fp);
	fprintf(fp, "  replay-window %u replay %u failed %u%s",
		s->replay_window, s->replay, s->integrity_failed, _SL_);
}

static const char *strxf_time(__u64 time)
{
	static char str[32];

	if (time == 0)
		strcpy(str, "-");
	else {
		time_t t;
		struct tm *tp;

		/* XXX: treat time in the same manner of kernel's
		 * net/xfrm/xfrm_{user,state}.c
		 */
		t = (long)time;
		tp = localtime(&t);

		strftime(str, sizeof(str), "%Y-%m-%d %T", tp);
	}

	return str;
}

static void xfrm_lifetime_print(struct xfrm_lifetime_cfg *cfg,
			 struct xfrm_lifetime_cur *cur,
			 FILE *fp, const char *prefix)
{
	if (cfg) {
		if (prefix)
			fputs(prefix, fp);
		fprintf(fp, "lifetime config:%s", _SL_);

		if (prefix)
			fputs(prefix, fp);
		fprintf(fp, "  limit: soft %s(bytes),",
			strxf_limit(cfg->soft_byte_limit));
		fprintf(fp, " hard %s(bytes)%s",
			strxf_limit(cfg->hard_byte_limit), _SL_);

		if (prefix)
			fputs(prefix, fp);
		fprintf(fp, "  limit: soft %s(packets),",
			strxf_limit(cfg->soft_packet_limit));
		fprintf(fp, " hard %s(packets)%s",
			strxf_limit(cfg->hard_packet_limit), _SL_);

		if (prefix)
			fputs(prefix, fp);
		fprintf(fp, "  expire add: soft %llu(sec), hard %llu(sec)%s",
			(unsigned long long) cfg->soft_add_expires_seconds,
			(unsigned long long) cfg->hard_add_expires_seconds,
			_SL_);

		if (prefix)
			fputs(prefix, fp);
		fprintf(fp, "  expire use: soft %llu(sec), hard %llu(sec)%s",
			(unsigned long long) cfg->soft_use_expires_seconds,
			(unsigned long long) cfg->hard_use_expires_seconds,
			_SL_);
	}
	if (cur) {
		if (prefix)
			fputs(prefix, fp);
		fprintf(fp, "lifetime current:%s", _SL_);

		if (prefix)
			fputs(prefix, fp);
		fprintf(fp, "  %llu(bytes), %llu(packets)%s",
			(unsigned long long) cur->bytes,
			(unsigned long long) cur->packets,
			 _SL_);

		if (prefix)
			fputs(prefix, fp);
		fprintf(fp, "  add %s ", strxf_time(cur->add_time));
		fprintf(fp, "use %s%s", strxf_time(cur->use_time), _SL_);
	}
}

void xfrm_selector_print(struct xfrm_selector *sel, __u16 family,
			 FILE *fp, const char *prefix)
{
	__u16 f;

	f = sel->family;
	if (f == AF_UNSPEC)
		f = family;
	if (f == AF_UNSPEC)
		f = preferred_family;

	if (prefix)
		fputs(prefix, fp);

	fprintf(fp, "src %s/%u ",
		rt_addr_n2a(f, sizeof(sel->saddr), &sel->saddr),
		sel->prefixlen_s);

	fprintf(fp, "dst %s/%u ",
		rt_addr_n2a(f, sizeof(sel->daddr), &sel->daddr),
		sel->prefixlen_d);

	if (sel->proto)
		fprintf(fp, "proto %s ", strxf_proto(sel->proto));
	switch (sel->proto) {
	case IPPROTO_TCP:
	case IPPROTO_UDP:
	case IPPROTO_SCTP:
	case IPPROTO_DCCP:
	default: /* XXX */
		if (sel->sport_mask)
			fprintf(fp, "sport %u ", ntohs(sel->sport));
		if (sel->dport_mask)
			fprintf(fp, "dport %u ", ntohs(sel->dport));
		break;
	case IPPROTO_ICMP:
	case IPPROTO_ICMPV6:
		/* type/code is stored at sport/dport in selector */
		if (sel->sport_mask)
			fprintf(fp, "type %u ", ntohs(sel->sport));
		if (sel->dport_mask)
			fprintf(fp, "code %u ", ntohs(sel->dport));
		break;
	case IPPROTO_GRE:
		if (sel->sport_mask || sel->dport_mask)
			fprintf(fp, "key %u ",
				(((__u32)ntohs(sel->sport)) << 16) +
				ntohs(sel->dport));
		break;
	case IPPROTO_MH:
		if (sel->sport_mask)
			fprintf(fp, "type %u ", ntohs(sel->sport));
		if (sel->dport_mask) {
			if (show_stats > 0)
				fprintf(fp, "(dport) 0x%.4x ", sel->dport);
		}
		break;
	}

	if (sel->ifindex > 0)
		fprintf(fp, "dev %s ", ll_index_to_name(sel->ifindex));

	if (show_stats > 0)
		fprintf(fp, "uid %u", sel->user);

	fprintf(fp, "%s", _SL_);
}

static void __xfrm_algo_print(struct xfrm_algo *algo, int type, int len,
			      FILE *fp, const char *prefix, int newline,
			      bool nokeys)
{
	int keylen;
	int i;

	if (prefix)
		fputs(prefix, fp);

	fprintf(fp, "%s ", strxf_algotype(type));

	if (len < sizeof(*algo)) {
		fprintf(fp, "(ERROR truncated)");
		goto fin;
	}
	len -= sizeof(*algo);

	fprintf(fp, "%s ", algo->alg_name);

	keylen = algo->alg_key_len / 8;
	if (len < keylen) {
		fprintf(fp, "(ERROR truncated)");
		goto fin;
	}

	if (nokeys)
		fprintf(fp, "<<Keys hidden>>");
	else if (keylen > 0) {
		fprintf(fp, "0x");
		for (i = 0; i < keylen; i++)
			fprintf(fp, "%.2x", (unsigned char)algo->alg_key[i]);

		if (show_stats > 0)
			fprintf(fp, " (%d bits)", algo->alg_key_len);
	}

 fin:
	if (newline)
		fprintf(fp, "%s", _SL_);
}

static inline void xfrm_algo_print(struct xfrm_algo *algo, int type, int len,
				   FILE *fp, const char *prefix, bool nokeys)
{
	return __xfrm_algo_print(algo, type, len, fp, prefix, 1, nokeys);
}

static void xfrm_aead_print(struct xfrm_algo_aead *algo, int len,
			    FILE *fp, const char *prefix, bool nokeys)
{
	struct xfrm_algo *base_algo = alloca(sizeof(*base_algo) + algo->alg_key_len / 8);

	memcpy(base_algo->alg_name, algo->alg_name, sizeof(base_algo->alg_name));
	base_algo->alg_key_len = algo->alg_key_len;
	memcpy(base_algo->alg_key, algo->alg_key, algo->alg_key_len / 8);

	__xfrm_algo_print(base_algo, XFRMA_ALG_AEAD, len, fp, prefix, 0,
			  nokeys);

	fprintf(fp, " %d", algo->alg_icv_len);

	fprintf(fp, "%s", _SL_);
}

static void xfrm_auth_trunc_print(struct xfrm_algo_auth *algo, int len,
				  FILE *fp, const char *prefix, bool nokeys)
{
	struct xfrm_algo *base_algo = alloca(sizeof(*base_algo) + algo->alg_key_len / 8);

	memcpy(base_algo->alg_name, algo->alg_name, sizeof(base_algo->alg_name));
	base_algo->alg_key_len = algo->alg_key_len;
	memcpy(base_algo->alg_key, algo->alg_key, algo->alg_key_len / 8);

	__xfrm_algo_print(base_algo, XFRMA_ALG_AUTH_TRUNC, len, fp, prefix, 0,
			  nokeys);

	fprintf(fp, " %d", algo->alg_trunc_len);

	fprintf(fp, "%s", _SL_);
}

static void xfrm_tmpl_print(struct xfrm_user_tmpl *tmpls, int len,
			    FILE *fp, const char *prefix)
{
	int ntmpls = len / sizeof(struct xfrm_user_tmpl);
	int i;

	if (ntmpls <= 0) {
		if (prefix)
			fputs(prefix, fp);
		fprintf(fp, "(ERROR \"tmpl\" truncated)");
		fprintf(fp, "%s", _SL_);
		return;
	}

	for (i = 0; i < ntmpls; i++) {
		struct xfrm_user_tmpl *tmpl = &tmpls[i];

		if (prefix)
			fputs(prefix, fp);

		xfrm_id_info_print(&tmpl->saddr, &tmpl->id, tmpl->mode,
				   tmpl->reqid, tmpl->family, 0, fp, prefix, "tmpl ");

		if (show_stats > 0 || tmpl->optional) {
			if (prefix)
				fputs(prefix, fp);
			fprintf(fp, "\t");
			switch (tmpl->optional) {
			case 0:
				if (show_stats > 0)
					fprintf(fp, "level required ");
				break;
			case 1:
				fprintf(fp, "level use ");
				break;
			default:
				fprintf(fp, "level %u ", tmpl->optional);
				break;
			}

			if (show_stats > 0)
				fprintf(fp, "share %s ", strxf_share(tmpl->share));

			fprintf(fp, "%s", _SL_);
		}

		if (show_stats > 0) {
			if (prefix)
				fputs(prefix, fp);
			fprintf(fp, "\t");
			fprintf(fp, "%s-mask %s ",
				strxf_algotype(XFRMA_ALG_CRYPT),
				strxf_mask32(tmpl->ealgos));
			fprintf(fp, "%s-mask %s ",
				strxf_algotype(XFRMA_ALG_AUTH),
				strxf_mask32(tmpl->aalgos));
			fprintf(fp, "%s-mask %s",
				strxf_algotype(XFRMA_ALG_COMP),
				strxf_mask32(tmpl->calgos));

			fprintf(fp, "%s", _SL_);
		}
	}
}

static void xfrm_output_mark_print(struct rtattr *tb[], FILE *fp)
{
	__u32 output_mark = rta_getattr_u32(tb[XFRMA_OUTPUT_MARK]);

	fprintf(fp, "output-mark 0x%x", output_mark);
	if (tb[XFRMA_SET_MARK_MASK]) {
		__u32 mask = rta_getattr_u32(tb[XFRMA_SET_MARK_MASK]);
		fprintf(fp, "/0x%x", mask);
	}
}

int xfrm_parse_mark(struct xfrm_mark *mark, int *argcp, char ***argvp)
{
	int argc = *argcp;
	char **argv = *argvp;

	NEXT_ARG();
	if (get_u32(&mark->v, *argv, 0)) {
		invarg("MARK value is invalid\n", *argv);
	}
	if (argc > 1)
		NEXT_ARG();
	else { /* last entry on parse line */
		mark->m = 0xffffffff;
		goto done;
	}

	if (strcmp(*argv, "mask") == 0) {
		NEXT_ARG();
		if (get_u32(&mark->m, *argv, 0)) {
			invarg("MASK value is invalid\n", *argv);
		}
	} else {
		mark->m = 0xffffffff;
		PREV_ARG();
	}

done:
	*argcp = argc;
	*argvp = argv;

	return 0;
}

void xfrm_xfrma_print(struct rtattr *tb[], __u16 family,
		      FILE *fp, const char *prefix, bool nokeys)
{
	if (tb[XFRMA_MARK]) {
		struct rtattr *rta = tb[XFRMA_MARK];
		struct xfrm_mark *m = RTA_DATA(rta);

		fprintf(fp, "\tmark %#x/%#x ", m->v, m->m);

		if (tb[XFRMA_OUTPUT_MARK])
			xfrm_output_mark_print(tb, fp);
		fprintf(fp, "%s", _SL_);
	} else if (tb[XFRMA_OUTPUT_MARK]) {
		fprintf(fp, "\t");

		xfrm_output_mark_print(tb, fp);
		fprintf(fp, "%s", _SL_);
	}

	if (tb[XFRMA_ALG_AUTH] && !tb[XFRMA_ALG_AUTH_TRUNC]) {
		struct rtattr *rta = tb[XFRMA_ALG_AUTH];

		xfrm_algo_print(RTA_DATA(rta), XFRMA_ALG_AUTH, RTA_PAYLOAD(rta),
				fp, prefix, nokeys);
	}

	if (tb[XFRMA_ALG_AUTH_TRUNC]) {
		struct rtattr *rta = tb[XFRMA_ALG_AUTH_TRUNC];

		xfrm_auth_trunc_print(RTA_DATA(rta), RTA_PAYLOAD(rta), fp,
				      prefix, nokeys);
	}

	if (tb[XFRMA_ALG_AEAD]) {
		struct rtattr *rta = tb[XFRMA_ALG_AEAD];

		xfrm_aead_print(RTA_DATA(rta), RTA_PAYLOAD(rta), fp, prefix,
				nokeys);
	}

	if (tb[XFRMA_ALG_CRYPT]) {
		struct rtattr *rta = tb[XFRMA_ALG_CRYPT];

		xfrm_algo_print(RTA_DATA(rta), XFRMA_ALG_CRYPT,
				RTA_PAYLOAD(rta), fp, prefix, nokeys);
	}

	if (tb[XFRMA_ALG_COMP]) {
		struct rtattr *rta = tb[XFRMA_ALG_COMP];

		xfrm_algo_print(RTA_DATA(rta), XFRMA_ALG_COMP, RTA_PAYLOAD(rta),
				fp, prefix, nokeys);
	}

	if (tb[XFRMA_ENCAP]) {
		struct xfrm_encap_tmpl *e;

		if (prefix)
			fputs(prefix, fp);
		fprintf(fp, "encap ");

		if (RTA_PAYLOAD(tb[XFRMA_ENCAP]) < sizeof(*e)) {
			fprintf(fp, "(ERROR truncated)");
			fprintf(fp, "%s", _SL_);
			return;
		}
		e = RTA_DATA(tb[XFRMA_ENCAP]);

		fprintf(fp, "type ");
		switch (e->encap_type) {
		case UDP_ENCAP_ESPINUDP_NON_IKE:
			fprintf(fp, "espinudp-nonike ");
			break;
		case UDP_ENCAP_ESPINUDP:
			fprintf(fp, "espinudp ");
			break;
		case TCP_ENCAP_ESPINTCP:
			fprintf(fp, "espintcp ");
			break;
		default:
			fprintf(fp, "%u ", e->encap_type);
			break;
		}
		fprintf(fp, "sport %u ", ntohs(e->encap_sport));
		fprintf(fp, "dport %u ", ntohs(e->encap_dport));

		fprintf(fp, "addr %s",
			rt_addr_n2a(family, sizeof(e->encap_oa), &e->encap_oa));
		fprintf(fp, "%s", _SL_);
	}

	if (tb[XFRMA_TMPL]) {
		struct rtattr *rta = tb[XFRMA_TMPL];

		xfrm_tmpl_print(RTA_DATA(rta),
				RTA_PAYLOAD(rta), fp, prefix);
	}

	if (tb[XFRMA_COADDR]) {
		const xfrm_address_t *coa;

		if (prefix)
			fputs(prefix, fp);
		fprintf(fp, "coa ");

		coa = RTA_DATA(tb[XFRMA_COADDR]);
		if (RTA_PAYLOAD(tb[XFRMA_COADDR]) < sizeof(*coa)) {
			fprintf(fp, "(ERROR truncated)");
			fprintf(fp, "%s", _SL_);
			return;
		}

		fprintf(fp, "%s",
			rt_addr_n2a(family, sizeof(*coa), coa));
		fprintf(fp, "%s", _SL_);
	}

	if (tb[XFRMA_LASTUSED]) {
		__u64 lastused;

		if (prefix)
			fputs(prefix, fp);
		fprintf(fp, "lastused ");

		if (RTA_PAYLOAD(tb[XFRMA_LASTUSED]) < sizeof(lastused)) {
			fprintf(fp, "(ERROR truncated)");
			fprintf(fp, "%s", _SL_);
			return;
		}

		lastused = rta_getattr_u64(tb[XFRMA_LASTUSED]);

		fprintf(fp, "%s", strxf_time(lastused));
		fprintf(fp, "%s", _SL_);
	}

	if (tb[XFRMA_REPLAY_VAL]) {
		struct xfrm_replay_state *replay;

		if (prefix)
			fputs(prefix, fp);
		fprintf(fp, "anti-replay context: ");

		if (RTA_PAYLOAD(tb[XFRMA_REPLAY_VAL]) < sizeof(*replay)) {
			fprintf(fp, "(ERROR truncated)");
			fprintf(fp, "%s", _SL_);
			return;
		}

		replay = RTA_DATA(tb[XFRMA_REPLAY_VAL]);
		fprintf(fp, "seq 0x%x, oseq 0x%x, bitmap 0x%08x",
			replay->seq, replay->oseq, replay->bitmap);
		fprintf(fp, "%s", _SL_);
	}

	if (tb[XFRMA_REPLAY_ESN_VAL]) {
		struct xfrm_replay_state_esn *replay;
		unsigned int i, j;

		if (prefix)
			fputs(prefix, fp);
		fprintf(fp, "anti-replay esn context:");

		if (RTA_PAYLOAD(tb[XFRMA_REPLAY_ESN_VAL]) < sizeof(*replay)) {
			fprintf(fp, "(ERROR truncated)");
			fprintf(fp, "%s", _SL_);
			return;
		}
		fprintf(fp, "%s", _SL_);

		replay = RTA_DATA(tb[XFRMA_REPLAY_ESN_VAL]);
		if (prefix)
			fputs(prefix, fp);
		fprintf(fp, " seq-hi 0x%x, seq 0x%x, oseq-hi 0x%0x, oseq 0x%0x",
			replay->seq_hi, replay->seq, replay->oseq_hi,
			replay->oseq);
		fprintf(fp, "%s", _SL_);
		if (prefix)
			fputs(prefix, fp);
		fprintf(fp, " replay_window %u, bitmap-length %u",
			replay->replay_window, replay->bmp_len);
		for (i = replay->bmp_len, j = 0; i; i--) {
			if (j++ % 8 == 0) {
				fprintf(fp, "%s", _SL_);
				if (prefix)
					fputs(prefix, fp);
				fprintf(fp, " ");
			}
			fprintf(fp, "%08x ", replay->bmp[i - 1]);
		}
		fprintf(fp, "%s", _SL_);
	}
	if (tb[XFRMA_OFFLOAD_DEV]) {
		struct xfrm_user_offload *xuo;

		if (prefix)
			fputs(prefix, fp);
		fprintf(fp, "crypto offload parameters: ");

		if (RTA_PAYLOAD(tb[XFRMA_OFFLOAD_DEV]) < sizeof(*xuo)) {
			fprintf(fp, "(ERROR truncated)");
			fprintf(fp, "%s", _SL_);
			return;
		}

		xuo = (struct xfrm_user_offload *)
			RTA_DATA(tb[XFRMA_OFFLOAD_DEV]);
		fprintf(fp, "dev %s dir %s", ll_index_to_name(xuo->ifindex),
			(xuo->flags & XFRM_OFFLOAD_INBOUND) ? "in" : "out");
		fprintf(fp, "%s", _SL_);
	}
	if (tb[XFRMA_IF_ID]) {
		__u32 if_id = rta_getattr_u32(tb[XFRMA_IF_ID]);

		if (prefix)
			fputs(prefix, fp);
		fprintf(fp, "if_id %#x", if_id);
		fprintf(fp, "%s", _SL_);
	}
}

static int xfrm_selector_iszero(struct xfrm_selector *s)
{
	struct xfrm_selector s0 = {};

	return (memcmp(&s0, s, sizeof(s0)) == 0);
}

static void xfrm_sec_ctx_print(FILE *fp, struct rtattr *attr)
{
	struct xfrm_user_sec_ctx *sctx;

	fprintf(fp, "\tsecurity context ");

	if (RTA_PAYLOAD(attr) < sizeof(*sctx))
		fprintf(fp, "(ERROR truncated)");

	sctx = RTA_DATA(attr);
	fprintf(fp, "%.*s %s", sctx->ctx_len, (char *)(sctx + 1), _SL_);
}

void xfrm_state_info_print(struct xfrm_usersa_info *xsinfo,
			    struct rtattr *tb[], FILE *fp, const char *prefix,
			    const char *title, bool nokeys)
{
	char buf[STRBUF_SIZE] = {};
	int force_spi = xfrm_xfrmproto_is_ipsec(xsinfo->id.proto);

	xfrm_id_info_print(&xsinfo->saddr, &xsinfo->id, xsinfo->mode,
			   xsinfo->reqid, xsinfo->family, force_spi, fp,
			   prefix, title);

	if (prefix)
		strlcat(buf, prefix, sizeof(buf));
	strlcat(buf, "\t", sizeof(buf));

	fputs(buf, fp);
	fprintf(fp, "replay-window %u ", xsinfo->replay_window);
	if (show_stats > 0)
		fprintf(fp, "seq 0x%08u ", xsinfo->seq);
	if (show_stats > 0 || xsinfo->flags) {
		__u8 flags = xsinfo->flags;

		fprintf(fp, "flag ");
		XFRM_FLAG_PRINT(fp, flags, XFRM_STATE_NOECN, "noecn");
		XFRM_FLAG_PRINT(fp, flags, XFRM_STATE_DECAP_DSCP, "decap-dscp");
		XFRM_FLAG_PRINT(fp, flags, XFRM_STATE_NOPMTUDISC, "nopmtudisc");
		XFRM_FLAG_PRINT(fp, flags, XFRM_STATE_WILDRECV, "wildrecv");
		XFRM_FLAG_PRINT(fp, flags, XFRM_STATE_ICMP, "icmp");
		XFRM_FLAG_PRINT(fp, flags, XFRM_STATE_AF_UNSPEC, "af-unspec");
		XFRM_FLAG_PRINT(fp, flags, XFRM_STATE_ALIGN4, "align4");
		XFRM_FLAG_PRINT(fp, flags, XFRM_STATE_ESN, "esn");
		if (flags)
			fprintf(fp, "%x", flags);
	}
	if (show_stats > 0 && tb[XFRMA_SA_EXTRA_FLAGS]) {
		__u32 extra_flags = rta_getattr_u32(tb[XFRMA_SA_EXTRA_FLAGS]);

		fprintf(fp, "extra_flag ");
		XFRM_FLAG_PRINT(fp, extra_flags,
				XFRM_SA_XFLAG_DONT_ENCAP_DSCP,
				"dont-encap-dscp");
		XFRM_FLAG_PRINT(fp, extra_flags,
				XFRM_SA_XFLAG_OSEQ_MAY_WRAP,
				"oseq-may-wrap");
		if (extra_flags)
			fprintf(fp, "%x", extra_flags);
	}
	if (show_stats > 0)
		fprintf(fp, " (0x%s)", strxf_mask8(xsinfo->flags));
	fprintf(fp, "%s", _SL_);

	xfrm_xfrma_print(tb, xsinfo->family, fp, buf, nokeys);

	if (!xfrm_selector_iszero(&xsinfo->sel)) {
		char sbuf[STRBUF_SIZE];

		memcpy(sbuf, buf, sizeof(sbuf));
		strlcat(sbuf, "sel ", sizeof(sbuf));

		xfrm_selector_print(&xsinfo->sel, xsinfo->family, fp, sbuf);
	}

	if (show_stats > 0) {
		xfrm_lifetime_print(&xsinfo->lft, &xsinfo->curlft, fp, buf);
		xfrm_stats_print(&xsinfo->stats, fp, buf);
	}

	if (tb[XFRMA_SEC_CTX])
		xfrm_sec_ctx_print(fp, tb[XFRMA_SEC_CTX]);
}

void xfrm_policy_info_print(struct xfrm_userpolicy_info *xpinfo,
			    struct rtattr *tb[], FILE *fp, const char *prefix,
			    const char *title)
{
	char buf[STRBUF_SIZE] = {};

	xfrm_selector_print(&xpinfo->sel, preferred_family, fp, title);

	if (tb[XFRMA_SEC_CTX])
		xfrm_sec_ctx_print(fp, tb[XFRMA_SEC_CTX]);

	if (prefix)
		strlcat(buf, prefix, sizeof(buf));
	strlcat(buf, "\t", sizeof(buf));

	fputs(buf, fp);
	if (xpinfo->dir >= XFRM_POLICY_MAX) {
		xpinfo->dir -= XFRM_POLICY_MAX;
		fprintf(fp, "socket ");
	} else
		fprintf(fp, "dir ");

	switch (xpinfo->dir) {
	case XFRM_POLICY_IN:
		fprintf(fp, "in");
		break;
	case XFRM_POLICY_OUT:
		fprintf(fp, "out");
		break;
	case XFRM_POLICY_FWD:
		fprintf(fp, "fwd");
		break;
	default:
		fprintf(fp, "%u", xpinfo->dir);
		break;
	}
	fprintf(fp, " ");

	switch (xpinfo->action) {
	case XFRM_POLICY_ALLOW:
		if (show_stats > 0)
			fprintf(fp, "action allow ");
		break;
	case XFRM_POLICY_BLOCK:
		fprintf(fp, "action block ");
		break;
	default:
		fprintf(fp, "action %u ", xpinfo->action);
		break;
	}

	if (show_stats)
		fprintf(fp, "index %u ", xpinfo->index);
	fprintf(fp, "priority %u ", xpinfo->priority);

	if (tb[XFRMA_POLICY_TYPE]) {
		struct xfrm_userpolicy_type *upt;

		fprintf(fp, "ptype ");

		if (RTA_PAYLOAD(tb[XFRMA_POLICY_TYPE]) < sizeof(*upt))
			fprintf(fp, "(ERROR truncated)");

		upt = RTA_DATA(tb[XFRMA_POLICY_TYPE]);
		fprintf(fp, "%s ", strxf_ptype(upt->type));
	}

	if (show_stats > 0)
		fprintf(fp, "share %s ", strxf_share(xpinfo->share));

	if (show_stats > 0 || xpinfo->flags) {
		__u8 flags = xpinfo->flags;

		fprintf(fp, "flag ");
		XFRM_FLAG_PRINT(fp, flags, XFRM_POLICY_LOCALOK, "localok");
		XFRM_FLAG_PRINT(fp, flags, XFRM_POLICY_ICMP, "icmp");
		if (flags)
			fprintf(fp, "%x", flags);
	}
	if (show_stats > 0)
		fprintf(fp, " (0x%s)", strxf_mask8(xpinfo->flags));
	fprintf(fp, "%s", _SL_);

	if (show_stats > 0)
		xfrm_lifetime_print(&xpinfo->lft, &xpinfo->curlft, fp, buf);

	xfrm_xfrma_print(tb, xpinfo->sel.family, fp, buf, false);
}

int xfrm_id_parse(xfrm_address_t *saddr, struct xfrm_id *id, __u16 *family,
		  int loose, int *argcp, char ***argvp)
{
	int argc = *argcp;
	char **argv = *argvp;
	inet_prefix dst = {};
	inet_prefix src = {};

	while (1) {
		if (strcmp(*argv, "src") == 0) {
			NEXT_ARG();

			get_prefix(&src, *argv, preferred_family);
			if (src.family == AF_UNSPEC)
				invarg("value after \"src\" has an unrecognized address family", *argv);
			if (family)
				*family = src.family;

			memcpy(saddr, &src.data, sizeof(*saddr));

			filter.id_src_mask = src.bitlen;

		} else if (strcmp(*argv, "dst") == 0) {
			NEXT_ARG();

			get_prefix(&dst, *argv, preferred_family);
			if (dst.family == AF_UNSPEC)
				invarg("value after \"dst\" has an unrecognized address family", *argv);
			if (family)
				*family = dst.family;

			memcpy(&id->daddr, &dst.data, sizeof(id->daddr));

			filter.id_dst_mask = dst.bitlen;

		} else if (strcmp(*argv, "proto") == 0) {
			int ret;

			NEXT_ARG();

			ret = xfrm_xfrmproto_getbyname(*argv);
			if (ret < 0)
				invarg("XFRM-PROTO value is invalid", *argv);

			id->proto = (__u8)ret;

			filter.id_proto_mask = XFRM_FILTER_MASK_FULL;

		} else if (strcmp(*argv, "spi") == 0) {
			NEXT_ARG();
			if (get_be32(&id->spi, *argv, 0))
				invarg("SPI value is invalid", *argv);

			filter.id_spi_mask = XFRM_FILTER_MASK_FULL;

		} else {
			PREV_ARG(); /* back track */
			break;
		}

		if (!NEXT_ARG_OK())
			break;
		NEXT_ARG();
	}

	if (src.family && dst.family && (src.family != dst.family))
		invarg("the same address family is required between values after \"src\" and \"dst\"", *argv);

	if (id->spi && id->proto) {
		if (xfrm_xfrmproto_is_ro(id->proto)) {
			fprintf(stderr, "\"spi\" is invalid with XFRM-PROTO value \"%s\"\n",
				strxf_xfrmproto(id->proto));
			exit(1);
		} else if (id->proto == IPPROTO_COMP && ntohl(id->spi) >= 0x10000) {
			fprintf(stderr, "SPI value is too large with XFRM-PROTO value \"%s\"\n",
				strxf_xfrmproto(id->proto));
			exit(1);
		}
	}

	if (loose == 0 && id->proto == 0)
		missarg("XFRM-PROTO");
	if (argc == *argcp)
		missarg("ID");

	*argcp = argc;
	*argvp = argv;

	return 0;
}

int xfrm_mode_parse(__u8 *mode, int *argcp, char ***argvp)
{
	int argc = *argcp;
	char **argv = *argvp;

	if (matches(*argv, "transport") == 0)
		*mode = XFRM_MODE_TRANSPORT;
	else if (matches(*argv, "tunnel") == 0)
		*mode = XFRM_MODE_TUNNEL;
	else if (matches(*argv, "ro") == 0)
		*mode = XFRM_MODE_ROUTEOPTIMIZATION;
	else if (matches(*argv, "in_trigger") == 0)
		*mode = XFRM_MODE_IN_TRIGGER;
	else if (matches(*argv, "beet") == 0)
		*mode = XFRM_MODE_BEET;
	else
		invarg("MODE value is invalid", *argv);

	*argcp = argc;
	*argvp = argv;

	return 0;
}

int xfrm_encap_type_parse(__u16 *type, int *argcp, char ***argvp)
{
	int argc = *argcp;
	char **argv = *argvp;

	if (strcmp(*argv, "espinudp-nonike") == 0)
		*type = UDP_ENCAP_ESPINUDP_NON_IKE;
	else if (strcmp(*argv, "espinudp") == 0)
		*type = UDP_ENCAP_ESPINUDP;
	else if (strcmp(*argv, "espintcp") == 0)
		*type = TCP_ENCAP_ESPINTCP;
	else
		invarg("ENCAP-TYPE value is invalid", *argv);

	*argcp = argc;
	*argvp = argv;

	return 0;
}

/* NOTE: reqid is used by host-byte order */
int xfrm_reqid_parse(__u32 *reqid, int *argcp, char ***argvp)
{
	int argc = *argcp;
	char **argv = *argvp;

	if (get_u32(reqid, *argv, 0))
		invarg("REQID value is invalid", *argv);

	*argcp = argc;
	*argvp = argv;

	return 0;
}

static int xfrm_selector_upspec_parse(struct xfrm_selector *sel,
				      int *argcp, char ***argvp)
{
	int argc = *argcp;
	char **argv = *argvp;
	char *sportp = NULL;
	char *dportp = NULL;
	char *typep = NULL;
	char *codep = NULL;
	char *grekey = NULL;

	while (1) {
		if (strcmp(*argv, "proto") == 0) {
			__u8 upspec;

			NEXT_ARG();

			if (strcmp(*argv, "any") == 0)
				upspec = 0;
			else {
				struct protoent *pp;

				pp = getprotobyname(*argv);
				if (pp)
					upspec = pp->p_proto;
				else {
					if (get_u8(&upspec, *argv, 0))
						invarg("PROTO value is invalid", *argv);
				}
			}
			sel->proto = upspec;

			filter.upspec_proto_mask = XFRM_FILTER_MASK_FULL;

		} else if (strcmp(*argv, "sport") == 0) {
			sportp = *argv;

			NEXT_ARG();

			if (get_be16(&sel->sport, *argv, 0))
				invarg("value after \"sport\" is invalid", *argv);
			if (sel->sport)
				sel->sport_mask = ~((__u16)0);

			filter.upspec_sport_mask = XFRM_FILTER_MASK_FULL;

		} else if (strcmp(*argv, "dport") == 0) {
			dportp = *argv;

			NEXT_ARG();

			if (get_be16(&sel->dport, *argv, 0))
				invarg("value after \"dport\" is invalid", *argv);
			if (sel->dport)
				sel->dport_mask = ~((__u16)0);

			filter.upspec_dport_mask = XFRM_FILTER_MASK_FULL;

		} else if (strcmp(*argv, "type") == 0) {
			typep = *argv;

			NEXT_ARG();

			if (get_u16(&sel->sport, *argv, 0) ||
			    (sel->sport & ~((__u16)0xff)))
				invarg("value after \"type\" is invalid", *argv);
			sel->sport = htons(sel->sport);
			sel->sport_mask = ~((__u16)0);

			filter.upspec_sport_mask = XFRM_FILTER_MASK_FULL;


		} else if (strcmp(*argv, "code") == 0) {
			codep = *argv;

			NEXT_ARG();

			if (get_u16(&sel->dport, *argv, 0) ||
			    (sel->dport & ~((__u16)0xff)))
				invarg("value after \"code\" is invalid", *argv);
			sel->dport = htons(sel->dport);
			sel->dport_mask = ~((__u16)0);

			filter.upspec_dport_mask = XFRM_FILTER_MASK_FULL;

		} else if (strcmp(*argv, "key") == 0) {
			unsigned int uval;

			grekey = *argv;

			NEXT_ARG();

			if (strchr(*argv, '.'))
				uval = htonl(get_addr32(*argv));
			else {
				if (get_unsigned(&uval, *argv, 0) < 0) {
					fprintf(stderr, "value after \"key\" is invalid\n");
					exit(-1);
				}
			}

			sel->sport = htons(uval >> 16);
			sel->dport = htons(uval & 0xffff);
			sel->sport_mask = ~((__u16)0);
			sel->dport_mask = ~((__u16)0);

			filter.upspec_dport_mask = XFRM_FILTER_MASK_FULL;

		} else {
			PREV_ARG(); /* back track */
			break;
		}

		if (!NEXT_ARG_OK())
			break;
		NEXT_ARG();
	}
	if (argc == *argcp)
		missarg("UPSPEC");
	if (sportp || dportp) {
		switch (sel->proto) {
		case IPPROTO_TCP:
		case IPPROTO_UDP:
		case IPPROTO_SCTP:
		case IPPROTO_DCCP:
		case IPPROTO_IP: /* to allow shared SA for different protocols */
			break;
		default:
			fprintf(stderr, "\"sport\" and \"dport\" are invalid with PROTO value \"%s\"\n", strxf_proto(sel->proto));
			exit(1);
		}
	}
	if (typep || codep) {
		switch (sel->proto) {
		case IPPROTO_ICMP:
		case IPPROTO_ICMPV6:
		case IPPROTO_MH:
			break;
		default:
			fprintf(stderr, "\"type\" and \"code\" are invalid with PROTO value \"%s\"\n", strxf_proto(sel->proto));
			exit(1);
		}
	}
	if (grekey) {
		switch (sel->proto) {
		case IPPROTO_GRE:
			break;
		default:
			fprintf(stderr, "\"key\" is invalid with PROTO value \"%s\"\n", strxf_proto(sel->proto));
			exit(1);
		}
	}

	*argcp = argc;
	*argvp = argv;

	return 0;
}

int xfrm_selector_parse(struct xfrm_selector *sel, int *argcp, char ***argvp)
{
	int argc = *argcp;
	char **argv = *argvp;
	inet_prefix dst = {};
	inet_prefix src = {};
	char *upspecp = NULL;

	while (1) {
		if (strcmp(*argv, "src") == 0) {
			NEXT_ARG();

			get_prefix(&src, *argv, preferred_family);
			if (src.family == AF_UNSPEC)
				invarg("value after \"src\" has an unrecognized address family", *argv);
			sel->family = src.family;

			memcpy(&sel->saddr, &src.data, sizeof(sel->saddr));
			sel->prefixlen_s = src.bitlen;

			filter.sel_src_mask = src.bitlen;

		} else if (strcmp(*argv, "dst") == 0) {
			NEXT_ARG();

			get_prefix(&dst, *argv, preferred_family);
			if (dst.family == AF_UNSPEC)
				invarg("value after \"dst\" has an unrecognized address family", *argv);
			sel->family = dst.family;

			memcpy(&sel->daddr, &dst.data, sizeof(sel->daddr));
			sel->prefixlen_d = dst.bitlen;

			filter.sel_dst_mask = dst.bitlen;

		} else if (strcmp(*argv, "dev") == 0) {
			int ifindex;

			NEXT_ARG();

			if (strcmp(*argv, "none") == 0)
				ifindex = 0;
			else {
				ifindex = ll_name_to_index(*argv);
				if (ifindex <= 0)
					invarg("DEV value is invalid", *argv);
			}
			sel->ifindex = ifindex;

			filter.sel_dev_mask = XFRM_FILTER_MASK_FULL;

		} else {
			if (upspecp) {
				PREV_ARG(); /* back track */
				break;
			} else {
				upspecp = *argv;
				xfrm_selector_upspec_parse(sel, &argc, &argv);
			}
		}

		if (!NEXT_ARG_OK())
			break;

		NEXT_ARG();
	}

	if (src.family && dst.family && (src.family != dst.family))
		invarg("the same address family is required between values after \"src\" and \"dst\"", *argv);

	if (argc == *argcp)
		missarg("SELECTOR");

	*argcp = argc;
	*argvp = argv;

	return 0;
}

int xfrm_lifetime_cfg_parse(struct xfrm_lifetime_cfg *lft,
			    int *argcp, char ***argvp)
{
	int argc = *argcp;
	char **argv = *argvp;
	int ret;

	if (strcmp(*argv, "time-soft") == 0) {
		NEXT_ARG();
		ret = get_u64(&lft->soft_add_expires_seconds, *argv, 0);
		if (ret)
			invarg("value after \"time-soft\" is invalid", *argv);
	} else if (strcmp(*argv, "time-hard") == 0) {
		NEXT_ARG();
		ret = get_u64(&lft->hard_add_expires_seconds, *argv, 0);
		if (ret)
			invarg("value after \"time-hard\" is invalid", *argv);
	} else if (strcmp(*argv, "time-use-soft") == 0) {
		NEXT_ARG();
		ret = get_u64(&lft->soft_use_expires_seconds, *argv, 0);
		if (ret)
			invarg("value after \"time-use-soft\" is invalid", *argv);
	} else if (strcmp(*argv, "time-use-hard") == 0) {
		NEXT_ARG();
		ret = get_u64(&lft->hard_use_expires_seconds, *argv, 0);
		if (ret)
			invarg("value after \"time-use-hard\" is invalid", *argv);
	} else if (strcmp(*argv, "byte-soft") == 0) {
		NEXT_ARG();
		ret = get_u64(&lft->soft_byte_limit, *argv, 0);
		if (ret)
			invarg("value after \"byte-soft\" is invalid", *argv);
	} else if (strcmp(*argv, "byte-hard") == 0) {
		NEXT_ARG();
		ret = get_u64(&lft->hard_byte_limit, *argv, 0);
		if (ret)
			invarg("value after \"byte-hard\" is invalid", *argv);
	} else if (strcmp(*argv, "packet-soft") == 0) {
		NEXT_ARG();
		ret = get_u64(&lft->soft_packet_limit, *argv, 0);
		if (ret)
			invarg("value after \"packet-soft\" is invalid", *argv);
	} else if (strcmp(*argv, "packet-hard") == 0) {
		NEXT_ARG();
		ret = get_u64(&lft->hard_packet_limit, *argv, 0);
		if (ret)
			invarg("value after \"packet-hard\" is invalid", *argv);
	} else
		invarg("LIMIT value is invalid", *argv);

	*argcp = argc;
	*argvp = argv;

	return 0;
}

int do_xfrm(int argc, char **argv)
{
	memset(&filter, 0, sizeof(filter));

	if (argc < 1)
		usage();

	if (matches(*argv, "state") == 0 ||
	    matches(*argv, "sa") == 0)
		return do_xfrm_state(argc-1, argv+1);
	else if (matches(*argv, "policy") == 0)
		return do_xfrm_policy(argc-1, argv+1);
	else if (matches(*argv, "monitor") == 0)
		return do_xfrm_monitor(argc-1, argv+1);
	else if (matches(*argv, "help") == 0) {
		usage();
		fprintf(stderr, "xfrm Object \"%s\" is unknown.\n", *argv);
		exit(-1);
	}
	usage();
}
