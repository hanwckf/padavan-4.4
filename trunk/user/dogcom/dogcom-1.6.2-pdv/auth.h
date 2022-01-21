#ifndef AUTH_H_
#define AUTH_H_

#ifdef WIN32
    #include <winsock2.h>
#else
    #include <netinet/in.h>
#endif

enum {
    CHECK_MAC          = 0x01,
    SERVER_BUSY        = 0x02,
    WRONG_PASS         = 0x03,
    NOT_ENOUGH         = 0x04,
    FREEZE_UP          = 0x05,
    NOT_ON_THIS_IP     = 0x07,
    NOT_ON_THIS_MAC    = 0x0B,
    TOO_MUCH_IP        = 0x14,
    UPDATE_CLIENT      = 0x15,
    NOT_ON_THIS_IP_MAC = 0x16,
    MUST_USE_DHCP      = 0x17
};

int dhcp_challenge(int sockfd, struct sockaddr_in addr, unsigned char seed[]);
int dhcp_login(int sockfd, struct sockaddr_in addr, unsigned char seed[], unsigned char auth_information[], int try_JLUversion);
int pppoe_challenge(int sockfd, struct sockaddr_in addr, int *pppoe_counter, unsigned char seed[], unsigned char sip[], int *encrypt_mode);
int pppoe_login(int sockfd, struct sockaddr_in addr, int *pppoe_counter, unsigned char seed[], unsigned char sip[], int *first, int *encrypt_mode, int *encrypt_type);
int dogcom(int try_times);
void print_packet(char msg[10], unsigned char *packet, int length);
void logging(char msg[10], unsigned char *packet, int length);
void get_lasterror(char *msg);

#endif // AUTH_H_