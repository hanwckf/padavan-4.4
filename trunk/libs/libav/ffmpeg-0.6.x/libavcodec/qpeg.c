/*
 * QPEG codec
 * Copyright (c) 2004 Konstantin Shishkov
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

/**
 * @file
 * QPEG codec.
 */

#include "avcodec.h"

typedef struct QpegContext{
    AVCodecContext *avctx;
    AVFrame pic;
    uint8_t *refdata;
} QpegContext;

static void qpeg_decode_intra(const uint8_t *src, uint8_t *dst, int size,
                            int stride, int width, int height)
{
    int i;
    int code;
    int c0, c1;
    int run, copy;
    int filled = 0;
    int rows_to_go;

    rows_to_go = height;
    height--;
    dst = dst + height * stride;

    while((size > 0) && (rows_to_go > 0)) {
        code = *src++;
        size--;
        run = copy = 0;
        if(code == 0xFC) /* end-of-picture code */
            break;
        if(code >= 0xF8) { /* very long run */
            c0 = *src++;
            c1 = *src++;
            size -= 2;
            run = ((code & 0x7) << 16) + (c0 << 8) + c1 + 2;
        } else if (code >= 0xF0) { /* long run */
            c0 = *src++;
            size--;
            run = ((code & 0xF) << 8) + c0 + 2;
        } else if (code >= 0xE0) { /* short run */
            run = (code & 0x1F) + 2;
        } else if (code >= 0xC0) { /* very long copy */
            c0 = *src++;
            c1 = *src++;
            size -= 2;
            copy = ((code & 0x3F) << 16) + (c0 << 8) + c1 + 1;
        } else if (code >= 0x80) { /* long copy */
            c0 = *src++;
            size--;
            copy = ((code & 0x7F) << 8) + c0 + 1;
        } else { /* short copy */
            copy = code + 1;
        }

        /* perform actual run or copy */
        if(run) {
            int p;

            p = *src++;
            size--;
            for(i = 0; i < run; i++) {
                dst[filled++] = p;
                if (filled >= width) {
                    filled = 0;
                    dst -= stride;
                    rows_to_go--;
                    if(rows_to_go <= 0)
                        break;
                }
            }
        } else {
            size -= copy;
            for(i = 0; i < copy; i++) {
                dst[filled++] = *src++;
                if (filled >= width) {
                    filled = 0;
                    dst -= stride;
                    rows_to_go--;
                    if(rows_to_go <= 0)
                        break;
                }
            }
        }
    }
}

static const int qpeg_table_h[16] =
 { 0x00, 0x20, 0x20, 0x20, 0x18, 0x10, 0x10, 0x20, 0x10, 0x08, 0x18, 0x08, 0x08, 0x18, 0x10, 0x04};
static const int qpeg_table_w[16] =
 { 0x00, 0x20, 0x18, 0x08, 0x18, 0x10, 0x20, 0x10, 0x08, 0x10, 0x20, 0x20, 0x08, 0x10, 0x18, 0x04};

/* Decodes delta frames */
static void qpeg_decode_inter(const uint8_t *src, uint8_t *dst, int size,
                            int stride, int width, int height,
                            int delta, const uint8_t *ctable, uint8_t *refdata)
{
    int i, j;
    int code;
    int filled = 0;
    int orig_height;

    /* copy prev frame */
    for(i = 0; i < height; i++)
        memcpy(refdata + (i * width), dst + (i * stride), width);

    orig_height = height;
    height--;
    dst = dst + height * stride;

    while((size > 0) && (height >= 0)) {
        code = *src++;
        size--;

        if(delta) {
            /* motion compensation */
            while((code & 0xF0) == 0xF0) {
                if(delta == 1) {
                    int me_idx;
                    int me_w, me_h, me_x, me_y;
                    uint8_t *me_plane;
                    int corr, val;

                    /* get block size by index */
                    me_idx = code & 0xF;
                    me_w = qpeg_table_w[me_idx];
                    me_h = qpeg_table_h[me_idx];

                    /* extract motion vector */
                    corr = *src++;
                    size--;

                    val = corr >> 4;
                    if(val > 7)
                        val -= 16;
                    me_x = val;

                    val = corr & 0xF;
                    if(val > 7)
                        val -= 16;
                    me_y = val;

                    /* check motion vector */
                    if ((me_x + filled < 0) || (me_x + me_w + filled > width) ||
                       (height - me_y - me_h < 0) || (height - me_y > orig_height) ||
                       (filled + me_w > width) || (height - me_h < 0))
                        av_log(NULL, AV_LOG_ERROR, "Bogus motion vector (%i,%i), block size %ix%i at %i,%i\n",
                               me_x, me_y, me_w, me_h, filled, height);
                    else {
                        /* do motion compensation */
                        me_plane = refdata + (filled + me_x) + (height - me_y) * width;
                        for(j = 0; j < me_h; j++) {
                            for(i = 0; i < me_w; i++)
                                dst[filled + i - (j * stride)] = me_plane[i - (j * width)];
                        }
                    }
                }
                code = *src++;
                size--;
            }
        }

        if(code == 0xE0) /* end-of-picture code */
            break;
        if(code > 0xE0) { /* run code: 0xE1..0xFF */
            int p;

            code &= 0x1F;
            p = *src++;
            size--;
            for(i = 0; i <= code; i++) {
                dst[filled++] = p;
                if(filled >= width) {
                    filled = 0;
                    dst -= stride;
                    height--;
                }
            }
        } else if(code >= 0xC0) { /* copy code: 0xC0..0xDF */
            code &= 0x1F;

            for(i = 0; i <= code; i++) {
                dst[filled++] = *src++;
                if(filled >= width) {
                    filled = 0;
                    dst -= stride;
                    height--;
                }
            }
            size -= code + 1;
        } else if(code >= 0x80) { /* skip code: 0x80..0xBF */
            int skip;

            code &= 0x3F;
            /* codes 0x80 and 0x81 are actually escape codes,
               skip value minus constant is in the next byte */
            if(!code)
                skip = (*src++) + 64;
            else if(code == 1)
                skip = (*src++) + 320;
            else
                skip = code;
            filled += skip;
            while( filled >= width) {
                filled -= width;
                dst -= stride;
                height--;
                if(height < 0)
                    break;
            }
        } else {
            /* zero code treated as one-pixel skip */
            if(code)
                dst[filled++] = ctable[code & 0x7F];
            else
                filled++;
            if(filled >= width) {
                filled = 0;
                dst -= stride;
                height--;
            }
        }
    }
}

static int decode_frame(AVCodecContext *avctx,
                        void *data, int *data_size,
                        AVPacket *avpkt)
{
    const uint8_t *buf = avpkt->data;
    int buf_size = avpkt->size;
    QpegContext * const a = avctx->priv_data;
    AVFrame * const p= (AVFrame*)&a->pic;
    uint8_t* outdata;
    int delta;

    if(p->data[0])
        avctx->release_buffer(avctx, p);

    p->reference= 0;
    if(avctx->get_buffer(avctx, p) < 0){
        av_log(avctx, AV_LOG_ERROR, "get_buffer() failed\n");
        return -1;
    }
    outdata = a->pic.data[0];
    if(buf[0x85] == 0x10) {
        qpeg_decode_intra(buf+0x86, outdata, buf_size - 0x86, a->pic.linesize[0], avctx->width, avctx->height);
    } else {
        delta = buf[0x85];
        qpeg_decode_inter(buf+0x86, outdata, buf_size - 0x86, a->pic.linesize[0], avctx->width, avctx->height, delta, buf + 4, a->refdata);
    }

    /* make the palette available on the way out */
    memcpy(a->pic.data[1], a->avctx->palctrl->palette, AVPALETTE_SIZE);
    if (a->avctx->palctrl->palette_changed) {
        a->pic.palette_has_changed = 1;
        a->avctx->palctrl->palette_changed = 0;
    }

    *data_size = sizeof(AVFrame);
    *(AVFrame*)data = a->pic;

    return buf_size;
}

static av_cold int decode_init(AVCodecContext *avctx){
    QpegContext * const a = avctx->priv_data;

    if (!avctx->palctrl) {
        av_log(avctx, AV_LOG_FATAL, "Missing required palette via palctrl\n");
        return -1;
    }
    a->avctx = avctx;
    avctx->pix_fmt= PIX_FMT_PAL8;
    a->refdata = av_malloc(avctx->width * avctx->height);

    return 0;
}

static av_cold int decode_end(AVCodecContext *avctx){
    QpegContext * const a = avctx->priv_data;
    AVFrame * const p= (AVFrame*)&a->pic;

    if(p->data[0])
        avctx->release_buffer(avctx, p);

    av_free(a->refdata);
    return 0;
}

AVCodec qpeg_decoder = {
    "qpeg",
    AVMEDIA_TYPE_VIDEO,
    CODEC_ID_QPEG,
    sizeof(QpegContext),
    decode_init,
    NULL,
    decode_end,
    decode_frame,
    CODEC_CAP_DR1,
    .long_name = NULL_IF_CONFIG_SMALL("Q-team QPEG"),
};
