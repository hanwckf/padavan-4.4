/*
 * HTTP authentication
 * Copyright (c) 2010 Martin Storsjo
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "httpauth.h"
#include "libavutil/base64.h"
#include "libavutil/avstring.h"
#include "internal.h"
#include "libavutil/random_seed.h"
#include "libavutil/md5.h"
#include "avformat.h"
#include <ctype.h>

static void parse_key_value(const char *params,
                            void (*callback_get_buf)(HTTPAuthState *state,
                            const char *key, int key_len,
                            char **dest, int *dest_len), HTTPAuthState *state)
{
    const char *ptr = params;

    /* Parse key=value pairs. */
    for (;;) {
        const char *key;
        char *dest = NULL, *dest_end;
        int key_len, dest_len = 0;

        /* Skip whitespace and potential commas. */
        while (*ptr && (isspace(*ptr) || *ptr == ','))
            ptr++;
        if (!*ptr)
            break;

        key = ptr;

        if (!(ptr = strchr(key, '=')))
            break;
        ptr++;
        key_len = ptr - key;

        callback_get_buf(state, key, key_len, &dest, &dest_len);
        dest_end = dest + dest_len - 1;

        if (*ptr == '\"') {
            ptr++;
            while (*ptr && *ptr != '\"') {
                if (*ptr == '\\') {
                    if (!ptr[1])
                        break;
                    if (dest && dest < dest_end)
                        *dest++ = ptr[1];
                    ptr += 2;
                } else {
                    if (dest && dest < dest_end)
                        *dest++ = *ptr;
                    ptr++;
                }
            }
            if (*ptr == '\"')
                ptr++;
        } else {
            for (; *ptr && !(isspace(*ptr) || *ptr == ','); ptr++)
                if (dest && dest < dest_end)
                    *dest++ = *ptr;
        }
        if (dest)
            *dest = 0;
    }
}

static void handle_basic_params(HTTPAuthState *state, const char *key,
                                int key_len, char **dest, int *dest_len)
{
    if (!strncmp(key, "realm=", key_len)) {
        *dest     =        state->realm;
        *dest_len = sizeof(state->realm);
    }
}

static void handle_digest_params(HTTPAuthState *state, const char *key,
                                 int key_len, char **dest, int *dest_len)
{
    DigestParams *digest = &state->digest_params;

    if (!strncmp(key, "realm=", key_len)) {
        *dest     =        state->realm;
        *dest_len = sizeof(state->realm);
    } else if (!strncmp(key, "nonce=", key_len)) {
        *dest     =        digest->nonce;
        *dest_len = sizeof(digest->nonce);
    } else if (!strncmp(key, "opaque=", key_len)) {
        *dest     =        digest->opaque;
        *dest_len = sizeof(digest->opaque);
    } else if (!strncmp(key, "algorithm=", key_len)) {
        *dest     =        digest->algorithm;
        *dest_len = sizeof(digest->algorithm);
    } else if (!strncmp(key, "qop=", key_len)) {
        *dest     =        digest->qop;
        *dest_len = sizeof(digest->qop);
    }
}

static void handle_digest_update(HTTPAuthState *state, const char *key,
                                 int key_len, char **dest, int *dest_len)
{
    DigestParams *digest = &state->digest_params;

    if (!strncmp(key, "nextnonce=", key_len)) {
        *dest     =        digest->nonce;
        *dest_len = sizeof(digest->nonce);
    }
}

static void choose_qop(char *qop, int size)
{
    char *ptr = strstr(qop, "auth");
    char *end = ptr + strlen("auth");

    if (ptr && (!*end || isspace(*end) || *end == ',') &&
        (ptr == qop || isspace(ptr[-1]) || ptr[-1] == ',')) {
        av_strlcpy(qop, "auth", size);
    } else {
        qop[0] = 0;
    }
}

void ff_http_auth_handle_header(HTTPAuthState *state, const char *key,
                                const char *value)
{
    if (!strcmp(key, "WWW-Authenticate")) {
        const char *p;
        if (av_stristart(value, "Basic ", &p) &&
            state->auth_type <= HTTP_AUTH_BASIC) {
            state->auth_type = HTTP_AUTH_BASIC;
            state->realm[0] = 0;
            parse_key_value(p, handle_basic_params, state);
        } else if (av_stristart(value, "Digest ", &p) &&
                   state->auth_type <= HTTP_AUTH_DIGEST) {
            state->auth_type = HTTP_AUTH_DIGEST;
            memset(&state->digest_params, 0, sizeof(DigestParams));
            state->realm[0] = 0;
            parse_key_value(p, handle_digest_params, state);
            choose_qop(state->digest_params.qop,
                       sizeof(state->digest_params.qop));
        }
    } else if (!strcmp(key, "Authentication-Info")) {
        parse_key_value(value, handle_digest_update, state);
    }
}


static void update_md5_strings(struct AVMD5 *md5ctx, ...)
{
    va_list vl;

    va_start(vl, md5ctx);
    while (1) {
        const char* str = va_arg(vl, const char*);
        if (!str)
            break;
        av_md5_update(md5ctx, str, strlen(str));
    }
    va_end(vl);
}

/* Generate a digest reply, according to RFC 2617. */
static char *make_digest_auth(HTTPAuthState *state, const char *username,
                              const char *password, const char *uri,
                              const char *method)
{
    DigestParams *digest = &state->digest_params;
    int len;
    uint32_t cnonce_buf[2];
    char cnonce[17];
    char nc[9];
    int i;
    char A1hash[33], A2hash[33], response[33];
    struct AVMD5 *md5ctx;
    uint8_t hash[16];
    char *authstr;

    digest->nc++;
    snprintf(nc, sizeof(nc), "%08x", digest->nc);

    /* Generate a client nonce. */
    for (i = 0; i < 2; i++)
        cnonce_buf[i] = ff_random_get_seed();
    ff_data_to_hex(cnonce, (const uint8_t*) cnonce_buf, sizeof(cnonce_buf), 1);
    cnonce[2*sizeof(cnonce_buf)] = 0;

    md5ctx = av_malloc(av_md5_size);
    if (!md5ctx)
        return NULL;

    av_md5_init(md5ctx);
    update_md5_strings(md5ctx, username, ":", state->realm, ":", password, NULL);
    av_md5_final(md5ctx, hash);
    ff_data_to_hex(A1hash, hash, 16, 1);
    A1hash[32] = 0;

    if (!strcmp(digest->algorithm, "") || !strcmp(digest->algorithm, "MD5")) {
    } else if (!strcmp(digest->algorithm, "MD5-sess")) {
        av_md5_init(md5ctx);
        update_md5_strings(md5ctx, A1hash, ":", digest->nonce, ":", cnonce, NULL);
        av_md5_final(md5ctx, hash);
        ff_data_to_hex(A1hash, hash, 16, 1);
        A1hash[32] = 0;
    } else {
        /* Unsupported algorithm */
        av_free(md5ctx);
        return NULL;
    }

    av_md5_init(md5ctx);
    update_md5_strings(md5ctx, method, ":", uri, NULL);
    av_md5_final(md5ctx, hash);
    ff_data_to_hex(A2hash, hash, 16, 1);
    A2hash[32] = 0;

    av_md5_init(md5ctx);
    update_md5_strings(md5ctx, A1hash, ":", digest->nonce, NULL);
    if (!strcmp(digest->qop, "auth") || !strcmp(digest->qop, "auth-int")) {
        update_md5_strings(md5ctx, ":", nc, ":", cnonce, ":", digest->qop, NULL);
    }
    update_md5_strings(md5ctx, ":", A2hash, NULL);
    av_md5_final(md5ctx, hash);
    ff_data_to_hex(response, hash, 16, 1);
    response[32] = 0;

    av_free(md5ctx);

    if (!strcmp(digest->qop, "") || !strcmp(digest->qop, "auth")) {
    } else if (!strcmp(digest->qop, "auth-int")) {
        /* qop=auth-int not supported */
        return NULL;
    } else {
        /* Unsupported qop value. */
        return NULL;
    }

    len = strlen(username) + strlen(state->realm) + strlen(digest->nonce) +
              strlen(uri) + strlen(response) + strlen(digest->algorithm) +
              strlen(digest->opaque) + strlen(digest->qop) + strlen(cnonce) +
              strlen(nc) + 150;

    authstr = av_malloc(len);
    if (!authstr)
        return NULL;
    snprintf(authstr, len, "Authorization: Digest ");

    /* TODO: Escape the quoted strings properly. */
    av_strlcatf(authstr, len, "username=\"%s\"",   username);
    av_strlcatf(authstr, len, ",realm=\"%s\"",     state->realm);
    av_strlcatf(authstr, len, ",nonce=\"%s\"",     digest->nonce);
    av_strlcatf(authstr, len, ",uri=\"%s\"",       uri);
    av_strlcatf(authstr, len, ",response=\"%s\"",  response);
    if (digest->algorithm[0])
        av_strlcatf(authstr, len, ",algorithm=%s",  digest->algorithm);
    if (digest->opaque[0])
        av_strlcatf(authstr, len, ",opaque=\"%s\"", digest->opaque);
    if (digest->qop[0]) {
        av_strlcatf(authstr, len, ",qop=\"%s\"",    digest->qop);
        av_strlcatf(authstr, len, ",cnonce=\"%s\"", cnonce);
        av_strlcatf(authstr, len, ",nc=%s",         nc);
    }

    av_strlcatf(authstr, len, "\r\n");

    return authstr;
}

char *ff_http_auth_create_response(HTTPAuthState *state, const char *auth,
                                   const char *path, const char *method)
{
    char *authstr = NULL;

    if (!auth || !strchr(auth, ':'))
        return NULL;

    if (state->auth_type == HTTP_AUTH_BASIC) {
        int auth_b64_len = (strlen(auth) + 2) / 3 * 4 + 1;
        int len = auth_b64_len + 30;
        char *ptr;
        authstr = av_malloc(len);
        if (!authstr)
            return NULL;
        snprintf(authstr, len, "Authorization: Basic ");
        ptr = authstr + strlen(authstr);
        av_base64_encode(ptr, auth_b64_len, auth, strlen(auth));
        av_strlcat(ptr, "\r\n", len);
    } else if (state->auth_type == HTTP_AUTH_DIGEST) {
        char *username = av_strdup(auth), *password;

        if (!username)
            return NULL;

        if ((password = strchr(username, ':'))) {
            *password++ = 0;
            authstr = make_digest_auth(state, username, password, path, method);
        }
        av_free(username);
    }
    return authstr;
}

