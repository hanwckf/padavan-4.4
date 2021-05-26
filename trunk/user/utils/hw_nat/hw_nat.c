#include <stdlib.h>             
#include <stdio.h>             
#include <string.h>           
#include <sys/ioctl.h>
#include <fcntl.h>
#include <getopt.h>
#include <strings.h>
#include <unistd.h>

#include <generated/autoconf.h>

#include "hwnat_ioctl.h"
#include "hwnat_api.h"
#include "util.h"

void show_usage(void)
{
#if defined (CONFIG_HW_NAT_MANUAL_MODE)
    printf("Add Static Entry\n");
    printf("hw_nat -a -h [SMAC] -i [DMAC] -1 [type:hnapt(0)/ipv6_rout(5)]-j [Sip] -2 [Dip] -l [Sp] -3 [Dp]\n");
    printf("	   -n [New_Sip] -o [New_Dip] -p [New_Sp] -q [New_Dp] -4 [Vlan_layer] -5 [dscp]\n");
    printf("	   -s [VLAN1_ID] -S [VLAN2_ID] -v [Tcp/Udp] -w [OutIf:CPU/GE1/GE2]\n");
    printf("Ex(ipv4): hw_nat -a -h 00:0C:43:28:80:11 -i E0:18:77:BD:D5:18 -1 0 -j 10.10.10.3 -2 10.10.20.3\n");
    printf("-l 1000 -3 2000 -n 10.10.20.254 -o 10.10.20.3 -p 1000 -q 2000 -4 1 -s 1 -v Tcp -w GE2 -5 32\n\n");
    
    printf("Ex(ipv6): hw_nat -a -h 00:0C:43:28:02:14 -i 00:1B:21:00:9B:03 -1 5 -j 2001:1111:2222:3333:0000:0000:0000:2\n");
    printf("-2 2001:aaa:6401:101:8000:0000:0000:2 -l 3000 -3 2000 -4 0 -v Tcp -w GE1 -5 32\n\n");

    printf("Del Static Entry\n");
    printf("hw_nat -b -1 [type:hnapt(0)/ipv6_rout(5)] -j [Sip] -2 [Dip] -l [Sp] -3 [Dp] -v [Tcp/Udp] \n"); 
    printf("Ex: hw_nat -b -j 10.10.10.3 -k 10.10.20.3 -l 30 -m 40 -v Tcp\n\n");
#elif defined (CONFIG_HW_NAT_SEMI_AUTO_MODE)
    printf("Add semi-auto Entry\n");
    printf("hw_nat -a -1 [type:hnapt(0)/ipv6_rout(5)]-j [Sip] -2 [Dip] -l [Sp] -m [Dp]\n");
    printf("hw_nat -a -1 0 -j 10.10.10.3 -2 10.10.20.3 -l 1000 -3 2000 \n");
    printf("hw_nat -a -1 5 -j 2001:1111:2222:3333:0000:0000:0000:2 -2 2001:aaa:6401:101:8000:0000:0000:2 -l 3000 -3 2000\n");
    printf("Delete semi-auto Entry\n");
    printf("hw_nat -b -1 0 -j 10.10.10.3 -2 10.10.20.3 -l 1000 -3 2000\n");
    printf("hw_nat -b -1 5 -j 2001:1111:2222:3333:0000:0000:0000:2 -2 2001:aaa:6401:101:8000:0000:0000:2 -l 3000 -3 2000\n");
#endif
    printf("Show Foe Entry\n");
    printf("hw_nat -c [entry_num]\n");
    printf("Ex: hw_nat -c 1234\n\n");
    
    printf("Set Debug Level (0:disable) \n");
    printf("hw_nat -d [0~7]\n");
    printf("Ex: hw_nat -d \n\n");
    
    printf("Show All Foe Invalid Entry\n");
    printf("Ex: hw_nat -e\n\n");
    
    printf("Show All Foe Unbinded Entry\n");
    printf("Ex: hw_nat -f\n\n");
    
    printf("Show All Foe Binded Entry\n");
    printf("Ex: hw_nat -g\n\n");

    printf("Unbind Foe Entry\n");
    printf("hw_nat -x [entry_num]\n");
    printf("Ex: hw_nat -x 1234\n\n");

    printf("Set Foe Entry to PacketDrop\n");
    printf("hw_nat -k [entry_num]\n");
    printf("Ex: hw_nat -k 1234\n\n");


#if defined (CONFIG_PPE_MCAST)
    printf("Add member port in multicast entry\n");
    printf("Ex: hw_nat -B [vid] [mac] [px_en] [px_qos_en] [mc_qos_qid]\n\n");
    
    printf("Del member port multicast entry\n");
    printf("Ex: hw_nat -C [vid] [mac] [px_en] [px_qos_en] [mc_qos_qid]\n\n");
    
    printf("Dump all multicast entry\n");
    printf("Ex: hw_nat -D\n\n");
#endif

    printf("Set PPE Cofigurations:\n");
    printf("Set HNAT binding threshold per second (d=30)\n");
    printf("Ex: hw_nat -N [1~65535]\n\n");

    printf("Set HNAT Max entries allowed build when Free Entries>3/4, >1/2, <1/2 (d=100, 50, 25)\n");
    printf("Ex: hw_nat -O [1~16383][1~16383][1~16383]\n\n");


    printf("Set HNAT TCP/UDP keepalive interval (d=1, 1)(unit:4sec)\n");
    printf("Ex: hw_nat -Q [1~255][1~255]\n\n");

    printf("Set HNAT Life time of unbind entry (d=3)(unit:1Sec)\n");
    printf("Ex: hw_nat -T [1~255]\n\n");

    printf("Set HNAT Life time of Binded TCP/UDP/FIN entry(d=5, 5, 5)(unit:1Sec) \n");
    printf("Ex: hw_nat -U [1~65535][1~65535][1~65535]\n\n");

    printf("Set LAN/WAN port VLAN ID\n");
    printf("Ex: hw_nat -V [LAN_VID] [WAN_VID]\n\n");
    printf("Ex: hw_nat -V 1 2\n\n");
    
    printf("Only Speed UP (0=Upstream, 1=Downstream, 2=Bi-Direction) flow \n");
    printf("Ex: hw_nat -Z 1\n\n");

#if defined (CONFIG_RALINK_MT7620) || defined (CONFIG_RALINK_MT7621)
    printf("Switch Ds-Lite and Map-E(0=Ds-Lite, 1=Map-E,(d=0)):\n");
    printf("Ex: hw_nat -W [0/1]\n\n");
#endif
#if defined (CONFIG_PPE_MIB)
    printf("Get ppe entry mib counter\n");
    printf("hw_nat -M -1 [type:hnapt(0)/ipv6_rout(5)]-j [Sip] -2 [Dip] -l [Sp] -m [Dp]\n");
    printf("hw_nat -M -1 0 -j 10.10.10.3 -2 10.10.20.3 -l 1000 -3 2000 \n");
    printf("hw_nat -M -1 5 -j 2001:1111:2222:3333:0000:0000:0000:2 -2 2001:aaa:6401:101:8000:0000:0000:2 -l 3000 -3 2000\n");
#endif
#if defined (CONFIG_HW_NAT_IPI)
    printf("Set HNAT IPI control from extif:\n");
    printf("EX: hw_nat -G [ipi_enable] [queue_thresh] [drop_pkt] [ipi_cnt_mod]\n\n");
    printf("Set HNAT IPI control from ppehit:\n");
    printf("EX: hw_nat -L [ipi_enable] [queue_thresh] [drop_pkt] [ipi_cnt_mod]\n\n");
    printf("hw_nat -G 1 1000 20000 5\n");
    printf("hw_nat -L 1 1000 20000 44\n");
#endif
}

int main(int argc, char *argv[])
{
    int opt;

    //char options[] = "aefgy?c:x:k:d:A:B:C:DN:O:P:Q:T:U:V:Z:";
    char options[] = "abefgyzMD?c:m:r:A:B:C:E:F:G:H:I:J:K:L:R:S:d:h:i:j:1:k:2:l:3:n:o:p:q:4:s:t:u:v:w:5:x:y:N:O:P:Q:T:U:W:Z:L:6";

    int fd, method = -1;
    int i=0;
    unsigned int entry_num;
    unsigned int debug;
    unsigned int type;
    unsigned int swit;
    struct hwnat_args *args;
    struct hwnat_tuple args2;
    struct hwnat_ac_args args3;
    struct hwnat_config_args args4;
#if defined (CONFIG_HW_NAT_IPI)    
    struct hwnat_ipi_args args6;
    struct hwnat_ipi_args args7;
#endif

#if defined (CONFIG_PPE_MCAST)
    struct hwnat_mcast_args args5;
    unsigned char mac[6];
#endif

    int	   result;

    fd = open("/dev/"HW_NAT_DEVNAME, O_RDONLY);
    if (fd < 0)
    {
	printf("Open %s pseudo device failed\n","/dev/"HW_NAT_DEVNAME);
	return 0;
    }

    if(argc < 2) {
	show_usage();
        close(fd);
	return 0;
    }

    /* Max table size is 16K */
    args=malloc(sizeof(struct hwnat_args)+sizeof(struct hwnat_tuple)*1024*16);
    if (NULL == args)
    {
	    printf(" Allocate memory for hwnat_args and hwnat_tuple failed.\n");
            close(fd);
	    return 0;
    }

    while ((opt = getopt (argc, argv, options)) != -1) {
	switch (opt) {
	case 'h':
		 str_to_mac(args2.smac, optarg);
		 break;
	case 'i':
		 str_to_mac(args2.dmac, optarg);
		 break;
	case '1':
		 type = strtoll(optarg, NULL, 10);
		 args2.pkt_type = type;
		 break;
	case 'j':
		if (type == 0) {
			str_to_ip(&args2.ing_sipv4, optarg);
		} else if(type == 5) {
			str_to_ipv6(&args2.ing_sipv6_0, optarg, 0);
			str_to_ipv6(&args2.ing_sipv6_1, optarg, 1);
			str_to_ipv6(&args2.ing_sipv6_2, optarg, 2);
			str_to_ipv6(&args2.ing_sipv6_3, optarg, 3);
		} else {
			printf("hwnat type error\n");
			return 0;
		}
		 break;
	case '2':
		if (type == 0) {
			str_to_ip(&args2.ing_dipv4, optarg);
		} else if(type == 5) {
			str_to_ipv6(&args2.ing_dipv6_0, optarg, 0);
			str_to_ipv6(&args2.ing_dipv6_1, optarg, 1);
			str_to_ipv6(&args2.ing_dipv6_2, optarg, 2);
			str_to_ipv6(&args2.ing_dipv6_3, optarg, 3);
		} else {
			printf("hwnat type error\n");
			return 0;
		}	
		 break;
	case 'l':
		 args2.ing_sp = strtoll(optarg, NULL, 10);
		 break;
	case '3':
		 args2.ing_dp = strtoll(optarg, NULL, 10);
		 break;
	case 'n':
		 str_to_ip(&args2.eg_sipv4, optarg);
		 break;
	case 'o':
		 str_to_ip(&args2.eg_dipv4, optarg);
		 break;
	case 'p':
		 args2.eg_sp = strtoll(optarg, NULL, 10);
		 break;
	case 'q':
		 args2.eg_dp = strtoll(optarg, NULL, 10);
		 break;
	case '4':
		 args2.vlan_layer = strtoll(optarg, NULL, 10);

		 break;
	case 's':
		 args2.vlan1 = strtoll(optarg, NULL, 10);
                 break;
	case 'S':
		 args2.vlan2 = strtoll(optarg, NULL, 10);
		 break;
	case 't':
		 if(strcasecmp(optarg,"Ins")==0){
			 args2.pppoe_act=1;
		 }else if(strcasecmp(optarg,"Del")==0){
			 args2.pppoe_act=0;
		 }else{
			 printf("Error: -t No/Mod/Ins/Del\n");
			 return 0;
		 }
		 break;
	case 'u':
		 args2.pppoe_id = strtoll(optarg, NULL, 10);
	case 'v':
		 if(strcasecmp(optarg,"Tcp")==0){
			 args2.is_udp=0;
		 }else if(strcasecmp(optarg,"Udp")==0){
			 args2.is_udp=1;
		 }else {
			 printf("Error: -v Tcp/Udp\n");
			 return 0;
		 }
		 break;
	case 'w':
		 if(strcasecmp(optarg,"CPU")==0){
			 args2.dst_port=0; 
		 }else if(strcasecmp(optarg,"GE1")==0){
			 args2.dst_port=1; 
		 }else if(strcasecmp(optarg,"GE2")==0){
			 args2.dst_port=2; 
		 }else {
			 printf("Error: -w CPU/GE1/GE2\n");
			 return 0;
		 }
		 break;
	case '5':
		 args2.dscp = strtoll(optarg, NULL, 10);
	case 'a':
		method = HW_NAT_ADD_ENTRY;
		break;

	case 'b':
		method = HW_NAT_DEL_ENTRY;
		break;
	case 'z':
		method = HW_NAT_DUMP_CACHE_ENTRY;
		break;
	case 'c':
		method = HW_NAT_DUMP_ENTRY;
		entry_num = strtoll(optarg, NULL, 10);
		break;
	case 'x':
		method = HW_NAT_UNBIND_ENTRY;
		entry_num = strtoll(optarg, NULL, 10);
		break;
	case 'k':
		method = HW_NAT_DROP_ENTRY;
		entry_num = strtoll(optarg, NULL, 10);
		break;
	case 'd':
		method = HW_NAT_DEBUG;
		debug = strtoll(optarg, NULL, 10);
		break;
#if 0
	case 'W':
		method = HW_NAT_SWITCH_DSL_MAPE;
		swit = strtoll(optarg, NULL, 10);
		break;
#endif
	case 'e':
		method = HW_NAT_GET_ALL_ENTRIES;
		args->entry_state=0; /* invalid entry */
		break;
	case 'f':
		method = HW_NAT_GET_ALL_ENTRIES;
		args->entry_state=1; /* unbinded entry */
		break;
	case 'g':
		method = HW_NAT_GET_ALL_ENTRIES;
		args->entry_state=2; /* binded entry */
		break;
	case 'y':
		method = HW_NAT_TBL_CLEAR;
		break;
	case 'A':
		method = HW_NAT_GET_AC_CNT;
		args3.ag_index = strtoll(optarg, NULL, 10);
		break;
#if defined (CONFIG_PPE_MCAST)
	case 'B':
		method = HW_NAT_MCAST_INS;
		args5.mc_vid = strtoll(argv[2], NULL, 10);
		str_to_mac(mac, argv[3]);
		memcpy(args5.dst_mac, mac, sizeof(mac));
		args5.mc_px_en = strtoll(argv[4], NULL, 10);
		args5.mc_px_qos_en = strtoll(argv[5], NULL, 10);
		args5.mc_qos_qid = strtoll(argv[6], NULL, 10);
		break;
	case 'C':
		method = HW_NAT_MCAST_DEL;
		args5.mc_vid = strtoll(argv[2], NULL, 10);
		str_to_mac(mac, argv[3]);
		memcpy(args5.dst_mac, mac, sizeof(mac));
		memcpy(args5.dst_mac, mac, sizeof(mac));
		args5.mc_px_en = strtoll(argv[4], NULL, 10);
		args5.mc_px_qos_en = strtoll(argv[5], NULL, 10);
		args5.mc_qos_qid = strtoll(argv[6], NULL, 10);
		break;
	case 'D':
		method = HW_NAT_MCAST_DUMP;
		break;
#endif
#if defined (CONFIG_PPE_MIB)
	case 'm':
		method = HW_NAT_MIB_DUMP;
		entry_num = strtoll(optarg, NULL, 10);
		break;
	case 'r':
		method = HW_NAT_MIB_DRAM_DUMP;
		entry_num = strtoll(optarg, NULL, 10);
		break;
	case 'M':
		method = HW_NAT_MIB_GET;
		break;
#endif
	case 'N':
		method = HW_NAT_BIND_THRESHOLD;
		args4.bind_threshold = strtoll(argv[2], NULL, 10);
		break;
	case 'O':
		method = HW_NAT_MAX_ENTRY_LMT;
		args4.foe_qut_lmt = strtoll(argv[2], NULL, 10);
		args4.foe_half_lmt = strtoll(argv[3], NULL, 10);
		args4.foe_full_lmt  = strtoll(argv[4], NULL, 10);
		break;
	case 'P':
		method = HW_NAT_RULE_SIZE;
		args4.pre_acl    = strtoll(argv[2], NULL, 10);
		args4.pre_meter  = strtoll(argv[3], NULL, 10);
		args4.pre_ac     = strtoll(argv[4], NULL, 10);
		args4.post_meter = strtoll(argv[5], NULL, 10);
		args4.post_ac    = strtoll(argv[6], NULL, 10);
		break;
	case 'Q':
		method = HW_NAT_KA_INTERVAL;
		args4.foe_tcp_ka = strtoll(argv[2], NULL, 10);
		args4.foe_udp_ka = strtoll(argv[3], NULL, 10);
		break;
	case 'T':
		method = HW_NAT_UB_LIFETIME;
		args4.foe_unb_dlta = strtoll(argv[2], NULL, 10);
		break;
	case 'U':
		method = HW_NAT_BIND_LIFETIME;
		args4.foe_tcp_dlta = strtoll(argv[2], NULL, 10);
		args4.foe_udp_dlta = strtoll(argv[3], NULL, 10);
		args4.foe_fin_dlta = strtoll(argv[4], NULL, 10);
		break;
	case 'V':
		method = HW_NAT_VLAN_ID;
		args4.lan_vid = strtoll(argv[2], NULL, 10);
		args4.wan_vid = strtoll(argv[3], NULL, 10);
		break;
	case 'Z':
		method = HW_NAT_BIND_DIRECTION;
		args4.bind_dir = strtoll(optarg, NULL, 10);
		break;
#if defined (CONFIG_HW_NAT_IPI)
	case 'G': /*FIXME..........................*/
		method = HW_NAT_IPI_CTRL_FROM_EXTIF;
		args6.hnat_ipi_enable = strtoll(argv[2], NULL, 10);
		args6.queue_thresh = strtoll(argv[3], NULL, 10);
		args6.drop_pkt = strtoll(argv[4], NULL, 10);
		args6.ipi_cnt_mod = strtoll(argv[5], NULL, 10);
		//printf("##### hnat_ipi_enable=%d, queue_thresh=%d, drop_pkt=%d #####\n", 
		//		args6.hnat_ipi_enable, args6.queue_thresh, args6.drop_pkt);
		break;
	case 'L': /*FIXME..........................*/
		method = HW_NAT_IPI_CTRL_FROM_PPEHIT;
		args7.hnat_ipi_enable = strtoll(argv[2], NULL, 10);
		args7.queue_thresh = strtoll(argv[3], NULL, 10);
		args7.drop_pkt = strtoll(argv[4], NULL, 10);
		args7.ipi_cnt_mod = strtoll(argv[5], NULL, 10);
		//printf("##### hnat_ipi_enable2=%d, queue_thresh2=%d, drop_pkt2=%d #####\n", 
		//		args7.hnat_ipi_enable2, args7.queue_thresh2, args7.drop_pkt);
		break;
#endif
	case '6':
		method = HW_NAT_DPORT;
		break;
	case '?':
		show_usage();

	}
    } 

    switch(method){
#if defined (CONFIG_HW_NAT_MANUAL_MODE) || defined (CONFIG_HW_NAT_SEMI_AUTO_MODE)
    case HW_NAT_ADD_ENTRY:
	    result = HwNatAddEntry(&args2);
	    break;
    case HW_NAT_DEL_ENTRY:
	    result = HwNatDelEntry(&args2);
	    break;
#endif
    case HW_NAT_GET_ALL_ENTRIES:
	    HwNatGetAllEntries(args);

	    printf("Total Entry Count = %d\n",args->num_of_entries);	
	    for(i=0;i<args->num_of_entries;i++){
		if(args->entries[i].pkt_type==0) { //IPV4_NAPT
		    printf("IPv4_NAPT=%d : %u.%u.%u.%u:%d->%u.%u.%u.%u:%d => %u.%u.%u.%u:%d->%u.%u.%u.%u:%d\n", \
			    args->entries[i].hash_index, \
			    NIPQUAD(args->entries[i].ing_sipv4), \
			    args->entries[i].ing_sp, \
			    NIPQUAD(args->entries[i].ing_dipv4), \
			    args->entries[i].ing_dp, \
			    NIPQUAD(args->entries[i].eg_sipv4), \
		            args->entries[i].eg_sp, \
		            NIPQUAD(args->entries[i].eg_dipv4), \
		            args->entries[i].eg_dp);
		} else if(args->entries[i].pkt_type==1) { //IPV4_NAT
		    printf("IPv4_NAT=%d : %u.%u.%u.%u->%u.%u.%u.%u => %u.%u.%u.%u->%u.%u.%u.%u\n", \
			    args->entries[i].hash_index, \
			    NIPQUAD(args->entries[i].ing_sipv4), \
			    NIPQUAD(args->entries[i].ing_dipv4), \
			    NIPQUAD(args->entries[i].eg_sipv4), \
			    NIPQUAD(args->entries[i].eg_dipv4)); 
		} else if(args->entries[i].pkt_type==2) { //IPV6_ROUTING
		    printf("IPv6_1T= %d /DIP: %x:%x:%x:%x:%x:%x:%x:%x\n", \
		    args->entries[i].hash_index, \
		    NIPHALF(args->entries[i].ing_dipv6_0), \
		    NIPHALF(args->entries[i].ing_dipv6_1), \
		    NIPHALF(args->entries[i].ing_dipv6_2), \
		    NIPHALF(args->entries[i].ing_dipv6_3));
		} else if(args->entries[i].pkt_type==3) { //IPV4_DSLITE
		    printf("DS-Lite= %d : %u.%u.%u.%u:%d->%u.%u.%u.%u:%d (%x:%x:%x:%x:%x:%x:%x:%x -> %x:%x:%x:%x:%x:%x:%x:%x) \n", \
			    args->entries[i].hash_index, \
			    NIPQUAD(args->entries[i].ing_sipv4),  \
			    args->entries[i].ing_sp,     \
			    NIPQUAD(args->entries[i].ing_dipv4),  \
			    args->entries[i].ing_dp, \
			    NIPHALF(args->entries[i].eg_sipv6_0), \
			    NIPHALF(args->entries[i].eg_sipv6_1), \
			    NIPHALF(args->entries[i].eg_sipv6_2), \
			    NIPHALF(args->entries[i].eg_sipv6_3), \
			    NIPHALF(args->entries[i].eg_dipv6_0), \
			    NIPHALF(args->entries[i].eg_dipv6_1), \
			    NIPHALF(args->entries[i].eg_dipv6_2), \
			    NIPHALF(args->entries[i].eg_dipv6_3));
		} else if(args->entries[i].pkt_type==4) { //IPV6_3T_ROUTE
		    printf("IPv6_3T= %d SIP: %x:%x:%x:%x:%x:%x:%x:%x DIP: %x:%x:%x:%x:%x:%x:%x:%x\n", \
		    args->entries[i].hash_index, \
		    NIPHALF(args->entries[i].ing_sipv6_0), \
		    NIPHALF(args->entries[i].ing_sipv6_1), \
		    NIPHALF(args->entries[i].ing_sipv6_2), \
		    NIPHALF(args->entries[i].ing_sipv6_3), \
		    NIPHALF(args->entries[i].ing_dipv6_0), \
		    NIPHALF(args->entries[i].ing_dipv6_1), \
		    NIPHALF(args->entries[i].ing_dipv6_2), \
		    NIPHALF(args->entries[i].ing_dipv6_3));
		} else if(args->entries[i].pkt_type==5) { //IPV6_5T_ROUTE
		    if(args->entries[i].ipv6_flowlabel==1) {
			printf("IPv6_5T= %d SIP: %x:%x:%x:%x:%x:%x:%x:%x DIP: %x:%x:%x:%x:%x:%x:%x:%x (Flow Label=%x)\n", \
				args->entries[i].hash_index, \
				NIPHALF(args->entries[i].ing_sipv6_0), \
				NIPHALF(args->entries[i].ing_sipv6_1), \
				NIPHALF(args->entries[i].ing_sipv6_2), \
				NIPHALF(args->entries[i].ing_sipv6_3), \
				NIPHALF(args->entries[i].ing_dipv6_0), \
				NIPHALF(args->entries[i].ing_dipv6_1), \
				NIPHALF(args->entries[i].ing_dipv6_2), \
				NIPHALF(args->entries[i].ing_dipv6_3), \
				((args->entries[i].ing_sp << 16) | (args->entries[i].ing_dp))&0xFFFFF);
		    }else {
			printf("IPv6_5T= %d SIP: %x:%x:%x:%x:%x:%x:%x:%x (SP:%d) DIP: %x:%x:%x:%x:%x:%x:%x:%x (DP=%d)\n", \
				args->entries[i].hash_index, \
				NIPHALF(args->entries[i].ing_sipv6_0), \
				NIPHALF(args->entries[i].ing_sipv6_1), \
				NIPHALF(args->entries[i].ing_sipv6_2), \
				NIPHALF(args->entries[i].ing_sipv6_3), \
				args->entries[i].ing_sp, \
				NIPHALF(args->entries[i].ing_dipv6_0), \
				NIPHALF(args->entries[i].ing_dipv6_1), \
				NIPHALF(args->entries[i].ing_dipv6_2), \
				NIPHALF(args->entries[i].ing_dipv6_3), \
				args->entries[i].ing_dp);
		    }

		} else if(args->entries[i].pkt_type==7) { //IPV6_6RD
		    if(args->entries[i].ipv6_flowlabel==1) {
			printf("6RD= %d %x:%x:%x:%x:%x:%x:%x:%x->%x:%x:%x:%x:%x:%x:%x:%x [Flow Label=%x]\n", \
				args->entries[i].hash_index, \
				NIPHALF(args->entries[i].ing_sipv6_0), \
				NIPHALF(args->entries[i].ing_sipv6_1), \
				NIPHALF(args->entries[i].ing_sipv6_2), \
				NIPHALF(args->entries[i].ing_sipv6_3), \
				NIPHALF(args->entries[i].ing_dipv6_0), \
				NIPHALF(args->entries[i].ing_dipv6_1), \
				NIPHALF(args->entries[i].ing_dipv6_2), \
				NIPHALF(args->entries[i].ing_dipv6_3), \
				((args->entries[i].ing_sp << 16) | (args->entries[i].ing_dp))&0xFFFFF);
				printf("(%u.%u.%u.%u->%u.%u.%u.%u)\n", NIPQUAD(args->entries[i].eg_sipv4), NIPQUAD(args->entries[i].eg_dipv4));
		    }else {
			printf("6RD= %d /SIP: %x:%x:%x:%x:%x:%x:%x:%x [SP:%d] /DIP: %x:%x:%x:%x:%x:%x:%x:%x [DP=%d]", \
				args->entries[i].hash_index, \
				NIPHALF(args->entries[i].ing_sipv6_0), \
				NIPHALF(args->entries[i].ing_sipv6_1), \
				NIPHALF(args->entries[i].ing_sipv6_2), \
				NIPHALF(args->entries[i].ing_sipv6_3), \
				args->entries[i].ing_sp, \
				NIPHALF(args->entries[i].ing_dipv6_0), \
				NIPHALF(args->entries[i].ing_dipv6_1), \
				NIPHALF(args->entries[i].ing_dipv6_2), \
				NIPHALF(args->entries[i].ing_dipv6_3), \
				args->entries[i].ing_dp); 
			printf("(%u.%u.%u.%u->%u.%u.%u.%u)\n", NIPQUAD(args->entries[i].eg_sipv4), NIPQUAD(args->entries[i].eg_dipv4));
		    }
		} else{
		    printf("unknown packet type! (pkt_type=%d) \n", args->entries[i].pkt_type);
		}
	    }
	    result = args->result;
	    break;
    case HW_NAT_DUMP_CACHE_ENTRY:
	    result = HwNatCacheDumpEntry();
	    break;
    case HW_NAT_DUMP_ENTRY:
	    result = HwNatDumpEntry(entry_num);
	    break;
    case HW_NAT_UNBIND_ENTRY:
	    result = HwNatUnBindEntry(entry_num);
	    break;
    case HW_NAT_DROP_ENTRY:
	    result = HwNatDropEntry(entry_num);
	    break;
    case HW_NAT_DEBUG:
	    result = HwNatDebug(debug);
	    break;
#if 0
	case HW_NAT_SWITCH_DSL_MAPE:
	    result = HwNatSwitchDsliteMape(swit);
	    break;
#endif
    case HW_NAT_GET_AC_CNT:
	    HwNatGetAGCnt(&args3);
	    printf("Byte cnt=%llu\n", args3.ag_byte_cnt);
	    printf("Pkt cnt=%llu\n", args3.ag_pkt_cnt);
	    result = args3.result;
	    break;

    case HW_NAT_BIND_THRESHOLD:
	    HwNatSetBindThreshold(&args4);
	    result = args4.result;
	    break;
    case HW_NAT_MAX_ENTRY_LMT:
	    HwNatSetMaxEntryRateLimit(&args4);
	    result = args4.result;
	    break;
    case HW_NAT_RULE_SIZE:
	    HwNatSetRuleSize(&args4);
	    result = args4.result;
	    break;
    case HW_NAT_KA_INTERVAL:
	    HwNatSetKaInterval(&args4);
	    result = args4.result;
	    break;
    case HW_NAT_UB_LIFETIME:
	    HwNatSetUnbindLifeTime(&args4);
	    result = args4.result;
	    break;
    case HW_NAT_BIND_LIFETIME:
	    result = HwNatSetBindLifeTime(&args4);
	    break;
    case HW_NAT_VLAN_ID:
	    result = HwNatSetVID(&args4);
	    break;
    case HW_NAT_BIND_DIRECTION:
	    result = HwNatSetBindDir(&args4);
	    break;
#if defined (CONFIG_PPE_MCAST)
    case HW_NAT_MCAST_INS:
	    result = HwNatMcastIns(&args5);
	    break;
    case HW_NAT_MCAST_DEL:
	    result = HwNatMcastDel(&args5);
	    break;
    case HW_NAT_MCAST_DUMP:
	    result = HwNatMcastDump();
	    break;
#endif
#if defined (CONFIG_PPE_MIB)
    case HW_NAT_MIB_DUMP:
	    result = HwNatMibDumpEntry(entry_num);
	    break;
    case HW_NAT_MIB_DRAM_DUMP:
	    result = HwNatMibDramDumpEntry(entry_num);
	    break;
    case HW_NAT_MIB_GET:
	    result = HwNatMibGet(&args2);
	    break;
#endif
    case HW_NAT_TBL_CLEAR:
            result = HwNatTblClear();
            break;
#if defined (CONFIG_HW_NAT_IPI)
    case HW_NAT_IPI_CTRL_FROM_EXTIF:
	    result = HwNatIPICtrlFromExtIf(&args6);
	    break;
    case HW_NAT_IPI_CTRL_FROM_PPEHIT:
	    result = HwNatIPICtrlFromPPEHit(&args7);
	    break;
#endif
    case HW_NAT_DPORT:
    	    result = HwNatDumpDport();
    	    break;
    default:
	    result = HWNAT_FAIL;

    }

    if(result==HWNAT_SUCCESS){
	printf("done\n");
    }else if(result==HWNAT_ENTRY_NOT_FOUND) {
	printf("entry not found\n");
    }else {
	printf("fail\n");
    }

    free(args);
    close(fd);
    return 0;
}
