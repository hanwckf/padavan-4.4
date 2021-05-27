#ifndef _ICONV_H
#define _ICONV_H 1

#define _LIBICONV_VERSION 0x010B    /* version number: (major<<8) + minor */

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int _libiconv_version; /* Likewise */

typedef long iconv_t;

extern iconv_t
iconv_open(const char *tocode, const char *fromcode);

extern size_t
iconv(iconv_t cd, char **inbuf, size_t *inbytesleft,
                  char **outbuf, size_t *outbytesleft);

extern int
iconv_close(iconv_t cd);

#define libiconv_set_relocation_prefix(...) do {} while(0)

#ifdef __cplusplus
}
#endif

#endif /* _ICONV_H */
