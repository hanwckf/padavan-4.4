#include <stdlib.h>             
#include <stdio.h>             
#include <string.h>           
#include <sys/ioctl.h>
#include <fcntl.h>
#include <getopt.h>
#include <unistd.h>

#include <generated/autoconf.h>

#include "hwnat_ioctl.h"

int HwNatDumpDport(void)
{
    int fd;
    struct hwnat_args opt;
  
    fd = open("/dev/"HW_NAT_DEVNAME, O_RDONLY);
    if (fd < 0)
    {
	printf("Open %s pseudo device failed\n","/dev/"HW_NAT_DEVNAME);
	return HWNAT_FAIL;
    }
    if(ioctl(fd, HW_NAT_DPORT, &opt)<0) {
	printf("HW_NAT_API: ioctl error\n");
	close(fd);
	return HWNAT_FAIL;
    }

    close(fd);
    return HWNAT_SUCCESS;
}
#if defined (CONFIG_HW_NAT_IPI)
int HwNatIPICtrlFromExtIf(struct hwnat_ipi_args *opt)
{
    int fd;

    fd = open("/dev/"HW_NAT_DEVNAME, O_RDONLY);
    if (fd < 0)
    {
	printf("Open %s pseudo device failed\n","/dev/"HW_NAT_DEVNAME);
	return HWNAT_FAIL;
    }

    if(ioctl(fd, HW_NAT_IPI_CTRL_FROM_EXTIF, opt)<0) {
	printf("HW_NAT_API: ioctl error\n");
	close(fd);
	return HWNAT_FAIL;
    }

    close(fd);
    return HWNAT_SUCCESS;
}

int HwNatIPICtrlFromPPEHit(struct hwnat_ipi_args *opt)
{
    int fd;

    fd = open("/dev/"HW_NAT_DEVNAME, O_RDONLY);
    if (fd < 0)
    {
	printf("Open %s pseudo device failed\n","/dev/"HW_NAT_DEVNAME);
	return HWNAT_FAIL;
    }

    if(ioctl(fd, HW_NAT_IPI_CTRL_FROM_PPEHIT, opt)<0) {
	printf("HW_NAT_API: ioctl error\n");
	close(fd);
	return HWNAT_FAIL;
    }

    close(fd);
    return HWNAT_SUCCESS;
}
#endif

int HwNatDelEntry(struct hwnat_tuple *opt)
{
    int fd;

    fd = open("/dev/"HW_NAT_DEVNAME, O_RDONLY);
    if (fd < 0)
    {
	printf("Open %s pseudo device failed\n","/dev/"HW_NAT_DEVNAME);
	return HWNAT_FAIL;
    }
    if(ioctl(fd, HW_NAT_DEL_ENTRY, opt)<0) {
	printf("HW_NAT_API: ioctl error\n");
	close(fd);
	return HWNAT_FAIL;
    }

    close(fd);
    return HWNAT_SUCCESS;

}

int HwNatAddEntry(struct hwnat_tuple *opt)
{
    int fd;

    fd = open("/dev/"HW_NAT_DEVNAME, O_RDONLY);
    if (fd < 0)
    {
	printf("Open %s pseudo device failed\n","/dev/"HW_NAT_DEVNAME);
	return HWNAT_FAIL;
    }

    if(ioctl(fd, HW_NAT_ADD_ENTRY, opt)<0) {
	printf("HW_NAT_API: ioctl error\n");
	close(fd);
	return HWNAT_FAIL;
    }

    close(fd);
    return HWNAT_SUCCESS;

}

#if defined (CONFIG_PPE_MIB)
int HwNatMibDumpEntry(unsigned int entry_num)
{
    struct hwnat_mib_args opt;
    int fd;
    opt.entry_num=entry_num;
    printf("!!!!!!!!HwNatMibDumpEntry\n");
    fd = open("/dev/"HW_NAT_DEVNAME, O_RDONLY);
    if (fd < 0)
    {
	printf("Open %s pseudo device failed\n","/dev/"HW_NAT_DEVNAME);
	return HWNAT_FAIL;
    }

    if(ioctl(fd, HW_NAT_MIB_DUMP, &opt)<0) {
	printf("HW_NAT_API: ioctl error\n");
	close(fd);
	return HWNAT_FAIL;
    }

    close(fd);
    return HWNAT_SUCCESS;
}
int HwNatMibDramDumpEntry(unsigned int entry_num)
{
    struct hwnat_mib_args opt;
    int fd;

    opt.entry_num=entry_num;

    fd = open("/dev/"HW_NAT_DEVNAME, O_RDONLY);
    if (fd < 0)
    {
	printf("Open %s pseudo device failed\n","/dev/"HW_NAT_DEVNAME);
	return HWNAT_FAIL;
    }

    if(ioctl(fd, HW_NAT_MIB_DRAM_DUMP, &opt)<0) {
	printf("HW_NAT_API: ioctl error\n");
	close(fd);
	return HWNAT_FAIL;
    }

    close(fd);
    return HWNAT_SUCCESS;
}

int HwNatMibGet(struct hwnat_tuple *opt)
{
    int fd;

    fd = open("/dev/"HW_NAT_DEVNAME, O_RDONLY);
    if (fd < 0)
    {
	printf("Open %s pseudo device failed\n","/dev/"HW_NAT_DEVNAME);
	return HWNAT_FAIL;
    }

    if(ioctl(fd, HW_NAT_MIB_GET, opt)<0) {
	printf("HW_NAT_API: ioctl error\n");
	close(fd);
	return HWNAT_FAIL;
    }

    close(fd);
    return HWNAT_SUCCESS;

}
#endif

int HwNatTblClear(void)
{
    struct hwnat_args opt;
    int fd;

    fd = open("/dev/"HW_NAT_DEVNAME, O_RDONLY);
    if (fd < 0)
    {
	printf("Open %s pseudo device failed\n","/dev/"HW_NAT_DEVNAME);
	return HWNAT_FAIL;
    }

    if(ioctl(fd, HW_NAT_TBL_CLEAR, &opt)<0) {
	printf("HW_NAT_API: ioctl error\n");
	close(fd);
	return HWNAT_FAIL;
    }

    close(fd);
    return HWNAT_SUCCESS;
}

int HwNatDumpEntry(unsigned int entry_num)
{
    struct hwnat_args opt;
    int fd;

    opt.entry_num=entry_num;

    fd = open("/dev/"HW_NAT_DEVNAME, O_RDONLY);
    if (fd < 0)
    {
	printf("Open %s pseudo device failed\n","/dev/"HW_NAT_DEVNAME);
	return HWNAT_FAIL;
    }

    if(ioctl(fd, HW_NAT_DUMP_ENTRY, &opt)<0) {
	printf("HW_NAT_API: ioctl error\n");
	close(fd);
	return HWNAT_FAIL;
    }

    close(fd);
    return HWNAT_SUCCESS;
}

int HwNatBindEntry(unsigned int entry_num)
{
    struct hwnat_args opt;
    int fd;

    opt.entry_num=entry_num;

    fd = open("/dev/"HW_NAT_DEVNAME, O_RDONLY);
    if (fd < 0)
    {
	printf("Open %s pseudo device failed\n","/dev/"HW_NAT_DEVNAME);
	return HWNAT_FAIL;
    }

    if(ioctl(fd, HW_NAT_BIND_ENTRY, &opt)<0) {
	printf("HW_NAT_API: ioctl error\n");
	close(fd);
	return HWNAT_FAIL;
    }

    close(fd);
    return HWNAT_SUCCESS;
}

int HwNatUnBindEntry(unsigned int entry_num)
{
    struct hwnat_args opt;
    int fd;

    opt.entry_num=entry_num;

    fd = open("/dev/"HW_NAT_DEVNAME, O_RDONLY);
    if (fd < 0)
    {
	printf("Open %s pseudo device failed\n","/dev/"HW_NAT_DEVNAME);
	return HWNAT_FAIL;
    }

    if(ioctl(fd, HW_NAT_UNBIND_ENTRY, &opt)<0) {
	printf("HW_NAT_API: ioctl error\n");
	close(fd);
	return  HWNAT_FAIL;
    }

    close(fd);
    return  HWNAT_SUCCESS;
}

int HwNatDropEntry(unsigned int entry_num)
{
    struct hwnat_args opt;
    int fd;

    opt.entry_num=entry_num;

    fd = open("/dev/"HW_NAT_DEVNAME, O_RDONLY);
    if (fd < 0)
    {
	printf("Open %s pseudo device failed\n","/dev/"HW_NAT_DEVNAME);
	return HWNAT_FAIL;
    }

    if(ioctl(fd, HW_NAT_DROP_ENTRY, &opt)<0) {
	printf("HW_NAT_API: ioctl error\n");
	close(fd);
	return  HWNAT_FAIL;
    }

    close(fd);
    return  HWNAT_SUCCESS;
}



int HwNatInvalidEntry(unsigned int entry_num)
{
    struct hwnat_args opt;
    int fd;

    opt.entry_num=entry_num;

    fd = open("/dev/"HW_NAT_DEVNAME, O_RDONLY);
    if (fd < 0)
    {
	printf("Open %s pseudo device failed\n","/dev/"HW_NAT_DEVNAME);
	return HWNAT_FAIL;
    }

    if(ioctl(fd, HW_NAT_INVALID_ENTRY, &opt)<0) {
	printf("HW_NAT_API: ioctl error\n");
	close(fd);
	return HWNAT_FAIL;
    }

    close(fd);
    return HWNAT_SUCCESS;
}

int HwNatCacheDumpEntry(void)
{

    struct hwnat_args opt;
    int fd;

    fd = open("/dev/"HW_NAT_DEVNAME, O_RDONLY);
    if (fd < 0)
    {
	printf("Open %s pseudo device failed\n","/dev/"HW_NAT_DEVNAME);
	return HWNAT_FAIL;
    }
    if(ioctl(fd, HW_NAT_DUMP_CACHE_ENTRY, &opt)<0) {


	printf("HW_NAT_API: ioctl error\n");
	close(fd);
	return HWNAT_FAIL;
    }

    close(fd);
    return HWNAT_SUCCESS;
}

int HwNatGetAGCnt(struct hwnat_ac_args *opt)
{
    int fd;

    fd = open("/dev/"HW_NAT_DEVNAME, O_RDONLY);
    if (fd < 0)
    {
	printf("Open %s pseudo device failed\n","/dev/"HW_NAT_DEVNAME);
	return HWNAT_FAIL;
    }

    if(ioctl(fd, HW_NAT_GET_AC_CNT, opt)<0) {
	printf("HW_NAT_API: ioctl error\n");
	close(fd);
	return HWNAT_FAIL;
    }

    close(fd);
    return HWNAT_SUCCESS;

}

int HwNatSetBindThreshold(struct hwnat_config_args *opt)
{
    int fd;

    fd = open("/dev/"HW_NAT_DEVNAME, O_RDONLY);
    if (fd < 0)
    {
	printf("Open %s pseudo device failed\n","/dev/"HW_NAT_DEVNAME);
	return HWNAT_FAIL;
    }

    if(ioctl(fd, HW_NAT_BIND_THRESHOLD, opt)<0) {
	printf("HW_NAT_API: ioctl error\n");
	close(fd);
	return HWNAT_FAIL;
    }

    close(fd);
    return HWNAT_SUCCESS;

}

int HwNatSetMaxEntryRateLimit(struct hwnat_config_args *opt)
{
    int fd;

    fd = open("/dev/"HW_NAT_DEVNAME, O_RDONLY);
    if (fd < 0)
    {
	printf("Open %s pseudo device failed\n","/dev/"HW_NAT_DEVNAME);
	return HWNAT_FAIL;
    }

    if(ioctl(fd, HW_NAT_MAX_ENTRY_LMT, opt)<0) {
	printf("HW_NAT_API: ioctl error\n");
	close(fd);
	return HWNAT_FAIL;
    }

    close(fd);
    return HWNAT_SUCCESS;

}


int HwNatSetRuleSize(struct hwnat_config_args *opt)
{
    int fd;

    fd = open("/dev/"HW_NAT_DEVNAME, O_RDONLY);
    if (fd < 0)
    {
	printf("Open %s pseudo device failed\n","/dev/"HW_NAT_DEVNAME);
	return HWNAT_FAIL;
    }

    if(ioctl(fd, HW_NAT_RULE_SIZE, opt)<0) {
	printf("HW_NAT_API: ioctl error\n");
	close(fd);
	return HWNAT_FAIL;
    }

    close(fd);
    return HWNAT_SUCCESS;

}

int HwNatSetKaInterval(struct hwnat_config_args *opt)
{
    int fd;

    fd = open("/dev/"HW_NAT_DEVNAME, O_RDONLY);
    if (fd < 0)
    {
	printf("Open %s pseudo device failed\n","/dev/"HW_NAT_DEVNAME);
	return HWNAT_FAIL;
    }

    if(ioctl(fd, HW_NAT_KA_INTERVAL, opt)<0) {
	printf("HW_NAT_API: ioctl error\n");
	close(fd);
	return HWNAT_FAIL;
    }

    close(fd);
    return HWNAT_SUCCESS;

}

int HwNatSetUnbindLifeTime(struct hwnat_config_args *opt)
{
    int fd;

    fd = open("/dev/"HW_NAT_DEVNAME, O_RDONLY);
    if (fd < 0)
    {
	printf("Open %s pseudo device failed\n","/dev/"HW_NAT_DEVNAME);
	return HWNAT_FAIL;
    }

    if(ioctl(fd, HW_NAT_UB_LIFETIME, opt)<0) {
	printf("HW_NAT_API: ioctl error\n");
	close(fd);
	return HWNAT_FAIL;
    }

    close(fd);
    return HWNAT_SUCCESS;

}

int HwNatSetBindLifeTime(struct hwnat_config_args *opt)
{
    int fd;

    fd = open("/dev/"HW_NAT_DEVNAME, O_RDONLY);
    if (fd < 0)
    {
	printf("Open %s pseudo device failed\n","/dev/"HW_NAT_DEVNAME);
	return HWNAT_FAIL;
    }

    if(ioctl(fd, HW_NAT_BIND_LIFETIME, opt)<0) {
	printf("HW_NAT_API: ioctl error\n");
	close(fd);
	return HWNAT_FAIL;
    }

    close(fd);
    return HWNAT_SUCCESS;

}

int HwNatSetVID(struct hwnat_config_args *opt)
{
    int fd;

    fd = open("/dev/"HW_NAT_DEVNAME, O_RDONLY);
    if (fd < 0)
    {
	printf("Open %s pseudo device failed\n","/dev/"HW_NAT_DEVNAME);
	return HWNAT_FAIL;
    }

    if(ioctl(fd, HW_NAT_VLAN_ID, opt)<0) {
	printf("HW_NAT_API: ioctl error\n");
	close(fd);
	return HWNAT_FAIL;
    }

    close(fd);
    return HWNAT_SUCCESS;
}

int HwNatSetBindDir(struct hwnat_config_args *opt)
{
    int fd;

    fd = open("/dev/"HW_NAT_DEVNAME, O_RDONLY);
    if (fd < 0)
    {
	printf("Open %s pseudo device failed\n","/dev/"HW_NAT_DEVNAME);
	return HWNAT_FAIL;
    }

    if(ioctl(fd, HW_NAT_BIND_DIRECTION, opt)<0) {
	printf("HW_NAT_API: ioctl error\n");
	close(fd);
	return HWNAT_FAIL;
    }

    close(fd);
    return HWNAT_SUCCESS;
}

int HwNatGetAllEntries(struct hwnat_args *opt)
{
    int fd=0;

    fd = open("/dev/"HW_NAT_DEVNAME, O_RDONLY);
    if (fd < 0)
    {
	printf("Open %s pseudo device failed\n","/dev/"HW_NAT_DEVNAME);
	return HWNAT_FAIL;
    }

    if(ioctl(fd, HW_NAT_GET_ALL_ENTRIES, opt)<0) {
	printf("HW_NAT_API: ioctl error\n");
	close(fd);
	return HWNAT_FAIL;
    }

    close(fd);
    

    return HWNAT_SUCCESS;

}

int HwNatDebug(unsigned int debug)
{
    struct hwnat_args opt;
    int fd;

    opt.debug=debug;

    fd = open("/dev/"HW_NAT_DEVNAME, O_RDONLY);
    if (fd < 0)
    {
	printf("Open %s pseudo device failed\n","/dev/"HW_NAT_DEVNAME);
	return HWNAT_FAIL;
    }

    if(ioctl(fd, HW_NAT_DEBUG, &opt)<0) {
	printf("HW_NAT_API: ioctl error\n");
	close(fd);
	return HWNAT_FAIL;
    }

    close(fd);
    return HWNAT_SUCCESS;
}

#if 0
int HwNatSwitchDsliteMape(unsigned int swit)
{
	struct hwnat_args opt;
    int fd;

    opt.swit=swit;

    fd = open("/dev/"HW_NAT_DEVNAME, O_RDONLY);
    if (fd < 0)
    {
	printf("Open %s pseudo device failed\n","/dev/"HW_NAT_DEVNAME);
	return HWNAT_FAIL;
    }

    if(ioctl(fd, HW_NAT_SWITCH_DSL_MAPE, &opt)<0) {
	printf("HW_NAT_API: ioctl error\n");
	close(fd);
	return HWNAT_FAIL;
    }

    close(fd);
    return HWNAT_SUCCESS;
}
#endif

#ifdef CONFIG_PPE_MCAST
int HwNatMcastIns(struct hwnat_mcast_args *opt)
{
    int fd;

    fd = open("/dev/"HW_NAT_DEVNAME, O_RDONLY);
    if (fd < 0)
    {
	printf("Open %s pseudo device failed\n","/dev/"HW_NAT_DEVNAME);
	return HWNAT_FAIL;
    }

    if(ioctl(fd, HW_NAT_MCAST_INS, opt)<0) {
	printf("HW_NAT_API: ioctl error\n");
	close(fd);
	return HWNAT_FAIL;
    }

    close(fd);
    return HWNAT_SUCCESS;
}

int HwNatMcastDel(struct hwnat_mcast_args *opt)
{
    int fd;

    fd = open("/dev/"HW_NAT_DEVNAME, O_RDONLY);
    if (fd < 0)
    {
	printf("Open %s pseudo device failed\n","/dev/"HW_NAT_DEVNAME);
	return HWNAT_FAIL;
    }

    if(ioctl(fd, HW_NAT_MCAST_DEL, opt)<0) {
	printf("HW_NAT_API: ioctl error\n");
	close(fd);
	return HWNAT_FAIL;
    }

    close(fd);
    return HWNAT_SUCCESS;
}

int HwNatMcastDump(void)
{
    int fd;

    fd = open("/dev/"HW_NAT_DEVNAME, O_RDONLY);
    if (fd < 0)
    {
	printf("Open %s pseudo device failed\n","/dev/"HW_NAT_DEVNAME);
	return HWNAT_FAIL;
    }

    if(ioctl(fd, HW_NAT_MCAST_DUMP, NULL)<0) {
	printf("HW_NAT_API: ioctl error\n");
	close(fd);
	return HWNAT_FAIL;
    }

    close(fd);
    return HWNAT_SUCCESS;
}
#endif
