#ifndef __HW_NAT_API
#define __HW_NAT_API

#define NIPQUAD(addr) \
	((unsigned char *)&addr)[3], \
        ((unsigned char *)&addr)[2], \
        ((unsigned char *)&addr)[1], \
        ((unsigned char *)&addr)[0]
#define NIPHALF(addr) \
        ((unsigned short *)&addr)[1], \
        ((unsigned short *)&addr)[0]

int HwNatAddEntry(struct hwnat_tuple *opt);
int HwNatDelEntry(struct hwnat_tuple *opt);
int HwNatDumpEntry(unsigned int entry_num);
int HwNatBindEntry(unsigned int entry_num);
int HwNatUnBindEntry(unsigned int entry_num);
int HwNatDropEntry(unsigned int entry_num);
int HwNatInvalidEntry(unsigned int entry_num);
int HwNatDscpRemarkEbl(struct hwnat_qos_args *opt);
int HwNatVpriRemarkEbl(struct hwnat_qos_args *opt);
int HwNatSetFoeWeight(struct hwnat_qos_args *opt);
int HwNatSetAclWeight(struct hwnat_qos_args *opt);
int HwNatSetDscpWeight(struct hwnat_qos_args *opt);
int HwNatSetVpriWeight(struct hwnat_qos_args *opt);
int HwNatSetDscp_Up(struct hwnat_qos_args *opt);
int HwNatSetUp_InDscp(struct hwnat_qos_args *opt);
int HwNatSetUp_OutDscp(struct hwnat_qos_args *opt);
int HwNatSetUp_Vpri(struct hwnat_qos_args *opt);
int HwNatSetUp_Ac(struct hwnat_qos_args *opt);
int HwNatSetSchMode(struct hwnat_qos_args *opt);
int HwNatSetSchWeight(struct hwnat_qos_args *opt);
int HwNatSetBindThreshold(struct hwnat_config_args *opt);
int HwNatSetMaxEntryRateLimit(struct hwnat_config_args *opt);
int HwNatSetRuleSize(struct hwnat_config_args *opt);
int HwNatSetKaInterval(struct hwnat_config_args *opt);
int HwNatSetUnbindLifeTime(struct hwnat_config_args *opt);
int HwNatSetBindLifeTime(struct hwnat_config_args *opt);
int HwNatSetVID(struct hwnat_config_args *opt);
int HwNatSetBindDir(struct hwnat_config_args *opt);
int HwNatGetAllEntries(struct hwnat_args *opt);
int HwNatDebug(unsigned int debug);
int HwNatGetAGCnt(struct hwnat_ac_args *opt);
#if defined (CONFIG_PPE_MCAST)
int HwNatMcastIns(struct hwnat_mcast_args *opt);
int HwNatMcastDel(struct hwnat_mcast_args *opt);
int HwNatMcastDump(void);
#endif
#if defined (CONFIG_PPE_MIB)
int HwNatMibDumpEntry(unsigned int entry_num);
#endif
int HwNatTblClear(void);
int HwNatCacheDumpEntry(void);
int HwNatDumpDport(void);
#endif
