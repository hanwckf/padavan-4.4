/*
 * tc_red.c		RED maintenance routines.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Authors:	Alexey Kuznetsov, <kuznet@ms2.inr.ac.ru>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include "utils.h"
#include "tc_core.h"
#include "tc_util.h"
#include "tc_red.h"

/*
   Plog = log(prob/(qmax - qmin))
 */
int tc_red_eval_P(unsigned int qmin, unsigned int qmax, double prob)
{
	int i = qmax - qmin;

	if (!i)
		return 0;
	if (i < 0)
		return -1;

	prob /= i;

	for (i = 0; i < 32; i++) {
		if (prob > 1.0)
			break;
		prob *= 2;
	}
	if (i >= 32)
		return -1;
	return i;
}

/*
   burst + 1 - qmin/avpkt < (1-(1-W)^burst)/W
 */

int tc_red_eval_ewma(unsigned int qmin, unsigned int burst, unsigned int avpkt)
{
	int wlog = 1;
	double W = 0.5;
	double a = (double)burst + 1 - (double)qmin/avpkt;

	if (a < 1.0) {
		fprintf(stderr, "tc_red_eval_ewma() burst %u is too small ? Try burst %u\n",
				burst, 1 + qmin/avpkt);
		return -1;
	}
	for (wlog = 1; wlog < 32; wlog++, W /= 2) {
		if (a <= (1 - pow(1-W, burst))/W)
			return wlog;
	}
	return -1;
}

/*
   Stab[t>>Scell_log] = -log(1-W) * t/xmit_time
 */

int tc_red_eval_idle_damping(int Wlog, unsigned int avpkt, unsigned int bps, __u8 *sbuf)
{
	double xmit_time = tc_calc_xmittime(bps, avpkt);
	double lW = -log(1.0 - 1.0/(1<<Wlog))/xmit_time;
	double maxtime = 31/lW;
	int clog;
	int i;

	for (clog = 0; clog < 32; clog++) {
		if (maxtime/(1<<clog) < 512)
			break;
	}
	if (clog >= 32)
		return -1;

	sbuf[0] = 0;
	for (i = 1; i < 255; i++) {
		sbuf[i] = (i<<clog)*lW;
		if (sbuf[i] > 31)
			sbuf[i] = 31;
	}
	sbuf[255] = 31;
	return clog;
}

void tc_red_print_flags(__u32 flags)
{
	if (flags & TC_RED_ECN)
		print_bool(PRINT_ANY, "ecn", "ecn ", true);
	else
		print_bool(PRINT_ANY, "ecn", NULL, false);

	if (flags & TC_RED_HARDDROP)
		print_bool(PRINT_ANY, "harddrop", "harddrop ", true);
	else
		print_bool(PRINT_ANY, "harddrop", NULL, false);

	if (flags & TC_RED_ADAPTATIVE)
		print_bool(PRINT_ANY, "adaptive", "adaptive ", true);
	else
		print_bool(PRINT_ANY, "adaptive", NULL, false);

	if (flags & TC_RED_NODROP)
		print_bool(PRINT_ANY, "nodrop", "nodrop ", true);
	else
		print_bool(PRINT_ANY, "nodrop", NULL, false);
}
