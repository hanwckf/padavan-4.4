/*
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

#include "libavcodec/dsputil.h"
#include "fft.h"

av_cold void ff_fft_init_mmx(FFTContext *s)
{
#if HAVE_YASM
    int has_vectors = mm_support();
    if (has_vectors & FF_MM_SSE && HAVE_SSE) {
        /* SSE for P3/P4/K8 */
        s->imdct_calc  = ff_imdct_calc_sse;
        s->imdct_half  = ff_imdct_half_sse;
        s->fft_permute = ff_fft_permute_sse;
        s->fft_calc    = ff_fft_calc_sse;
    } else if (has_vectors & FF_MM_3DNOWEXT && HAVE_AMD3DNOWEXT) {
        /* 3DNowEx for K7 */
        s->imdct_calc = ff_imdct_calc_3dn2;
        s->imdct_half = ff_imdct_half_3dn2;
        s->fft_calc   = ff_fft_calc_3dn2;
    } else if (has_vectors & FF_MM_3DNOW && HAVE_AMD3DNOW) {
        /* 3DNow! for K6-2/3 */
        s->imdct_calc = ff_imdct_calc_3dn;
        s->imdct_half = ff_imdct_half_3dn;
        s->fft_calc   = ff_fft_calc_3dn;
    }
#endif
}
