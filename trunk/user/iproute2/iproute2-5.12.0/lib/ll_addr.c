/*
 * ll_addr.c
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
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include <linux/netdevice.h>
#include <linux/if_arp.h>
#include <linux/sockios.h>

#include "rt_names.h"
#include "utils.h"

const char *ll_addr_n2a(const unsigned char *addr, int alen, int type,
			char *buf, int blen)
{
	int i;
	int l;

	if (alen == 4 &&
	    (type == ARPHRD_TUNNEL || type == ARPHRD_SIT
	     || type == ARPHRD_IPGRE))
		return inet_ntop(AF_INET, addr, buf, blen);

	if (alen == 16 && (type == ARPHRD_TUNNEL6 || type == ARPHRD_IP6GRE))
		return inet_ntop(AF_INET6, addr, buf, blen);

	snprintf(buf, blen, "%02x", addr[0]);
	for (i = 1, l = 2; i < alen && l < blen; i++, l += 3)
		snprintf(buf + l, blen - l, ":%02x", addr[i]);
	return buf;
}

/*NB: lladdr is char * (rather than u8 *) because sa_data is char * (1003.1g) */
int ll_addr_a2n(char *lladdr, int len, const char *arg)
{
	if (strchr(arg, '.')) {
		inet_prefix pfx;
		if (get_addr_1(&pfx, arg, AF_INET)) {
			fprintf(stderr, "\"%s\" is invalid lladdr.\n", arg);
			return -1;
		}
		if (len < 4)
			return -1;
		memcpy(lladdr, pfx.data, 4);
		return 4;
	} else {
		int i;

		for (i = 0; i < len; i++) {
			int temp;
			char *cp = strchr(arg, ':');
			if (cp) {
				*cp = 0;
				cp++;
			}
			if (sscanf(arg, "%x", &temp) != 1) {
				fprintf(stderr, "\"%s\" is invalid lladdr.\n",
					arg);
				return -1;
			}
			if (temp < 0 || temp > 255) {
				fprintf(stderr, "\"%s\" is invalid lladdr.\n",
					arg);
				return -1;
			}
			lladdr[i] = temp;
			if (!cp)
				break;
			arg = cp;
		}
		return i + 1;
	}
}
