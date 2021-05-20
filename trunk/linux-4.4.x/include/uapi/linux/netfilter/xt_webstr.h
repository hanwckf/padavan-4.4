/*
 * Copyright (C) 2009 - 2010, CyberTAN Corporation
 *
 * All Rights Reserved.
 * You can redistribute it and/or modify it under the terms of the GPL v2
 *
 * THIS SOFTWARE IS OFFERED "AS IS", AND CYBERTAN GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. CYBERTAN
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NON INFRINGEMENT CONCERNING THIS SOFTWARE.
 */

/*
Includes Intel Corporation's changes/modifications dated: 2014.
Changed/modified portions - Copyright © 2014, Intel Corporation.
*/
#ifndef _XT_WEBSTR_H
#define _XT_WEBSTR_H

#define BM_MAX_NLEN 256
#define BM_MAX_HLEN 1024

#define BLK_JAVA		0x01
#define BLK_ACTIVE		0x02
#define BLK_COOKIE		0x04
#define BLK_PROXY		0x08

typedef char *(*proc_xt_search) (char *, char *, int, int);

struct xt_webstr_info {
    char string[BM_MAX_NLEN];
    u_int16_t invert;
    u_int16_t len;
    u_int8_t type;
};

enum xt_webstr_type
{
    XT_WEBSTR_HOST,
    XT_WEBSTR_URL,
    XT_WEBSTR_CONTENT
};

#endif /* _XT_WEBSTR_H */
