/*
 * V210 decoder
 *
 * Copyright (C) 2009 Michael Niedermayer <michaelni@gmx.at>
 * Copyright (c) 2009 Baptiste Coudurier <baptiste dot coudurier at gmail dot com>
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
#include "libavutil/bswap.h"

static av_cold int decode_init(AVCodecContext *avctx)
{
    if (avctx->width & 1) {
        av_log(avctx, AV_LOG_ERROR, "v210 needs even width\n");
        return -1;
    }
    avctx->pix_fmt             = PIX_FMT_YUV422P16;
    avctx->bits_per_raw_sample = 10;

    avctx->coded_frame         = avcodec_alloc_frame();

    return 0;
}

static int decode_frame(AVCodecContext *avctx, void *data, int *data_size,
                        AVPacket *avpkt)
{
    int h, w;
    AVFrame *pic = avctx->coded_frame;
    const uint8_t *psrc = avpkt->data;
    uint16_t *y, *u, *v;
    int aligned_width = ((avctx->width + 47) / 48) * 48;
    int stride = aligned_width * 8 / 3;

    if (pic->data[0])
        avctx->release_buffer(avctx, pic);

    if (avpkt->size < stride * avctx->height) {
        av_log(avctx, AV_LOG_ERROR, "packet too small\n");
        return -1;
    }

    pic->reference = 0;
    if (avctx->get_buffer(avctx, pic) < 0)
        return -1;

    y = (uint16_t*)pic->data[0];
    u = (uint16_t*)pic->data[1];
    v = (uint16_t*)pic->data[2];
    pic->pict_type = FF_I_TYPE;
    pic->key_frame = 1;

#define READ_PIXELS(a, b, c)         \
    do {                             \
        val  = le2me_32(*src++);     \
        *a++ =  val <<  6;           \
        *b++ = (val >>  4) & 0xFFC0; \
        *c++ = (val >> 14) & 0xFFC0; \
    } while (0)

    for (h = 0; h < avctx->height; h++) {
        const uint32_t *src = (const uint32_t*)psrc;
        uint32_t val;
        for (w = 0; w < avctx->width - 5; w += 6) {
            READ_PIXELS(u, y, v);
            READ_PIXELS(y, u, y);
            READ_PIXELS(v, y, u);
            READ_PIXELS(y, v, y);
        }
        if (w < avctx->width - 1) {
            READ_PIXELS(u, y, v);

            val  = le2me_32(*src++);
            *y++ =  val <<  6;
        }
        if (w < avctx->width - 3) {
            *u++ = (val >>  4) & 0xFFC0;
            *y++ = (val >> 14) & 0xFFC0;

            val  = le2me_32(*src++);
            *v++ =  val <<  6;
            *y++ = (val >>  4) & 0xFFC0;
        }

        psrc += stride;
        y += pic->linesize[0] / 2 - avctx->width;
        u += pic->linesize[1] / 2 - avctx->width / 2;
        v += pic->linesize[2] / 2 - avctx->width / 2;
    }

    *data_size = sizeof(AVFrame);
    *(AVFrame*)data = *avctx->coded_frame;

    return avpkt->size;
}

static av_cold int decode_close(AVCodecContext *avctx)
{
    AVFrame *pic = avctx->coded_frame;
    if (pic->data[0])
        avctx->release_buffer(avctx, pic);
    av_freep(&avctx->coded_frame);

    return 0;
}

AVCodec v210_decoder = {
    "v210",
    AVMEDIA_TYPE_VIDEO,
    CODEC_ID_V210,
    0,
    decode_init,
    NULL,
    decode_close,
    decode_frame,
    CODEC_CAP_DR1,
    .long_name = NULL_IF_CONFIG_SMALL("Uncompressed 4:2:2 10-bit"),
};
