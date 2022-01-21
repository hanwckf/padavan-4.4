#ifndef EAPOL_H__
#define EAPOL_H__

#include "libs/common.h"

#define IDEN_LEN    UNAME_LEN

#define TRY_TIMES   (3)
/* 每次请求超过TIMEOUT秒，就重新请求一次 */
#define TIMEOUT     (3)
/* eap 在EAP_KPALV_TIMEOUT秒内没有回应，认为不需要心跳 */
#define EAP_KPALV_TIMEOUT   (420)   /* 7分钟 */

/* ethii层取0x888e表示上层是8021.x */
#define ETHII_8021X (0x888e)

#define EAPOL_VER       (0x01)
#define EAPOL_PACKET    (0x00)
#define EAPOL_START     (0x01)
#define EAPOL_LOGOFF    (0x02)
/* 貌似请求下线的id都是这个 */
#define EAPOL_LOGOFF_ID (255)

#define EAP_CODE_REQ    (0x01)
#define EAP_CODE_RES    (0x02)
#define EAP_CODE_SUCS   (0x03)
#define EAP_CODE_FAIL   (0x04)
#define EAP_TYPE_IDEN   (0x01)
#define EAP_TYPE_MD5    (0x04)


#pragma pack(1)
/* ethii 帧 */
/* 其实这个和struct ether_header是一样的结构 */
typedef struct {
    uchar dst_mac[ETH_ALEN];
    uchar src_mac[ETH_ALEN];
    uint16 type;        /* 取值0x888e，表明是8021.x */
}ethII_t;
/* eapol 帧 */
typedef struct {
    uchar ver;          /* 取值0x01 */
    /*
     * 0x00: eapol-packet
     * 0x01: eapol-start
     * 0x02: eapol-logoff
     */
    uchar type;
    uint16 len;
}eapol_t;
/* eap报文头 */
typedef struct {
    /*
     * 0x01: request
     * 0x02: response
     * 0x03: success
     * 0x04: failure
     */
    uchar code;
    uchar id;
    uint16 len;
    /*
     * 0x01: identity
     * 0x04: md5-challenge
     */
    uchar type;
}eap_t;
/* 报文体 */
#define MD5_SIZE    16
#define STUFF_LEN   (64)
typedef union {
    uchar identity[IDEN_LEN];
    struct {
        uchar _size;
        uchar _md5value[MD5_SIZE];
        uchar _exdata[STUFF_LEN];
    }md5clg;
}eapbody_t;
#define md5size     md5clg._size
#define md5value    md5clg._md5value
#define md5exdata   md5clg._exdata
#pragma pack()

/*
 * eap认证
 * uname: 用户名
 * pwd: 密码
 * @return: 0: 成功
 *          1: 用户不存在
 *          2: 密码错误
 *          3: 其他超时
 *          4: 服务器拒绝请求登录
 *          -1: 没有找到合适网络接口
 *          -2: 没有找到服务器
 */
extern int eaplogin(char const *uname, char const *pwd);
/*
 * eap下线
 */
extern int eaplogoff(void);
/*
 * eap重新登录
 */
extern int eaprefresh(char const *uname, char const *pwd);
/*
 * 用来设置ifname
 */
extern void setifname(char const *ifname);
// #ifdef WIN32
/*
 * 由于windows下实现进程的特殊性，这里把eap_daemon导出给main_cli使用
 * ifname: 心跳的物理接口名字
 * @return: 0: keep alive 进程正常退出，也许并不需要心跳进程
 *         !0: 错误原因导致keep alive 进程退出，也许是没法创建进程
 */
// extern int eap_daemon(char const *ifname);
// #endif /* WINDOWS */
#undef IDEN_LEN

#endif
