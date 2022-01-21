#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#ifdef WIN32
#include <winsock2.h>
typedef int socklen_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "libs/md4.h"
#include "libs/md5.h"
#include "libs/sha1.h"
#include "auth.h"
#include "configparse.h"
#include "keepalive.h"
#include "debug.h"

#define BIND_PORT 61440
#define DEST_PORT 61440

int dhcp_challenge(int sockfd, struct sockaddr_in addr, unsigned char seed[]) {
    unsigned char challenge_packet[20], recv_packet[1024];
    memset(challenge_packet, 0, 20);
    challenge_packet[0] = 0x01;
    challenge_packet[1] = 0x02;
    challenge_packet[2] = rand() & 0xff;
    challenge_packet[3] = rand() & 0xff;
    challenge_packet[4] = drcom_config.AUTH_VERSION[0];

    sendto(sockfd, challenge_packet, 20, 0, (struct sockaddr *)&addr, sizeof(addr));

    if (verbose_flag) {
        print_packet("[Challenge sent] ", challenge_packet, 20);
    }
    if (logging_flag) {
        logging("[Challenge sent] ", challenge_packet, 20);
    }
#ifdef TEST
    unsigned char test[4] = {0x52, 0x6c, 0xe4, 0x00};
    memcpy(seed, test, 4);
    print_packet("[TEST MODE]<PREP SEED> ", seed, 4);
    return 0;
#endif

    socklen_t addrlen = sizeof(addr);
    if (recvfrom(sockfd, recv_packet, 1024, 0, (struct sockaddr *)&addr, &addrlen) < 0) {
#ifdef WIN32
        get_lasterror("Failed to recv data");
#else
        perror("Failed to recv data");
#endif
        return 1;
    }

    if (verbose_flag) {
        print_packet("[Challenge recv] ", recv_packet, 76);
    }
    if (logging_flag) {
        logging("[Challenge recv] ", recv_packet, 76);
    }

    if (recv_packet[0] != 0x02) {
        printf("Bad challenge response received.\n");
        return 1;
    }

    memcpy(seed, &recv_packet[4], 4);
#ifdef DEBUG
    print_packet("<GET SEED> ", seed, 4);
#endif

    return 0;
}

int dhcp_login(int sockfd, struct sockaddr_in addr, unsigned char seed[], unsigned char auth_information[], int try_JLUversion) {
    unsigned int login_packet_size;
    unsigned int length_padding = 0;
    int JLU_padding = 0;

    if (strlen(drcom_config.password) > 8) {
        length_padding = strlen(drcom_config.password) - 8 + (length_padding%2);
        if (try_JLUversion) {
            printf("Start JLU mode.\n");
            if (logging_flag) {
                logging("Start JLU mode.", NULL, 0);
            }
            if (strlen(drcom_config.password) != 16) {
                JLU_padding = strlen(drcom_config.password) / 4;
            }
            length_padding = 28 + (strlen(drcom_config.password) - 8) + JLU_padding;
        }
    }
    if (drcom_config.ror_version) {
        login_packet_size = 338 + length_padding;
    } else {
        login_packet_size = 330;
    }
    unsigned char login_packet[login_packet_size], recv_packet[1024], MD5A[16], MACxorMD5A[6], MD5B[16], checksum1[8], checksum2[4];
    memset(login_packet, 0, login_packet_size);
    memset(recv_packet, 0, 100);

    // build login-packet
    login_packet[0] = 0x03;
    login_packet[1] = 0x01;
    login_packet[2] = 0x00;
    login_packet[3] = strlen(drcom_config.username) + 20;
    int MD5A_len = 6 + strlen(drcom_config.password);
    unsigned char MD5A_str[MD5A_len];
    MD5A_str[0] = 0x03;
    MD5A_str[1] = 0x01;
    memcpy(MD5A_str + 2, seed, 4);
    memcpy(MD5A_str + 6, drcom_config.password, strlen(drcom_config.password));
    MD5(MD5A_str, MD5A_len, MD5A);
    memcpy(login_packet + 4, MD5A, 16);
    memcpy(login_packet + 20, drcom_config.username, strlen(drcom_config.username));
    memcpy(login_packet + 56, &drcom_config.CONTROLCHECKSTATUS, 1);
    memcpy(login_packet + 57, &drcom_config.ADAPTERNUM, 1);
    uint64_t sum = 0;
    uint64_t mac = 0;
    // unpack
    for (int i = 0; i < 6; i++) {
        sum = (int)MD5A[i] + sum * 256;
    }
    // unpack
    for (int i = 0; i < 6; i++) {
        mac = (int)drcom_config.mac[i] + mac * 256;
    }
    sum ^= mac;
    // pack
    for (int i = 6; i > 0; i--) {
        MACxorMD5A[i - 1] = (unsigned char)(sum % 256);
        sum /= 256;
    }
    memcpy(login_packet + 58, MACxorMD5A, sizeof(MACxorMD5A));
    int MD5B_len = 9 + strlen(drcom_config.password);
    unsigned char MD5B_str[MD5B_len];
    memset(MD5B_str, 0, MD5B_len);
    MD5B_str[0] = 0x01;
    memcpy(MD5B_str + 1, drcom_config.password, strlen(drcom_config.password));
    memcpy(MD5B_str + strlen(drcom_config.password) + 1, seed, 4);
    MD5(MD5B_str, MD5B_len, MD5B);
    memcpy(login_packet + 64, MD5B, 16);
    login_packet[80] = 0x01;
    unsigned char host_ip[4];
    sscanf(drcom_config.host_ip, "%hhd.%hhd.%hhd.%hhd",
           &host_ip[0],
           &host_ip[1],
           &host_ip[2],
           &host_ip[3]);
    memcpy(login_packet + 81, host_ip, 4);
    unsigned char checksum1_str[101], checksum1_tmp[4] = {0x14, 0x00, 0x07, 0x0b};
    memcpy(checksum1_str, login_packet, 97);
    memcpy(checksum1_str + 97, checksum1_tmp, 4);
    MD5(checksum1_str, 101, checksum1);
    memcpy(login_packet + 97, checksum1, 8);
    memcpy(login_packet + 105, &drcom_config.IPDOG, 1);
    memcpy(login_packet + 110, &drcom_config.host_name, strlen(drcom_config.host_name));
    unsigned char PRIMARY_DNS[4];
    sscanf(drcom_config.PRIMARY_DNS, "%hhd.%hhd.%hhd.%hhd",
           &PRIMARY_DNS[0],
           &PRIMARY_DNS[1],
           &PRIMARY_DNS[2],
           &PRIMARY_DNS[3]);
    memcpy(login_packet + 142, PRIMARY_DNS, 4);
    unsigned char dhcp_server[4];
    sscanf(drcom_config.dhcp_server, "%hhd.%hhd.%hhd.%hhd",
           &dhcp_server[0],
           &dhcp_server[1],
           &dhcp_server[2],
           &dhcp_server[3]);
    memcpy(login_packet + 146, dhcp_server, 4);
    unsigned char OSVersionInfoSize[4] = {0x94};
    unsigned char OSMajor[4] = {0x05};
    unsigned char OSMinor[4] = {0x01};
    unsigned char OSBuild[4] = {0x28, 0x0a};
    unsigned char PlatformID[4] = {0x02};
    if (try_JLUversion) {
        OSVersionInfoSize[0] = 0x94;
        OSMajor[0] = 0x06;
        OSMinor[0] = 0x02;
        OSBuild[0] = 0xf0;
        OSBuild[1] = 0x23;
        PlatformID[0] = 0x02;
        unsigned char ServicePack[40] = {0x33, 0x64, 0x63, 0x37, 0x39, 0x66, 0x35, 0x32, 0x31, 0x32, 0x65, 0x38, 0x31, 0x37, 0x30, 0x61, 0x63, 0x66, 0x61, 0x39, 0x65, 0x63, 0x39, 0x35, 0x66, 0x31, 0x64, 0x37, 0x34, 0x39, 0x31, 0x36, 0x35, 0x34, 0x32, 0x62, 0x65, 0x37, 0x62, 0x31};
        unsigned char hostname[9] = {0x44, 0x72, 0x43, 0x4f, 0x4d, 0x00, 0xcf, 0x07, 0x68};
        memcpy(login_packet + 182, hostname, 9);
        memcpy(login_packet + 246, ServicePack, 40);
    }
    memcpy(login_packet + 162, OSVersionInfoSize, 4);
    memcpy(login_packet + 166, OSMajor, 4);
    memcpy(login_packet + 170, OSMinor, 4);
    memcpy(login_packet + 174, OSBuild, 4);
    memcpy(login_packet + 178, PlatformID, 4);
    if (!try_JLUversion) {
        memcpy(login_packet + 182, &drcom_config.host_os, strlen(drcom_config.host_os));
    }
    memcpy(login_packet + 310, drcom_config.AUTH_VERSION, 2);
    int counter = 312;
    unsigned int ror_padding = 0;
    if (strlen(drcom_config.password) <= 8) {
        ror_padding = 8 - strlen(drcom_config.password);
    } else {
        if ((strlen(drcom_config.password)-8) % 2) { ror_padding = 1; }
        if (try_JLUversion) { ror_padding = JLU_padding; }
    }
    if (drcom_config.ror_version) {
        MD5(MD5A_str, MD5A_len, MD5A);
        login_packet[counter + 1] = strlen(drcom_config.password);
        counter += 2;
        for(int i = 0, x = 0; i < strlen(drcom_config.password); i++) {
            x = (int)MD5A[i] ^ (int)drcom_config.password[i];
            login_packet[counter + i] = (unsigned char)(((x << 3) & 0xff) + (x >> 5));
        }
        counter += strlen(drcom_config.password);
        // print_packet("TEST ", ror, strlen(drcom_config.password));
    } else {
        ror_padding = 2;
    }
    login_packet[counter] = 0x02;
    login_packet[counter + 1] = 0x0c;
    unsigned char checksum2_str[counter + 18]; // [counter + 14 + 4]
    memset(checksum2_str, 0, counter + 18);
    unsigned char checksum2_tmp[6] = {0x01, 0x26, 0x07, 0x11};
    memcpy(checksum2_str, login_packet, counter + 2);
    memcpy(checksum2_str + counter + 2, checksum2_tmp, 6);
    memcpy(checksum2_str + counter + 8, drcom_config.mac, 6);
    sum = 1234;
    uint64_t ret = 0;
    for (int i = 0; i < counter + 14; i += 4) {
        ret = 0;
        // reverse unsigned char array[4]
        for(int j = 4; j > 0; j--) {
            ret = ret * 256 + (int)checksum2_str[i + j - 1];
        }
        sum ^= ret;
    }
    sum = (1968 * sum) & 0xffffffff;
    for (int j = 0; j < 4; j++) {
        checksum2[j] = (unsigned char)(sum >> (j * 8) & 0xff);
    }
    memcpy(login_packet + counter + 2, checksum2, 4);
    memcpy(login_packet + counter + 8, drcom_config.mac, 6);
    login_packet[counter + ror_padding + 14] = 0xe9;
    login_packet[counter + ror_padding + 15] = 0x13;
    if (try_JLUversion) {
        login_packet[counter + ror_padding + 14] = 0x60;
        login_packet[counter + ror_padding + 15] = 0xa2;
    }

    sendto(sockfd, login_packet, sizeof(login_packet), 0, (struct sockaddr *)&addr, sizeof(addr));

    if (verbose_flag) {
        print_packet("[Login sent] ", login_packet, sizeof(login_packet));
    }
    if (logging_flag) {
        logging("[Login sent] ", login_packet, sizeof(login_packet));
    }

#ifdef TEST
    unsigned char test[16] = {0x44, 0x72, 0x63, 0x6f, 0x77, 0x27, 0x20, 0xca, 0xed, 0x05, 0x6e, 0x35, 0xaa, 0x8b, 0x01, 0xfb};
    memcpy(auth_information, test, 16);
    print_packet("[TEST MODE]<PREP AUTH_INFORMATION> ", auth_information, 16);
    return 0;
#endif

    socklen_t addrlen = sizeof(addr);
    if (recvfrom(sockfd, recv_packet, 1024, 0, (struct sockaddr *)&addr, &addrlen) < 0) {
#ifdef WIN32
        get_lasterror("Failed to recv data");
#else
        perror("Failed to recv data");
#endif
        return 1;
    }

    if (recv_packet[0] != 0x04) {
        if (verbose_flag) {
            print_packet("[login recv] ", recv_packet, 100);
        }
        printf("<<< Login failed >>>\n");
        if (logging_flag) {
            logging("[login recv] ", recv_packet, 100);
            logging("<<< Login failed >>>", NULL, 0);
        }
        char err_msg[256];
        if (recv_packet[0] == 0x05) {
            switch (recv_packet[4]) {
                case CHECK_MAC:
                    strcpy(err_msg, "[Tips] Someone is using this account with wired.");
                    break;
                case SERVER_BUSY:
                    strcpy(err_msg, "[Tips] The server is busy, please log back in again.");
                    break;
                case WRONG_PASS:
                    strcpy(err_msg, "[Tips] Account and password not match.");
                    break;
                case NOT_ENOUGH:
                    strcpy(err_msg, "[Tips] The cumulative time or traffic for this account has exceeded the limit.");
                    break;
                case FREEZE_UP:
                    strcpy(err_msg, "[Tips] This account is suspended.");
                    break;
                case NOT_ON_THIS_IP:
                    strcpy(err_msg, "[Tips] IP address does not match, this account can only be used in the specified IP address.");
                    break;
                case NOT_ON_THIS_MAC:
                    strcpy(err_msg, "[Tips] MAC address does not match, this account can only be used in the specified IP and MAC address.");
                    break;
                case TOO_MUCH_IP:
                    strcpy(err_msg, "[Tips] This account has too many IP addresses.");
                    break;
                case UPDATE_CLIENT:
                    strcpy(err_msg, "[Tips] The client version is incorrect.");
                    break;
                case NOT_ON_THIS_IP_MAC:
                    strcpy(err_msg, "[Tips] This account can only be used on specified MAC and IP address.");
                    break;
                case MUST_USE_DHCP:
                    strcpy(err_msg, "[Tips] Your PC set up a static IP, please change to DHCP, and then re-login.");
                    break;
                default:
                    strcpy(err_msg, "[Tips] Unknown error number.");
                    break;
            }
            printf("%s\n", err_msg);
            if (logging_flag) {
                logging(err_msg, NULL, 0);
            }
        }
        return 1;
    } else {
        if (verbose_flag) {
            print_packet("[login recv] ", recv_packet, 100);
        }
        printf("<<< Logged in >>>\n");
        if (logging_flag) {
            logging("[login recv] ", recv_packet, 100);
            logging("<<< Logged in >>>", NULL, 0);
        }
    }

    memcpy(auth_information, &recv_packet[23], 16);
#ifdef DEBUG
    print_packet("<GET AUTH_INFORMATION> ", auth_information, 16);
#endif

    if(recvfrom(sockfd, recv_packet, 1024, 0, (struct sockaddr *)&addr, &addrlen) >= 0) {
        DEBUG_PRINT(("Get notice packet."));
    }

    return 0;
}

int pppoe_challenge(int sockfd, struct sockaddr_in addr, int *pppoe_counter, unsigned char seed[], unsigned char sip[], int *encrypt_mode) {
    unsigned char challenge_packet[8], recv_packet[1024];
    memset(challenge_packet, 0, 8);
    unsigned char challenge_tmp[5] = {0x07, 0x00, 0x08, 0x00, 0x01};
    memcpy(challenge_packet, challenge_tmp, 5);
    challenge_packet[1] = *pppoe_counter % 0xFF;
    (*pppoe_counter)++;

    sendto(sockfd, challenge_packet, 8, 0, (struct sockaddr *)&addr, sizeof(addr));

    if (verbose_flag) {
        print_packet("[Challenge sent] ", challenge_packet, 8);
    }
    if (logging_flag) {
        logging("[Challenge sent] ", challenge_packet, 8);
    }
#ifdef TEST
    unsigned char test1[4] = {0x26, 0xe6, 0xe1, 0x02};
    unsigned char test2[4] = {0xc0, 0xa8, 0x01, 0x0b};
    memcpy(seed, test1, 4);
    memcpy(sip, test2, 4);
    *encrypt_mode = 1; /* encrypt_mode test switch [0 or 1] */
    print_packet("[TEST MODE]<PREP SEED> ", seed, 4);
    print_packet("[TEST MODE]<PREP SIP> ", sip, 4);
    printf("[TEST MODE]<PREP ENCRYPT_MODE> %d\n", *encrypt_mode);
    return 0;
#endif

    socklen_t addrlen = sizeof(addr);
    if (recvfrom(sockfd, recv_packet, 1024, 0, (struct sockaddr *)&addr, &addrlen) < 0) {
#ifdef WIN32
        get_lasterror("Failed to recv data");
#else
        perror("Failed to recv data");
#endif
        return 1;
    }

    if (verbose_flag) {
        print_packet("[Challenge recv] ", recv_packet, 32);
    }
    if (logging_flag) {
        logging("[Challenge recv] ", recv_packet, 32);
    }

    if (recv_packet[0] != 0x07) {
        printf("Bad challenge response received.\n");
        return 1;
    }
    if (recv_packet[5] != 0x00) {
        *encrypt_mode = 1;
    } else {
        *encrypt_mode = 0;
    }

#ifdef FORCE_ENCRYPT
    *encrypt_mode = 1;
#endif

    memcpy(seed, &recv_packet[8], 4);
    memcpy(sip, &recv_packet[12], 4);
    memcpy(drcom_config.KEEP_ALIVE_VERSION, &recv_packet[28], 2);
#ifdef DEBUG
    print_packet("<GET SEED> ", seed, 4);
    print_packet("<GET SIP> ", sip, 4);
    printf("<GET ENCRYPT_MODE> %d", *encrypt_mode);
#endif

    return 0;
}

int pppoe_login(int sockfd, struct sockaddr_in addr, int *pppoe_counter, unsigned char seed[], unsigned char sip[], int *login_first, int *encrypt_mode, int *encrypt_type) {
    unsigned char login_packet[96], recv_packet[1024];
    memset(login_packet, 0, 96);
    unsigned char login_tmp[5] = {0x07, 0x00, 0x60, 0x00, 0x03};
    memcpy(login_packet, login_tmp, 5);
    login_packet[1] = *pppoe_counter % 0xFF;
    (*pppoe_counter)++;
    memcpy(login_packet + 12, sip, 4);
    if (*login_first) {
        login_packet[17] = 0x62;
    } else {
        login_packet[17] = 0x63;
    }
    memcpy(login_packet + 19, &drcom_config.pppoe_flag, 1);
    memcpy(login_packet + 20, seed, 4);
    unsigned char crc[8] = {0};
    *encrypt_type = seed[0] & 3;
    if (!*encrypt_mode) {
        *encrypt_type = 0;
    }
    gen_crc(seed, *encrypt_type, crc);
    unsigned char crc_tmp[32] = {0};
    memcpy(crc_tmp, login_packet, 32);
    memcpy(crc_tmp + 24, crc, 8);
    uint64_t ret = 0;
    uint64_t sum = 0;
    unsigned char crc2[4] = {0};
    if (*encrypt_type == 0) {
        for (int i = 0; i < 32; i += 4) {
            ret = 0;
            for(int j = 4; j > 0; j--) {
                ret = ret * 256 + (int)crc_tmp[i + j - 1];
            }
            sum ^= ret;
            sum &= 0xffffffff;
        }
        sum = sum * 19680126 & 0xffffffff;
        for (int i = 0; i < 4; i++) {
            crc2[i] = (unsigned char)(sum % 256);
            sum /= 256;
        }
        memcpy(login_packet + 24, crc2, 4);
    } else {
        memcpy(login_packet + 24, crc, 8);
    }
    // login_packet[39] = 0x8b;
    // memcpy(login_packet + 40, sip, 4);
    // unsigned char smask[4] = {0xff, 0xff, 0xff, 0xff};
    // memcpy(login_packet + 44, smask, 4);
    // login_packet[54] = 0x40;

    sendto(sockfd, login_packet, 96, 0, (struct sockaddr *)&addr, sizeof(addr));
    if (verbose_flag) {
        print_packet("[PPPoE_login sent] ", login_packet, 96);
    }
    if (logging_flag) {
        logging("[PPPoE_login sent] ", login_packet, 96);
    }
#ifdef TEST
    return 0;
#endif

    socklen_t addrlen = sizeof(addr);
    if (recvfrom(sockfd, recv_packet, 1024, 0, (struct sockaddr *)&addr, &addrlen) < 0) {
#ifdef WIN32
        get_lasterror("Failed to recv data");
#else
        perror("Failed to recv data");
#endif
        return 1;
    }

    if (verbose_flag) {
        print_packet("[PPPoE_login recv] ", recv_packet, 48);
    }
    if (logging_flag) {
        logging("[PPPoE_login recv] ", recv_packet, 48);
    }

    if (recv_packet[0] != 0x07) {
        printf("Bad pppoe_login response received.\n");
        return 1;
    }

    if(recvfrom(sockfd, recv_packet, 1024, 0, (struct sockaddr *)&addr, &addrlen) >= 0) {
        DEBUG_PRINT(("Get notice packet."));
    }

    return 0;
}

int dogcom(int try_times) {
#ifdef WIN32
    WORD sockVersion = MAKEWORD(2, 2);
    WSADATA wsaData;
    if(WSAStartup(sockVersion, &wsaData) != 0) {
        return 1;
    }
#endif
    int sockfd;

    struct sockaddr_in bind_addr;
    memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sin_family = AF_INET;
    if (verbose_flag) {
        printf("You are binding at %s!\n\n", bind_ip);
    }
#ifdef WIN32
    bind_addr.sin_addr.S_un.S_addr = inet_addr(bind_ip);
#else
    bind_addr.sin_addr.s_addr = inet_addr(bind_ip);
#endif
    bind_addr.sin_port = htons(BIND_PORT);

    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
#ifdef WIN32
    dest_addr.sin_addr.S_un.S_addr = inet_addr(drcom_config.server);
#else
    dest_addr.sin_addr.s_addr = inet_addr(drcom_config.server);
#endif
    dest_addr.sin_port = htons(DEST_PORT);

    srand(time(NULL));

    // create socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
#ifdef WIN32
        get_lasterror("Failed to create socket");
#else
        perror("Failed to create socket");
#endif
        return 1;
    }
    // bind socket
    if (bind(sockfd, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0) {
#ifdef WIN32
        get_lasterror("Failed to bind socket");
#else
        perror("Failed to bind socket");
#endif
        return 1;
    }

    // set timeout
#ifdef WIN32
    int timeout = 3000;
#else
    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
#endif
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
#ifdef WIN32
        get_lasterror("Failed to set sock opt");
#else
        perror("Failed to set sock opt");
#endif
        return 1;
    }

    // start dogcoming
    if (strcmp(mode, "dhcp") == 0) {
        int login_failed_attempts = 0;
        int try_JLUversion = 0;
        for(int try_counter = 0; try_counter < try_times; try_counter++) {
            if(eternal_flag) {
                try_counter = 0;
            }
            unsigned char seed[4];
            unsigned char auth_information[16];
            if (dhcp_challenge(sockfd, dest_addr, seed)) {
                printf("Retrying...\n");
                if (logging_flag) {
                    logging("Retrying...", NULL, 0);
                }
                sleep(3);
            } else {
                usleep(200000); // 0.2 sec
                if (login_failed_attempts > 2) {
                    try_JLUversion = 1;
                }
                if (!dhcp_login(sockfd, dest_addr, seed, auth_information, try_JLUversion)) {
                    int keepalive_counter = 0;
                    int keepalive_try_counter = 0;
                    int first = 1;
                    while (1) {
                        if (!keepalive_1(sockfd, dest_addr, seed, auth_information)) {
                            usleep(200000); // 0.2 sec
                            if (keepalive_2(sockfd, dest_addr, &keepalive_counter, &first, 0)) {
                                continue;
                            }
                            if (verbose_flag) {
                                printf("Keepalive in loop.\n");
                            }
                            if (logging_flag) {
                                logging("Keepalive in loop.", NULL, 0);
                            }
                            sleep(20);
                        } else {
                            if (keepalive_try_counter > 5) {
                                break;
                            }
                            keepalive_try_counter ++;
                            continue;
                        }
                    }
                } else {
                    login_failed_attempts += 1;
                    printf("Retrying...\n");
                    if (logging_flag) {
                        logging("Retrying...", NULL, 0);
                    }
                    sleep(3);
                };
            }
        }
    } else if (strcmp(mode, "pppoe") == 0) {
        int pppoe_counter = 0;
        int keepalive_counter = 0;
        unsigned char seed[4], sip[4];  /* pppoe's seed == dhcp's KEEP_ALIVE_VERSION */
        int login_first = 1;
        int first = 1;
        int encrypt_mode = 0;
        int encrypt_type = 0;
        int try_counter = 0;
        while(1) {
            if (pppoe_challenge(sockfd, dest_addr, &pppoe_counter, seed, sip, &encrypt_mode)) {
                printf("Retrying...\n");
                if (logging_flag) {
                    logging("Retrying...", NULL, 0);
                }
                login_first = 1;
                try_counter++;
                if(eternal_flag) {
                    try_counter = 0;
                }
                if (try_counter >= try_times) {
                    break;
                }
                sleep(5);
                continue;
            } else {
                usleep(200000); // 0.2 sec
                if (pppoe_login(sockfd, dest_addr, &pppoe_counter, seed, sip, &login_first, &encrypt_mode, &encrypt_type)) {
                    continue;
                } else {
                    login_first = 0;
                    if (keepalive_2(sockfd, dest_addr, &keepalive_counter, &first, &encrypt_type)) {
                        continue;
                    } else {
                        if (verbose_flag) {
                            printf("PPPoE in loop.\n");
                        }
                        if (logging_flag) {
                            logging("PPPoE in loop.", NULL, 0);
                        }
                        sleep(10);
                        continue;
                    }
                }
            }
        }
    }

    printf(">>>>> Failed to keep in touch with server, exiting <<<<<\n\n");
    if (logging_flag) {
        logging(">>>>> Failed to keep in touch with server, exiting <<<<<", NULL, 0);
    }
#ifdef WIN32
    closesocket(sockfd);
    WSACleanup();
#else
    close(sockfd);
#endif
    return 1;
}


void print_packet(char msg[10], unsigned char *packet, int length) {
    printf("%s", msg);
    for (int i = 0; i < length; i++) {
        printf("%02x", packet[i]);
    }
    printf("\n");
}

void logging(char msg[10], unsigned char *packet, int length) {
    FILE *ptr_file;
    ptr_file = fopen(log_path, "a");

    char *wday[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    time_t timep;
    struct tm *p;
    time(&timep);
    p = localtime(&timep);
    fprintf(ptr_file, "[%04d/%02d/%02d %s %02d:%02d:%02d] ",
            (1900 + p -> tm_year), (1 + p -> tm_mon), p -> tm_mday, wday[p -> tm_wday], p -> tm_hour, p -> tm_min, p -> tm_sec);    

    fprintf(ptr_file, "%s", msg);
    for (int i = 0; i < length; i++) {
        fprintf(ptr_file, "%02x", packet[i]);
    }
    fprintf(ptr_file, "\n");

    fclose(ptr_file);
}

#ifdef WIN32
void get_lasterror(char *msg) {
    char err_msg[256];
    err_msg[0] = '\0';
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        err_msg,
        sizeof(err_msg),
        NULL);
    fprintf(stderr,"%s: %s", msg, err_msg);
}
#endif