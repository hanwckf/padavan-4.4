/*
 * Copyright (C) 2008 Michael Niedermayer
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

#include "parser.h"

static int parse(AVCodecParserContext *s,
                           AVCodecContext *avctx,
                           const uint8_t **poutbuf, int *poutbuf_size,
                           const uint8_t *buf, int buf_size)
{
    if(avctx->codec_id == CODEC_ID_THEORA)
        s->pict_type= (buf[0]&0x40) ? FF_P_TYPE : FF_I_TYPE;
    else
        s->pict_type= (buf[0]&0x80) ? FF_P_TYPE : FF_I_TYPE;

    *poutbuf = buf;
    *poutbuf_size = buf_size;
    return buf_size;
}

AVCodecParser vp3_parser = {
    { CODEC_ID_THEORA, CODEC_ID_VP3,
      CODEC_ID_VP6,    CODEC_ID_VP6F, CODEC_ID_VP6A },
    0,
    NULL,
    parse,
};
