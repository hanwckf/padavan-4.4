#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "configparse.h"
#include "debug.h"

int verbose_flag = 0;
int logging_flag = 0;
int eapol_flag = 0;
int eternal_flag = 0;
char *log_path;
char mode[10];
char bind_ip[20];
struct config drcom_config;

static int read_d_config(char *buf, int size);
static int read_p_config(char *buf, int size);

int config_parse(char *filepath) {
    FILE *ptr_file;
    char buf[100];

    ptr_file = fopen(filepath, "r");
    if (!ptr_file) {
        printf("Failed to read config file.\n");
        exit(1);
    }

    while (fgets(buf, sizeof(buf), ptr_file)) {
        if (strcmp(mode, "dhcp") == 0) {
            read_d_config(buf, sizeof(buf));
        } else if (strcmp(mode, "pppoe") == 0) {
            read_p_config(buf, sizeof(buf));
        }
    }
    if (verbose_flag) {
        printf("\n\n");
    }
    fclose(ptr_file);

    return 0;
}

static int read_d_config(char *buf, int size) {
    if (verbose_flag) {
        printf("%s", buf);
    }

    char *delim = " ='\r\n";
    char *delim2 = "\\x";
    char *key;
    char *value;
    if (strlen(key = strtok(buf, delim))) {
        value = strtok(NULL, delim);
    }
    drcom_config.keepalive1_mod = 0;

    if (strcmp(key, "server") == 0) {
        strcpy(drcom_config.server, value);
        DEBUG_PRINT(("[PARSER_DEBUG]%s\n", drcom_config.server));
    } else if (strcmp(key, "username") == 0) {
        strcpy(drcom_config.username, value);
        DEBUG_PRINT(("[PARSER_DEBUG]%s\n", drcom_config.username));
    } else if (strcmp(key, "password") == 0) {
        strcpy(drcom_config.password, value);
        DEBUG_PRINT(("[PARSER_DEBUG]%s\n", drcom_config.password));
    } else if (strcmp(key, "CONTROLCHECKSTATUS") == 0) {
        value = strtok(value, delim2);
        sscanf(value, "%02hhx", &drcom_config.CONTROLCHECKSTATUS);
        DEBUG_PRINT(("[PARSER_DEBUG]0x%02x\n", drcom_config.CONTROLCHECKSTATUS));
    } else if (strcmp(key, "ADAPTERNUM") == 0) {
        value = strtok(value, delim2);
        sscanf(value, "%02hhx", &drcom_config.ADAPTERNUM);
        DEBUG_PRINT(("[PARSER_DEBUG]0x%02x\n", drcom_config.ADAPTERNUM));
    } else if (strcmp(key, "host_ip") == 0) {
        strcpy(drcom_config.host_ip, value);
        DEBUG_PRINT(("[PARSER_DEBUG]%s\n", drcom_config.host_ip));
    } else if (strcmp(key, "IPDOG") == 0) {
        value = strtok(value, delim2);
        sscanf(value, "%02hhx", &drcom_config.IPDOG);
        DEBUG_PRINT(("[PARSER_DEBUG]0x%02x\n", drcom_config.IPDOG));
    } else if (strcmp(key, "host_name") == 0) {
        strcpy(drcom_config.host_name, value);
        DEBUG_PRINT(("[PARSER_DEBUG]%s\n", drcom_config.host_name));
    } else if (strcmp(key, "PRIMARY_DNS") == 0) {
        strcpy(drcom_config.PRIMARY_DNS, value);
        DEBUG_PRINT(("[PARSER_DEBUG]%s\n", drcom_config.PRIMARY_DNS));
    } else if (strcmp(key, "dhcp_server") == 0) {
        strcpy(drcom_config.dhcp_server, value);
        DEBUG_PRINT(("[PARSER_DEBUG]%s\n", drcom_config.dhcp_server));
    } else if (strcmp(key, "AUTH_VERSION") == 0) {
        char *v1 = strtok(value, delim2);
        char *v2 = strtok(NULL, delim2);
        sscanf(v1, "%02hhx", v1);
        sscanf(v2, "%02hhx", v2);
        memcpy(&drcom_config.AUTH_VERSION[0], v1, 1);
        memcpy(&drcom_config.AUTH_VERSION[1], v2, 1);
        DEBUG_PRINT(("[PARSER_DEBUG]0x%02x\n", drcom_config.AUTH_VERSION[0]));
        DEBUG_PRINT(("[PARSER_DEBUG]0x%02x\n", drcom_config.AUTH_VERSION[1]));
    } else if (strcmp(key, "mac") == 0) {
        char *delim3 = "x";
        // strsep(&value, delim3);
        value = strtok(value, delim3);
        value = strtok(NULL, delim3);
        sscanf(value, "%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx",
               &drcom_config.mac[0],
               &drcom_config.mac[1],
               &drcom_config.mac[2],
               &drcom_config.mac[3],
               &drcom_config.mac[4],
               &drcom_config.mac[5]);
#ifdef DEBUG
        printf("[PARSER_DEBUG]0x");
        for (int i = 0; i < 6; i++) {
            printf("%02x", drcom_config.mac[i]);
        }
        printf("\n");
#endif
    } else if (strcmp(key, "host_os") == 0) {
        strcpy(drcom_config.host_os, value);
        DEBUG_PRINT(("[PARSER_DEBUG]%s\n", drcom_config.host_os));
    } else if (strcmp(key, "KEEP_ALIVE_VERSION") == 0) {
        char *v1 = strtok(value, delim2);
        char *v2 = strtok(NULL, delim2);
        sscanf(v1, "%02hhx", v1);
        sscanf(v2, "%02hhx", v2);
        memcpy(&drcom_config.KEEP_ALIVE_VERSION[0], v1, 1);
        memcpy(&drcom_config.KEEP_ALIVE_VERSION[1], v2, 1);
        DEBUG_PRINT(("[PARSER_DEBUG]0x%02x\n", drcom_config.KEEP_ALIVE_VERSION[0]));
        DEBUG_PRINT(("[PARSER_DEBUG]0x%02x\n", drcom_config.KEEP_ALIVE_VERSION[1]));
    } else if (strcmp(key, "ror_version") == 0) {
        if (strcmp(value, "True") == 0) {
            drcom_config.ror_version = 1;
        } else  {
            drcom_config.ror_version = 0;
        }
        DEBUG_PRINT(("\n[PARSER_DEBUG]\n%d\n", drcom_config.ror_version));
    } else if (strcmp(key, "keepalive1_mod") == 0) {
        if (strcmp(value, "True") == 0) {
            drcom_config.keepalive1_mod = 1;
        } else  {
            drcom_config.keepalive1_mod = 0;
        }
        DEBUG_PRINT(("\n[PARSER_DEBUG]\n%d\n", drcom_config.keepalive1_mod));
    } else {
        return 1;
    }

    return 0;
}

static int read_p_config(char *buf, int size) {
    if (verbose_flag) {
        printf("%s", buf);
    }

    char *delim = " ='\r\n";
    char *delim2 = "\\x";
    char *key;
    char *value;
    if (strlen(key = strtok(buf, delim))) {
        value = strtok(NULL, delim);
    }

    if (strcmp(key, "server") == 0) {
        strcpy(drcom_config.server, value);
        DEBUG_PRINT(("[PARSER_DEBUG]%s\n", drcom_config.server));
    } else if (strcmp(key, "pppoe_flag") == 0) {
        value = strtok(value, delim2);
        sscanf(value, "%02hhx", &drcom_config.pppoe_flag);
        DEBUG_PRINT(("[PARSER_DEBUG]0x%02x\n", drcom_config.pppoe_flag));
    } else if (strcmp(key, "keep_alive2_flag") == 0) {
        value = strtok(value, delim2);
        sscanf(value, "%02hhx", &drcom_config.keep_alive2_flag);
        DEBUG_PRINT(("\n[PARSER_DEBUG]0x%02x\n", drcom_config.keep_alive2_flag));
    } else {
        return 1;
    }

    return 0;
}