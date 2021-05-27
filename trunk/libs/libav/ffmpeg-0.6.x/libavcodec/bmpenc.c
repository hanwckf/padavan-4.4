/*
 * BMP image format encoder
 * Copyright (c) 2006, 2007 Michel Bardiaux
 * Copyright (c) 2009 Daniel Verkamp <daniel at drv.nu>
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
#include "bmp.h"

static const uint32_t monoblack_pal[] = { 0x000000, 0xFFFFFF };
static const uint32_t rgb565_masks[]  = { 0xF800, 0x07E0, 0x001F };

static av_cold int bmp_encode_init(AVCodecContext *avctx){
    BMPContext *s = avctx->priv_data;

    avcodec_get_frame_defaults((AVFrame*)&s->picture);
    avctx->coded_frame = (AVFrame*)&s->picture;

    return 0;
}

static int bmp_encode_frame(AVCodecContext *avctx, unsigned char *buf, int buf_size, void *data){
    BMPContext *s = avctx->priv_data;
    AVFrame *pict = data;
    AVFrame * const p= (AVFrame*)&s->picture;
    int n_bytes_image, n_bytes_per_row, n_bytes, i, n, hsize;
    const uint32_t *pal = NULL;
    int pad_bytes_per_row, bit_count, pal_entries = 0, compression = BMP_RGB;
    uint8_t *ptr;
    unsigned char* buf0 = buf;
    *p = *pict;
    p->pict_type= FF_I_TYPE;
    p->key_frame= 1;
    switch (avctx->pix_fmt) {
    case PIX_FMT_BGR24:
        bit_count = 24;
        break;
    case PIX_FMT_RGB555:
        bit_count = 16;
        break;
    case PIX_FMT_RGB565:
        bit_count = 16;
        compression = BMP_BITFIELDS;
        pal = rgb565_masks; // abuse pal to hold color masks
        pal_entries = 3;
        break;
    case PIX_FMT_RGB8:
    case PIX_FMT_BGR8:
    case PIX_FMT_RGB4_BYTE:
    case PIX_FMT_BGR4_BYTE:
    case PIX_FMT_GRAY8:
    case PIX_FMT_PAL8:
        bit_count = 8;
        pal = (uint32_t *)p->data[1];
        break;
    case PIX_FMT_MONOBLACK:
        bit_count = 1;
        pal = monoblack_pal;
        break;
    default:
        return -1;
    }
    if (pal && !pal_entries) pal_entries = 1 << bit_count;
    n_bytes_per_row = ((int64_t)avctx->width * (int64_t)bit_count + 7LL) >> 3LL;
    pad_bytes_per_row = (4 - n_bytes_per_row) & 3;
    n_bytes_image = avctx->height * (n_bytes_per_row + pad_bytes_per_row);

    // STRUCTURE.field refer to the MSVC documentation for BITMAPFILEHEADER
    // and related pages.
#define SIZE_BITMAPFILEHEADER 14
#define SIZE_BITMAPINFOHEADER 40
    hsize = SIZE_BITMAPFILEHEADER + SIZE_BITMAPINFOHEADER + (pal_entries << 2);
    n_bytes = n_bytes_image + hsize;
    if(n_bytes>buf_size) {
        av_log(avctx, AV_LOG_ERROR, "buf size too small (need %d, got %d)\n", n_bytes, buf_size);
        return -1;
    }
    bytestream_put_byte(&buf, 'B');                   // BITMAPFILEHEADER.bfType
    bytestream_put_byte(&buf, 'M');                   // do.
    bytestream_put_le32(&buf, n_bytes);               // BITMAPFILEHEADER.bfSize
    bytestream_put_le16(&buf, 0);                     // BITMAPFILEHEADER.bfReserved1
    bytestream_put_le16(&buf, 0);                     // BITMAPFILEHEADER.bfReserved2
    bytestream_put_le32(&buf, hsize);                 // BITMAPFILEHEADER.bfOffBits
    bytestream_put_le32(&buf, SIZE_BITMAPINFOHEADER); // BITMAPINFOHEADER.biSize
    bytestream_put_le32(&buf, avctx->width);          // BITMAPINFOHEADER.biWidth
    bytestream_put_le32(&buf, avctx->height);         // BITMAPINFOHEADER.biHeight
    bytestream_put_le16(&buf, 1);                     // BITMAPINFOHEADER.biPlanes
    bytestream_put_le16(&buf, bit_count);             // BITMAPINFOHEADER.biBitCount
    bytestream_put_le32(&buf, compression);           // BITMAPINFOHEADER.biCompression
    bytestream_put_le32(&buf, n_bytes_image);         // BITMAPINFOHEADER.biSizeImage
    bytestream_put_le32(&buf, 0);                     // BITMAPINFOHEADER.biXPelsPerMeter
    bytestream_put_le32(&buf, 0);                     // BITMAPINFOHEADER.biYPelsPerMeter
    bytestream_put_le32(&buf, 0);                     // BITMAPINFOHEADER.biClrUsed
    bytestream_put_le32(&buf, 0);                     // BITMAPINFOHEADER.biClrImportant
    for (i = 0; i < pal_entries; i++)
        bytestream_put_le32(&buf, pal[i] & 0xFFFFFF);
    // BMP files are bottom-to-top so we start from the end...
    ptr = p->data[0] + (avctx->height - 1) * p->linesize[0];
    buf = buf0 + hsize;
    for(i = 0; i < avctx->height; i++) {
        if (bit_count == 16) {
            const uint16_t *src = (const uint16_t *) ptr;
            uint16_t *dst = (uint16_t *) buf;
            for(n = 0; n < avctx->width; n++)
                AV_WL16(dst + n, src[n]);
        } else {
            memcpy(buf, ptr, n_bytes_per_row);
        }
        buf += n_bytes_per_row;
        memset(buf, 0, pad_bytes_per_row);
        buf += pad_bytes_per_row;
        ptr -= p->linesize[0]; // ... and go back
    }
    return n_bytes;
}

AVCodec bmp_encoder = {
    "bmp",
    AVMEDIA_TYPE_VIDEO,
    CODEC_ID_BMP,
    sizeof(BMPContext),
    bmp_encode_init,
    bmp_encode_frame,
    NULL, //encode_end,
    .pix_fmts = (const enum PixelFormat[]){
        PIX_FMT_BGR24,
        PIX_FMT_RGB555, PIX_FMT_RGB565,
        PIX_FMT_RGB8, PIX_FMT_BGR8, PIX_FMT_RGB4_BYTE, PIX_FMT_BGR4_BYTE, PIX_FMT_GRAY8, PIX_FMT_PAL8,
        PIX_FMT_MONOBLACK,
        PIX_FMT_NONE},
    .long_name = NULL_IF_CONFIG_SMALL("BMP image"),
};
