/*
 * H.26L/H.264/AVC/JVT/14496-10/... loop filter
 * Copyright (c) 2003 Michael Niedermayer <michaelni@gmx.at>
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
 * H.264 / AVC / MPEG4 part10 loop filter.
 * @author Michael Niedermayer <michaelni@gmx.at>
 */

#include "libavutil/intreadwrite.h"
#include "internal.h"
#include "dsputil.h"
#include "avcodec.h"
#include "mpegvideo.h"
#include "h264.h"
#include "mathops.h"
#include "rectangle.h"

//#undef NDEBUG
#include <assert.h>

/* Deblocking filter (p153) */
static const uint8_t alpha_table[52*3] = {
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  4,  4,  5,  6,
     7,  8,  9, 10, 12, 13, 15, 17, 20, 22,
    25, 28, 32, 36, 40, 45, 50, 56, 63, 71,
    80, 90,101,113,127,144,162,182,203,226,
   255,255,
   255,255,255,255,255,255,255,255,255,255,255,255,255,
   255,255,255,255,255,255,255,255,255,255,255,255,255,
   255,255,255,255,255,255,255,255,255,255,255,255,255,
   255,255,255,255,255,255,255,255,255,255,255,255,255,
};
static const uint8_t beta_table[52*3] = {
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  2,  2,  2,  3,
     3,  3,  3,  4,  4,  4,  6,  6,  7,  7,
     8,  8,  9,  9, 10, 10, 11, 11, 12, 12,
    13, 13, 14, 14, 15, 15, 16, 16, 17, 17,
    18, 18,
    18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
    18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
    18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
    18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
};
static const uint8_t tc0_table[52*3][4] = {
    {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 },
    {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 },
    {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 },
    {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 },
    {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 },
    {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 },
    {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 },
    {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 },
    {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 },
    {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 },
    {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 },
    {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 1 },
    {-1, 0, 0, 1 }, {-1, 0, 0, 1 }, {-1, 0, 0, 1 }, {-1, 0, 1, 1 }, {-1, 0, 1, 1 }, {-1, 1, 1, 1 },
    {-1, 1, 1, 1 }, {-1, 1, 1, 1 }, {-1, 1, 1, 1 }, {-1, 1, 1, 2 }, {-1, 1, 1, 2 }, {-1, 1, 1, 2 },
    {-1, 1, 1, 2 }, {-1, 1, 2, 3 }, {-1, 1, 2, 3 }, {-1, 2, 2, 3 }, {-1, 2, 2, 4 }, {-1, 2, 3, 4 },
    {-1, 2, 3, 4 }, {-1, 3, 3, 5 }, {-1, 3, 4, 6 }, {-1, 3, 4, 6 }, {-1, 4, 5, 7 }, {-1, 4, 5, 8 },
    {-1, 4, 6, 9 }, {-1, 5, 7,10 }, {-1, 6, 8,11 }, {-1, 6, 8,13 }, {-1, 7,10,14 }, {-1, 8,11,16 },
    {-1, 9,12,18 }, {-1,10,13,20 }, {-1,11,15,23 }, {-1,13,17,25 },
    {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 },
    {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 },
    {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 },
    {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 },
    {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 },
    {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 },
    {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 },
    {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 },
    {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 },
};

static void av_always_inline filter_mb_edgev( uint8_t *pix, int stride, int16_t bS[4], unsigned int qp, H264Context *h) {
    const unsigned int index_a = qp + h->slice_alpha_c0_offset;
    const int alpha = alpha_table[index_a];
    const int beta  = beta_table[qp + h->slice_beta_offset];
    if (alpha ==0 || beta == 0) return;

    if( bS[0] < 4 ) {
        int8_t tc[4];
        tc[0] = tc0_table[index_a][bS[0]];
        tc[1] = tc0_table[index_a][bS[1]];
        tc[2] = tc0_table[index_a][bS[2]];
        tc[3] = tc0_table[index_a][bS[3]];
        h->h264dsp.h264_h_loop_filter_luma(pix, stride, alpha, beta, tc);
    } else {
        h->h264dsp.h264_h_loop_filter_luma_intra(pix, stride, alpha, beta);
    }
}
static void av_always_inline filter_mb_edgecv( uint8_t *pix, int stride, int16_t bS[4], unsigned int qp, H264Context *h ) {
    const unsigned int index_a = qp + h->slice_alpha_c0_offset;
    const int alpha = alpha_table[index_a];
    const int beta  = beta_table[qp + h->slice_beta_offset];
    if (alpha ==0 || beta == 0) return;

    if( bS[0] < 4 ) {
        int8_t tc[4];
        tc[0] = tc0_table[index_a][bS[0]]+1;
        tc[1] = tc0_table[index_a][bS[1]]+1;
        tc[2] = tc0_table[index_a][bS[2]]+1;
        tc[3] = tc0_table[index_a][bS[3]]+1;
        h->h264dsp.h264_h_loop_filter_chroma(pix, stride, alpha, beta, tc);
    } else {
        h->h264dsp.h264_h_loop_filter_chroma_intra(pix, stride, alpha, beta);
    }
}

static void filter_mb_mbaff_edgev( H264Context *h, uint8_t *pix, int stride, int16_t bS[4], int bsi, int qp ) {
    int i;
    int index_a = qp + h->slice_alpha_c0_offset;
    int alpha = alpha_table[index_a];
    int beta  = beta_table[qp + h->slice_beta_offset];
    for( i = 0; i < 8; i++, pix += stride) {
        const int bS_index = (i >> 1) * bsi;

        if( bS[bS_index] == 0 ) {
            continue;
        }

        if( bS[bS_index] < 4 ) {
            const int tc0 = tc0_table[index_a][bS[bS_index]];
            const int p0 = pix[-1];
            const int p1 = pix[-2];
            const int p2 = pix[-3];
            const int q0 = pix[0];
            const int q1 = pix[1];
            const int q2 = pix[2];

            if( FFABS( p0 - q0 ) < alpha &&
                FFABS( p1 - p0 ) < beta &&
                FFABS( q1 - q0 ) < beta ) {
                int tc = tc0;
                int i_delta;

                if( FFABS( p2 - p0 ) < beta ) {
                    if(tc0)
                    pix[-2] = p1 + av_clip( ( p2 + ( ( p0 + q0 + 1 ) >> 1 ) - ( p1 << 1 ) ) >> 1, -tc0, tc0 );
                    tc++;
                }
                if( FFABS( q2 - q0 ) < beta ) {
                    if(tc0)
                    pix[1] = q1 + av_clip( ( q2 + ( ( p0 + q0 + 1 ) >> 1 ) - ( q1 << 1 ) ) >> 1, -tc0, tc0 );
                    tc++;
                }

                i_delta = av_clip( (((q0 - p0 ) << 2) + (p1 - q1) + 4) >> 3, -tc, tc );
                pix[-1] = av_clip_uint8( p0 + i_delta );    /* p0' */
                pix[0]  = av_clip_uint8( q0 - i_delta );    /* q0' */
                tprintf(h->s.avctx, "filter_mb_mbaff_edgev i:%d, qp:%d, indexA:%d, alpha:%d, beta:%d, tc:%d\n# bS:%d -> [%02x, %02x, %02x, %02x, %02x, %02x] =>[%02x, %02x, %02x, %02x]\n", i, qp[qp_index], index_a, alpha, beta, tc, bS[bS_index], pix[-3], p1, p0, q0, q1, pix[2], p1, pix[-1], pix[0], q1);
            }
        }else{
            const int p0 = pix[-1];
            const int p1 = pix[-2];
            const int p2 = pix[-3];

            const int q0 = pix[0];
            const int q1 = pix[1];
            const int q2 = pix[2];

            if( FFABS( p0 - q0 ) < alpha &&
                FFABS( p1 - p0 ) < beta &&
                FFABS( q1 - q0 ) < beta ) {

                if(FFABS( p0 - q0 ) < (( alpha >> 2 ) + 2 )){
                    if( FFABS( p2 - p0 ) < beta)
                    {
                        const int p3 = pix[-4];
                        /* p0', p1', p2' */
                        pix[-1] = ( p2 + 2*p1 + 2*p0 + 2*q0 + q1 + 4 ) >> 3;
                        pix[-2] = ( p2 + p1 + p0 + q0 + 2 ) >> 2;
                        pix[-3] = ( 2*p3 + 3*p2 + p1 + p0 + q0 + 4 ) >> 3;
                    } else {
                        /* p0' */
                        pix[-1] = ( 2*p1 + p0 + q1 + 2 ) >> 2;
                    }
                    if( FFABS( q2 - q0 ) < beta)
                    {
                        const int q3 = pix[3];
                        /* q0', q1', q2' */
                        pix[0] = ( p1 + 2*p0 + 2*q0 + 2*q1 + q2 + 4 ) >> 3;
                        pix[1] = ( p0 + q0 + q1 + q2 + 2 ) >> 2;
                        pix[2] = ( 2*q3 + 3*q2 + q1 + q0 + p0 + 4 ) >> 3;
                    } else {
                        /* q0' */
                        pix[0] = ( 2*q1 + q0 + p1 + 2 ) >> 2;
                    }
                }else{
                    /* p0', q0' */
                    pix[-1] = ( 2*p1 + p0 + q1 + 2 ) >> 2;
                    pix[ 0] = ( 2*q1 + q0 + p1 + 2 ) >> 2;
                }
                tprintf(h->s.avctx, "filter_mb_mbaff_edgev i:%d, qp:%d, indexA:%d, alpha:%d, beta:%d\n# bS:4 -> [%02x, %02x, %02x, %02x, %02x, %02x] =>[%02x, %02x, %02x, %02x, %02x, %02x]\n", i, qp[qp_index], index_a, alpha, beta, p2, p1, p0, q0, q1, q2, pix[-3], pix[-2], pix[-1], pix[0], pix[1], pix[2]);
            }
        }
    }
}
static void filter_mb_mbaff_edgecv( H264Context *h, uint8_t *pix, int stride, int16_t bS[4], int bsi, int qp ) {
    int i;
    int index_a = qp + h->slice_alpha_c0_offset;
    int alpha = alpha_table[index_a];
    int beta  = beta_table[qp + h->slice_beta_offset];
    for( i = 0; i < 4; i++, pix += stride) {
        const int bS_index = i*bsi;

        if( bS[bS_index] == 0 ) {
            continue;
        }

        if( bS[bS_index] < 4 ) {
            const int tc = tc0_table[index_a][bS[bS_index]] + 1;
            const int p0 = pix[-1];
            const int p1 = pix[-2];
            const int q0 = pix[0];
            const int q1 = pix[1];

            if( FFABS( p0 - q0 ) < alpha &&
                FFABS( p1 - p0 ) < beta &&
                FFABS( q1 - q0 ) < beta ) {
                const int i_delta = av_clip( (((q0 - p0 ) << 2) + (p1 - q1) + 4) >> 3, -tc, tc );

                pix[-1] = av_clip_uint8( p0 + i_delta );    /* p0' */
                pix[0]  = av_clip_uint8( q0 - i_delta );    /* q0' */
                tprintf(h->s.avctx, "filter_mb_mbaff_edgecv i:%d, qp:%d, indexA:%d, alpha:%d, beta:%d, tc:%d\n# bS:%d -> [%02x, %02x, %02x, %02x, %02x, %02x] =>[%02x, %02x, %02x, %02x]\n", i, qp[qp_index], index_a, alpha, beta, tc, bS[bS_index], pix[-3], p1, p0, q0, q1, pix[2], p1, pix[-1], pix[0], q1);
            }
        }else{
            const int p0 = pix[-1];
            const int p1 = pix[-2];
            const int q0 = pix[0];
            const int q1 = pix[1];

            if( FFABS( p0 - q0 ) < alpha &&
                FFABS( p1 - p0 ) < beta &&
                FFABS( q1 - q0 ) < beta ) {

                pix[-1] = ( 2*p1 + p0 + q1 + 2 ) >> 2;   /* p0' */
                pix[0]  = ( 2*q1 + q0 + p1 + 2 ) >> 2;   /* q0' */
                tprintf(h->s.avctx, "filter_mb_mbaff_edgecv i:%d\n# bS:4 -> [%02x, %02x, %02x, %02x, %02x, %02x] =>[%02x, %02x, %02x, %02x, %02x, %02x]\n", i, pix[-3], p1, p0, q0, q1, pix[2], pix[-3], pix[-2], pix[-1], pix[0], pix[1], pix[2]);
            }
        }
    }
}

static void av_always_inline filter_mb_edgeh( uint8_t *pix, int stride, int16_t bS[4], unsigned int qp, H264Context *h ) {
    const unsigned int index_a = qp + h->slice_alpha_c0_offset;
    const int alpha = alpha_table[index_a];
    const int beta  = beta_table[qp + h->slice_beta_offset];
    if (alpha ==0 || beta == 0) return;

    if( bS[0] < 4 ) {
        int8_t tc[4];
        tc[0] = tc0_table[index_a][bS[0]];
        tc[1] = tc0_table[index_a][bS[1]];
        tc[2] = tc0_table[index_a][bS[2]];
        tc[3] = tc0_table[index_a][bS[3]];
        h->h264dsp.h264_v_loop_filter_luma(pix, stride, alpha, beta, tc);
    } else {
        h->h264dsp.h264_v_loop_filter_luma_intra(pix, stride, alpha, beta);
    }
}

static void av_always_inline filter_mb_edgech( uint8_t *pix, int stride, int16_t bS[4], unsigned int qp, H264Context *h ) {
    const unsigned int index_a = qp + h->slice_alpha_c0_offset;
    const int alpha = alpha_table[index_a];
    const int beta  = beta_table[qp + h->slice_beta_offset];
    if (alpha ==0 || beta == 0) return;

    if( bS[0] < 4 ) {
        int8_t tc[4];
        tc[0] = tc0_table[index_a][bS[0]]+1;
        tc[1] = tc0_table[index_a][bS[1]]+1;
        tc[2] = tc0_table[index_a][bS[2]]+1;
        tc[3] = tc0_table[index_a][bS[3]]+1;
        h->h264dsp.h264_v_loop_filter_chroma(pix, stride, alpha, beta, tc);
    } else {
        h->h264dsp.h264_v_loop_filter_chroma_intra(pix, stride, alpha, beta);
    }
}

void ff_h264_filter_mb_fast( H264Context *h, int mb_x, int mb_y, uint8_t *img_y, uint8_t *img_cb, uint8_t *img_cr, unsigned int linesize, unsigned int uvlinesize) {
    MpegEncContext * const s = &h->s;
    int mb_xy;
    int mb_type, left_type;
    int qp, qp0, qp1, qpc, qpc0, qpc1, qp_thresh;

    mb_xy = h->mb_xy;

    if(!h->top_type || !h->h264dsp.h264_loop_filter_strength || h->pps.chroma_qp_diff) {
        ff_h264_filter_mb(h, mb_x, mb_y, img_y, img_cb, img_cr, linesize, uvlinesize);
        return;
    }
    assert(!FRAME_MBAFF);
    left_type= h->left_type[0];

    mb_type = s->current_picture.mb_type[mb_xy];
    qp = s->current_picture.qscale_table[mb_xy];
    qp0 = s->current_picture.qscale_table[mb_xy-1];
    qp1 = s->current_picture.qscale_table[h->top_mb_xy];
    qpc = get_chroma_qp( h, 0, qp );
    qpc0 = get_chroma_qp( h, 0, qp0 );
    qpc1 = get_chroma_qp( h, 0, qp1 );
    qp0 = (qp + qp0 + 1) >> 1;
    qp1 = (qp + qp1 + 1) >> 1;
    qpc0 = (qpc + qpc0 + 1) >> 1;
    qpc1 = (qpc + qpc1 + 1) >> 1;
    qp_thresh = 15+52 - h->slice_alpha_c0_offset;
    if(qp <= qp_thresh && qp0 <= qp_thresh && qp1 <= qp_thresh &&
       qpc <= qp_thresh && qpc0 <= qp_thresh && qpc1 <= qp_thresh)
        return;

    if( IS_INTRA(mb_type) ) {
        int16_t bS4[4] = {4,4,4,4};
        int16_t bS3[4] = {3,3,3,3};
        int16_t *bSH = FIELD_PICTURE ? bS3 : bS4;
        if(left_type)
            filter_mb_edgev( &img_y[4*0], linesize, bS4, qp0, h);
        if( IS_8x8DCT(mb_type) ) {
            filter_mb_edgev( &img_y[4*2], linesize, bS3, qp, h);
            filter_mb_edgeh( &img_y[4*0*linesize], linesize, bSH, qp1, h);
            filter_mb_edgeh( &img_y[4*2*linesize], linesize, bS3, qp, h);
        } else {
            filter_mb_edgev( &img_y[4*1], linesize, bS3, qp, h);
            filter_mb_edgev( &img_y[4*2], linesize, bS3, qp, h);
            filter_mb_edgev( &img_y[4*3], linesize, bS3, qp, h);
            filter_mb_edgeh( &img_y[4*0*linesize], linesize, bSH, qp1, h);
            filter_mb_edgeh( &img_y[4*1*linesize], linesize, bS3, qp, h);
            filter_mb_edgeh( &img_y[4*2*linesize], linesize, bS3, qp, h);
            filter_mb_edgeh( &img_y[4*3*linesize], linesize, bS3, qp, h);
        }
        if(left_type){
            filter_mb_edgecv( &img_cb[2*0], uvlinesize, bS4, qpc0, h);
            filter_mb_edgecv( &img_cr[2*0], uvlinesize, bS4, qpc0, h);
        }
        filter_mb_edgecv( &img_cb[2*2], uvlinesize, bS3, qpc, h);
        filter_mb_edgecv( &img_cr[2*2], uvlinesize, bS3, qpc, h);
        filter_mb_edgech( &img_cb[2*0*uvlinesize], uvlinesize, bSH, qpc1, h);
        filter_mb_edgech( &img_cb[2*2*uvlinesize], uvlinesize, bS3, qpc, h);
        filter_mb_edgech( &img_cr[2*0*uvlinesize], uvlinesize, bSH, qpc1, h);
        filter_mb_edgech( &img_cr[2*2*uvlinesize], uvlinesize, bS3, qpc, h);
        return;
    } else {
        LOCAL_ALIGNED_8(int16_t, bS, [2], [4][4]);
        int edges;
        if( IS_8x8DCT(mb_type) && (h->cbp&7) == 7 ) {
            edges = 4;
            AV_WN64A(bS[0][0], 0x0002000200020002ULL);
            AV_WN64A(bS[0][2], 0x0002000200020002ULL);
            AV_WN64A(bS[1][0], 0x0002000200020002ULL);
            AV_WN64A(bS[1][2], 0x0002000200020002ULL);
        } else {
            int mask_edge1 = (3*(((5*mb_type)>>5)&1)) | (mb_type>>4); //(mb_type & (MB_TYPE_16x16 | MB_TYPE_8x16)) ? 3 : (mb_type & MB_TYPE_16x8) ? 1 : 0;
            int mask_edge0 = 3*((mask_edge1>>1) & ((5*left_type)>>5)&1); // (mb_type & (MB_TYPE_16x16 | MB_TYPE_8x16)) && (h->left_type[0] & (MB_TYPE_16x16 | MB_TYPE_8x16)) ? 3 : 0;
            int step =  1+(mb_type>>24); //IS_8x8DCT(mb_type) ? 2 : 1;
            edges = 4 - 3*((mb_type>>3) & !(h->cbp & 15)); //(mb_type & MB_TYPE_16x16) && !(h->cbp & 15) ? 1 : 4;
            h->h264dsp.h264_loop_filter_strength( bS, h->non_zero_count_cache, h->ref_cache, h->mv_cache,
                                              h->list_count==2, edges, step, mask_edge0, mask_edge1, FIELD_PICTURE);
        }
        if( IS_INTRA(left_type) )
            AV_WN64A(bS[0][0], 0x0004000400040004ULL);
        if( IS_INTRA(h->top_type) )
            AV_WN64A(bS[1][0], FIELD_PICTURE ? 0x0003000300030003ULL : 0x0004000400040004ULL);

#define FILTER(hv,dir,edge)\
        if(AV_RN64A(bS[dir][edge])) {                                   \
            filter_mb_edge##hv( &img_y[4*edge*(dir?linesize:1)], linesize, bS[dir][edge], edge ? qp : qp##dir, h );\
            if(!(edge&1)) {\
                filter_mb_edgec##hv( &img_cb[2*edge*(dir?uvlinesize:1)], uvlinesize, bS[dir][edge], edge ? qpc : qpc##dir, h );\
                filter_mb_edgec##hv( &img_cr[2*edge*(dir?uvlinesize:1)], uvlinesize, bS[dir][edge], edge ? qpc : qpc##dir, h );\
            }\
        }
        if(left_type)
            FILTER(v,0,0);
        if( edges == 1 ) {
            FILTER(h,1,0);
        } else if( IS_8x8DCT(mb_type) ) {
            FILTER(v,0,2);
            FILTER(h,1,0);
            FILTER(h,1,2);
        } else {
            FILTER(v,0,1);
            FILTER(v,0,2);
            FILTER(v,0,3);
            FILTER(h,1,0);
            FILTER(h,1,1);
            FILTER(h,1,2);
            FILTER(h,1,3);
        }
#undef FILTER
    }
}

static int check_mv(H264Context *h, long b_idx, long bn_idx, int mvy_limit){
    int v;

    v= h->ref_cache[0][b_idx] != h->ref_cache[0][bn_idx];
    if(!v && h->ref_cache[0][b_idx]!=-1)
        v= h->mv_cache[0][b_idx][0] - h->mv_cache[0][bn_idx][0] + 3 >= 7U |
           FFABS( h->mv_cache[0][b_idx][1] - h->mv_cache[0][bn_idx][1] ) >= mvy_limit;

    if(h->list_count==2){
        if(!v)
            v = h->ref_cache[1][b_idx] != h->ref_cache[1][bn_idx] |
                h->mv_cache[1][b_idx][0] - h->mv_cache[1][bn_idx][0] + 3 >= 7U |
                FFABS( h->mv_cache[1][b_idx][1] - h->mv_cache[1][bn_idx][1] ) >= mvy_limit;

        if(v){
            if(h->ref_cache[0][b_idx] != h->ref_cache[1][bn_idx] |
               h->ref_cache[1][b_idx] != h->ref_cache[0][bn_idx])
                return 1;
            return
                h->mv_cache[0][b_idx][0] - h->mv_cache[1][bn_idx][0] + 3 >= 7U |
                FFABS( h->mv_cache[0][b_idx][1] - h->mv_cache[1][bn_idx][1] ) >= mvy_limit |
                h->mv_cache[1][b_idx][0] - h->mv_cache[0][bn_idx][0] + 3 >= 7U |
                FFABS( h->mv_cache[1][b_idx][1] - h->mv_cache[0][bn_idx][1] ) >= mvy_limit;
        }
    }

    return v;
}

static av_always_inline void filter_mb_dir(H264Context *h, int mb_x, int mb_y, uint8_t *img_y, uint8_t *img_cb, uint8_t *img_cr, unsigned int linesize, unsigned int uvlinesize, int mb_xy, int mb_type, int mvy_limit, int first_vertical_edge_done, int dir) {
    MpegEncContext * const s = &h->s;
    int edge;
    const int mbm_xy = dir == 0 ? mb_xy -1 : h->top_mb_xy;
    const int mbm_type = dir == 0 ? h->left_type[0] : h->top_type;

    // how often to recheck mv-based bS when iterating between edges
    static const uint8_t mask_edge_tab[2][8]={{0,3,3,3,1,1,1,1},
                                              {0,3,1,1,3,3,3,3}};
    const int mask_edge = mask_edge_tab[dir][(mb_type>>3)&7];
    const int edges = mask_edge== 3 && !(h->cbp&15) ? 1 : 4;

    // how often to recheck mv-based bS when iterating along each edge
    const int mask_par0 = mb_type & (MB_TYPE_16x16 | (MB_TYPE_8x16 >> dir));

    if(mbm_type && !first_vertical_edge_done){

        if (FRAME_MBAFF && (dir == 1) && ((mb_y&1) == 0)
            && IS_INTERLACED(mbm_type&~mb_type)
            ) {
            // This is a special case in the norm where the filtering must
            // be done twice (one each of the field) even if we are in a
            // frame macroblock.
            //
            unsigned int tmp_linesize   = 2 *   linesize;
            unsigned int tmp_uvlinesize = 2 * uvlinesize;
            int mbn_xy = mb_xy - 2 * s->mb_stride;
            int j;

            for(j=0; j<2; j++, mbn_xy += s->mb_stride){
                DECLARE_ALIGNED(8, int16_t, bS)[4];
                int qp;
                if( IS_INTRA(mb_type|s->current_picture.mb_type[mbn_xy]) ) {
                    AV_WN64A(bS, 0x0003000300030003ULL);
                } else {
                    if(!CABAC && IS_8x8DCT(s->current_picture.mb_type[mbn_xy])){
                        bS[0]= 1+((h->cbp_table[mbn_xy] & 4)||h->non_zero_count_cache[scan8[0]+0]);
                        bS[1]= 1+((h->cbp_table[mbn_xy] & 4)||h->non_zero_count_cache[scan8[0]+1]);
                        bS[2]= 1+((h->cbp_table[mbn_xy] & 8)||h->non_zero_count_cache[scan8[0]+2]);
                        bS[3]= 1+((h->cbp_table[mbn_xy] & 8)||h->non_zero_count_cache[scan8[0]+3]);
                    }else{
                    const uint8_t *mbn_nnz = h->non_zero_count[mbn_xy] + 4+3*8;
                    int i;
                    for( i = 0; i < 4; i++ ) {
                        bS[i] = 1 + !!(h->non_zero_count_cache[scan8[0]+i] | mbn_nnz[i]);
                    }
                    }
                }
                // Do not use s->qscale as luma quantizer because it has not the same
                // value in IPCM macroblocks.
                qp = ( s->current_picture.qscale_table[mb_xy] + s->current_picture.qscale_table[mbn_xy] + 1 ) >> 1;
                tprintf(s->avctx, "filter mb:%d/%d dir:%d edge:%d, QPy:%d ls:%d uvls:%d", mb_x, mb_y, dir, edge, qp, tmp_linesize, tmp_uvlinesize);
                { int i; for (i = 0; i < 4; i++) tprintf(s->avctx, " bS[%d]:%d", i, bS[i]); tprintf(s->avctx, "\n"); }
                filter_mb_edgeh( &img_y[j*linesize], tmp_linesize, bS, qp, h );
                filter_mb_edgech( &img_cb[j*uvlinesize], tmp_uvlinesize, bS,
                                ( h->chroma_qp[0] + get_chroma_qp( h, 0, s->current_picture.qscale_table[mbn_xy] ) + 1 ) >> 1, h);
                filter_mb_edgech( &img_cr[j*uvlinesize], tmp_uvlinesize, bS,
                                ( h->chroma_qp[1] + get_chroma_qp( h, 1, s->current_picture.qscale_table[mbn_xy] ) + 1 ) >> 1, h);
            }
        }else{
            DECLARE_ALIGNED(8, int16_t, bS)[4];
            int qp;

            if( IS_INTRA(mb_type|mbm_type)) {
                AV_WN64A(bS, 0x0003000300030003ULL);
                if (   (!IS_INTERLACED(mb_type|mbm_type))
                    || ((FRAME_MBAFF || (s->picture_structure != PICT_FRAME)) && (dir == 0))
                )
                    AV_WN64A(bS, 0x0004000400040004ULL);
            } else {
                int i;
                int mv_done;

                if( dir && FRAME_MBAFF && IS_INTERLACED(mb_type ^ mbm_type)) {
                    AV_WN64A(bS, 0x0001000100010001ULL);
                    mv_done = 1;
                }
                else if( mask_par0 && ((mbm_type & (MB_TYPE_16x16 | (MB_TYPE_8x16 >> dir)))) ) {
                    int b_idx= 8 + 4;
                    int bn_idx= b_idx - (dir ? 8:1);

                    bS[0] = bS[1] = bS[2] = bS[3] = check_mv(h, 8 + 4, bn_idx, mvy_limit);
                    mv_done = 1;
                }
                else
                    mv_done = 0;

                for( i = 0; i < 4; i++ ) {
                    int x = dir == 0 ? 0 : i;
                    int y = dir == 0 ? i    : 0;
                    int b_idx= 8 + 4 + x + 8*y;
                    int bn_idx= b_idx - (dir ? 8:1);

                    if( h->non_zero_count_cache[b_idx] |
                        h->non_zero_count_cache[bn_idx] ) {
                        bS[i] = 2;
                    }
                    else if(!mv_done)
                    {
                        bS[i] = check_mv(h, b_idx, bn_idx, mvy_limit);
                    }
                }
            }

            /* Filter edge */
            // Do not use s->qscale as luma quantizer because it has not the same
            // value in IPCM macroblocks.
            if(bS[0]+bS[1]+bS[2]+bS[3]){
                qp = ( s->current_picture.qscale_table[mb_xy] + s->current_picture.qscale_table[mbm_xy] + 1 ) >> 1;
                //tprintf(s->avctx, "filter mb:%d/%d dir:%d edge:%d, QPy:%d, QPc:%d, QPcn:%d\n", mb_x, mb_y, dir, edge, qp, h->chroma_qp[0], s->current_picture.qscale_table[mbn_xy]);
                tprintf(s->avctx, "filter mb:%d/%d dir:%d edge:%d, QPy:%d ls:%d uvls:%d", mb_x, mb_y, dir, edge, qp, linesize, uvlinesize);
                //{ int i; for (i = 0; i < 4; i++) tprintf(s->avctx, " bS[%d]:%d", i, bS[i]); tprintf(s->avctx, "\n"); }
                if( dir == 0 ) {
                    filter_mb_edgev( &img_y[0], linesize, bS, qp, h );
                    {
                        int qp= ( h->chroma_qp[0] + get_chroma_qp( h, 0, s->current_picture.qscale_table[mbm_xy] ) + 1 ) >> 1;
                        filter_mb_edgecv( &img_cb[0], uvlinesize, bS, qp, h);
                        if(h->pps.chroma_qp_diff)
                            qp= ( h->chroma_qp[1] + get_chroma_qp( h, 1, s->current_picture.qscale_table[mbm_xy] ) + 1 ) >> 1;
                        filter_mb_edgecv( &img_cr[0], uvlinesize, bS, qp, h);
                    }
                } else {
                    filter_mb_edgeh( &img_y[0], linesize, bS, qp, h );
                    {
                        int qp= ( h->chroma_qp[0] + get_chroma_qp( h, 0, s->current_picture.qscale_table[mbm_xy] ) + 1 ) >> 1;
                        filter_mb_edgech( &img_cb[0], uvlinesize, bS, qp, h);
                        if(h->pps.chroma_qp_diff)
                            qp= ( h->chroma_qp[1] + get_chroma_qp( h, 1, s->current_picture.qscale_table[mbm_xy] ) + 1 ) >> 1;
                        filter_mb_edgech( &img_cr[0], uvlinesize, bS, qp, h);
                    }
                }
            }
        }
    }

    /* Calculate bS */
    for( edge = 1; edge < edges; edge++ ) {
        DECLARE_ALIGNED(8, int16_t, bS)[4];
        int qp;

        if( IS_8x8DCT(mb_type & (edge<<24)) ) // (edge&1) && IS_8x8DCT(mb_type)
            continue;

        if( IS_INTRA(mb_type)) {
            AV_WN64A(bS, 0x0003000300030003ULL);
        } else {
            int i;
            int mv_done;

            if( edge & mask_edge ) {
                AV_ZERO64(bS);
                mv_done = 1;
            }
            else if( mask_par0 ) {
                int b_idx= 8 + 4 + edge * (dir ? 8:1);
                int bn_idx= b_idx - (dir ? 8:1);

                bS[0] = bS[1] = bS[2] = bS[3] = check_mv(h, b_idx, bn_idx, mvy_limit);
                mv_done = 1;
            }
            else
                mv_done = 0;

            for( i = 0; i < 4; i++ ) {
                int x = dir == 0 ? edge : i;
                int y = dir == 0 ? i    : edge;
                int b_idx= 8 + 4 + x + 8*y;
                int bn_idx= b_idx - (dir ? 8:1);

                if( h->non_zero_count_cache[b_idx] |
                    h->non_zero_count_cache[bn_idx] ) {
                    bS[i] = 2;
                }
                else if(!mv_done)
                {
                    bS[i] = check_mv(h, b_idx, bn_idx, mvy_limit);
                }
            }

            if(bS[0]+bS[1]+bS[2]+bS[3] == 0)
                continue;
        }

        /* Filter edge */
        // Do not use s->qscale as luma quantizer because it has not the same
        // value in IPCM macroblocks.
        qp = s->current_picture.qscale_table[mb_xy];
        //tprintf(s->avctx, "filter mb:%d/%d dir:%d edge:%d, QPy:%d, QPc:%d, QPcn:%d\n", mb_x, mb_y, dir, edge, qp, h->chroma_qp[0], s->current_picture.qscale_table[mbn_xy]);
        tprintf(s->avctx, "filter mb:%d/%d dir:%d edge:%d, QPy:%d ls:%d uvls:%d", mb_x, mb_y, dir, edge, qp, linesize, uvlinesize);
        //{ int i; for (i = 0; i < 4; i++) tprintf(s->avctx, " bS[%d]:%d", i, bS[i]); tprintf(s->avctx, "\n"); }
        if( dir == 0 ) {
            filter_mb_edgev( &img_y[4*edge], linesize, bS, qp, h );
            if( (edge&1) == 0 ) {
                filter_mb_edgecv( &img_cb[2*edge], uvlinesize, bS, h->chroma_qp[0], h);
                filter_mb_edgecv( &img_cr[2*edge], uvlinesize, bS, h->chroma_qp[1], h);
            }
        } else {
            filter_mb_edgeh( &img_y[4*edge*linesize], linesize, bS, qp, h );
            if( (edge&1) == 0 ) {
                filter_mb_edgech( &img_cb[2*edge*uvlinesize], uvlinesize, bS, h->chroma_qp[0], h);
                filter_mb_edgech( &img_cr[2*edge*uvlinesize], uvlinesize, bS, h->chroma_qp[1], h);
            }
        }
    }
}

void ff_h264_filter_mb( H264Context *h, int mb_x, int mb_y, uint8_t *img_y, uint8_t *img_cb, uint8_t *img_cr, unsigned int linesize, unsigned int uvlinesize) {
    MpegEncContext * const s = &h->s;
    const int mb_xy= mb_x + mb_y*s->mb_stride;
    const int mb_type = s->current_picture.mb_type[mb_xy];
    const int mvy_limit = IS_INTERLACED(mb_type) ? 2 : 4;
    int first_vertical_edge_done = 0;
    av_unused int dir;

    if (FRAME_MBAFF
            // and current and left pair do not have the same interlaced type
            && IS_INTERLACED(mb_type^h->left_type[0])
            // and left mb is in available to us
            && h->left_type[0]) {
        /* First vertical edge is different in MBAFF frames
         * There are 8 different bS to compute and 2 different Qp
         */
        DECLARE_ALIGNED(8, int16_t, bS)[8];
        int qp[2];
        int bqp[2];
        int rqp[2];
        int mb_qp, mbn0_qp, mbn1_qp;
        int i;
        first_vertical_edge_done = 1;

        if( IS_INTRA(mb_type) ) {
            AV_WN64A(&bS[0], 0x0004000400040004ULL);
            AV_WN64A(&bS[4], 0x0004000400040004ULL);
        } else {
            static const uint8_t offset[2][2][8]={
                {
                    {7+8*0, 7+8*0, 7+8*0, 7+8*0, 7+8*1, 7+8*1, 7+8*1, 7+8*1},
                    {7+8*2, 7+8*2, 7+8*2, 7+8*2, 7+8*3, 7+8*3, 7+8*3, 7+8*3},
                },{
                    {7+8*0, 7+8*1, 7+8*2, 7+8*3, 7+8*0, 7+8*1, 7+8*2, 7+8*3},
                    {7+8*0, 7+8*1, 7+8*2, 7+8*3, 7+8*0, 7+8*1, 7+8*2, 7+8*3},
                }
            };
            const uint8_t *off= offset[MB_FIELD][mb_y&1];
            for( i = 0; i < 8; i++ ) {
                int j= MB_FIELD ? i>>2 : i&1;
                int mbn_xy = h->left_mb_xy[j];
                int mbn_type= h->left_type[j];

                if( IS_INTRA( mbn_type ) )
                    bS[i] = 4;
                else{
                    bS[i] = 1 + !!(h->non_zero_count_cache[12+8*(i>>1)] |
                         ((!h->pps.cabac && IS_8x8DCT(mbn_type)) ?
                            (h->cbp_table[mbn_xy] & ((MB_FIELD ? (i&2) : (mb_y&1)) ? 8 : 2))
                                                                       :
                            h->non_zero_count[mbn_xy][ off[i] ]));
                }
            }
        }

        mb_qp = s->current_picture.qscale_table[mb_xy];
        mbn0_qp = s->current_picture.qscale_table[h->left_mb_xy[0]];
        mbn1_qp = s->current_picture.qscale_table[h->left_mb_xy[1]];
        qp[0] = ( mb_qp + mbn0_qp + 1 ) >> 1;
        bqp[0] = ( get_chroma_qp( h, 0, mb_qp ) +
                   get_chroma_qp( h, 0, mbn0_qp ) + 1 ) >> 1;
        rqp[0] = ( get_chroma_qp( h, 1, mb_qp ) +
                   get_chroma_qp( h, 1, mbn0_qp ) + 1 ) >> 1;
        qp[1] = ( mb_qp + mbn1_qp + 1 ) >> 1;
        bqp[1] = ( get_chroma_qp( h, 0, mb_qp ) +
                   get_chroma_qp( h, 0, mbn1_qp ) + 1 ) >> 1;
        rqp[1] = ( get_chroma_qp( h, 1, mb_qp ) +
                   get_chroma_qp( h, 1, mbn1_qp ) + 1 ) >> 1;

        /* Filter edge */
        tprintf(s->avctx, "filter mb:%d/%d MBAFF, QPy:%d/%d, QPb:%d/%d QPr:%d/%d ls:%d uvls:%d", mb_x, mb_y, qp[0], qp[1], bqp[0], bqp[1], rqp[0], rqp[1], linesize, uvlinesize);
        { int i; for (i = 0; i < 8; i++) tprintf(s->avctx, " bS[%d]:%d", i, bS[i]); tprintf(s->avctx, "\n"); }
        if(MB_FIELD){
            filter_mb_mbaff_edgev ( h, img_y                ,   linesize, bS  , 1, qp [0] );
            filter_mb_mbaff_edgev ( h, img_y  + 8*  linesize,   linesize, bS+4, 1, qp [1] );
            filter_mb_mbaff_edgecv( h, img_cb,                uvlinesize, bS  , 1, bqp[0] );
            filter_mb_mbaff_edgecv( h, img_cb + 4*uvlinesize, uvlinesize, bS+4, 1, bqp[1] );
            filter_mb_mbaff_edgecv( h, img_cr,                uvlinesize, bS  , 1, rqp[0] );
            filter_mb_mbaff_edgecv( h, img_cr + 4*uvlinesize, uvlinesize, bS+4, 1, rqp[1] );
        }else{
            filter_mb_mbaff_edgev ( h, img_y              , 2*  linesize, bS  , 2, qp [0] );
            filter_mb_mbaff_edgev ( h, img_y  +   linesize, 2*  linesize, bS+1, 2, qp [1] );
            filter_mb_mbaff_edgecv( h, img_cb,              2*uvlinesize, bS  , 2, bqp[0] );
            filter_mb_mbaff_edgecv( h, img_cb + uvlinesize, 2*uvlinesize, bS+1, 2, bqp[1] );
            filter_mb_mbaff_edgecv( h, img_cr,              2*uvlinesize, bS  , 2, rqp[0] );
            filter_mb_mbaff_edgecv( h, img_cr + uvlinesize, 2*uvlinesize, bS+1, 2, rqp[1] );
        }
    }

#if CONFIG_SMALL
    for( dir = 0; dir < 2; dir++ )
        filter_mb_dir(h, mb_x, mb_y, img_y, img_cb, img_cr, linesize, uvlinesize, mb_xy, mb_type, mvy_limit, dir ? 0 : first_vertical_edge_done, dir);
#else
    filter_mb_dir(h, mb_x, mb_y, img_y, img_cb, img_cr, linesize, uvlinesize, mb_xy, mb_type, mvy_limit, first_vertical_edge_done, 0);
    filter_mb_dir(h, mb_x, mb_y, img_y, img_cb, img_cr, linesize, uvlinesize, mb_xy, mb_type, mvy_limit, 0, 1);
#endif
}
