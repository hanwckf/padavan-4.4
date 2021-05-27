/*
 * Assorted DPCM codecs
 * Copyright (c) 2003 The ffmpeg Project
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
 * @file: dpcm.c
 * Assorted DPCM (differential pulse code modulation) audio codecs
 * by Mike Melanson (melanson@pcisys.net)
 * Xan DPCM decoder by Mario Brito (mbrito@student.dei.uc.pt)
 * for more information on the specific data formats, visit:
 *   http://www.pcisys.net/~melanson/codecs/simpleaudio.html
 * SOL DPCMs implemented by Konstantin Shishkov
 *
 * Note about using the Xan DPCM decoder: Xan DPCM is used in AVI files
 * found in the Wing Commander IV computer game. These AVI files contain
 * WAVEFORMAT headers which report the audio format as 0x01: raw PCM.
 * Clearly incorrect. To detect Xan DPCM, you will probably have to
 * special-case your AVI demuxer to use Xan DPCM if the file uses 'Xxan'
 * (Xan video) for its video codec. Alternately, such AVI files also contain
 * the fourcc 'Axan' in the 'auds' chunk of the AVI header.
 */

#include "libavutil/intreadwrite.h"
#include "avcodec.h"

typedef struct DPCMContext {
    int channels;
    short roq_square_array[256];
    long sample[2];//for SOL_DPCM
    const int *sol_table;//for SOL_DPCM
} DPCMContext;

#define SE_16BIT(x)  if (x & 0x8000) x -= 0x10000;

static const int interplay_delta_table[] = {
         0,      1,      2,      3,      4,      5,      6,      7,
         8,      9,     10,     11,     12,     13,     14,     15,
        16,     17,     18,     19,     20,     21,     22,     23,
        24,     25,     26,     27,     28,     29,     30,     31,
        32,     33,     34,     35,     36,     37,     38,     39,
        40,     41,     42,     43,     47,     51,     56,     61,
        66,     72,     79,     86,     94,    102,    112,    122,
       133,    145,    158,    173,    189,    206,    225,    245,
       267,    292,    318,    348,    379,    414,    452,    493,
       538,    587,    640,    699,    763,    832,    908,    991,
      1081,   1180,   1288,   1405,   1534,   1673,   1826,   1993,
      2175,   2373,   2590,   2826,   3084,   3365,   3672,   4008,
      4373,   4772,   5208,   5683,   6202,   6767,   7385,   8059,
      8794,   9597,  10472,  11428,  12471,  13609,  14851,  16206,
     17685,  19298,  21060,  22981,  25078,  27367,  29864,  32589,
    -29973, -26728, -23186, -19322, -15105, -10503,  -5481,     -1,
         1,      1,   5481,  10503,  15105,  19322,  23186,  26728,
     29973, -32589, -29864, -27367, -25078, -22981, -21060, -19298,
    -17685, -16206, -14851, -13609, -12471, -11428, -10472,  -9597,
     -8794,  -8059,  -7385,  -6767,  -6202,  -5683,  -5208,  -4772,
     -4373,  -4008,  -3672,  -3365,  -3084,  -2826,  -2590,  -2373,
     -2175,  -1993,  -1826,  -1673,  -1534,  -1405,  -1288,  -1180,
     -1081,   -991,   -908,   -832,   -763,   -699,   -640,   -587,
      -538,   -493,   -452,   -414,   -379,   -348,   -318,   -292,
      -267,   -245,   -225,   -206,   -189,   -173,   -158,   -145,
      -133,   -122,   -112,   -102,    -94,    -86,    -79,    -72,
       -66,    -61,    -56,    -51,    -47,    -43,    -42,    -41,
       -40,    -39,    -38,    -37,    -36,    -35,    -34,    -33,
       -32,    -31,    -30,    -29,    -28,    -27,    -26,    -25,
       -24,    -23,    -22,    -21,    -20,    -19,    -18,    -17,
       -16,    -15,    -14,    -13,    -12,    -11,    -10,     -9,
        -8,     -7,     -6,     -5,     -4,     -3,     -2,     -1

};

static const int sol_table_old[16] =
    { 0x0,  0x1,  0x2 , 0x3,  0x6,  0xA,  0xF, 0x15,
    -0x15, -0xF, -0xA, -0x6, -0x3, -0x2, -0x1, 0x0};

static const int sol_table_new[16] =
    { 0x0,  0x1,  0x2,  0x3,  0x6,  0xA,  0xF,  0x15,
      0x0, -0x1, -0x2, -0x3, -0x6, -0xA, -0xF, -0x15};

static const int sol_table_16[128] = {
    0x000, 0x008, 0x010, 0x020, 0x030, 0x040, 0x050, 0x060, 0x070, 0x080,
    0x090, 0x0A0, 0x0B0, 0x0C0, 0x0D0, 0x0E0, 0x0F0, 0x100, 0x110, 0x120,
    0x130, 0x140, 0x150, 0x160, 0x170, 0x180, 0x190, 0x1A0, 0x1B0, 0x1C0,
    0x1D0, 0x1E0, 0x1F0, 0x200, 0x208, 0x210, 0x218, 0x220, 0x228, 0x230,
    0x238, 0x240, 0x248, 0x250, 0x258, 0x260, 0x268, 0x270, 0x278, 0x280,
    0x288, 0x290, 0x298, 0x2A0, 0x2A8, 0x2B0, 0x2B8, 0x2C0, 0x2C8, 0x2D0,
    0x2D8, 0x2E0, 0x2E8, 0x2F0, 0x2F8, 0x300, 0x308, 0x310, 0x318, 0x320,
    0x328, 0x330, 0x338, 0x340, 0x348, 0x350, 0x358, 0x360, 0x368, 0x370,
    0x378, 0x380, 0x388, 0x390, 0x398, 0x3A0, 0x3A8, 0x3B0, 0x3B8, 0x3C0,
    0x3C8, 0x3D0, 0x3D8, 0x3E0, 0x3E8, 0x3F0, 0x3F8, 0x400, 0x440, 0x480,
    0x4C0, 0x500, 0x540, 0x580, 0x5C0, 0x600, 0x640, 0x680, 0x6C0, 0x700,
    0x740, 0x780, 0x7C0, 0x800, 0x900, 0xA00, 0xB00, 0xC00, 0xD00, 0xE00,
    0xF00, 0x1000, 0x1400, 0x1800, 0x1C00, 0x2000, 0x3000, 0x4000
};



static av_cold int dpcm_decode_init(AVCodecContext *avctx)
{
    DPCMContext *s = avctx->priv_data;
    int i;
    short square;

    s->channels = avctx->channels;
    s->sample[0] = s->sample[1] = 0;

    switch(avctx->codec->id) {

    case CODEC_ID_ROQ_DPCM:
        /* initialize square table */
        for (i = 0; i < 128; i++) {
            square = i * i;
            s->roq_square_array[i] = square;
            s->roq_square_array[i + 128] = -square;
        }
        break;


    case CODEC_ID_SOL_DPCM:
        switch(avctx->codec_tag){
        case 1:
            s->sol_table=sol_table_old;
            s->sample[0] = s->sample[1] = 0x80;
            break;
        case 2:
            s->sol_table=sol_table_new;
            s->sample[0] = s->sample[1] = 0x80;
            break;
        case 3:
            s->sol_table=sol_table_16;
            break;
        default:
            av_log(avctx, AV_LOG_ERROR, "Unknown SOL subcodec\n");
            return -1;
        }
        break;

    default:
        break;
    }

    avctx->sample_fmt = SAMPLE_FMT_S16;
    return 0;
}

static int dpcm_decode_frame(AVCodecContext *avctx,
                             void *data, int *data_size,
                             AVPacket *avpkt)
{
    const uint8_t *buf = avpkt->data;
    int buf_size = avpkt->size;
    DPCMContext *s = avctx->priv_data;
    int in, out = 0;
    int predictor[2];
    int channel_number = 0;
    int stereo = s->channels - 1;
    short *output_samples = data;
    int shift[2];
    unsigned char byte;
    short diff;

    if (!buf_size)
        return 0;

    if (stereo && (buf_size & 1))
        buf_size--;

    // almost every DPCM variant expands one byte of data into two
    if(*data_size/2 < buf_size)
        return -1;

    switch(avctx->codec->id) {

    case CODEC_ID_ROQ_DPCM:
        if (s->channels == 1)
            predictor[0] = AV_RL16(&buf[6]);
        else {
            predictor[0] = buf[7] << 8;
            predictor[1] = buf[6] << 8;
        }
        SE_16BIT(predictor[0]);
        SE_16BIT(predictor[1]);

        /* decode the samples */
        for (in = 8, out = 0; in < buf_size; in++, out++) {
            predictor[channel_number] += s->roq_square_array[buf[in]];
            predictor[channel_number] = av_clip_int16(predictor[channel_number]);
            output_samples[out] = predictor[channel_number];

            /* toggle channel */
            channel_number ^= s->channels - 1;
        }
        break;

    case CODEC_ID_INTERPLAY_DPCM:
        in = 6;  /* skip over the stream mask and stream length */
        predictor[0] = AV_RL16(&buf[in]);
        in += 2;
        SE_16BIT(predictor[0])
        output_samples[out++] = predictor[0];
        if (s->channels == 2) {
            predictor[1] = AV_RL16(&buf[in]);
            in += 2;
            SE_16BIT(predictor[1])
            output_samples[out++] = predictor[1];
        }

        while (in < buf_size) {
            predictor[channel_number] += interplay_delta_table[buf[in++]];
            predictor[channel_number] = av_clip_int16(predictor[channel_number]);
            output_samples[out++] = predictor[channel_number];

            /* toggle channel */
            channel_number ^= s->channels - 1;
        }

        break;

    case CODEC_ID_XAN_DPCM:
        in = 0;
        shift[0] = shift[1] = 4;
        predictor[0] = AV_RL16(&buf[in]);
        in += 2;
        SE_16BIT(predictor[0]);
        if (s->channels == 2) {
            predictor[1] = AV_RL16(&buf[in]);
            in += 2;
            SE_16BIT(predictor[1]);
        }

        while (in < buf_size) {
            byte = buf[in++];
            diff = (byte & 0xFC) << 8;
            if ((byte & 0x03) == 3)
                shift[channel_number]++;
            else
                shift[channel_number] -= (2 * (byte & 3));
            /* saturate the shifter to a lower limit of 0 */
            if (shift[channel_number] < 0)
                shift[channel_number] = 0;

            diff >>= shift[channel_number];
            predictor[channel_number] += diff;

            predictor[channel_number] = av_clip_int16(predictor[channel_number]);
            output_samples[out++] = predictor[channel_number];

            /* toggle channel */
            channel_number ^= s->channels - 1;
        }
        break;
    case CODEC_ID_SOL_DPCM:
        in = 0;
        if (avctx->codec_tag != 3) {
            if(*data_size/4 < buf_size)
                return -1;
            while (in < buf_size) {
                int n1, n2;
                n1 = (buf[in] >> 4) & 0xF;
                n2 = buf[in++] & 0xF;
                s->sample[0] += s->sol_table[n1];
                if (s->sample[0] < 0) s->sample[0] = 0;
                if (s->sample[0] > 255) s->sample[0] = 255;
                output_samples[out++] = (s->sample[0] - 128) << 8;
                s->sample[s->channels - 1] += s->sol_table[n2];
                if (s->sample[s->channels - 1] < 0) s->sample[s->channels - 1] = 0;
                if (s->sample[s->channels - 1] > 255) s->sample[s->channels - 1] = 255;
                output_samples[out++] = (s->sample[s->channels - 1] - 128) << 8;
            }
        } else {
            while (in < buf_size) {
                int n;
                n = buf[in++];
                if (n & 0x80) s->sample[channel_number] -= s->sol_table[n & 0x7F];
                else s->sample[channel_number] += s->sol_table[n & 0x7F];
                s->sample[channel_number] = av_clip_int16(s->sample[channel_number]);
                output_samples[out++] = s->sample[channel_number];
                /* toggle channel */
                channel_number ^= s->channels - 1;
            }
        }
        break;
    }

    *data_size = out * sizeof(short);
    return avpkt->size;
}

#define DPCM_DECODER(id, name, long_name_)      \
AVCodec name ## _decoder = {                    \
    #name,                                      \
    AVMEDIA_TYPE_AUDIO,                         \
    id,                                         \
    sizeof(DPCMContext),                        \
    dpcm_decode_init,                           \
    NULL,                                       \
    NULL,                                       \
    dpcm_decode_frame,                          \
    .long_name = NULL_IF_CONFIG_SMALL(long_name_), \
};

DPCM_DECODER(CODEC_ID_INTERPLAY_DPCM, interplay_dpcm, "DPCM Interplay");
DPCM_DECODER(CODEC_ID_ROQ_DPCM, roq_dpcm, "DPCM id RoQ");
DPCM_DECODER(CODEC_ID_SOL_DPCM, sol_dpcm, "DPCM Sol");
DPCM_DECODER(CODEC_ID_XAN_DPCM, xan_dpcm, "DPCM Xan");
