/*
 * ALAC audio encoder
 * Copyright (c) 2008  Jaikrishnan Menon <realityman@gmx.net>
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
#include "put_bits.h"
#include "dsputil.h"
#include "internal.h"
#include "lpc.h"
#include "mathops.h"

#define DEFAULT_FRAME_SIZE        4096
#define DEFAULT_SAMPLE_SIZE       16
#define MAX_CHANNELS              8
#define ALAC_EXTRADATA_SIZE       36
#define ALAC_FRAME_HEADER_SIZE    55
#define ALAC_FRAME_FOOTER_SIZE    3

#define ALAC_ESCAPE_CODE          0x1FF
#define ALAC_MAX_LPC_ORDER        30
#define DEFAULT_MAX_PRED_ORDER    6
#define DEFAULT_MIN_PRED_ORDER    4
#define ALAC_MAX_LPC_PRECISION    9
#define ALAC_MAX_LPC_SHIFT        9

#define ALAC_CHMODE_LEFT_RIGHT    0
#define ALAC_CHMODE_LEFT_SIDE     1
#define ALAC_CHMODE_RIGHT_SIDE    2
#define ALAC_CHMODE_MID_SIDE      3

typedef struct RiceContext {
    int history_mult;
    int initial_history;
    int k_modifier;
    int rice_modifier;
} RiceContext;

typedef struct AlacLPCContext {
    int lpc_order;
    int lpc_coeff[ALAC_MAX_LPC_ORDER+1];
    int lpc_quant;
} AlacLPCContext;

typedef struct AlacEncodeContext {
    int frame_size;                     /**< current frame size               */
    int verbatim;                       /**< current frame verbatim mode flag */
    int compression_level;
    int min_prediction_order;
    int max_prediction_order;
    int max_coded_frame_size;
    int write_sample_size;
    int32_t sample_buf[MAX_CHANNELS][DEFAULT_FRAME_SIZE];
    int32_t predictor_buf[DEFAULT_FRAME_SIZE];
    int interlacing_shift;
    int interlacing_leftweight;
    PutBitContext pbctx;
    RiceContext rc;
    AlacLPCContext lpc[MAX_CHANNELS];
    LPCContext lpc_ctx;
    AVCodecContext *avctx;
} AlacEncodeContext;


static void init_sample_buffers(AlacEncodeContext *s,
                                const int16_t *input_samples)
{
    int ch, i;

    for (ch = 0; ch < s->avctx->channels; ch++) {
        const int16_t *sptr = input_samples + ch;
        for (i = 0; i < s->frame_size; i++) {
            s->sample_buf[ch][i] = *sptr;
            sptr += s->avctx->channels;
        }
    }
}

static void encode_scalar(AlacEncodeContext *s, int x,
                          int k, int write_sample_size)
{
    int divisor, q, r;

    k = FFMIN(k, s->rc.k_modifier);
    divisor = (1<<k) - 1;
    q = x / divisor;
    r = x % divisor;

    if (q > 8) {
        // write escape code and sample value directly
        put_bits(&s->pbctx, 9, ALAC_ESCAPE_CODE);
        put_bits(&s->pbctx, write_sample_size, x);
    } else {
        if (q)
            put_bits(&s->pbctx, q, (1<<q) - 1);
        put_bits(&s->pbctx, 1, 0);

        if (k != 1) {
            if (r > 0)
                put_bits(&s->pbctx, k, r+1);
            else
                put_bits(&s->pbctx, k-1, 0);
        }
    }
}

static void write_frame_header(AlacEncodeContext *s)
{
    int encode_fs = 0;

    if (s->frame_size < DEFAULT_FRAME_SIZE)
        encode_fs = 1;

    put_bits(&s->pbctx, 3,  s->avctx->channels-1);  // No. of channels -1
    put_bits(&s->pbctx, 16, 0);                     // Seems to be zero
    put_bits(&s->pbctx, 1,  encode_fs);             // Sample count is in the header
    put_bits(&s->pbctx, 2,  0);                     // FIXME: Wasted bytes field
    put_bits(&s->pbctx, 1,  s->verbatim);           // Audio block is verbatim
    if (encode_fs)
        put_bits32(&s->pbctx, s->frame_size);       // No. of samples in the frame
}

static void calc_predictor_params(AlacEncodeContext *s, int ch)
{
    int32_t coefs[MAX_LPC_ORDER][MAX_LPC_ORDER];
    int shift[MAX_LPC_ORDER];
    int opt_order;

    if (s->compression_level == 1) {
        s->lpc[ch].lpc_order = 6;
        s->lpc[ch].lpc_quant = 6;
        s->lpc[ch].lpc_coeff[0] =  160;
        s->lpc[ch].lpc_coeff[1] = -190;
        s->lpc[ch].lpc_coeff[2] =  170;
        s->lpc[ch].lpc_coeff[3] = -130;
        s->lpc[ch].lpc_coeff[4] =   80;
        s->lpc[ch].lpc_coeff[5] =  -25;
    } else {
        opt_order = ff_lpc_calc_coefs(&s->lpc_ctx, s->sample_buf[ch],
                                      s->frame_size,
                                      s->min_prediction_order,
                                      s->max_prediction_order,
                                      ALAC_MAX_LPC_PRECISION, coefs, shift,
                                      FF_LPC_TYPE_LEVINSON, 0,
                                      ORDER_METHOD_EST, ALAC_MAX_LPC_SHIFT, 1);

        s->lpc[ch].lpc_order = opt_order;
        s->lpc[ch].lpc_quant = shift[opt_order-1];
        memcpy(s->lpc[ch].lpc_coeff, coefs[opt_order-1], opt_order*sizeof(int));
    }
}

static int estimate_stereo_mode(int32_t *left_ch, int32_t *right_ch, int n)
{
    int i, best;
    int32_t lt, rt;
    uint64_t sum[4];
    uint64_t score[4];

    /* calculate sum of 2nd order residual for each channel */
    sum[0] = sum[1] = sum[2] = sum[3] = 0;
    for (i = 2; i < n; i++) {
        lt =  left_ch[i] - 2 *  left_ch[i - 1] +  left_ch[i - 2];
        rt = right_ch[i] - 2 * right_ch[i - 1] + right_ch[i - 2];
        sum[2] += FFABS((lt + rt) >> 1);
        sum[3] += FFABS(lt - rt);
        sum[0] += FFABS(lt);
        sum[1] += FFABS(rt);
    }

    /* calculate score for each mode */
    score[0] = sum[0] + sum[1];
    score[1] = sum[0] + sum[3];
    score[2] = sum[1] + sum[3];
    score[3] = sum[2] + sum[3];

    /* return mode with lowest score */
    best = 0;
    for (i = 1; i < 4; i++) {
        if (score[i] < score[best])
            best = i;
    }
    return best;
}

static void alac_stereo_decorrelation(AlacEncodeContext *s)
{
    int32_t *left = s->sample_buf[0], *right = s->sample_buf[1];
    int i, mode, n = s->frame_size;
    int32_t tmp;

    mode = estimate_stereo_mode(left, right, n);

    switch (mode) {
    case ALAC_CHMODE_LEFT_RIGHT:
        s->interlacing_leftweight = 0;
        s->interlacing_shift      = 0;
        break;
    case ALAC_CHMODE_LEFT_SIDE:
        for (i = 0; i < n; i++)
            right[i] = left[i] - right[i];
        s->interlacing_leftweight = 1;
        s->interlacing_shift      = 0;
        break;
    case ALAC_CHMODE_RIGHT_SIDE:
        for (i = 0; i < n; i++) {
            tmp = right[i];
            right[i] = left[i] - right[i];
            left[i]  = tmp + (right[i] >> 31);
        }
        s->interlacing_leftweight = 1;
        s->interlacing_shift      = 31;
        break;
    default:
        for (i = 0; i < n; i++) {
            tmp = left[i];
            left[i]  = (tmp + right[i]) >> 1;
            right[i] =  tmp - right[i];
        }
        s->interlacing_leftweight = 1;
        s->interlacing_shift      = 1;
        break;
    }
}

static void alac_linear_predictor(AlacEncodeContext *s, int ch)
{
    int i;
    AlacLPCContext lpc = s->lpc[ch];

    if (lpc.lpc_order == 31) {
        s->predictor_buf[0] = s->sample_buf[ch][0];

        for (i = 1; i < s->frame_size; i++) {
            s->predictor_buf[i] = s->sample_buf[ch][i    ] -
                                  s->sample_buf[ch][i - 1];
        }

        return;
    }

    // generalised linear predictor

    if (lpc.lpc_order > 0) {
        int32_t *samples  = s->sample_buf[ch];
        int32_t *residual = s->predictor_buf;

        // generate warm-up samples
        residual[0] = samples[0];
        for (i = 1; i <= lpc.lpc_order; i++)
            residual[i] = sign_extend(samples[i] - samples[i-1], s->write_sample_size);

        // perform lpc on remaining samples
        for (i = lpc.lpc_order + 1; i < s->frame_size; i++) {
            int sum = 1 << (lpc.lpc_quant - 1), res_val, j;

            for (j = 0; j < lpc.lpc_order; j++) {
                sum += (samples[lpc.lpc_order-j] - samples[0]) *
                       lpc.lpc_coeff[j];
            }

            sum >>= lpc.lpc_quant;
            sum += samples[0];
            residual[i] = sign_extend(samples[lpc.lpc_order+1] - sum,
                                      s->write_sample_size);
            res_val = residual[i];

            if (res_val) {
                int index = lpc.lpc_order - 1;
                int neg = (res_val < 0);

                while (index >= 0 && (neg ? (res_val < 0) : (res_val > 0))) {
                    int val  = samples[0] - samples[lpc.lpc_order - index];
                    int sign = (val ? FFSIGN(val) : 0);

                    if (neg)
                        sign *= -1;

                    lpc.lpc_coeff[index] -= sign;
                    val *= sign;
                    res_val -= (val >> lpc.lpc_quant) * (lpc.lpc_order - index);
                    index--;
                }
            }
            samples++;
        }
    }
}

static void alac_entropy_coder(AlacEncodeContext *s)
{
    unsigned int history = s->rc.initial_history;
    int sign_modifier = 0, i, k;
    int32_t *samples = s->predictor_buf;

    for (i = 0; i < s->frame_size;) {
        int x;

        k = av_log2((history >> 9) + 3);

        x  = -2 * (*samples) -1;
        x ^= x >> 31;

        samples++;
        i++;

        encode_scalar(s, x - sign_modifier, k, s->write_sample_size);

        history += x * s->rc.history_mult -
                   ((history * s->rc.history_mult) >> 9);

        sign_modifier = 0;
        if (x > 0xFFFF)
            history = 0xFFFF;

        if (history < 128 && i < s->frame_size) {
            unsigned int block_size = 0;

            k = 7 - av_log2(history) + ((history + 16) >> 6);

            while (*samples == 0 && i < s->frame_size) {
                samples++;
                i++;
                block_size++;
            }
            encode_scalar(s, block_size, k, 16);
            sign_modifier = (block_size <= 0xFFFF);
            history = 0;
        }

    }
}

static int write_frame(AlacEncodeContext *s, AVPacket *avpkt,
                       const int16_t *samples)
{
    int i, j;
    int prediction_type = 0;
    PutBitContext *pb = &s->pbctx;

    init_put_bits(pb, avpkt->data, avpkt->size);

    if (s->verbatim) {
        write_frame_header(s);
        for (i = 0; i < s->frame_size * s->avctx->channels; i++)
            put_sbits(pb, 16, *samples++);
    } else {
        init_sample_buffers(s, samples);
        write_frame_header(s);

        if (s->avctx->channels == 2)
            alac_stereo_decorrelation(s);
        put_bits(pb, 8, s->interlacing_shift);
        put_bits(pb, 8, s->interlacing_leftweight);

        for (i = 0; i < s->avctx->channels; i++) {
            calc_predictor_params(s, i);

            put_bits(pb, 4, prediction_type);
            put_bits(pb, 4, s->lpc[i].lpc_quant);

            put_bits(pb, 3, s->rc.rice_modifier);
            put_bits(pb, 5, s->lpc[i].lpc_order);
            // predictor coeff. table
            for (j = 0; j < s->lpc[i].lpc_order; j++)
                put_sbits(pb, 16, s->lpc[i].lpc_coeff[j]);
        }

        // apply lpc and entropy coding to audio samples

        for (i = 0; i < s->avctx->channels; i++) {
            alac_linear_predictor(s, i);

            // TODO: determine when this will actually help. for now it's not used.
            if (prediction_type == 15) {
                // 2nd pass 1st order filter
                for (j = s->frame_size - 1; j > 0; j--)
                    s->predictor_buf[j] -= s->predictor_buf[j - 1];
            }

            alac_entropy_coder(s);
        }
    }
    put_bits(pb, 3, 7);
    flush_put_bits(pb);
    return put_bits_count(pb) >> 3;
}

static av_always_inline int get_max_frame_size(int frame_size, int ch, int bps)
{
    int header_bits = 23 + 32 * (frame_size < DEFAULT_FRAME_SIZE);
    return FFALIGN(header_bits + bps * ch * frame_size + 3, 8) / 8;
}

static av_cold int alac_encode_close(AVCodecContext *avctx)
{
    AlacEncodeContext *s = avctx->priv_data;
    ff_lpc_end(&s->lpc_ctx);
    av_freep(&avctx->extradata);
    avctx->extradata_size = 0;
    av_freep(&avctx->coded_frame);
    return 0;
}

static av_cold int alac_encode_init(AVCodecContext *avctx)
{
    AlacEncodeContext *s = avctx->priv_data;
    int ret;
    uint8_t *alac_extradata;

    avctx->frame_size = s->frame_size = DEFAULT_FRAME_SIZE;

    if (avctx->sample_fmt != AV_SAMPLE_FMT_S16) {
        av_log(avctx, AV_LOG_ERROR, "only pcm_s16 input samples are supported\n");
        return -1;
    }

    /* TODO: Correctly implement multi-channel ALAC.
             It is similar to multi-channel AAC, in that it has a series of
             single-channel (SCE), channel-pair (CPE), and LFE elements. */
    if (avctx->channels > 2) {
        av_log(avctx, AV_LOG_ERROR, "only mono or stereo input is currently supported\n");
        return AVERROR_PATCHWELCOME;
    }

    // Set default compression level
    if (avctx->compression_level == FF_COMPRESSION_DEFAULT)
        s->compression_level = 2;
    else
        s->compression_level = av_clip(avctx->compression_level, 0, 2);

    // Initialize default Rice parameters
    s->rc.history_mult    = 40;
    s->rc.initial_history = 10;
    s->rc.k_modifier      = 14;
    s->rc.rice_modifier   = 4;

    s->max_coded_frame_size = get_max_frame_size(avctx->frame_size,
                                                 avctx->channels,
                                                 DEFAULT_SAMPLE_SIZE);

    // FIXME: consider wasted_bytes
    s->write_sample_size  = DEFAULT_SAMPLE_SIZE + avctx->channels - 1;

    avctx->extradata = av_mallocz(ALAC_EXTRADATA_SIZE + FF_INPUT_BUFFER_PADDING_SIZE);
    if (!avctx->extradata) {
        ret = AVERROR(ENOMEM);
        goto error;
    }
    avctx->extradata_size = ALAC_EXTRADATA_SIZE;

    alac_extradata = avctx->extradata;
    AV_WB32(alac_extradata,    ALAC_EXTRADATA_SIZE);
    AV_WB32(alac_extradata+4,  MKBETAG('a','l','a','c'));
    AV_WB32(alac_extradata+12, avctx->frame_size);
    AV_WB8 (alac_extradata+17, DEFAULT_SAMPLE_SIZE);
    AV_WB8 (alac_extradata+21, avctx->channels);
    AV_WB32(alac_extradata+24, s->max_coded_frame_size);
    AV_WB32(alac_extradata+28,
            avctx->sample_rate * avctx->channels * DEFAULT_SAMPLE_SIZE); // average bitrate
    AV_WB32(alac_extradata+32, avctx->sample_rate);

    // Set relevant extradata fields
    if (s->compression_level > 0) {
        AV_WB8(alac_extradata+18, s->rc.history_mult);
        AV_WB8(alac_extradata+19, s->rc.initial_history);
        AV_WB8(alac_extradata+20, s->rc.k_modifier);
    }

    s->min_prediction_order = DEFAULT_MIN_PRED_ORDER;
    if (avctx->min_prediction_order >= 0) {
        if (avctx->min_prediction_order < MIN_LPC_ORDER ||
           avctx->min_prediction_order > ALAC_MAX_LPC_ORDER) {
            av_log(avctx, AV_LOG_ERROR, "invalid min prediction order: %d\n",
                   avctx->min_prediction_order);
            ret = AVERROR(EINVAL);
            goto error;
        }

        s->min_prediction_order = avctx->min_prediction_order;
    }

    s->max_prediction_order = DEFAULT_MAX_PRED_ORDER;
    if (avctx->max_prediction_order >= 0) {
        if (avctx->max_prediction_order < MIN_LPC_ORDER ||
            avctx->max_prediction_order > ALAC_MAX_LPC_ORDER) {
            av_log(avctx, AV_LOG_ERROR, "invalid max prediction order: %d\n",
                   avctx->max_prediction_order);
            ret = AVERROR(EINVAL);
            goto error;
        }

        s->max_prediction_order = avctx->max_prediction_order;
    }

    if (s->max_prediction_order < s->min_prediction_order) {
        av_log(avctx, AV_LOG_ERROR,
               "invalid prediction orders: min=%d max=%d\n",
               s->min_prediction_order, s->max_prediction_order);
        ret = AVERROR(EINVAL);
        goto error;
    }

    avctx->coded_frame = avcodec_alloc_frame();
    if (!avctx->coded_frame) {
        ret = AVERROR(ENOMEM);
        goto error;
    }

    s->avctx = avctx;

    if ((ret = ff_lpc_init(&s->lpc_ctx, avctx->frame_size,
                           s->max_prediction_order,
                           FF_LPC_TYPE_LEVINSON)) < 0) {
        goto error;
    }

    return 0;
error:
    alac_encode_close(avctx);
    return ret;
}

static int alac_encode_frame(AVCodecContext *avctx, AVPacket *avpkt,
                             const AVFrame *frame, int *got_packet_ptr)
{
    AlacEncodeContext *s = avctx->priv_data;
    int out_bytes, max_frame_size, ret;
    const int16_t *samples = (const int16_t *)frame->data[0];

    s->frame_size = frame->nb_samples;

    if (avctx->frame_size < DEFAULT_FRAME_SIZE)
        max_frame_size = get_max_frame_size(s->frame_size, avctx->channels,
                                            DEFAULT_SAMPLE_SIZE);
    else
        max_frame_size = s->max_coded_frame_size;

    if ((ret = ff_alloc_packet2(avctx, avpkt, 2 * max_frame_size)))
        return ret;

    /* use verbatim mode for compression_level 0 */
    s->verbatim = !s->compression_level;

    out_bytes = write_frame(s, avpkt, samples);

    if (out_bytes > max_frame_size) {
        /* frame too large. use verbatim mode */
        s->verbatim = 1;
        out_bytes = write_frame(s, avpkt, samples);
    }

    avpkt->size = out_bytes;
    *got_packet_ptr = 1;
    return 0;
}

AVCodec ff_alac_encoder = {
    .name           = "alac",
    .type           = AVMEDIA_TYPE_AUDIO,
    .id             = CODEC_ID_ALAC,
    .priv_data_size = sizeof(AlacEncodeContext),
    .init           = alac_encode_init,
    .encode2        = alac_encode_frame,
    .close          = alac_encode_close,
    .capabilities   = CODEC_CAP_SMALL_LAST_FRAME,
    .sample_fmts    = (const enum AVSampleFormat[]){ AV_SAMPLE_FMT_S16,
                                                     AV_SAMPLE_FMT_NONE },
    .long_name      = NULL_IF_CONFIG_SMALL("ALAC (Apple Lossless Audio Codec)"),
};
