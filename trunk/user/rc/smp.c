/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include <stdarg.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

#include "rc.h"

static void
irq_affinity_set(int irq_num, int cpu)
{
	char proc_path[40];

	snprintf(proc_path, sizeof(proc_path), "/proc/irq/%d/smp_affinity", irq_num);
	fput_int(proc_path, cpu);
}

static void
rps_queue_set(const char *ifname, int cpu_mask)
{
	char proc_path[64], cmx[4];

	snprintf(proc_path, sizeof(proc_path), "/sys/class/net/%s/queues/rx-%d/rps_cpus", ifname, 0);
	snprintf(cmx, sizeof(cmx), "%x", cpu_mask);
	fput_string(proc_path, cmx);
}

static void
xps_queue_set(const char *ifname, int cpu_mask)
{
	char proc_path[64], cmx[4];

	snprintf(proc_path, sizeof(proc_path), "/sys/class/net/%s/queues/tx-%d/xps_cpus", ifname, 0);
	snprintf(cmx, sizeof(cmx), "%x", cpu_mask);
	fput_string(proc_path, cmx);
}

struct smp_irq_layout_t {
	unsigned int irq;
	unsigned int cpu_mask;
};

struct smp_trx_layout_t {
	const char* ifname;
	unsigned int cpu_mask;
};

#define SMP_MASK_CPU0	(1U << 0)
#define SMP_MASK_CPU1	(1U << 1)
#define SMP_MASK_CPU2	(1U << 2)
#define SMP_MASK_CPU3	(1U << 3)

#if defined (CONFIG_RALINK_MT7621)
#define GIC_OFFSET		7

#define GIC_IRQ_FE		(GIC_OFFSET+3)
#define GIC_IRQ_PCIE0	(GIC_OFFSET+4)
#define GIC_IRQ_PCIE1	(GIC_OFFSET+24)
#define GIC_IRQ_PCIE2	(GIC_OFFSET+25)
#define GIC_IRQ_SDXC	(GIC_OFFSET+20)
#define GIC_IRQ_XHCI	(GIC_OFFSET+22)
#define GIC_IRQ_EIP93	(GIC_OFFSET+19)

#define PPPOE_RPS_MAP	(SMP_MASK_CPU1|SMP_MASK_CPU2|SMP_MASK_CPU3)

static const struct smp_irq_layout_t mt7621a_irq[] = {
	{ GIC_IRQ_FE,    SMP_MASK_CPU1 },	/* GMAC  -> CPU:0, VPE:1 */
	{ GIC_IRQ_EIP93, SMP_MASK_CPU1 },	/* EIP93 -> CPU:0, VPE:1 */
	{ GIC_IRQ_PCIE0, SMP_MASK_CPU3 },	/* PCIe0 -> CPU:1, VPE:0 (usually rai0) */
	{ GIC_IRQ_PCIE1, SMP_MASK_CPU2 },	/* PCIe1 -> CPU:1, VPE:1 (usually ra0) */
	{ GIC_IRQ_PCIE2, SMP_MASK_CPU0 },	/* PCIe2 -> CPU:0, VPE:0 (usually ahci) */
	{ GIC_IRQ_SDXC,  SMP_MASK_CPU2 },	/* SDXC  -> CPU:1, VPE:0 */
	{ GIC_IRQ_XHCI,  SMP_MASK_CPU3 },	/* xHCI  -> CPU:1, VPE:1 */
};

static const struct smp_trx_layout_t mt7621a_rps[] = {
	{ IFNAME_MAC,		SMP_MASK_CPU0 | SMP_MASK_CPU2 },	/* eth2 */
	{ IFNAME_MAC2,		SMP_MASK_CPU0 | SMP_MASK_CPU2 },	/* eth3 */
	{ IFNAME_2G_MAIN,	SMP_MASK_CPU0 | SMP_MASK_CPU1 },
	{ IFNAME_2G_GUEST,	SMP_MASK_CPU0 | SMP_MASK_CPU1 },
	{ IFNAME_2G_APCLI,	SMP_MASK_CPU0 | SMP_MASK_CPU1 },
	{ IFNAME_2G_WDS0,	SMP_MASK_CPU0 | SMP_MASK_CPU1 },
	{ IFNAME_2G_WDS1,	SMP_MASK_CPU0 | SMP_MASK_CPU1 },
	{ IFNAME_2G_WDS2,	SMP_MASK_CPU0 | SMP_MASK_CPU1 },
	{ IFNAME_2G_WDS3,	SMP_MASK_CPU0 | SMP_MASK_CPU1 },
#if BOARD_HAS_5G_RADIO
	{ IFNAME_5G_MAIN,	SMP_MASK_CPU0 | SMP_MASK_CPU1 },
	{ IFNAME_5G_GUEST,	SMP_MASK_CPU0 | SMP_MASK_CPU1 },
	{ IFNAME_5G_APCLI,	SMP_MASK_CPU0 | SMP_MASK_CPU1 },
	{ IFNAME_5G_WDS0,	SMP_MASK_CPU0 | SMP_MASK_CPU1 },
	{ IFNAME_5G_WDS1,	SMP_MASK_CPU0 | SMP_MASK_CPU1 },
	{ IFNAME_5G_WDS2,	SMP_MASK_CPU0 | SMP_MASK_CPU1 },
	{ IFNAME_5G_WDS3,	SMP_MASK_CPU0 | SMP_MASK_CPU1 },
#endif
};

static const struct smp_trx_layout_t mt7621a_xps[] = {
	{ IFNAME_MAC,		SMP_MASK_CPU1 },	/* eth2 */
	{ IFNAME_MAC2,		SMP_MASK_CPU1 },	/* eth3 */
	{ IFNAME_2G_MAIN,	0 },
	{ IFNAME_2G_GUEST,	0 },
	{ IFNAME_2G_APCLI,	0 },
	{ IFNAME_2G_WDS0,	0 },
	{ IFNAME_2G_WDS1,	0 },
	{ IFNAME_2G_WDS2,	0 },
	{ IFNAME_2G_WDS3,	0 },
#if BOARD_HAS_5G_RADIO
	{ IFNAME_5G_MAIN,	0 },
	{ IFNAME_5G_GUEST,  0 },
	{ IFNAME_5G_APCLI,	0 },
	{ IFNAME_5G_WDS0,	0 },
	{ IFNAME_5G_WDS1,	0 },
	{ IFNAME_5G_WDS2,	0 },
	{ IFNAME_5G_WDS3,	0 },
#endif
};

#else
#error "undefined SoC with SMP!"
#endif

void
set_cpu_affinity(void)
{
	int i;

	/* set CPU affinity */
	for (i = 0; i < ARRAY_SIZE(mt7621a_irq); i++)
		irq_affinity_set(mt7621a_irq[i].irq, mt7621a_irq[i].cpu_mask);

	/* set network interfaces RPS/XPS mask */
	for (i = 0; i < ARRAY_SIZE(mt7621a_rps); i++) {
		if (!is_interface_exist(mt7621a_rps[i].ifname))
			continue;
		rps_queue_set(mt7621a_rps[i].ifname, mt7621a_rps[i].cpu_mask);
	}

	for (i = 0; i < ARRAY_SIZE(mt7621a_xps); i++) {
		if (!is_interface_exist(mt7621a_xps[i].ifname))
			continue;
		xps_queue_set(mt7621a_xps[i].ifname, mt7621a_xps[i].cpu_mask);
		
	}
}

void
set_vpn_balancing(const char *vpn_ifname, int is_server)
{
	return;
}

void
set_pppoe_balancing()
{
	rps_queue_set(IFNAME_MAC, PPPOE_RPS_MAP);
	rps_queue_set(IFNAME_MAC2, PPPOE_RPS_MAP);
}
