/*
 * PAM image format
 * Copyright (c) 2002, 2003 Fabrice Bellard
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

#include "avcodec.h"
#include "bytestream.h"
#include "pnm.h"


static int pam_encode_frame(AVCodecContext *avctx, unsigned char *outbuf,
                            int buf_size, void *data)
{
    PNMContext *s     = avctx->priv_data;
    AVFrame *pict     = data;
    AVFrame * const p = (AVFrame*)&s->picture;
    int i, h, w, n, linesize, depth, maxval;
    const char *tuple_type;
    uint8_t *ptr;

    if (buf_size < avpicture_get_size(avctx->pix_fmt, avctx->width, avctx->height) + 200) {
        av_log(avctx, AV_LOG_ERROR, "encoded frame too large\n");
        return -1;
    }

    *p           = *pict;
    p->pict_type = FF_I_TYPE;
    p->key_frame = 1;

    s->bytestream_start =
    s->bytestream       = outbuf;
    s->bytestream_end   = outbuf+buf_size;

    h = avctx->height;
    w = avctx->width;
    switch (avctx->pix_fmt) {
    case PIX_FMT_MONOWHITE:
        n          = (w + 7) >> 3;
        depth      = 1;
        maxval     = 1;
        tuple_type = "BLACKANDWHITE";
        break;
    case PIX_FMT_GRAY8:
        n          = w;
        depth      = 1;
        maxval     = 255;
        tuple_type = "GRAYSCALE";
        break;
    case PIX_FMT_RGB24:
        n          = w * 3;
        depth      = 3;
        maxval     = 255;
        tuple_type = "RGB";
        break;
    case PIX_FMT_RGB32:
        n          = w * 4;
        depth      = 4;
        maxval     = 255;
        tuple_type = "RGB_ALPHA";
        break;
    default:
        return -1;
    }
    snprintf(s->bytestream, s->bytestream_end - s->bytestream,
             "P7\nWIDTH %d\nHEIGHT %d\nDEPTH %d\nMAXVAL %d\nTUPLETYPE %s\nENDHDR\n",
             w, h, depth, maxval, tuple_type);
    s->bytestream += strlen(s->bytestream);

    ptr      = p->data[0];
    linesize = p->linesize[0];

    if (avctx->pix_fmt == PIX_FMT_RGB32) {
        int j;
        unsigned int v;

        for (i = 0; i < h; i++) {
            for (j = 0; j < w; j++) {
                v = ((uint32_t *)ptr)[j];
                bytestream_put_be24(&s->bytestream, v);
                *s->bytestream++ = v >> 24;
            }
            ptr += linesize;
        }
    } else {
        for (i = 0; i < h; i++) {
            memcpy(s->bytestream, ptr, n);
            s->bytestream += n;
            ptr           += linesize;
        }
    }
    return s->bytestream - s->bytestream_start;
}


AVCodec pam_encoder = {
    "pam",
    AVMEDIA_TYPE_VIDEO,
    CODEC_ID_PAM,
    sizeof(PNMContext),
    ff_pnm_init,
    pam_encode_frame,
    .pix_fmts  = (const enum PixelFormat[]){PIX_FMT_RGB24, PIX_FMT_RGB32, PIX_FMT_GRAY8, PIX_FMT_MONOWHITE, PIX_FMT_NONE},
    .long_name = NULL_IF_CONFIG_SMALL("PAM (Portable AnyMap) image"),
};
