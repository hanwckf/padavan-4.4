#ifndef KEEPALIVE_H_
#define KEEPALIVE_H_

int keepalive_1(int sockfd, struct sockaddr_in addr, unsigned char seed[], unsigned char auth_information[]);
int keepalive_2(int sockfd, struct sockaddr_in addr, int *keepalive_counter, int *first, int *encrypt_type);
void gen_crc(unsigned char seed[], int encrypt_type, unsigned char crc[]);
void keepalive_2_packetbuilder(unsigned char keepalive_2_packet[], int keepalive_counter, int filepacket, int type, int encrypt_type);

#endif // KEEPALIVE_H_