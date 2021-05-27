/*
 * rt_names.c		rtnetlink names DB.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Authors:	Alexey Kuznetsov, <kuznet@ms2.inr.ac.ru>
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <dirent.h>
#include <limits.h>

#include <asm/types.h>
#include <linux/rtnetlink.h>

#include "rt_names.h"
#include "utils.h"

#define NAME_MAX_LEN 512

int numeric;

struct rtnl_hash_entry {
	struct rtnl_hash_entry	*next;
	const char		*name;
	unsigned int		id;
};

static int fread_id_name(FILE *fp, int *id, char *namebuf)
{
	char buf[NAME_MAX_LEN];

	while (fgets(buf, sizeof(buf), fp)) {
		char *p = buf;

		while (*p == ' ' || *p == '\t')
			p++;

		if (*p == '#' || *p == '\n' || *p == 0)
			continue;

		if (sscanf(p, "0x%x %s\n", id, namebuf) != 2 &&
				sscanf(p, "0x%x %s #", id, namebuf) != 2 &&
				sscanf(p, "%d %s\n", id, namebuf) != 2 &&
				sscanf(p, "%d %s #", id, namebuf) != 2) {
			strcpy(namebuf, p);
			return -1;
		}
		return 1;
	}
	return 0;
}

static void
rtnl_hash_initialize(const char *file, struct rtnl_hash_entry **hash, int size)
{
	struct rtnl_hash_entry *entry;
	FILE *fp;
	int id;
	char namebuf[NAME_MAX_LEN] = {0};
	int ret;

	fp = fopen(file, "r");
	if (!fp)
		return;

	while ((ret = fread_id_name(fp, &id, &namebuf[0]))) {
		if (ret == -1) {
			fprintf(stderr, "Database %s is corrupted at %s\n",
					file, namebuf);
			fclose(fp);
			return;
		}

		if (id < 0)
			continue;

		entry = malloc(sizeof(*entry));
		entry->id   = id;
		entry->name = strdup(namebuf);
		entry->next = hash[id & (size - 1)];
		hash[id & (size - 1)] = entry;
	}
	fclose(fp);
}

static void rtnl_tab_initialize(const char *file, char **tab, int size)
{
	FILE *fp;
	int id;
	char namebuf[NAME_MAX_LEN] = {0};
	int ret;

	fp = fopen(file, "r");
	if (!fp)
		return;

	while ((ret = fread_id_name(fp, &id, &namebuf[0]))) {
		if (ret == -1) {
			fprintf(stderr, "Database %s is corrupted at %s\n",
					file, namebuf);
			fclose(fp);
			return;
		}
		if (id < 0 || id > size)
			continue;

		tab[id] = strdup(namebuf);
	}
	fclose(fp);
}

static char *rtnl_rtprot_tab[256] = {
	[RTPROT_UNSPEC]	    = "unspec",
	[RTPROT_REDIRECT]   = "redirect",
	[RTPROT_KERNEL]	    = "kernel",
	[RTPROT_BOOT]	    = "boot",
	[RTPROT_STATIC]	    = "static",

	[RTPROT_GATED]	    = "gated",
	[RTPROT_RA]	    = "ra",
	[RTPROT_MRT]	    = "mrt",
	[RTPROT_ZEBRA]	    = "zebra",
	[RTPROT_BIRD]	    = "bird",
	[RTPROT_BABEL]	    = "babel",
	[RTPROT_DNROUTED]   = "dnrouted",
	[RTPROT_XORP]	    = "xorp",
	[RTPROT_NTK]	    = "ntk",
	[RTPROT_DHCP]	    = "dhcp",
	[RTPROT_KEEPALIVED] = "keepalived",
	[RTPROT_BGP]	    = "bgp",
	[RTPROT_ISIS]	    = "isis",
	[RTPROT_OSPF]	    = "ospf",
	[RTPROT_RIP]	    = "rip",
	[RTPROT_EIGRP]	    = "eigrp",
};


static int rtnl_rtprot_init;

static void rtnl_rtprot_initialize(void)
{
	struct dirent *de;
	DIR *d;

	rtnl_rtprot_init = 1;
	rtnl_tab_initialize(CONFDIR "/rt_protos",
			    rtnl_rtprot_tab, 256);

	d = opendir(CONFDIR "/rt_protos.d");
	if (!d)
		return;

	while ((de = readdir(d)) != NULL) {
		char path[PATH_MAX];
		size_t len;

		if (*de->d_name == '.')
			continue;

		/* only consider filenames ending in '.conf' */
		len = strlen(de->d_name);
		if (len <= 5)
			continue;
		if (strcmp(de->d_name + len - 5, ".conf"))
			continue;

		snprintf(path, sizeof(path), CONFDIR "/rt_protos.d/%s",
			 de->d_name);
		rtnl_tab_initialize(path, rtnl_rtprot_tab, 256);
	}
	closedir(d);
}

const char *rtnl_rtprot_n2a(int id, char *buf, int len)
{
	if (id < 0 || id >= 256 || numeric) {
		snprintf(buf, len, "%u", id);
		return buf;
	}
	if (!rtnl_rtprot_tab[id]) {
		if (!rtnl_rtprot_init)
			rtnl_rtprot_initialize();
	}
	if (rtnl_rtprot_tab[id])
		return rtnl_rtprot_tab[id];
	snprintf(buf, len, "%u", id);
	return buf;
}

int rtnl_rtprot_a2n(__u32 *id, const char *arg)
{
	static char *cache;
	static unsigned long res;
	char *end;
	int i;

	if (cache && strcmp(cache, arg) == 0) {
		*id = res;
		return 0;
	}

	if (!rtnl_rtprot_init)
		rtnl_rtprot_initialize();

	for (i = 0; i < 256; i++) {
		if (rtnl_rtprot_tab[i] &&
		    strcmp(rtnl_rtprot_tab[i], arg) == 0) {
			cache = rtnl_rtprot_tab[i];
			res = i;
			*id = res;
			return 0;
		}
	}

	res = strtoul(arg, &end, 0);
	if (!end || end == arg || *end || res > 255)
		return -1;
	*id = res;
	return 0;
}


static char *rtnl_rtscope_tab[256] = {
	[RT_SCOPE_UNIVERSE]	= "global",
	[RT_SCOPE_NOWHERE]	= "nowhere",
	[RT_SCOPE_HOST]		= "host",
	[RT_SCOPE_LINK]		= "link",
	[RT_SCOPE_SITE]		= "site",
};

static int rtnl_rtscope_init;

static void rtnl_rtscope_initialize(void)
{
	rtnl_rtscope_init = 1;
	rtnl_tab_initialize(CONFDIR "/rt_scopes",
			    rtnl_rtscope_tab, 256);
}

const char *rtnl_rtscope_n2a(int id, char *buf, int len)
{
	if (id < 0 || id >= 256 || numeric) {
		snprintf(buf, len, "%d", id);
		return buf;
	}

	if (!rtnl_rtscope_tab[id]) {
		if (!rtnl_rtscope_init)
			rtnl_rtscope_initialize();
	}

	if (rtnl_rtscope_tab[id])
		return rtnl_rtscope_tab[id];

	snprintf(buf, len, "%d", id);
	return buf;
}

int rtnl_rtscope_a2n(__u32 *id, const char *arg)
{
	static const char *cache;
	static unsigned long res;
	char *end;
	int i;

	if (cache && strcmp(cache, arg) == 0) {
		*id = res;
		return 0;
	}

	if (!rtnl_rtscope_init)
		rtnl_rtscope_initialize();

	for (i = 0; i < 256; i++) {
		if (rtnl_rtscope_tab[i] &&
		    strcmp(rtnl_rtscope_tab[i], arg) == 0) {
			cache = rtnl_rtscope_tab[i];
			res = i;
			*id = res;
			return 0;
		}
	}

	res = strtoul(arg, &end, 0);
	if (!end || end == arg || *end || res > 255)
		return -1;
	*id = res;
	return 0;
}


static char *rtnl_rtrealm_tab[256] = {
	"unknown",
};

static int rtnl_rtrealm_init;

static void rtnl_rtrealm_initialize(void)
{
	rtnl_rtrealm_init = 1;
	rtnl_tab_initialize(CONFDIR "/rt_realms",
			    rtnl_rtrealm_tab, 256);
}

const char *rtnl_rtrealm_n2a(int id, char *buf, int len)
{
	if (id < 0 || id >= 256 || numeric) {
		snprintf(buf, len, "%d", id);
		return buf;
	}
	if (!rtnl_rtrealm_tab[id]) {
		if (!rtnl_rtrealm_init)
			rtnl_rtrealm_initialize();
	}
	if (rtnl_rtrealm_tab[id])
		return rtnl_rtrealm_tab[id];
	snprintf(buf, len, "%d", id);
	return buf;
}


int rtnl_rtrealm_a2n(__u32 *id, const char *arg)
{
	static char *cache;
	static unsigned long res;
	char *end;
	int i;

	if (cache && strcmp(cache, arg) == 0) {
		*id = res;
		return 0;
	}

	if (!rtnl_rtrealm_init)
		rtnl_rtrealm_initialize();

	for (i = 0; i < 256; i++) {
		if (rtnl_rtrealm_tab[i] &&
		    strcmp(rtnl_rtrealm_tab[i], arg) == 0) {
			cache = rtnl_rtrealm_tab[i];
			res = i;
			*id = res;
			return 0;
		}
	}

	res = strtoul(arg, &end, 0);
	if (!end || end == arg || *end || res > 255)
		return -1;
	*id = res;
	return 0;
}


static struct rtnl_hash_entry dflt_table_entry  = { .name = "default" };
static struct rtnl_hash_entry main_table_entry  = { .name = "main" };
static struct rtnl_hash_entry local_table_entry = { .name = "local" };

static struct rtnl_hash_entry *rtnl_rttable_hash[256] = {
	[RT_TABLE_DEFAULT] = &dflt_table_entry,
	[RT_TABLE_MAIN]    = &main_table_entry,
	[RT_TABLE_LOCAL]   = &local_table_entry,
};

static int rtnl_rttable_init;

static void rtnl_rttable_initialize(void)
{
	struct dirent *de;
	DIR *d;
	int i;

	rtnl_rttable_init = 1;
	for (i = 0; i < 256; i++) {
		if (rtnl_rttable_hash[i])
			rtnl_rttable_hash[i]->id = i;
	}
	rtnl_hash_initialize(CONFDIR "/rt_tables",
			     rtnl_rttable_hash, 256);

	d = opendir(CONFDIR "/rt_tables.d");
	if (!d)
		return;

	while ((de = readdir(d)) != NULL) {
		char path[PATH_MAX];
		size_t len;

		if (*de->d_name == '.')
			continue;

		/* only consider filenames ending in '.conf' */
		len = strlen(de->d_name);
		if (len <= 5)
			continue;
		if (strcmp(de->d_name + len - 5, ".conf"))
			continue;

		snprintf(path, sizeof(path),
			 CONFDIR "/rt_tables.d/%s", de->d_name);
		rtnl_hash_initialize(path, rtnl_rttable_hash, 256);
	}
	closedir(d);
}

const char *rtnl_rttable_n2a(__u32 id, char *buf, int len)
{
	struct rtnl_hash_entry *entry;

	if (!rtnl_rttable_init)
		rtnl_rttable_initialize();
	entry = rtnl_rttable_hash[id & 255];
	while (entry && entry->id != id)
		entry = entry->next;
	if (!numeric && entry)
		return entry->name;
	snprintf(buf, len, "%u", id);
	return buf;
}

int rtnl_rttable_a2n(__u32 *id, const char *arg)
{
	static const char *cache;
	static unsigned long res;
	struct rtnl_hash_entry *entry;
	char *end;
	unsigned long i;

	if (cache && strcmp(cache, arg) == 0) {
		*id = res;
		return 0;
	}

	if (!rtnl_rttable_init)
		rtnl_rttable_initialize();

	for (i = 0; i < 256; i++) {
		entry = rtnl_rttable_hash[i];
		while (entry && strcmp(entry->name, arg))
			entry = entry->next;
		if (entry) {
			cache = entry->name;
			res = entry->id;
			*id = res;
			return 0;
		}
	}

	i = strtoul(arg, &end, 0);
	if (!end || end == arg || *end || i > RT_TABLE_MAX)
		return -1;
	*id = i;
	return 0;
}


static char *rtnl_rtdsfield_tab[256] = {
	"0",
};

static int rtnl_rtdsfield_init;

static void rtnl_rtdsfield_initialize(void)
{
	rtnl_rtdsfield_init = 1;
	rtnl_tab_initialize(CONFDIR "/rt_dsfield",
			    rtnl_rtdsfield_tab, 256);
}

const char *rtnl_dsfield_n2a(int id, char *buf, int len)
{
	const char *name;

	if (id < 0 || id >= 256) {
		snprintf(buf, len, "%d", id);
		return buf;
	}
	if (!numeric) {
		name = rtnl_dsfield_get_name(id);
		if (name != NULL)
			return name;
	}
	snprintf(buf, len, "0x%02x", id);
	return buf;
}

const char *rtnl_dsfield_get_name(int id)
{
	if (id < 0 || id >= 256)
		return NULL;
	if (!rtnl_rtdsfield_tab[id]) {
		if (!rtnl_rtdsfield_init)
			rtnl_rtdsfield_initialize();
	}
	return rtnl_rtdsfield_tab[id];
}


int rtnl_dsfield_a2n(__u32 *id, const char *arg)
{
	static char *cache;
	static unsigned long res;
	char *end;
	int i;

	if (cache && strcmp(cache, arg) == 0) {
		*id = res;
		return 0;
	}

	if (!rtnl_rtdsfield_init)
		rtnl_rtdsfield_initialize();

	for (i = 0; i < 256; i++) {
		if (rtnl_rtdsfield_tab[i] &&
		    strcmp(rtnl_rtdsfield_tab[i], arg) == 0) {
			cache = rtnl_rtdsfield_tab[i];
			res = i;
			*id = res;
			return 0;
		}
	}

	res = strtoul(arg, &end, 16);
	if (!end || end == arg || *end || res > 255)
		return -1;
	*id = res;
	return 0;
}


static struct rtnl_hash_entry dflt_group_entry = {
	.id = 0, .name = "default"
};

static struct rtnl_hash_entry *rtnl_group_hash[256] = {
	[0] = &dflt_group_entry,
};

static int rtnl_group_init;

static void rtnl_group_initialize(void)
{
	rtnl_group_init = 1;
	rtnl_hash_initialize(CONFDIR "/group",
			     rtnl_group_hash, 256);
}

int rtnl_group_a2n(int *id, const char *arg)
{
	static const char *cache;
	static unsigned long res;
	struct rtnl_hash_entry *entry;
	char *end;
	int i;

	if (cache && strcmp(cache, arg) == 0) {
		*id = res;
		return 0;
	}

	if (!rtnl_group_init)
		rtnl_group_initialize();

	for (i = 0; i < 256; i++) {
		entry = rtnl_group_hash[i];
		while (entry && strcmp(entry->name, arg))
			entry = entry->next;
		if (entry) {
			cache = entry->name;
			res = entry->id;
			*id = res;
			return 0;
		}
	}

	i = strtol(arg, &end, 0);
	if (!end || end == arg || *end || i < 0)
		return -1;
	*id = i;
	return 0;
}

const char *rtnl_group_n2a(int id, char *buf, int len)
{
	struct rtnl_hash_entry *entry;
	int i;

	if (!rtnl_group_init)
		rtnl_group_initialize();

	for (i = 0; !numeric && i < 256; i++) {
		entry = rtnl_group_hash[i];

		while (entry) {
			if (entry->id == id)
				return entry->name;
			entry = entry->next;
		}
	}

	snprintf(buf, len, "%d", id);
	return buf;
}

static char *nl_proto_tab[256] = {
	[NETLINK_ROUTE]          = "rtnl",
	[NETLINK_UNUSED]         = "unused",
	[NETLINK_USERSOCK]       = "usersock",
	[NETLINK_FIREWALL]       = "fw",
	[NETLINK_SOCK_DIAG]      = "tcpdiag",
	[NETLINK_NFLOG]          = "nflog",
	[NETLINK_XFRM]           = "xfrm",
	[NETLINK_SELINUX]        = "selinux",
	[NETLINK_ISCSI]          = "iscsi",
	[NETLINK_AUDIT]          = "audit",
	[NETLINK_FIB_LOOKUP]     = "fiblookup",
	[NETLINK_CONNECTOR]      = "connector",
	[NETLINK_NETFILTER]      = "nft",
	[NETLINK_IP6_FW]         = "ip6fw",
	[NETLINK_DNRTMSG]        = "dec-rt",
	[NETLINK_KOBJECT_UEVENT] = "uevent",
	[NETLINK_GENERIC]        = "genl",
	[NETLINK_SCSITRANSPORT]  = "scsi-trans",
	[NETLINK_ECRYPTFS]       = "ecryptfs",
	[NETLINK_RDMA]           = "rdma",
	[NETLINK_CRYPTO]         = "crypto",
};

static int nl_proto_init;

static void nl_proto_initialize(void)
{
	nl_proto_init = 1;
	rtnl_tab_initialize(CONFDIR "/nl_protos",
			    nl_proto_tab, 256);
}

const char *nl_proto_n2a(int id, char *buf, int len)
{
	if (id < 0 || id >= 256 || numeric) {
		snprintf(buf, len, "%d", id);
		return buf;
	}

	if (!nl_proto_init)
		nl_proto_initialize();

	if (nl_proto_tab[id])
		return nl_proto_tab[id];

	snprintf(buf, len, "%u", id);
	return buf;
}

int nl_proto_a2n(__u32 *id, const char *arg)
{
	static char *cache;
	static unsigned long res;
	char *end;
	int i;

	if (cache && strcmp(cache, arg) == 0) {
		*id = res;
		return 0;
	}

	if (!nl_proto_init)
		nl_proto_initialize();

	for (i = 0; i < 256; i++) {
		if (nl_proto_tab[i] &&
		    strcmp(nl_proto_tab[i], arg) == 0) {
			cache = nl_proto_tab[i];
			res = i;
			*id = res;
			return 0;
		}
	}

	res = strtoul(arg, &end, 0);
	if (!end || end == arg || *end || res > 255)
		return -1;
	*id = res;
	return 0;
}

#define PROTODOWN_REASON_NUM_BITS 32
static char *protodown_reason_tab[PROTODOWN_REASON_NUM_BITS] = {
};

static int protodown_reason_init;

static void protodown_reason_initialize(void)
{
	struct dirent *de;
	DIR *d;

	protodown_reason_init = 1;

	d = opendir(CONFDIR "/protodown_reasons.d");
	if (!d)
		return;

	while ((de = readdir(d)) != NULL) {
		char path[PATH_MAX];
		size_t len;

		if (*de->d_name == '.')
			continue;

		/* only consider filenames ending in '.conf' */
		len = strlen(de->d_name);
		if (len <= 5)
			continue;
		if (strcmp(de->d_name + len - 5, ".conf"))
			continue;

		snprintf(path, sizeof(path), CONFDIR "/protodown_reasons.d/%s",
			 de->d_name);
		rtnl_tab_initialize(path, protodown_reason_tab,
				    PROTODOWN_REASON_NUM_BITS);
	}
	closedir(d);
}

int protodown_reason_n2a(int id, char *buf, int len)
{
	if (id < 0 || id >= PROTODOWN_REASON_NUM_BITS)
		return -1;

	if (numeric) {
		snprintf(buf, len, "%d", id);
		return 0;
	}

	if (!protodown_reason_init)
		protodown_reason_initialize();

	if (protodown_reason_tab[id])
		snprintf(buf, len, "%s", protodown_reason_tab[id]);
	else
		snprintf(buf, len, "%d", id);

	return 0;
}

int protodown_reason_a2n(__u32 *id, const char *arg)
{
	static char *cache;
	static unsigned long res;
	char *end;
	int i;

	if (cache && strcmp(cache, arg) == 0) {
		*id = res;
		return 0;
	}

	if (!protodown_reason_init)
		protodown_reason_initialize();

	for (i = 0; i < PROTODOWN_REASON_NUM_BITS; i++) {
		if (protodown_reason_tab[i] &&
		    strcmp(protodown_reason_tab[i], arg) == 0) {
			cache = protodown_reason_tab[i];
			res = i;
			*id = res;
			return 0;
		}
	}

	res = strtoul(arg, &end, 0);
	if (!end || end == arg || *end || res >= PROTODOWN_REASON_NUM_BITS)
		return -1;
	*id = res;
	return 0;
}
