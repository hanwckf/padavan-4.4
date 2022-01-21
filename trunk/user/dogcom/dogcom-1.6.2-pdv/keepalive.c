#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef WIN32
#include <winsock2.h>
typedef int socklen_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#include "libs/md4.h"
#include "libs/md5.h"
#include "libs/sha1.h"
#include "keepalive.h"
#include "configparse.h"
#include "auth.h"
#include "debug.h"

int keepalive_1(int sockfd, struct sockaddr_in addr, unsigned char seed[], unsigned char auth_information[]) {
    if (drcom_config.keepalive1_mod) {
        unsigned char keepalive_1_packet1[8] = {0x07, 0x01, 0x08, 0x00, 0x01, 0x00, 0x00, 0x00};
        unsigned char recv_packet1[1024], keepalive_1_packet2[38], recv_packet2[1024];
        memset(keepalive_1_packet2, 0, 38);
        sendto(sockfd, keepalive_1_packet1, 8, 0, (struct sockaddr *)&addr, sizeof(addr));
        if (verbose_flag) {
            print_packet("[Keepalive1 sent] ", keepalive_1_packet1, 42);
        }
        if (logging_flag) {
            logging("[Keepalive1 sent] ", keepalive_1_packet1, 42);
        }
#ifdef TEST
        printf("[TEST MODE]IN TEST MODE, PASS\n");
        return 0;
#endif
        socklen_t addrlen = sizeof(addr);
        while(1) {
            if (recvfrom(sockfd, recv_packet1, 1024, 0, (struct sockaddr *)&addr, &addrlen) < 0) {
#ifdef WIN32
                get_lasterror("Failed to recv data");
#else
                perror("Failed to recv data");
#endif
                return 1;
            } else {
                if (verbose_flag) {
                    print_packet("[Keepalive1 challenge_recv] ", recv_packet1, 100);
                }
                if (logging_flag) {
                    logging("[Keepalive1 challenge_recv] ", recv_packet1, 100);
                }

                if (recv_packet1[0] == 0x07) {
                    break;
                } else if (recv_packet1[0] == 0x4d) {
                    DEBUG_PRINT(("Get notice packet.\n"));
                    continue;
                } else{
                    printf("Bad keepalive1 challenge response received.\n");
                    return 1;
                }
            }
        }

        unsigned char keepalive1_seed[4] = {0};
        int encrypt_type;
        unsigned char crc[8] = {0};
        memcpy(keepalive1_seed, &recv_packet1[8], 4);
        encrypt_type = keepalive1_seed[0] & 3;
        gen_crc(keepalive1_seed, encrypt_type, crc);
        keepalive_1_packet2[0] = 0xff;
        memcpy(keepalive_1_packet2+8, keepalive1_seed, 4);
        memcpy(keepalive_1_packet2+12, crc, 8);
        memcpy(keepalive_1_packet2+20, auth_information, 16);
        keepalive_1_packet2[36] = rand() & 0xff;
        keepalive_1_packet2[37] = rand() & 0xff;

        sendto(sockfd, keepalive_1_packet2, 42, 0, (struct sockaddr *)&addr, sizeof(addr));

        if (recvfrom(sockfd, recv_packet2, 1024, 0, (struct sockaddr *)&addr, &addrlen) < 0) {
#ifdef WIN32
            get_lasterror("Failed to recv data");
#else
            perror("Failed to recv data");
#endif
            return 1;
        } else {
            if (verbose_flag) {
                print_packet("[Keepalive1 recv] ", recv_packet2, 100);
            }
            if (logging_flag) {
                logging("[Keepalive1 recv] ", recv_packet2, 100);
            }

            if (recv_packet2[0] != 0x07) {
                printf("Bad keepalive1 response received.\n");
                return 1;
            }
        }

    } else {
        unsigned char keepalive_1_packet[42], recv_packet[1024], MD5A[16];
        memset(keepalive_1_packet, 0, 42);
        keepalive_1_packet[0] = 0xff;
        int MD5A_len = 6 + strlen(drcom_config.password);
        unsigned char MD5A_str[MD5A_len];
        MD5A_str[0] = 0x03;
        MD5A_str[1] = 0x01;
        memcpy(MD5A_str + 2, seed, 4);
        memcpy(MD5A_str + 6, drcom_config.password, strlen(drcom_config.password));
        MD5(MD5A_str, MD5A_len, MD5A);
        memcpy(keepalive_1_packet + 1, MD5A, 16);
        memcpy(keepalive_1_packet + 20, auth_information, 16);
        keepalive_1_packet[36] = rand() & 0xff;
        keepalive_1_packet[37] = rand() & 0xff;

        sendto(sockfd, keepalive_1_packet, 42, 0, (struct sockaddr *)&addr, sizeof(addr));

        if (verbose_flag) {
            print_packet("[Keepalive1 sent] ", keepalive_1_packet, 42);
        }
        if (logging_flag) {
            logging("[Keepalive1 sent] ", keepalive_1_packet, 42);
        }

#ifdef TEST
        printf("[TEST MODE]IN TEST MODE, PASS\n");
        return 0;
#endif

        socklen_t addrlen = sizeof(addr);
        while(1) {
            if (recvfrom(sockfd, recv_packet, 1024, 0, (struct sockaddr *)&addr, &addrlen) < 0) {
#ifdef WIN32
                get_lasterror("Failed to recv data");
#else
                perror("Failed to recv data");
#endif
                return 1;
            } else {
                if (verbose_flag) {
                    print_packet("[Keepalive1 recv] ", recv_packet, 100);
                }
                if (logging_flag) {
                    logging("[Keepalive1 recv] ", recv_packet, 100);
                }

                if (recv_packet[0] == 0x07) {
                    break;
                } else if (recv_packet[0] == 0x4d) {
                    DEBUG_PRINT(("Get notice packet."));
                    continue;
                } else{
                    printf("Bad keepalive1 response received.\n");
                    return 1;
                }
            }
        }
    }

    return 0;
}

void gen_crc(unsigned char seed[], int encrypt_type, unsigned char crc[]) {
    if (encrypt_type == 0) {
        char DRCOM_DIAL_EXT_PROTO_CRC_INIT[4] = {0xc7, 0x2f, 0x31, 0x01};
        char gencrc_tmp[4] = {0x7e};
        memcpy(crc, DRCOM_DIAL_EXT_PROTO_CRC_INIT, 4);
        memcpy(crc + 4, gencrc_tmp, 4);
    } else if (encrypt_type == 1) {
        unsigned char hash[32] = {0};
        MD5(seed, 4, hash);
        crc[0] = hash[2];
        crc[1] = hash[3];
        crc[2] = hash[8];
        crc[3] = hash[9];
        crc[4] = hash[5];
        crc[5] = hash[6];
        crc[6] = hash[13];
        crc[7] = hash[14];
    } else if (encrypt_type == 2) {
        unsigned char hash[32] = {0};
        MD4(seed, 4, hash);
        crc[0] = hash[1];
        crc[1] = hash[2];
        crc[2] = hash[8];
        crc[3] = hash[9];
        crc[4] = hash[4];
        crc[5] = hash[5];
        crc[6] = hash[11];
        crc[7] = hash[12];
    } else if (encrypt_type == 3) {
        unsigned char hash[32] = {0};
        SHA1(seed, 4, hash);
        crc[0] = hash[2];
        crc[1] = hash[3];
        crc[2] = hash[9];
        crc[3] = hash[10];
        crc[4] = hash[5];
        crc[5] = hash[6];
        crc[6] = hash[15];
        crc[7] = hash[16];
    }
}


void keepalive_2_packetbuilder(unsigned char keepalive_2_packet[], int keepalive_counter, int filepacket, int type, int encrypt_type){
    keepalive_2_packet[0] = 0x07;
    keepalive_2_packet[1] = keepalive_counter;
    keepalive_2_packet[2] = 0x28;
    keepalive_2_packet[4] = 0x0b;
    keepalive_2_packet[5] = type;
    if (filepacket) {
        keepalive_2_packet[6] = 0x0f;
        keepalive_2_packet[7] = 0x27;
    } else {
        memcpy(keepalive_2_packet + 6, drcom_config.KEEP_ALIVE_VERSION, 2);
    }
    keepalive_2_packet[8] = 0x2f;
    keepalive_2_packet[9] = 0x12;
    if(type == 3) {
        unsigned char host_ip[4] = {0};
        if (strcmp(mode, "dhcp") == 0) {
            sscanf(drcom_config.host_ip, "%hhd.%hhd.%hhd.%hhd",
                &host_ip[0],
                &host_ip[1],
                &host_ip[2],
                &host_ip[3]);
            memcpy(keepalive_2_packet + 28, host_ip, 4);
        } else if (strcmp(mode, "pppoe") == 0) {
            unsigned char crc[8] = {0};
            gen_crc(keepalive_2_packet, encrypt_type, crc);
            memcpy(keepalive_2_packet + 32, crc, 8);
        }
    }
}

int keepalive_2(int sockfd, struct sockaddr_in addr, int *keepalive_counter, int *first, int *encrypt_type) {
    unsigned char keepalive_2_packet[40], recv_packet[1024], tail[4];
    socklen_t addrlen = sizeof(addr);

#ifdef TEST
        printf("[TEST MODE]IN TEST MODE, PASS\n");
#else
    if (*first) {
        // send the file packet
        memset(keepalive_2_packet, 0, 40);
        if (strcmp(mode, "pppoe") == 0) {
            keepalive_2_packetbuilder(keepalive_2_packet, *keepalive_counter % 0xFF, *first, 1, *encrypt_type);
        } else {
            keepalive_2_packetbuilder(keepalive_2_packet, *keepalive_counter % 0xFF, *first, 1, 0);
        }
        (*keepalive_counter)++;

        sendto(sockfd, keepalive_2_packet, 40, 0, (struct sockaddr *)&addr, sizeof(addr));

        if (verbose_flag) {
            print_packet("[Keepalive2_file sent] ", keepalive_2_packet, 40);
        }
        if (logging_flag) {
            logging("[Keepalive2_file sent] ", keepalive_2_packet, 40);
        }
        if (recvfrom(sockfd, recv_packet, 1024, 0, (struct sockaddr *)&addr, &addrlen) < 0) {
#ifdef WIN32
            get_lasterror("Failed to recv data");
#else
            perror("Failed to recv data");
#endif
            return 1;
        }
        if (verbose_flag) {
            print_packet("[Keepalive2_file recv] ", recv_packet, 40);
        }
        if (logging_flag) {
            logging("[Keepalive2_file recv] ", recv_packet, 40);
        }

        if (recv_packet[0] == 0x07) {
            if (recv_packet[2] == 0x10) {
                if (verbose_flag) {
                    printf("Filepacket received.\n");
                }
            } else if (recv_packet[2] != 0x28) {
                if (verbose_flag) {
                    printf("Bad keepalive2 response received.\n");
                }
                return 1;
            }
        } else {
            printf("Bad keepalive2 response received.\n");
            return 1;
        }
    }
#endif

    // send the first packet
    *first = 0;
    memset(keepalive_2_packet, 0, 40);
    if (strcmp(mode, "pppoe") == 0) {
        keepalive_2_packetbuilder(keepalive_2_packet, *keepalive_counter % 0xFF, *first, 1, *encrypt_type);
    } else {
        keepalive_2_packetbuilder(keepalive_2_packet, *keepalive_counter % 0xFF, *first, 1, 0);
    }
    (*keepalive_counter)++;
    sendto(sockfd, keepalive_2_packet, 40, 0, (struct sockaddr *)&addr, sizeof(addr));

    if (verbose_flag) {
        print_packet("[Keepalive2_A sent] ", keepalive_2_packet, 40);
    }
    if (logging_flag) {
        logging("[Keepalive2_A sent] ", keepalive_2_packet, 40);
    }

#ifdef TEST
    unsigned char test[4] = {0x13, 0x38, 0xe2, 0x11};
    memcpy(tail, test, 4);
    print_packet("[TEST MODE]<PREP TAIL> ", tail, 4);
#else
    if (recvfrom(sockfd, recv_packet, 1024, 0, (struct sockaddr *)&addr, &addrlen) < 0) {
#ifdef WIN32
        get_lasterror("Failed to recv data");
#else
        perror("Failed to recv data");
#endif
        return 1;
    }
    if (verbose_flag) {
        print_packet("[Keepalive2_B recv] ", recv_packet, 40);
    }
    if (logging_flag) {
        logging("[Keepalive2_B recv] ", recv_packet, 40);
    }

    if (recv_packet[0] == 0x07) {
        if (recv_packet[2] != 0x28) {
            printf("Bad keepalive2 response received.\n");
            return 1;
        }
    } else {
        printf("Bad keepalive2 response received.\n");
        return 1;
    }
    memcpy(tail, &recv_packet[16], 4);
#endif

#ifdef DEBUG
    print_packet("<GET TAIL> ", tail, 4);
#endif

    // send the third packet
    memset(keepalive_2_packet, 0, 40);
    if (strcmp(mode, "pppoe") == 0) {
        keepalive_2_packetbuilder(keepalive_2_packet, *keepalive_counter % 0xFF, *first, 3, *encrypt_type);
    } else {
        keepalive_2_packetbuilder(keepalive_2_packet, *keepalive_counter % 0xFF, *first, 3, 0);
    }
    memcpy(keepalive_2_packet + 16, tail, 4);
    (*keepalive_counter)++;
    sendto(sockfd, keepalive_2_packet, 40, 0, (struct sockaddr *)&addr, sizeof(addr));

    if (verbose_flag) {
        print_packet("[Keepalive2_C sent] ", keepalive_2_packet, 40);
    }
    if (logging_flag) {
        logging("[Keepalive2_C sent] ", keepalive_2_packet, 40);
    }

#ifdef TEST
    printf("[TEST MODE]IN TEST MODE, PASS\n");
    exit(0);
#endif

    if (recvfrom(sockfd, recv_packet, 1024, 0, (struct sockaddr *)&addr, &addrlen) < 0) {
#ifdef WIN32
        get_lasterror("Failed to recv data");
#else
        perror("Failed to recv data");
#endif
        return 1;
    }
    if (verbose_flag) {
        print_packet("[Keepalive2_D recv] ", recv_packet, 40);
    }
    if (logging_flag) {
        logging("[Keepalive2_D recv] ", recv_packet, 40);
    }

    if (recv_packet[0] == 0x07) {
        if (recv_packet[2] != 0x28) {
            printf("Bad keepalive2 response received.\n");
            return 1;
        }
    } else {
        printf("Bad keepalive2 response received.\n");
        return 1;
    }

    return 0;
}