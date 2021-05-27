/*
 * LPCM codecs for PCM formats found in MPEG streams
 * Copyright (c) 2009 Christian Schmidt
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
 * PCM codecs for encodings found in MPEG streams (DVD/Blu-ray)
 */

#include "avcodec.h"
#include "bytestream.h"

/*
 * Channel Mapping according to
 * Blu-ray Disc Read-Only Format Version 1
 * Part 3: Audio Visual Basic Specifications
 * mono     M1    X
 * stereo   L     R
 * 3/0      L     R    C    X
 * 2/1      L     R    S    X
 * 3/1      L     R    C    S
 * 2/2      L     R    LS   RS
 * 3/2      L     R    C    LS    RS    X
 * 3/2+lfe  L     R    C    LS    RS    lfe
 * 3/4      L     R    C    LS    Rls   Rrs  RS   X
 * 3/4+lfe  L     R    C    LS    Rls   Rrs  RS   lfe
 */

/**
 * Parse the header of a LPCM frame read from a MPEG-TS stream
 * @param avctx the codec context
 * @param header pointer to the first four bytes of the data packet
 */
static int pcm_bluray_parse_header(AVCodecContext *avctx,
                                   const uint8_t *header)
{
    static const uint8_t bits_per_samples[4] = { 0, 16, 20, 24 };
    static const uint32_t channel_layouts[16] = {
        0, CH_LAYOUT_MONO, 0, CH_LAYOUT_STEREO, CH_LAYOUT_SURROUND,
        CH_LAYOUT_2_1, CH_LAYOUT_4POINT0, CH_LAYOUT_2_2, CH_LAYOUT_5POINT0,
        CH_LAYOUT_5POINT1, CH_LAYOUT_7POINT0, CH_LAYOUT_7POINT1, 0, 0, 0, 0
    };
    static const uint8_t channels[16] = {
        0, 1, 0, 2, 3, 3, 4, 4, 5, 6, 7, 8, 0, 0, 0, 0
    };
    uint8_t channel_layout = header[2] >> 4;

    if (avctx->debug & FF_DEBUG_PICT_INFO)
        dprintf(avctx, "pcm_bluray_parse_header: header = %02x%02x%02x%02x\n",
                header[0], header[1], header[2], header[3]);

    /* get the sample depth and derive the sample format from it */
    avctx->bits_per_coded_sample = bits_per_samples[header[3] >> 6];
    if (!avctx->bits_per_coded_sample) {
        av_log(avctx, AV_LOG_ERROR, "unsupported sample depth (0)\n");
        return -1;
    }
    avctx->sample_fmt = avctx->bits_per_coded_sample == 16 ? SAMPLE_FMT_S16 :
                                                             SAMPLE_FMT_S32;

    /* get the sample rate. Not all values are known or exist. */
    switch (header[2] & 0x0f) {
    case 1:
        avctx->sample_rate = 48000;
        break;
    case 4:
        avctx->sample_rate = 96000;
        break;
    case 5:
        avctx->sample_rate = 192000;
        break;
    default:
        avctx->sample_rate = 0;
        av_log(avctx, AV_LOG_ERROR, "unsupported sample rate (%d)\n",
               header[2] & 0x0f);
        return -1;
    }

    /*
     * get the channel number (and mapping). Not all values are known or exist.
     * It must be noted that the number of channels in the MPEG stream can
     * differ from the actual meaningful number, e.g. mono audio still has two
     * channels, one being empty.
     */
    avctx->channel_layout  = channel_layouts[channel_layout];
    avctx->channels        =        channels[channel_layout];
    if (!avctx->channels) {
        av_log(avctx, AV_LOG_ERROR, "unsupported channel configuration (%d)\n",
               channel_layout);
        return -1;
    }

    avctx->bit_rate = avctx->channels * avctx->sample_rate *
                      avctx->bits_per_coded_sample;

    if (avctx->debug & FF_DEBUG_PICT_INFO)
        dprintf(avctx,
                "pcm_bluray_parse_header: %d channels, %d bits per sample, %d kHz, %d kbit\n",
                avctx->channels, avctx->bits_per_coded_sample,
                avctx->sample_rate, avctx->bit_rate);
    return 0;
}

static int pcm_bluray_decode_frame(AVCodecContext *avctx,
                                   void *data,
                                   int *data_size,
                                   AVPacket *avpkt)
{
    const uint8_t *src = avpkt->data;
    int buf_size = avpkt->size;
    int num_source_channels, channel, retval;
    int sample_size, samples, output_size;
    int16_t *dst16 = data;
    int32_t *dst32 = data;

    if (buf_size < 4) {
        av_log(avctx, AV_LOG_ERROR, "PCM packet too small\n");
        return -1;
    }

    if (pcm_bluray_parse_header(avctx, src))
        return -1;
    src += 4;
    buf_size -= 4;

    /* There's always an even number of channels in the source */
    num_source_channels = FFALIGN(avctx->channels, 2);
    sample_size = (num_source_channels * avctx->bits_per_coded_sample) >> 3;
    samples = buf_size / sample_size;

    output_size = samples * avctx->channels *
                  (avctx->sample_fmt == SAMPLE_FMT_S32 ? 4 : 2);
    if (output_size > *data_size) {
        av_log(avctx, AV_LOG_ERROR,
               "Insufficient output buffer space (%d bytes, needed %d bytes)\n",
               *data_size, output_size);
        return -1;
    }
    *data_size = output_size;

    if (samples) {
        switch (avctx->channel_layout) {
            /* cases with same number of source and coded channels */
        case CH_LAYOUT_STEREO:
        case CH_LAYOUT_4POINT0:
        case CH_LAYOUT_2_2:
            samples *= num_source_channels;
            if (SAMPLE_FMT_S16 == avctx->sample_fmt) {
#if HAVE_BIGENDIAN
                memcpy(dst16, src, output_size);
#else
                do {
                    *dst16++ = bytestream_get_be16(&src);
                } while (--samples);
#endif
            } else {
                do {
                    *dst32++ = bytestream_get_be24(&src) << 8;
                } while (--samples);
            }
            break;
        /* cases where number of source channels = coded channels + 1 */
        case CH_LAYOUT_MONO:
        case CH_LAYOUT_SURROUND:
        case CH_LAYOUT_2_1:
        case CH_LAYOUT_5POINT0:
            if (SAMPLE_FMT_S16 == avctx->sample_fmt) {
                do {
#if HAVE_BIGENDIAN
                    memcpy(dst16, src, avctx->channels * 2);
                    dst16 += avctx->channels;
                    src += sample_size;
#else
                    channel = avctx->channels;
                    do {
                        *dst16++ = bytestream_get_be16(&src);
                    } while (--channel);
                    src += 2;
#endif
                } while (--samples);
            } else {
                do {
                    channel = avctx->channels;
                    do {
                        *dst32++ = bytestream_get_be24(&src) << 8;
                    } while (--channel);
                    src += 3;
                } while (--samples);
            }
            break;
            /* remapping: L, R, C, LBack, RBack, LF */
        case CH_LAYOUT_5POINT1:
            if (SAMPLE_FMT_S16 == avctx->sample_fmt) {
                do {
                    dst16[0] = bytestream_get_be16(&src);
                    dst16[1] = bytestream_get_be16(&src);
                    dst16[2] = bytestream_get_be16(&src);
                    dst16[4] = bytestream_get_be16(&src);
                    dst16[5] = bytestream_get_be16(&src);
                    dst16[3] = bytestream_get_be16(&src);
                    dst16 += 6;
                } while (--samples);
            } else {
                do {
                    dst32[0] = bytestream_get_be24(&src) << 8;
                    dst32[1] = bytestream_get_be24(&src) << 8;
                    dst32[2] = bytestream_get_be24(&src) << 8;
                    dst32[4] = bytestream_get_be24(&src) << 8;
                    dst32[5] = bytestream_get_be24(&src) << 8;
                    dst32[3] = bytestream_get_be24(&src) << 8;
                    dst32 += 6;
                } while (--samples);
            }
            break;
            /* remapping: L, R, C, LSide, LBack, RBack, RSide, <unused> */
        case CH_LAYOUT_7POINT0:
            if (SAMPLE_FMT_S16 == avctx->sample_fmt) {
                do {
                    dst16[0] = bytestream_get_be16(&src);
                    dst16[1] = bytestream_get_be16(&src);
                    dst16[2] = bytestream_get_be16(&src);
                    dst16[5] = bytestream_get_be16(&src);
                    dst16[3] = bytestream_get_be16(&src);
                    dst16[4] = bytestream_get_be16(&src);
                    dst16[6] = bytestream_get_be16(&src);
                    dst16 += 7;
                    src += 2;
                } while (--samples);
            } else {
                do {
                    dst32[0] = bytestream_get_be24(&src) << 8;
                    dst32[1] = bytestream_get_be24(&src) << 8;
                    dst32[2] = bytestream_get_be24(&src) << 8;
                    dst32[5] = bytestream_get_be24(&src) << 8;
                    dst32[3] = bytestream_get_be24(&src) << 8;
                    dst32[4] = bytestream_get_be24(&src) << 8;
                    dst32[6] = bytestream_get_be24(&src) << 8;
                    dst32 += 7;
                    src += 3;
                } while (--samples);
            }
            break;
            /* remapping: L, R, C, LSide, LBack, RBack, RSide, LF */
        case CH_LAYOUT_7POINT1:
            if (SAMPLE_FMT_S16 == avctx->sample_fmt) {
                do {
                    dst16[0] = bytestream_get_be16(&src);
                    dst16[1] = bytestream_get_be16(&src);
                    dst16[2] = bytestream_get_be16(&src);
                    dst16[6] = bytestream_get_be16(&src);
                    dst16[4] = bytestream_get_be16(&src);
                    dst16[5] = bytestream_get_be16(&src);
                    dst16[7] = bytestream_get_be16(&src);
                    dst16[3] = bytestream_get_be16(&src);
                    dst16 += 8;
                } while (--samples);
            } else {
                do {
                    dst32[0] = bytestream_get_be24(&src) << 8;
                    dst32[1] = bytestream_get_be24(&src) << 8;
                    dst32[2] = bytestream_get_be24(&src) << 8;
                    dst32[6] = bytestream_get_be24(&src) << 8;
                    dst32[4] = bytestream_get_be24(&src) << 8;
                    dst32[5] = bytestream_get_be24(&src) << 8;
                    dst32[7] = bytestream_get_be24(&src) << 8;
                    dst32[3] = bytestream_get_be24(&src) << 8;
                    dst32 += 8;
                } while (--samples);
            }
            break;
        }
    }

    retval = src - avpkt->data;
    if (avctx->debug & FF_DEBUG_BITSTREAM)
        dprintf(avctx, "pcm_bluray_decode_frame: decoded %d -> %d bytes\n",
                retval, *data_size);
    return retval;
}

AVCodec pcm_bluray_decoder = {
    "pcm_bluray",
    AVMEDIA_TYPE_AUDIO,
    CODEC_ID_PCM_BLURAY,
    0,
    NULL,
    NULL,
    NULL,
    pcm_bluray_decode_frame,
    .sample_fmts = (const enum SampleFormat[]){SAMPLE_FMT_S16, SAMPLE_FMT_S32,
                                         SAMPLE_FMT_NONE},
    .long_name = NULL_IF_CONFIG_SMALL("PCM signed 16|20|24-bit big-endian for Blu-ray media"),
};
