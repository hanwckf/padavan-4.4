#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/ioctl.h>

int
getnext (
        char *  src,
        int     separator,
        char *  dest
        )
{
        char *  c;
        int     len;

        if ( (src == NULL) || (dest == NULL) ) {
                return -1;
        }

        c = strchr(src, separator);
        if (c == NULL) {
                strcpy(dest, src);
                return -1;
        }
        len = c - src;
        strncpy(dest, src, len);
        dest[len] = '\0';
        return len + 1;
}

int
str_to_mac (
        unsigned char * mac,
        char *          str
        )
{
        int             len;
        char *          ptr = str;
        char            buf[128];
        int             i;

        for (i = 0; i < 5; i++) {
                if ((len = getnext(ptr, ':', buf)) == -1) {
                        return 1; /* parse error */
                }
                mac[i] = strtol(buf, NULL, 16);
                ptr += len;
        }
        mac[5] = strtol(ptr, NULL, 16);

        return 0;
}

int
str_to_ip (
        unsigned int * ip,
        char *          str
        )
{
        int             len;
        char *          ptr = str;
        char            buf[128];
        unsigned char   c[4];
        int             i;

        for (i = 0; i < 3; ++i) {
                if ((len = getnext(ptr, '.', buf)) == -1) {
                        return 1; /* parse error */
                }
                c[i] = atoi(buf);
                ptr += len;
        }
        c[3] = atoi(ptr);
        *ip = (c[0]<<24) + (c[1]<<16) + (c[2]<<8) + c[3];
        return 0;
}

int
str_to_ipv6 (
        unsigned int * ipv6,
        char *          str,
        unsigned int byte)
{
        int             len;
        char *          ptr = str;
        char            buf[128];
        unsigned short  c[8];
        int             i;
        for (i = 0; i < 7; i++) {
		
                if ((len = getnext(ptr, ':', buf)) == -1) {
                        return 1; /* parse error */
                }
                c[i] = strtoul(buf, NULL, 16);
                ptr += len;
                //printf("len=%d, c[%d]=%x\n",len, i, c[i]);
                
        }
        c[7] = atoi(ptr);
        *ipv6 = (c[2*byte] <<16) + (c[2*byte + 1]);
        return 0;
}

