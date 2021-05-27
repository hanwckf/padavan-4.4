/* $Id: getifstats.c,v 1.13 2015/06/09 15:33:24 nanard Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006-2013 Thomas Bernard
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <time.h>

#include "../config.h"
#include "../getifstats.h"

#ifdef GET_WIRELESS_STATS
#include <unistd.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <linux/wireless.h>
#endif /* GET_WIRELESS_STATS */

/* that is the answer */
#define BAUDRATE_DEFAULT 4200000

int
getifstats(const char * ifname, struct ifdata * data)
{
	FILE *f;
	char line[512];
	char * p;
	int i;
	int r = -1;
#if 0
	char fname[64];
#endif
#ifdef ENABLE_GETIFSTATS_CACHING
	static time_t cache_timestamp = 0;
	static struct ifdata cache_data;
	time_t current_time;
#endif /* ENABLE_GETIFSTATS_CACHING */
	if(!data)
		return -1;
	data->baudrate = BAUDRATE_DEFAULT;
	data->opackets = 0;
	data->ipackets = 0;
	data->obytes = 0;
	data->ibytes = 0;
	if(!ifname || ifname[0]=='\0')
		return -1;
#ifdef ENABLE_GETIFSTATS_CACHING
	current_time = time(NULL);
	if(current_time == ((time_t)-1)) {
		syslog(LOG_ERR, "getifstats() : time() error : %m");
	} else {
		if(current_time < cache_timestamp + GETIFSTATS_CACHING_DURATION) {
			/* return cached data */
			memcpy(data, &cache_data, sizeof(struct ifdata));
			return 0;
		}
	}
#endif /* ENABLE_GETIFSTATS_CACHING */
	f = fopen("/proc/net/dev", "r");
	if(!f) {
		syslog(LOG_ERR, "getifstats() : cannot open /proc/net/dev : %m");
		return -1;
	}
	/* discard the two header lines */
	if(!fgets(line, sizeof(line), f) || !fgets(line, sizeof(line), f)) {
		syslog(LOG_ERR, "getifstats() : error reading /proc/net/dev : %m");
	}
	while(fgets(line, sizeof(line), f)) {
		p = line;
		while(*p==' ') p++;
		i = 0;
		while(ifname[i] == *p) {
			p++; i++;
		}
		/* TODO : how to handle aliases ? */
		if(ifname[i] || *p != ':')
			continue;
		p++;
		while(*p==' ') p++;
		data->ibytes = strtoul(p, &p, 0);
		while(*p==' ') p++;
		data->ipackets = strtoul(p, &p, 0);
		/* skip 6 columns */
		for(i=6; i>0 && *p!='\0'; i--) {
			while(*p==' ') p++;
			while(*p!=' ' && *p) p++;
		}
		while(*p==' ') p++;
		data->obytes = strtoul(p, &p, 0);
		while(*p==' ') p++;
		data->opackets = strtoul(p, &p, 0);
		r = 0;
		break;
	}
	fclose(f);
#if 0 /* Disable get speed */
	/* get interface speed */
	/* NB! some interfaces, like ppp, don't support speed queries */
	snprintf(fname, sizeof(fname), "/sys/class/net/%s/speed", ifname);
	f = fopen(fname, "r");
	if(f) {
		if(fgets(line, sizeof(line), f)) {
			i = atoi(line);	/* 65535 means unknown */
			if(i > 0 && i < 65535)
				data->baudrate = 1000000*i;
		}
		fclose(f);
	} else {
		syslog(LOG_INFO, "cannot read %s file : %m", fname);
	}
#endif
#ifdef GET_WIRELESS_STATS
	if(data->baudrate == BAUDRATE_DEFAULT) {
		struct iwreq iwr;
		int s;
		s = socket(AF_INET, SOCK_DGRAM, 0);
		if(s >= 0) {
			strncpy(iwr.ifr_name, ifname, IFNAMSIZ);
			if(ioctl(s, SIOCGIWRATE, &iwr) >= 0) {
				data->baudrate = iwr.u.bitrate.value;
			}
			close(s);
		}
	}
#endif /* GET_WIRELESS_STATS */
#ifdef ENABLE_GETIFSTATS_CACHING
	if(r==0 && current_time!=((time_t)-1)) {
		/* cache the new data */
		cache_timestamp = current_time;
		memcpy(&cache_data, data, sizeof(struct ifdata));
	}
#endif /* ENABLE_GETIFSTATS_CACHING */
	return r;
}

