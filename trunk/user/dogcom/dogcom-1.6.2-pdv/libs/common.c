#ifdef linux

/*
 * 一些通用的代码
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include "common.h"

// #ifdef LINUX
#include <unistd.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <sys/select.h>
#include <net/if_arp.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netpacket/packet.h>
#define PATH_SEP   '/'
// #elif WIN32
// # include <windows.h>
// # include <pcap.h>
// # define PATH_SEP   '\\'
// #endif


extern int getexedir(char *exedir)
{
// #ifdef LINUX
    int cnt = readlink("/proc/self/exe", exedir, EXE_PATH_MAX);
// #elif WIN32
//     int cnt = GetModuleFileName(NULL, exedir, EXE_PATH_MAX);
// #endif
    if (cnt < 0 || cnt >= EXE_PATH_MAX)
        return -1;
    _D("exedir: %s\n", exedir);
    char *end = strrchr(exedir, PATH_SEP);
    if (!end) return -1;
    *(end+1) = '\0';
    _D("exedir: %s\n", exedir);
    return 0;
}

extern int mac_equal(uchar const *mac1, uchar const *mac2)
{
	int i;
	for (i = 0; i < ETH_ALEN; ++i) {
		if (mac1[i] != mac2[i])
			return 0;
	}

	return 1;
}
extern int ip_equal(int type, void const *ip1, void const *ip2)
{
	uchar const *p1 = (uchar const*)ip1;
	uchar const *p2 = (uchar const*)ip2;
	int len = 4;
	if (AF_INET6 == type) {
		len = 16;
	}
	int i;
	for (i = 0; i < len; ++i) {
		if (p1[i] != p2[i])
			return 0;
	}
	return 1;
}

static int is_filter(char const *ifname)
{
	/* 过滤掉无线，虚拟机接口等 */
	char const *filter[] = {
		/* windows */
		"Wireless", "Microsoft",
		"Virtual",
		/* linux */
		"lo", "wlan", "vboxnet",
		"ifb", "gre", "teql",
		"br", "imq", "ra",
		"wds", "sit", "apcli",
	};
	unsigned int i;
	for (i = 0; i < ARRAY_SIZE(filter); ++i) {
		if (strstr(ifname, filter[i]))
			return 1;
	}
	return 0;
}
// #ifdef LINUX
static char *get_ifname_from_buff(char *buff)
{
	char *s;
	while (isspace(*buff))
		++buff;
	s = buff;
	while (':' != *buff && '\0' != *buff)
		++buff;
	*buff = '\0';
	return s;
}
// #endif
/*
 * 获取所有网络接口
 * ifnames 实际获取的接口
 * cnt     两个作用，1：传入表示ifnames最多可以存储的接口个数
 *                   2：返回表示实际获取了的接口个数
 * 返回接口个数在cnt里
 * @return: >=0  成功，实际获取的接口个数
 *          -1 获取失败
 *          -2 cnt过小
 */
extern int getall_ifs(iflist_t *ifs, int *cnt)
{
	int i = 0;
	if (!ifs || *cnt <= 0) return -2;

// #ifdef LINUX    /* linux (unix osx?) */
#define _PATH_PROCNET_DEV "/proc/net/dev"
#define BUFF_LINE_MAX	(1024)
	char buff[BUFF_LINE_MAX];
	FILE *fd = fopen(_PATH_PROCNET_DEV, "r");
	char *name;
	if (NULL == fd) {
		perror("fopen");
		return -1;
	}
	/* _PATH_PROCNET_DEV文件格式如下,...表示后面我们不关心
	 * Inter-|   Receive ...
	 * face |bytes    packets ...
	 * eth0: 147125283  119599 ...
	 * wlan0:  229230    2635   ...
	 * lo: 10285509   38254  ...
	 */
	/* 略过开始两行 */
	fgets(buff, BUFF_LINE_MAX, fd);
	fgets(buff, BUFF_LINE_MAX, fd);
	while (NULL != fgets(buff, BUFF_LINE_MAX, fd)) {
		name = get_ifname_from_buff(buff);
		_D("%s\n", name);
		/* 过滤无关网络接口 */
		if (is_filter(name)) {
			_D("filtered %s.\n", name);
			continue;
		}
		strncpy(ifs[i].name, name, IFNAMSIZ);
		_D("ifs[%d].name: %s\n", i, ifs[i].name);
		++i;
		if (i >= *cnt) {
			fclose(fd);
			return -2;
		}
	}
	fclose(fd);

// #elif WIN32
// 	pcap_if_t *alldevs;
// 	char errbuf[PCAP_ERRBUF_SIZE];
// 	if (-1 == pcap_findalldevs(&alldevs, errbuf)) {
// 		_M("Get interfaces handler error: %s\n", errbuf);
// 		return -1;
// 	}
// 	for (pcap_if_t *d = alldevs; d; d = d->next) {
// 		if (is_filter(d->description)) {
// 			_D("filtered %s.\n", d->description);
// 			continue;
// 		}
// 		if (i >= *cnt) return -2;
// 		strncpy(ifs[i].name, d->name, IFNAMSIZ);
// 		strncpy(ifs[i].desc, d->description, IFDESCSIZ);
// 		++i;
// 	}
// 	pcap_freealldevs(alldevs);
// #endif

	*cnt = i;
	return i;
}

extern char const *format_time(void)
{
	static char buff[FORMAT_TIME_MAX];
	time_t rawtime;
	struct tm *timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	if (NULL == timeinfo) return NULL;
	strftime(buff, sizeof(buff), "%Y-%m-%d %H:%M:%S", timeinfo);

	return buff;
}
extern int copy(char const *f1, char const *f2)
{
	if (NULL == f1 || NULL == f2) return -1;
	FILE *src, *dst;
	src = fopen(f1, "r");
	dst = fopen(f2, "w");
	if (NULL == src || NULL == dst) return -1;
	char buff[1024];
	int n;
	while (0 < (n = fread(buff, 1, 1024, src)))
		fwrite(buff, 1, n, dst);

	fclose(src);
	fclose(dst);

	return 0;
}
/*
 * 本地是否是小端序
 * @return: !0: 是
 *           0: 不是(大端序)
 */
static int islsb()
{
	static uint16 a = 0x0001;
	return (int)(*(uchar*)&a);
}
static uint16 exorders(uint16 n)
{
	return ((n>>8)|(n<<8));
}
static uint32 exorderl(uint32 n)
{
	return (n>>24)|((n&0x00ff0000)>>8)|((n&0x0000ff00)<<8)|(n<<24);
}
extern uint16 htols(uint16 n)
{
	return islsb()?n:exorders(n);
}
extern uint16 htoms(uint16 n)
{
	return islsb()?exorders(n):n;
}
extern uint16 ltohs(uint16 n)
{
	return islsb()?n:exorders(n);
}
extern uint16 mtohs(uint16 n)
{
	return islsb()?exorders(n):n;
}
extern uint32 htoll(uint32 n)
{
	return islsb()?n:exorderl(n);
}
extern uint32 htoml(uint32 n)
{
	return islsb()?exorderl(n):n;
}
extern uint32 ltohl(uint32 n)
{
	return islsb()?n:exorderl(n);
}
extern uint32 mtohl(uint32 n)
{
	return islsb()?exorderl(n):n;
}
extern uchar const *format_mac(uchar const *macarr)
{
	static uchar formatmac[] =
		"xx:xx:xx:xx:xx:xx";
	if (NULL == macarr)
		return NULL;
	sprintf((char*)formatmac, "%.2X:%.2X:%.2X:%.2X:%.2X:%.2X",
			macarr[0], macarr[1], macarr[2],
			macarr[3], macarr[4], macarr[5]);
	return formatmac;
}
/*
 * 以16进制打印数据
 */
extern void format_data(uchar const *d, size_t len)
{
	int i;
	for (i = 0; i < (long)len; ++i) {
		if (i != 0 && i%16 == 0)
			_M("\n");
		_M("%02x ", d[i]);
	}
	_M("\n");
}

#ifdef LINUX
/*
 * 返回t1-t0的时间差
 * 由于这里精度没必要达到ns，故返回相差微秒ms
 * @return: 时间差，单位微秒(1s == 1000ms)
 */
extern long difftimespec(struct timespec t1, struct timespec t0)
{
	long d = t1.tv_sec-t0.tv_sec;
	d *= 1000;
	d += (t1.tv_nsec-t0.tv_nsec)/(long)(1e6);
	return d;
}

/*
 * 判断网络是否连通
 * 最长延时3s，也就是说如果3s内没有检测到数据回应，那么认为网络不通
 * TODO 使用icmp协议判断
 * @return: !0: 连通
 *           0: 没有连通
 */
extern int isnetok(char const *ifname)
{
	static char baidu[] = "baidu.com";
	sleep(100);
	return 1;
}

/*
 * 休眠ms微秒
 */
extern void msleep(long ms)
{
	struct timeval tv;
	tv.tv_sec = ms/1000;
	tv.tv_usec = ms%1000*1000;
	select(0, 0, 0, 0, &tv);
}
#endif /* LINUX */

#endif