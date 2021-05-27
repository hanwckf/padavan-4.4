/*
 * MMX optimized DSP utils
 * Copyright (c) 2000, 2001 Fabrice Bellard
 * Copyright (c) 2002-2004 Michael Niedermayer <michaelni@gmx.at>
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
 *
 * MMX optimization by Nick Kurshev <nickols_k@mail.ru>
 */

#include "libavutil/x86_cpu.h"
#include "libavcodec/dsputil.h"
#include "libavcodec/h264dsp.h"
#include "libavcodec/mpegvideo.h"
#include "libavcodec/simple_idct.h"
#include "dsputil_mmx.h"
#include "vp3dsp_mmx.h"
#include "vp3dsp_sse2.h"
#include "vp6dsp_mmx.h"
#include "vp6dsp_sse2.h"
#include "idct_xvid.h"

//#undef NDEBUG
//#include <assert.h>

int mm_flags; /* multimedia extension flags */

/* pixel operations */
DECLARE_ALIGNED(8,  const uint64_t, ff_bone) = 0x0101010101010101ULL;
DECLARE_ALIGNED(8,  const uint64_t, ff_wtwo) = 0x0002000200020002ULL;

DECLARE_ALIGNED(16, const uint64_t, ff_pdw_80000000)[2] =
{0x8000000080000000ULL, 0x8000000080000000ULL};

DECLARE_ALIGNED(8,  const uint64_t, ff_pw_3  ) = 0x0003000300030003ULL;
DECLARE_ALIGNED(8,  const uint64_t, ff_pw_4  ) = 0x0004000400040004ULL;
DECLARE_ALIGNED(16, const xmm_reg,  ff_pw_5  ) = {0x0005000500050005ULL, 0x0005000500050005ULL};
DECLARE_ALIGNED(16, const xmm_reg,  ff_pw_8  ) = {0x0008000800080008ULL, 0x0008000800080008ULL};
DECLARE_ALIGNED(8,  const uint64_t, ff_pw_15 ) = 0x000F000F000F000FULL;
DECLARE_ALIGNED(16, const xmm_reg,  ff_pw_16 ) = {0x0010001000100010ULL, 0x0010001000100010ULL};
DECLARE_ALIGNED(8,  const uint64_t, ff_pw_20 ) = 0x0014001400140014ULL;
DECLARE_ALIGNED(16, const xmm_reg,  ff_pw_28 ) = {0x001C001C001C001CULL, 0x001C001C001C001CULL};
DECLARE_ALIGNED(16, const xmm_reg,  ff_pw_32 ) = {0x0020002000200020ULL, 0x0020002000200020ULL};
DECLARE_ALIGNED(8,  const uint64_t, ff_pw_42 ) = 0x002A002A002A002AULL;
DECLARE_ALIGNED(16, const xmm_reg,  ff_pw_64 ) = {0x0040004000400040ULL, 0x0040004000400040ULL};
DECLARE_ALIGNED(8,  const uint64_t, ff_pw_96 ) = 0x0060006000600060ULL;
DECLARE_ALIGNED(8,  const uint64_t, ff_pw_128) = 0x0080008000800080ULL;
DECLARE_ALIGNED(8,  const uint64_t, ff_pw_255) = 0x00ff00ff00ff00ffULL;

DECLARE_ALIGNED(8,  const uint64_t, ff_pb_1  ) = 0x0101010101010101ULL;
DECLARE_ALIGNED(8,  const uint64_t, ff_pb_3  ) = 0x0303030303030303ULL;
DECLARE_ALIGNED(8,  const uint64_t, ff_pb_7  ) = 0x0707070707070707ULL;
DECLARE_ALIGNED(8,  const uint64_t, ff_pb_1F ) = 0x1F1F1F1F1F1F1F1FULL;
DECLARE_ALIGNED(8,  const uint64_t, ff_pb_3F ) = 0x3F3F3F3F3F3F3F3FULL;
DECLARE_ALIGNED(8,  const uint64_t, ff_pb_81 ) = 0x8181818181818181ULL;
DECLARE_ALIGNED(8,  const uint64_t, ff_pb_A1 ) = 0xA1A1A1A1A1A1A1A1ULL;
DECLARE_ALIGNED(8,  const uint64_t, ff_pb_FC ) = 0xFCFCFCFCFCFCFCFCULL;

DECLARE_ALIGNED(16, const double, ff_pd_1)[2] = { 1.0, 1.0 };
DECLARE_ALIGNED(16, const double, ff_pd_2)[2] = { 2.0, 2.0 };

#define JUMPALIGN() __asm__ volatile (ASMALIGN(3)::)
#define MOVQ_ZERO(regd)  __asm__ volatile ("pxor %%" #regd ", %%" #regd ::)

#define MOVQ_BFE(regd) \
    __asm__ volatile ( \
    "pcmpeqd %%" #regd ", %%" #regd " \n\t"\
    "paddb %%" #regd ", %%" #regd " \n\t" ::)

#ifndef PIC
#define MOVQ_BONE(regd)  __asm__ volatile ("movq %0, %%" #regd " \n\t" ::"m"(ff_bone))
#define MOVQ_WTWO(regd)  __asm__ volatile ("movq %0, %%" #regd " \n\t" ::"m"(ff_wtwo))
#else
// for shared library it's better to use this way for accessing constants
// pcmpeqd -> -1
#define MOVQ_BONE(regd) \
    __asm__ volatile ( \
    "pcmpeqd %%" #regd ", %%" #regd " \n\t" \
    "psrlw $15, %%" #regd " \n\t" \
    "packuswb %%" #regd ", %%" #regd " \n\t" ::)

#define MOVQ_WTWO(regd) \
    __asm__ volatile ( \
    "pcmpeqd %%" #regd ", %%" #regd " \n\t" \
    "psrlw $15, %%" #regd " \n\t" \
    "psllw $1, %%" #regd " \n\t"::)

#endif

// using regr as temporary and for the output result
// first argument is unmodifed and second is trashed
// regfe is supposed to contain 0xfefefefefefefefe
#define PAVGB_MMX_NO_RND(rega, regb, regr, regfe) \
    "movq " #rega ", " #regr "  \n\t"\
    "pand " #regb ", " #regr "  \n\t"\
    "pxor " #rega ", " #regb "  \n\t"\
    "pand " #regfe "," #regb "  \n\t"\
    "psrlq $1, " #regb "        \n\t"\
    "paddb " #regb ", " #regr " \n\t"

#define PAVGB_MMX(rega, regb, regr, regfe) \
    "movq " #rega ", " #regr "  \n\t"\
    "por  " #regb ", " #regr "  \n\t"\
    "pxor " #rega ", " #regb "  \n\t"\
    "pand " #regfe "," #regb "  \n\t"\
    "psrlq $1, " #regb "        \n\t"\
    "psubb " #regb ", " #regr " \n\t"

// mm6 is supposed to contain 0xfefefefefefefefe
#define PAVGBP_MMX_NO_RND(rega, regb, regr,  regc, regd, regp) \
    "movq " #rega ", " #regr "  \n\t"\
    "movq " #regc ", " #regp "  \n\t"\
    "pand " #regb ", " #regr "  \n\t"\
    "pand " #regd ", " #regp "  \n\t"\
    "pxor " #rega ", " #regb "  \n\t"\
    "pxor " #regc ", " #regd "  \n\t"\
    "pand %%mm6, " #regb "      \n\t"\
    "pand %%mm6, " #regd "      \n\t"\
    "psrlq $1, " #regb "        \n\t"\
    "psrlq $1, " #regd "        \n\t"\
    "paddb " #regb ", " #regr " \n\t"\
    "paddb " #regd ", " #regp " \n\t"

#define PAVGBP_MMX(rega, regb, regr, regc, regd, regp) \
    "movq " #rega ", " #regr "  \n\t"\
    "movq " #regc ", " #regp "  \n\t"\
    "por  " #regb ", " #regr "  \n\t"\
    "por  " #regd ", " #regp "  \n\t"\
    "pxor " #rega ", " #regb "  \n\t"\
    "pxor " #regc ", " #regd "  \n\t"\
    "pand %%mm6, " #regb "      \n\t"\
    "pand %%mm6, " #regd "      \n\t"\
    "psrlq $1, " #regd "        \n\t"\
    "psrlq $1, " #regb "        \n\t"\
    "psubb " #regb ", " #regr " \n\t"\
    "psubb " #regd ", " #regp " \n\t"

/***********************************/
/* MMX no rounding */
#define DEF(x, y) x ## _no_rnd_ ## y ##_mmx
#define SET_RND  MOVQ_WONE
#define PAVGBP(a, b, c, d, e, f)        PAVGBP_MMX_NO_RND(a, b, c, d, e, f)
#define PAVGB(a, b, c, e)               PAVGB_MMX_NO_RND(a, b, c, e)
#define OP_AVG(a, b, c, e)              PAVGB_MMX(a, b, c, e)

#include "dsputil_mmx_rnd_template.c"

#undef DEF
#undef SET_RND
#undef PAVGBP
#undef PAVGB
/***********************************/
/* MMX rounding */

#define DEF(x, y) x ## _ ## y ##_mmx
#define SET_RND  MOVQ_WTWO
#define PAVGBP(a, b, c, d, e, f)        PAVGBP_MMX(a, b, c, d, e, f)
#define PAVGB(a, b, c, e)               PAVGB_MMX(a, b, c, e)

#include "dsputil_mmx_rnd_template.c"

#undef DEF
#undef SET_RND
#undef PAVGBP
#undef PAVGB
#undef OP_AVG

/***********************************/
/* 3Dnow specific */

#define DEF(x) x ## _3dnow
#define PAVGB "pavgusb"
#define OP_AVG PAVGB

#include "dsputil_mmx_avg_template.c"

#undef DEF
#undef PAVGB
#undef OP_AVG

/***********************************/
/* MMX2 specific */

#define DEF(x) x ## _mmx2

/* Introduced only in MMX2 set */
#define PAVGB "pavgb"
#define OP_AVG PAVGB

#include "dsputil_mmx_avg_template.c"

#undef DEF
#undef PAVGB
#undef OP_AVG

#define put_no_rnd_pixels16_mmx put_pixels16_mmx
#define put_no_rnd_pixels8_mmx put_pixels8_mmx
#define put_pixels16_mmx2 put_pixels16_mmx
#define put_pixels8_mmx2 put_pixels8_mmx
#define put_pixels4_mmx2 put_pixels4_mmx
#define put_no_rnd_pixels16_mmx2 put_no_rnd_pixels16_mmx
#define put_no_rnd_pixels8_mmx2 put_no_rnd_pixels8_mmx
#define put_pixels16_3dnow put_pixels16_mmx
#define put_pixels8_3dnow put_pixels8_mmx
#define put_pixels4_3dnow put_pixels4_mmx
#define put_no_rnd_pixels16_3dnow put_no_rnd_pixels16_mmx
#define put_no_rnd_pixels8_3dnow put_no_rnd_pixels8_mmx

/***********************************/
/* standard MMX */

void put_pixels_clamped_mmx(const DCTELEM *block, uint8_t *pixels, int line_size)
{
    const DCTELEM *p;
    uint8_t *pix;

    /* read the pixels */
    p = block;
    pix = pixels;
    /* unrolled loop */
        __asm__ volatile(
                "movq   %3, %%mm0               \n\t"
                "movq   8%3, %%mm1              \n\t"
                "movq   16%3, %%mm2             \n\t"
                "movq   24%3, %%mm3             \n\t"
                "movq   32%3, %%mm4             \n\t"
                "movq   40%3, %%mm5             \n\t"
                "movq   48%3, %%mm6             \n\t"
                "movq   56%3, %%mm7             \n\t"
                "packuswb %%mm1, %%mm0          \n\t"
                "packuswb %%mm3, %%mm2          \n\t"
                "packuswb %%mm5, %%mm4          \n\t"
                "packuswb %%mm7, %%mm6          \n\t"
                "movq   %%mm0, (%0)             \n\t"
                "movq   %%mm2, (%0, %1)         \n\t"
                "movq   %%mm4, (%0, %1, 2)      \n\t"
                "movq   %%mm6, (%0, %2)         \n\t"
                ::"r" (pix), "r" ((x86_reg)line_size), "r" ((x86_reg)line_size*3), "m"(*p)
                :"memory");
        pix += line_size*4;
        p += 32;

    // if here would be an exact copy of the code above
    // compiler would generate some very strange code
    // thus using "r"
    __asm__ volatile(
            "movq       (%3), %%mm0             \n\t"
            "movq       8(%3), %%mm1            \n\t"
            "movq       16(%3), %%mm2           \n\t"
            "movq       24(%3), %%mm3           \n\t"
            "movq       32(%3), %%mm4           \n\t"
            "movq       40(%3), %%mm5           \n\t"
            "movq       48(%3), %%mm6           \n\t"
            "movq       56(%3), %%mm7           \n\t"
            "packuswb %%mm1, %%mm0              \n\t"
            "packuswb %%mm3, %%mm2              \n\t"
            "packuswb %%mm5, %%mm4              \n\t"
            "packuswb %%mm7, %%mm6              \n\t"
            "movq       %%mm0, (%0)             \n\t"
            "movq       %%mm2, (%0, %1)         \n\t"
            "movq       %%mm4, (%0, %1, 2)      \n\t"
            "movq       %%mm6, (%0, %2)         \n\t"
            ::"r" (pix), "r" ((x86_reg)line_size), "r" ((x86_reg)line_size*3), "r"(p)
            :"memory");
}

DECLARE_ASM_CONST(8, uint8_t, ff_vector128)[8] =
  { 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80 };

#define put_signed_pixels_clamped_mmx_half(off) \
            "movq    "#off"(%2), %%mm1          \n\t"\
            "movq 16+"#off"(%2), %%mm2          \n\t"\
            "movq 32+"#off"(%2), %%mm3          \n\t"\
            "movq 48+"#off"(%2), %%mm4          \n\t"\
            "packsswb  8+"#off"(%2), %%mm1      \n\t"\
            "packsswb 24+"#off"(%2), %%mm2      \n\t"\
            "packsswb 40+"#off"(%2), %%mm3      \n\t"\
            "packsswb 56+"#off"(%2), %%mm4      \n\t"\
            "paddb %%mm0, %%mm1                 \n\t"\
            "paddb %%mm0, %%mm2                 \n\t"\
            "paddb %%mm0, %%mm3                 \n\t"\
            "paddb %%mm0, %%mm4                 \n\t"\
            "movq %%mm1, (%0)                   \n\t"\
            "movq %%mm2, (%0, %3)               \n\t"\
            "movq %%mm3, (%0, %3, 2)            \n\t"\
            "movq %%mm4, (%0, %1)               \n\t"

void put_signed_pixels_clamped_mmx(const DCTELEM *block, uint8_t *pixels, int line_size)
{
    x86_reg line_skip = line_size;
    x86_reg line_skip3;

    __asm__ volatile (
            "movq "MANGLE(ff_vector128)", %%mm0 \n\t"
            "lea (%3, %3, 2), %1                \n\t"
            put_signed_pixels_clamped_mmx_half(0)
            "lea (%0, %3, 4), %0                \n\t"
            put_signed_pixels_clamped_mmx_half(64)
            :"+&r" (pixels), "=&r" (line_skip3)
            :"r" (block), "r"(line_skip)
            :"memory");
}

void add_pixels_clamped_mmx(const DCTELEM *block, uint8_t *pixels, int line_size)
{
    const DCTELEM *p;
    uint8_t *pix;
    int i;

    /* read the pixels */
    p = block;
    pix = pixels;
    MOVQ_ZERO(mm7);
    i = 4;
    do {
        __asm__ volatile(
                "movq   (%2), %%mm0     \n\t"
                "movq   8(%2), %%mm1    \n\t"
                "movq   16(%2), %%mm2   \n\t"
                "movq   24(%2), %%mm3   \n\t"
                "movq   %0, %%mm4       \n\t"
                "movq   %1, %%mm6       \n\t"
                "movq   %%mm4, %%mm5    \n\t"
                "punpcklbw %%mm7, %%mm4 \n\t"
                "punpckhbw %%mm7, %%mm5 \n\t"
                "paddsw %%mm4, %%mm0    \n\t"
                "paddsw %%mm5, %%mm1    \n\t"
                "movq   %%mm6, %%mm5    \n\t"
                "punpcklbw %%mm7, %%mm6 \n\t"
                "punpckhbw %%mm7, %%mm5 \n\t"
                "paddsw %%mm6, %%mm2    \n\t"
                "paddsw %%mm5, %%mm3    \n\t"
                "packuswb %%mm1, %%mm0  \n\t"
                "packuswb %%mm3, %%mm2  \n\t"
                "movq   %%mm0, %0       \n\t"
                "movq   %%mm2, %1       \n\t"
                :"+m"(*pix), "+m"(*(pix+line_size))
                :"r"(p)
                :"memory");
        pix += line_size*2;
        p += 16;
    } while (--i);
}

static void put_pixels4_mmx(uint8_t *block, const uint8_t *pixels, int line_size, int h)
{
    __asm__ volatile(
         "lea (%3, %3), %%"REG_a"       \n\t"
         ASMALIGN(3)
         "1:                            \n\t"
         "movd (%1), %%mm0              \n\t"
         "movd (%1, %3), %%mm1          \n\t"
         "movd %%mm0, (%2)              \n\t"
         "movd %%mm1, (%2, %3)          \n\t"
         "add %%"REG_a", %1             \n\t"
         "add %%"REG_a", %2             \n\t"
         "movd (%1), %%mm0              \n\t"
         "movd (%1, %3), %%mm1          \n\t"
         "movd %%mm0, (%2)              \n\t"
         "movd %%mm1, (%2, %3)          \n\t"
         "add %%"REG_a", %1             \n\t"
         "add %%"REG_a", %2             \n\t"
         "subl $4, %0                   \n\t"
         "jnz 1b                        \n\t"
         : "+g"(h), "+r" (pixels),  "+r" (block)
         : "r"((x86_reg)line_size)
         : "%"REG_a, "memory"
        );
}

static void put_pixels8_mmx(uint8_t *block, const uint8_t *pixels, int line_size, int h)
{
    __asm__ volatile(
         "lea (%3, %3), %%"REG_a"       \n\t"
         ASMALIGN(3)
         "1:                            \n\t"
         "movq (%1), %%mm0              \n\t"
         "movq (%1, %3), %%mm1          \n\t"
         "movq %%mm0, (%2)              \n\t"
         "movq %%mm1, (%2, %3)          \n\t"
         "add %%"REG_a", %1             \n\t"
         "add %%"REG_a", %2             \n\t"
         "movq (%1), %%mm0              \n\t"
         "movq (%1, %3), %%mm1          \n\t"
         "movq %%mm0, (%2)              \n\t"
         "movq %%mm1, (%2, %3)          \n\t"
         "add %%"REG_a", %1             \n\t"
         "add %%"REG_a", %2             \n\t"
         "subl $4, %0                   \n\t"
         "jnz 1b                        \n\t"
         : "+g"(h), "+r" (pixels),  "+r" (block)
         : "r"((x86_reg)line_size)
         : "%"REG_a, "memory"
        );
}

static void put_pixels16_mmx(uint8_t *block, const uint8_t *pixels, int line_size, int h)
{
    __asm__ volatile(
         "lea (%3, %3), %%"REG_a"       \n\t"
         ASMALIGN(3)
         "1:                            \n\t"
         "movq (%1), %%mm0              \n\t"
         "movq 8(%1), %%mm4             \n\t"
         "movq (%1, %3), %%mm1          \n\t"
         "movq 8(%1, %3), %%mm5         \n\t"
         "movq %%mm0, (%2)              \n\t"
         "movq %%mm4, 8(%2)             \n\t"
         "movq %%mm1, (%2, %3)          \n\t"
         "movq %%mm5, 8(%2, %3)         \n\t"
         "add %%"REG_a", %1             \n\t"
         "add %%"REG_a", %2             \n\t"
         "movq (%1), %%mm0              \n\t"
         "movq 8(%1), %%mm4             \n\t"
         "movq (%1, %3), %%mm1          \n\t"
         "movq 8(%1, %3), %%mm5         \n\t"
         "movq %%mm0, (%2)              \n\t"
         "movq %%mm4, 8(%2)             \n\t"
         "movq %%mm1, (%2, %3)          \n\t"
         "movq %%mm5, 8(%2, %3)         \n\t"
         "add %%"REG_a", %1             \n\t"
         "add %%"REG_a", %2             \n\t"
         "subl $4, %0                   \n\t"
         "jnz 1b                        \n\t"
         : "+g"(h), "+r" (pixels),  "+r" (block)
         : "r"((x86_reg)line_size)
         : "%"REG_a, "memory"
        );
}

static void put_pixels16_sse2(uint8_t *block, const uint8_t *pixels, int line_size, int h)
{
    __asm__ volatile(
         "1:                            \n\t"
         "movdqu (%1), %%xmm0           \n\t"
         "movdqu (%1,%3), %%xmm1        \n\t"
         "movdqu (%1,%3,2), %%xmm2      \n\t"
         "movdqu (%1,%4), %%xmm3        \n\t"
         "movdqa %%xmm0, (%2)           \n\t"
         "movdqa %%xmm1, (%2,%3)        \n\t"
         "movdqa %%xmm2, (%2,%3,2)      \n\t"
         "movdqa %%xmm3, (%2,%4)        \n\t"
         "subl $4, %0                   \n\t"
         "lea (%1,%3,4), %1             \n\t"
         "lea (%2,%3,4), %2             \n\t"
         "jnz 1b                        \n\t"
         : "+g"(h), "+r" (pixels),  "+r" (block)
         : "r"((x86_reg)line_size), "r"((x86_reg)3L*line_size)
         : "memory"
        );
}

static void avg_pixels16_sse2(uint8_t *block, const uint8_t *pixels, int line_size, int h)
{
    __asm__ volatile(
         "1:                            \n\t"
         "movdqu (%1), %%xmm0           \n\t"
         "movdqu (%1,%3), %%xmm1        \n\t"
         "movdqu (%1,%3,2), %%xmm2      \n\t"
         "movdqu (%1,%4), %%xmm3        \n\t"
         "pavgb  (%2), %%xmm0           \n\t"
         "pavgb  (%2,%3), %%xmm1        \n\t"
         "pavgb  (%2,%3,2), %%xmm2      \n\t"
         "pavgb  (%2,%4), %%xmm3        \n\t"
         "movdqa %%xmm0, (%2)           \n\t"
         "movdqa %%xmm1, (%2,%3)        \n\t"
         "movdqa %%xmm2, (%2,%3,2)      \n\t"
         "movdqa %%xmm3, (%2,%4)        \n\t"
         "subl $4, %0                   \n\t"
         "lea (%1,%3,4), %1             \n\t"
         "lea (%2,%3,4), %2             \n\t"
         "jnz 1b                        \n\t"
         : "+g"(h), "+r" (pixels),  "+r" (block)
         : "r"((x86_reg)line_size), "r"((x86_reg)3L*line_size)
         : "memory"
        );
}

#define CLEAR_BLOCKS(name,n) \
static void name(DCTELEM *blocks)\
{\
    __asm__ volatile(\
                "pxor %%mm7, %%mm7              \n\t"\
                "mov     %1, %%"REG_a"          \n\t"\
                "1:                             \n\t"\
                "movq %%mm7, (%0, %%"REG_a")    \n\t"\
                "movq %%mm7, 8(%0, %%"REG_a")   \n\t"\
                "movq %%mm7, 16(%0, %%"REG_a")  \n\t"\
                "movq %%mm7, 24(%0, %%"REG_a")  \n\t"\
                "add $32, %%"REG_a"             \n\t"\
                " js 1b                         \n\t"\
                : : "r" (((uint8_t *)blocks)+128*n),\
                    "i" (-128*n)\
                : "%"REG_a\
        );\
}
CLEAR_BLOCKS(clear_blocks_mmx, 6)
CLEAR_BLOCKS(clear_block_mmx, 1)

static void clear_block_sse(DCTELEM *block)
{
    __asm__ volatile(
        "xorps  %%xmm0, %%xmm0  \n"
        "movaps %%xmm0,    (%0) \n"
        "movaps %%xmm0,  16(%0) \n"
        "movaps %%xmm0,  32(%0) \n"
        "movaps %%xmm0,  48(%0) \n"
        "movaps %%xmm0,  64(%0) \n"
        "movaps %%xmm0,  80(%0) \n"
        "movaps %%xmm0,  96(%0) \n"
        "movaps %%xmm0, 112(%0) \n"
        :: "r"(block)
        : "memory"
    );
}

static void clear_blocks_sse(DCTELEM *blocks)
{\
    __asm__ volatile(
        "xorps  %%xmm0, %%xmm0  \n"
        "mov     %1, %%"REG_a"  \n"
        "1:                     \n"
        "movaps %%xmm0,    (%0, %%"REG_a") \n"
        "movaps %%xmm0,  16(%0, %%"REG_a") \n"
        "movaps %%xmm0,  32(%0, %%"REG_a") \n"
        "movaps %%xmm0,  48(%0, %%"REG_a") \n"
        "movaps %%xmm0,  64(%0, %%"REG_a") \n"
        "movaps %%xmm0,  80(%0, %%"REG_a") \n"
        "movaps %%xmm0,  96(%0, %%"REG_a") \n"
        "movaps %%xmm0, 112(%0, %%"REG_a") \n"
        "add $128, %%"REG_a"    \n"
        " js 1b                 \n"
        : : "r" (((uint8_t *)blocks)+128*6),
            "i" (-128*6)
        : "%"REG_a
    );
}

static void add_bytes_mmx(uint8_t *dst, uint8_t *src, int w){
    x86_reg i=0;
    __asm__ volatile(
        "jmp 2f                         \n\t"
        "1:                             \n\t"
        "movq  (%1, %0), %%mm0          \n\t"
        "movq  (%2, %0), %%mm1          \n\t"
        "paddb %%mm0, %%mm1             \n\t"
        "movq %%mm1, (%2, %0)           \n\t"
        "movq 8(%1, %0), %%mm0          \n\t"
        "movq 8(%2, %0), %%mm1          \n\t"
        "paddb %%mm0, %%mm1             \n\t"
        "movq %%mm1, 8(%2, %0)          \n\t"
        "add $16, %0                    \n\t"
        "2:                             \n\t"
        "cmp %3, %0                     \n\t"
        " js 1b                         \n\t"
        : "+r" (i)
        : "r"(src), "r"(dst), "r"((x86_reg)w-15)
    );
    for(; i<w; i++)
        dst[i+0] += src[i+0];
}

static void add_bytes_l2_mmx(uint8_t *dst, uint8_t *src1, uint8_t *src2, int w){
    x86_reg i=0;
    __asm__ volatile(
        "jmp 2f                         \n\t"
        "1:                             \n\t"
        "movq   (%2, %0), %%mm0         \n\t"
        "movq  8(%2, %0), %%mm1         \n\t"
        "paddb  (%3, %0), %%mm0         \n\t"
        "paddb 8(%3, %0), %%mm1         \n\t"
        "movq %%mm0,  (%1, %0)          \n\t"
        "movq %%mm1, 8(%1, %0)          \n\t"
        "add $16, %0                    \n\t"
        "2:                             \n\t"
        "cmp %4, %0                     \n\t"
        " js 1b                         \n\t"
        : "+r" (i)
        : "r"(dst), "r"(src1), "r"(src2), "r"((x86_reg)w-15)
    );
    for(; i<w; i++)
        dst[i] = src1[i] + src2[i];
}

#if HAVE_7REGS && HAVE_TEN_OPERANDS
static void add_hfyu_median_prediction_cmov(uint8_t *dst, const uint8_t *top, const uint8_t *diff, int w, int *left, int *left_top) {
    x86_reg w2 = -w;
    x86_reg x;
    int l = *left & 0xff;
    int tl = *left_top & 0xff;
    int t;
    __asm__ volatile(
        "mov    %7, %3 \n"
        "1: \n"
        "movzx (%3,%4), %2 \n"
        "mov    %2, %k3 \n"
        "sub   %b1, %b3 \n"
        "add   %b0, %b3 \n"
        "mov    %2, %1 \n"
        "cmp    %0, %2 \n"
        "cmovg  %0, %2 \n"
        "cmovg  %1, %0 \n"
        "cmp   %k3, %0 \n"
        "cmovg %k3, %0 \n"
        "mov    %7, %3 \n"
        "cmp    %2, %0 \n"
        "cmovl  %2, %0 \n"
        "add (%6,%4), %b0 \n"
        "mov   %b0, (%5,%4) \n"
        "inc    %4 \n"
        "jl 1b \n"
        :"+&q"(l), "+&q"(tl), "=&r"(t), "=&q"(x), "+&r"(w2)
        :"r"(dst+w), "r"(diff+w), "rm"(top+w)
    );
    *left = l;
    *left_top = tl;
}
#endif

#define H263_LOOP_FILTER \
        "pxor %%mm7, %%mm7              \n\t"\
        "movq  %0, %%mm0                \n\t"\
        "movq  %0, %%mm1                \n\t"\
        "movq  %3, %%mm2                \n\t"\
        "movq  %3, %%mm3                \n\t"\
        "punpcklbw %%mm7, %%mm0         \n\t"\
        "punpckhbw %%mm7, %%mm1         \n\t"\
        "punpcklbw %%mm7, %%mm2         \n\t"\
        "punpckhbw %%mm7, %%mm3         \n\t"\
        "psubw %%mm2, %%mm0             \n\t"\
        "psubw %%mm3, %%mm1             \n\t"\
        "movq  %1, %%mm2                \n\t"\
        "movq  %1, %%mm3                \n\t"\
        "movq  %2, %%mm4                \n\t"\
        "movq  %2, %%mm5                \n\t"\
        "punpcklbw %%mm7, %%mm2         \n\t"\
        "punpckhbw %%mm7, %%mm3         \n\t"\
        "punpcklbw %%mm7, %%mm4         \n\t"\
        "punpckhbw %%mm7, %%mm5         \n\t"\
        "psubw %%mm2, %%mm4             \n\t"\
        "psubw %%mm3, %%mm5             \n\t"\
        "psllw $2, %%mm4                \n\t"\
        "psllw $2, %%mm5                \n\t"\
        "paddw %%mm0, %%mm4             \n\t"\
        "paddw %%mm1, %%mm5             \n\t"\
        "pxor %%mm6, %%mm6              \n\t"\
        "pcmpgtw %%mm4, %%mm6           \n\t"\
        "pcmpgtw %%mm5, %%mm7           \n\t"\
        "pxor %%mm6, %%mm4              \n\t"\
        "pxor %%mm7, %%mm5              \n\t"\
        "psubw %%mm6, %%mm4             \n\t"\
        "psubw %%mm7, %%mm5             \n\t"\
        "psrlw $3, %%mm4                \n\t"\
        "psrlw $3, %%mm5                \n\t"\
        "packuswb %%mm5, %%mm4          \n\t"\
        "packsswb %%mm7, %%mm6          \n\t"\
        "pxor %%mm7, %%mm7              \n\t"\
        "movd %4, %%mm2                 \n\t"\
        "punpcklbw %%mm2, %%mm2         \n\t"\
        "punpcklbw %%mm2, %%mm2         \n\t"\
        "punpcklbw %%mm2, %%mm2         \n\t"\
        "psubusb %%mm4, %%mm2           \n\t"\
        "movq %%mm2, %%mm3              \n\t"\
        "psubusb %%mm4, %%mm3           \n\t"\
        "psubb %%mm3, %%mm2             \n\t"\
        "movq %1, %%mm3                 \n\t"\
        "movq %2, %%mm4                 \n\t"\
        "pxor %%mm6, %%mm3              \n\t"\
        "pxor %%mm6, %%mm4              \n\t"\
        "paddusb %%mm2, %%mm3           \n\t"\
        "psubusb %%mm2, %%mm4           \n\t"\
        "pxor %%mm6, %%mm3              \n\t"\
        "pxor %%mm6, %%mm4              \n\t"\
        "paddusb %%mm2, %%mm2           \n\t"\
        "packsswb %%mm1, %%mm0          \n\t"\
        "pcmpgtb %%mm0, %%mm7           \n\t"\
        "pxor %%mm7, %%mm0              \n\t"\
        "psubb %%mm7, %%mm0             \n\t"\
        "movq %%mm0, %%mm1              \n\t"\
        "psubusb %%mm2, %%mm0           \n\t"\
        "psubb %%mm0, %%mm1             \n\t"\
        "pand %5, %%mm1                 \n\t"\
        "psrlw $2, %%mm1                \n\t"\
        "pxor %%mm7, %%mm1              \n\t"\
        "psubb %%mm7, %%mm1             \n\t"\
        "movq %0, %%mm5                 \n\t"\
        "movq %3, %%mm6                 \n\t"\
        "psubb %%mm1, %%mm5             \n\t"\
        "paddb %%mm1, %%mm6             \n\t"

static void h263_v_loop_filter_mmx(uint8_t *src, int stride, int qscale){
    if(CONFIG_H263_DECODER || CONFIG_H263_ENCODER) {
    const int strength= ff_h263_loop_filter_strength[qscale];

    __asm__ volatile(

        H263_LOOP_FILTER

        "movq %%mm3, %1                 \n\t"
        "movq %%mm4, %2                 \n\t"
        "movq %%mm5, %0                 \n\t"
        "movq %%mm6, %3                 \n\t"
        : "+m" (*(uint64_t*)(src - 2*stride)),
          "+m" (*(uint64_t*)(src - 1*stride)),
          "+m" (*(uint64_t*)(src + 0*stride)),
          "+m" (*(uint64_t*)(src + 1*stride))
        : "g" (2*strength), "m"(ff_pb_FC)
    );
    }
}

static inline void transpose4x4(uint8_t *dst, uint8_t *src, int dst_stride, int src_stride){
    __asm__ volatile( //FIXME could save 1 instruction if done as 8x4 ...
        "movd  %4, %%mm0                \n\t"
        "movd  %5, %%mm1                \n\t"
        "movd  %6, %%mm2                \n\t"
        "movd  %7, %%mm3                \n\t"
        "punpcklbw %%mm1, %%mm0         \n\t"
        "punpcklbw %%mm3, %%mm2         \n\t"
        "movq %%mm0, %%mm1              \n\t"
        "punpcklwd %%mm2, %%mm0         \n\t"
        "punpckhwd %%mm2, %%mm1         \n\t"
        "movd  %%mm0, %0                \n\t"
        "punpckhdq %%mm0, %%mm0         \n\t"
        "movd  %%mm0, %1                \n\t"
        "movd  %%mm1, %2                \n\t"
        "punpckhdq %%mm1, %%mm1         \n\t"
        "movd  %%mm1, %3                \n\t"

        : "=m" (*(uint32_t*)(dst + 0*dst_stride)),
          "=m" (*(uint32_t*)(dst + 1*dst_stride)),
          "=m" (*(uint32_t*)(dst + 2*dst_stride)),
          "=m" (*(uint32_t*)(dst + 3*dst_stride))
        :  "m" (*(uint32_t*)(src + 0*src_stride)),
           "m" (*(uint32_t*)(src + 1*src_stride)),
           "m" (*(uint32_t*)(src + 2*src_stride)),
           "m" (*(uint32_t*)(src + 3*src_stride))
    );
}

static void h263_h_loop_filter_mmx(uint8_t *src, int stride, int qscale){
    if(CONFIG_H263_DECODER || CONFIG_H263_ENCODER) {
    const int strength= ff_h263_loop_filter_strength[qscale];
    DECLARE_ALIGNED(8, uint64_t, temp)[4];
    uint8_t *btemp= (uint8_t*)temp;

    src -= 2;

    transpose4x4(btemp  , src           , 8, stride);
    transpose4x4(btemp+4, src + 4*stride, 8, stride);
    __asm__ volatile(
        H263_LOOP_FILTER // 5 3 4 6

        : "+m" (temp[0]),
          "+m" (temp[1]),
          "+m" (temp[2]),
          "+m" (temp[3])
        : "g" (2*strength), "m"(ff_pb_FC)
    );

    __asm__ volatile(
        "movq %%mm5, %%mm1              \n\t"
        "movq %%mm4, %%mm0              \n\t"
        "punpcklbw %%mm3, %%mm5         \n\t"
        "punpcklbw %%mm6, %%mm4         \n\t"
        "punpckhbw %%mm3, %%mm1         \n\t"
        "punpckhbw %%mm6, %%mm0         \n\t"
        "movq %%mm5, %%mm3              \n\t"
        "movq %%mm1, %%mm6              \n\t"
        "punpcklwd %%mm4, %%mm5         \n\t"
        "punpcklwd %%mm0, %%mm1         \n\t"
        "punpckhwd %%mm4, %%mm3         \n\t"
        "punpckhwd %%mm0, %%mm6         \n\t"
        "movd %%mm5, (%0)               \n\t"
        "punpckhdq %%mm5, %%mm5         \n\t"
        "movd %%mm5, (%0,%2)            \n\t"
        "movd %%mm3, (%0,%2,2)          \n\t"
        "punpckhdq %%mm3, %%mm3         \n\t"
        "movd %%mm3, (%0,%3)            \n\t"
        "movd %%mm1, (%1)               \n\t"
        "punpckhdq %%mm1, %%mm1         \n\t"
        "movd %%mm1, (%1,%2)            \n\t"
        "movd %%mm6, (%1,%2,2)          \n\t"
        "punpckhdq %%mm6, %%mm6         \n\t"
        "movd %%mm6, (%1,%3)            \n\t"
        :: "r" (src),
           "r" (src + 4*stride),
           "r" ((x86_reg)   stride ),
           "r" ((x86_reg)(3*stride))
    );
    }
}

/* draw the edges of width 'w' of an image of size width, height
   this mmx version can only handle w==8 || w==16 */
static void draw_edges_mmx(uint8_t *buf, int wrap, int width, int height, int w)
{
    uint8_t *ptr, *last_line;
    int i;

    last_line = buf + (height - 1) * wrap;
    /* left and right */
    ptr = buf;
    if(w==8)
    {
        __asm__ volatile(
                "1:                             \n\t"
                "movd (%0), %%mm0               \n\t"
                "punpcklbw %%mm0, %%mm0         \n\t"
                "punpcklwd %%mm0, %%mm0         \n\t"
                "punpckldq %%mm0, %%mm0         \n\t"
                "movq %%mm0, -8(%0)             \n\t"
                "movq -8(%0, %2), %%mm1         \n\t"
                "punpckhbw %%mm1, %%mm1         \n\t"
                "punpckhwd %%mm1, %%mm1         \n\t"
                "punpckhdq %%mm1, %%mm1         \n\t"
                "movq %%mm1, (%0, %2)           \n\t"
                "add %1, %0                     \n\t"
                "cmp %3, %0                     \n\t"
                " jb 1b                         \n\t"
                : "+r" (ptr)
                : "r" ((x86_reg)wrap), "r" ((x86_reg)width), "r" (ptr + wrap*height)
        );
    }
    else
    {
        __asm__ volatile(
                "1:                             \n\t"
                "movd (%0), %%mm0               \n\t"
                "punpcklbw %%mm0, %%mm0         \n\t"
                "punpcklwd %%mm0, %%mm0         \n\t"
                "punpckldq %%mm0, %%mm0         \n\t"
                "movq %%mm0, -8(%0)             \n\t"
                "movq %%mm0, -16(%0)            \n\t"
                "movq -8(%0, %2), %%mm1         \n\t"
                "punpckhbw %%mm1, %%mm1         \n\t"
                "punpckhwd %%mm1, %%mm1         \n\t"
                "punpckhdq %%mm1, %%mm1         \n\t"
                "movq %%mm1, (%0, %2)           \n\t"
                "movq %%mm1, 8(%0, %2)          \n\t"
                "add %1, %0                     \n\t"
                "cmp %3, %0                     \n\t"
                " jb 1b                         \n\t"
                : "+r" (ptr)
                : "r" ((x86_reg)wrap), "r" ((x86_reg)width), "r" (ptr + wrap*height)
        );
    }

    for(i=0;i<w;i+=4) {
        /* top and bottom (and hopefully also the corners) */
        ptr= buf - (i + 1) * wrap - w;
        __asm__ volatile(
                "1:                             \n\t"
                "movq (%1, %0), %%mm0           \n\t"
                "movq %%mm0, (%0)               \n\t"
                "movq %%mm0, (%0, %2)           \n\t"
                "movq %%mm0, (%0, %2, 2)        \n\t"
                "movq %%mm0, (%0, %3)           \n\t"
                "add $8, %0                     \n\t"
                "cmp %4, %0                     \n\t"
                " jb 1b                         \n\t"
                : "+r" (ptr)
                : "r" ((x86_reg)buf - (x86_reg)ptr - w), "r" ((x86_reg)-wrap), "r" ((x86_reg)-wrap*3), "r" (ptr+width+2*w)
        );
        ptr= last_line + (i + 1) * wrap - w;
        __asm__ volatile(
                "1:                             \n\t"
                "movq (%1, %0), %%mm0           \n\t"
                "movq %%mm0, (%0)               \n\t"
                "movq %%mm0, (%0, %2)           \n\t"
                "movq %%mm0, (%0, %2, 2)        \n\t"
                "movq %%mm0, (%0, %3)           \n\t"
                "add $8, %0                     \n\t"
                "cmp %4, %0                     \n\t"
                " jb 1b                         \n\t"
                : "+r" (ptr)
                : "r" ((x86_reg)last_line - (x86_reg)ptr - w), "r" ((x86_reg)wrap), "r" ((x86_reg)wrap*3), "r" (ptr+width+2*w)
        );
    }
}

#define PAETH(cpu, abs3)\
static void add_png_paeth_prediction_##cpu(uint8_t *dst, uint8_t *src, uint8_t *top, int w, int bpp)\
{\
    x86_reg i = -bpp;\
    x86_reg end = w-3;\
    __asm__ volatile(\
        "pxor      %%mm7, %%mm7 \n"\
        "movd    (%1,%0), %%mm0 \n"\
        "movd    (%2,%0), %%mm1 \n"\
        "punpcklbw %%mm7, %%mm0 \n"\
        "punpcklbw %%mm7, %%mm1 \n"\
        "add       %4, %0 \n"\
        "1: \n"\
        "movq      %%mm1, %%mm2 \n"\
        "movd    (%2,%0), %%mm1 \n"\
        "movq      %%mm2, %%mm3 \n"\
        "punpcklbw %%mm7, %%mm1 \n"\
        "movq      %%mm2, %%mm4 \n"\
        "psubw     %%mm1, %%mm3 \n"\
        "psubw     %%mm0, %%mm4 \n"\
        "movq      %%mm3, %%mm5 \n"\
        "paddw     %%mm4, %%mm5 \n"\
        abs3\
        "movq      %%mm4, %%mm6 \n"\
        "pminsw    %%mm5, %%mm6 \n"\
        "pcmpgtw   %%mm6, %%mm3 \n"\
        "pcmpgtw   %%mm5, %%mm4 \n"\
        "movq      %%mm4, %%mm6 \n"\
        "pand      %%mm3, %%mm4 \n"\
        "pandn     %%mm3, %%mm6 \n"\
        "pandn     %%mm0, %%mm3 \n"\
        "movd    (%3,%0), %%mm0 \n"\
        "pand      %%mm1, %%mm6 \n"\
        "pand      %%mm4, %%mm2 \n"\
        "punpcklbw %%mm7, %%mm0 \n"\
        "movq      %6,    %%mm5 \n"\
        "paddw     %%mm6, %%mm0 \n"\
        "paddw     %%mm2, %%mm3 \n"\
        "paddw     %%mm3, %%mm0 \n"\
        "pand      %%mm5, %%mm0 \n"\
        "movq      %%mm0, %%mm3 \n"\
        "packuswb  %%mm3, %%mm3 \n"\
        "movd      %%mm3, (%1,%0) \n"\
        "add       %4, %0 \n"\
        "cmp       %5, %0 \n"\
        "jle 1b \n"\
        :"+r"(i)\
        :"r"(dst), "r"(top), "r"(src), "r"((x86_reg)bpp), "g"(end),\
         "m"(ff_pw_255)\
        :"memory"\
    );\
}

#define ABS3_MMX2\
        "psubw     %%mm5, %%mm7 \n"\
        "pmaxsw    %%mm7, %%mm5 \n"\
        "pxor      %%mm6, %%mm6 \n"\
        "pxor      %%mm7, %%mm7 \n"\
        "psubw     %%mm3, %%mm6 \n"\
        "psubw     %%mm4, %%mm7 \n"\
        "pmaxsw    %%mm6, %%mm3 \n"\
        "pmaxsw    %%mm7, %%mm4 \n"\
        "pxor      %%mm7, %%mm7 \n"

#define ABS3_SSSE3\
        "pabsw     %%mm3, %%mm3 \n"\
        "pabsw     %%mm4, %%mm4 \n"\
        "pabsw     %%mm5, %%mm5 \n"

PAETH(mmx2, ABS3_MMX2)
#if HAVE_SSSE3
PAETH(ssse3, ABS3_SSSE3)
#endif

#define QPEL_V_LOW(m3,m4,m5,m6, pw_20, pw_3, rnd, in0, in1, in2, in7, out, OP)\
        "paddw " #m4 ", " #m3 "           \n\t" /* x1 */\
        "movq "MANGLE(ff_pw_20)", %%mm4   \n\t" /* 20 */\
        "pmullw " #m3 ", %%mm4            \n\t" /* 20x1 */\
        "movq "#in7", " #m3 "             \n\t" /* d */\
        "movq "#in0", %%mm5               \n\t" /* D */\
        "paddw " #m3 ", %%mm5             \n\t" /* x4 */\
        "psubw %%mm5, %%mm4               \n\t" /* 20x1 - x4 */\
        "movq "#in1", %%mm5               \n\t" /* C */\
        "movq "#in2", %%mm6               \n\t" /* B */\
        "paddw " #m6 ", %%mm5             \n\t" /* x3 */\
        "paddw " #m5 ", %%mm6             \n\t" /* x2 */\
        "paddw %%mm6, %%mm6               \n\t" /* 2x2 */\
        "psubw %%mm6, %%mm5               \n\t" /* -2x2 + x3 */\
        "pmullw "MANGLE(ff_pw_3)", %%mm5  \n\t" /* -6x2 + 3x3 */\
        "paddw " #rnd ", %%mm4            \n\t" /* x2 */\
        "paddw %%mm4, %%mm5               \n\t" /* 20x1 - 6x2 + 3x3 - x4 */\
        "psraw $5, %%mm5                  \n\t"\
        "packuswb %%mm5, %%mm5            \n\t"\
        OP(%%mm5, out, %%mm7, d)

#define QPEL_BASE(OPNAME, ROUNDER, RND, OP_MMX2, OP_3DNOW)\
static void OPNAME ## mpeg4_qpel16_h_lowpass_mmx2(uint8_t *dst, uint8_t *src, int dstStride, int srcStride, int h){\
    uint64_t temp;\
\
    __asm__ volatile(\
        "pxor %%mm7, %%mm7                \n\t"\
        "1:                               \n\t"\
        "movq  (%0), %%mm0                \n\t" /* ABCDEFGH */\
        "movq %%mm0, %%mm1                \n\t" /* ABCDEFGH */\
        "movq %%mm0, %%mm2                \n\t" /* ABCDEFGH */\
        "punpcklbw %%mm7, %%mm0           \n\t" /* 0A0B0C0D */\
        "punpckhbw %%mm7, %%mm1           \n\t" /* 0E0F0G0H */\
        "pshufw $0x90, %%mm0, %%mm5       \n\t" /* 0A0A0B0C */\
        "pshufw $0x41, %%mm0, %%mm6       \n\t" /* 0B0A0A0B */\
        "movq %%mm2, %%mm3                \n\t" /* ABCDEFGH */\
        "movq %%mm2, %%mm4                \n\t" /* ABCDEFGH */\
        "psllq $8, %%mm2                  \n\t" /* 0ABCDEFG */\
        "psllq $16, %%mm3                 \n\t" /* 00ABCDEF */\
        "psllq $24, %%mm4                 \n\t" /* 000ABCDE */\
        "punpckhbw %%mm7, %%mm2           \n\t" /* 0D0E0F0G */\
        "punpckhbw %%mm7, %%mm3           \n\t" /* 0C0D0E0F */\
        "punpckhbw %%mm7, %%mm4           \n\t" /* 0B0C0D0E */\
        "paddw %%mm3, %%mm5               \n\t" /* b */\
        "paddw %%mm2, %%mm6               \n\t" /* c */\
        "paddw %%mm5, %%mm5               \n\t" /* 2b */\
        "psubw %%mm5, %%mm6               \n\t" /* c - 2b */\
        "pshufw $0x06, %%mm0, %%mm5       \n\t" /* 0C0B0A0A */\
        "pmullw "MANGLE(ff_pw_3)", %%mm6  \n\t" /* 3c - 6b */\
        "paddw %%mm4, %%mm0               \n\t" /* a */\
        "paddw %%mm1, %%mm5               \n\t" /* d */\
        "pmullw "MANGLE(ff_pw_20)", %%mm0 \n\t" /* 20a */\
        "psubw %%mm5, %%mm0               \n\t" /* 20a - d */\
        "paddw %6, %%mm6                  \n\t"\
        "paddw %%mm6, %%mm0               \n\t" /* 20a - 6b + 3c - d */\
        "psraw $5, %%mm0                  \n\t"\
        "movq %%mm0, %5                   \n\t"\
        /* mm1=EFGH, mm2=DEFG, mm3=CDEF, mm4=BCDE, mm7=0 */\
        \
        "movq 5(%0), %%mm0                \n\t" /* FGHIJKLM */\
        "movq %%mm0, %%mm5                \n\t" /* FGHIJKLM */\
        "movq %%mm0, %%mm6                \n\t" /* FGHIJKLM */\
        "psrlq $8, %%mm0                  \n\t" /* GHIJKLM0 */\
        "psrlq $16, %%mm5                 \n\t" /* HIJKLM00 */\
        "punpcklbw %%mm7, %%mm0           \n\t" /* 0G0H0I0J */\
        "punpcklbw %%mm7, %%mm5           \n\t" /* 0H0I0J0K */\
        "paddw %%mm0, %%mm2               \n\t" /* b */\
        "paddw %%mm5, %%mm3               \n\t" /* c */\
        "paddw %%mm2, %%mm2               \n\t" /* 2b */\
        "psubw %%mm2, %%mm3               \n\t" /* c - 2b */\
        "movq %%mm6, %%mm2                \n\t" /* FGHIJKLM */\
        "psrlq $24, %%mm6                 \n\t" /* IJKLM000 */\
        "punpcklbw %%mm7, %%mm2           \n\t" /* 0F0G0H0I */\
        "punpcklbw %%mm7, %%mm6           \n\t" /* 0I0J0K0L */\
        "pmullw "MANGLE(ff_pw_3)", %%mm3  \n\t" /* 3c - 6b */\
        "paddw %%mm2, %%mm1               \n\t" /* a */\
        "paddw %%mm6, %%mm4               \n\t" /* d */\
        "pmullw "MANGLE(ff_pw_20)", %%mm1 \n\t" /* 20a */\
        "psubw %%mm4, %%mm3               \n\t" /* - 6b +3c - d */\
        "paddw %6, %%mm1                  \n\t"\
        "paddw %%mm1, %%mm3               \n\t" /* 20a - 6b +3c - d */\
        "psraw $5, %%mm3                  \n\t"\
        "movq %5, %%mm1                   \n\t"\
        "packuswb %%mm3, %%mm1            \n\t"\
        OP_MMX2(%%mm1, (%1),%%mm4, q)\
        /* mm0= GHIJ, mm2=FGHI, mm5=HIJK, mm6=IJKL, mm7=0 */\
        \
        "movq 9(%0), %%mm1                \n\t" /* JKLMNOPQ */\
        "movq %%mm1, %%mm4                \n\t" /* JKLMNOPQ */\
        "movq %%mm1, %%mm3                \n\t" /* JKLMNOPQ */\
        "psrlq $8, %%mm1                  \n\t" /* KLMNOPQ0 */\
        "psrlq $16, %%mm4                 \n\t" /* LMNOPQ00 */\
        "punpcklbw %%mm7, %%mm1           \n\t" /* 0K0L0M0N */\
        "punpcklbw %%mm7, %%mm4           \n\t" /* 0L0M0N0O */\
        "paddw %%mm1, %%mm5               \n\t" /* b */\
        "paddw %%mm4, %%mm0               \n\t" /* c */\
        "paddw %%mm5, %%mm5               \n\t" /* 2b */\
        "psubw %%mm5, %%mm0               \n\t" /* c - 2b */\
        "movq %%mm3, %%mm5                \n\t" /* JKLMNOPQ */\
        "psrlq $24, %%mm3                 \n\t" /* MNOPQ000 */\
        "pmullw "MANGLE(ff_pw_3)", %%mm0  \n\t" /* 3c - 6b */\
        "punpcklbw %%mm7, %%mm3           \n\t" /* 0M0N0O0P */\
        "paddw %%mm3, %%mm2               \n\t" /* d */\
        "psubw %%mm2, %%mm0               \n\t" /* -6b + 3c - d */\
        "movq %%mm5, %%mm2                \n\t" /* JKLMNOPQ */\
        "punpcklbw %%mm7, %%mm2           \n\t" /* 0J0K0L0M */\
        "punpckhbw %%mm7, %%mm5           \n\t" /* 0N0O0P0Q */\
        "paddw %%mm2, %%mm6               \n\t" /* a */\
        "pmullw "MANGLE(ff_pw_20)", %%mm6 \n\t" /* 20a */\
        "paddw %6, %%mm0                  \n\t"\
        "paddw %%mm6, %%mm0               \n\t" /* 20a - 6b + 3c - d */\
        "psraw $5, %%mm0                  \n\t"\
        /* mm1=KLMN, mm2=JKLM, mm3=MNOP, mm4=LMNO, mm5=NOPQ mm7=0 */\
        \
        "paddw %%mm5, %%mm3               \n\t" /* a */\
        "pshufw $0xF9, %%mm5, %%mm6       \n\t" /* 0O0P0Q0Q */\
        "paddw %%mm4, %%mm6               \n\t" /* b */\
        "pshufw $0xBE, %%mm5, %%mm4       \n\t" /* 0P0Q0Q0P */\
        "pshufw $0x6F, %%mm5, %%mm5       \n\t" /* 0Q0Q0P0O */\
        "paddw %%mm1, %%mm4               \n\t" /* c */\
        "paddw %%mm2, %%mm5               \n\t" /* d */\
        "paddw %%mm6, %%mm6               \n\t" /* 2b */\
        "psubw %%mm6, %%mm4               \n\t" /* c - 2b */\
        "pmullw "MANGLE(ff_pw_20)", %%mm3 \n\t" /* 20a */\
        "pmullw "MANGLE(ff_pw_3)", %%mm4  \n\t" /* 3c - 6b */\
        "psubw %%mm5, %%mm3               \n\t" /* -6b + 3c - d */\
        "paddw %6, %%mm4                  \n\t"\
        "paddw %%mm3, %%mm4               \n\t" /* 20a - 6b + 3c - d */\
        "psraw $5, %%mm4                  \n\t"\
        "packuswb %%mm4, %%mm0            \n\t"\
        OP_MMX2(%%mm0, 8(%1), %%mm4, q)\
        \
        "add %3, %0                       \n\t"\
        "add %4, %1                       \n\t"\
        "decl %2                          \n\t"\
        " jnz 1b                          \n\t"\
        : "+a"(src), "+c"(dst), "+D"(h)\
        : "d"((x86_reg)srcStride), "S"((x86_reg)dstStride), /*"m"(ff_pw_20), "m"(ff_pw_3),*/ "m"(temp), "m"(ROUNDER)\
        : "memory"\
    );\
}\
\
static void OPNAME ## mpeg4_qpel16_h_lowpass_3dnow(uint8_t *dst, uint8_t *src, int dstStride, int srcStride, int h){\
    int i;\
    int16_t temp[16];\
    /* quick HACK, XXX FIXME MUST be optimized */\
    for(i=0; i<h; i++)\
    {\
        temp[ 0]= (src[ 0]+src[ 1])*20 - (src[ 0]+src[ 2])*6 + (src[ 1]+src[ 3])*3 - (src[ 2]+src[ 4]);\
        temp[ 1]= (src[ 1]+src[ 2])*20 - (src[ 0]+src[ 3])*6 + (src[ 0]+src[ 4])*3 - (src[ 1]+src[ 5]);\
        temp[ 2]= (src[ 2]+src[ 3])*20 - (src[ 1]+src[ 4])*6 + (src[ 0]+src[ 5])*3 - (src[ 0]+src[ 6]);\
        temp[ 3]= (src[ 3]+src[ 4])*20 - (src[ 2]+src[ 5])*6 + (src[ 1]+src[ 6])*3 - (src[ 0]+src[ 7]);\
        temp[ 4]= (src[ 4]+src[ 5])*20 - (src[ 3]+src[ 6])*6 + (src[ 2]+src[ 7])*3 - (src[ 1]+src[ 8]);\
        temp[ 5]= (src[ 5]+src[ 6])*20 - (src[ 4]+src[ 7])*6 + (src[ 3]+src[ 8])*3 - (src[ 2]+src[ 9]);\
        temp[ 6]= (src[ 6]+src[ 7])*20 - (src[ 5]+src[ 8])*6 + (src[ 4]+src[ 9])*3 - (src[ 3]+src[10]);\
        temp[ 7]= (src[ 7]+src[ 8])*20 - (src[ 6]+src[ 9])*6 + (src[ 5]+src[10])*3 - (src[ 4]+src[11]);\
        temp[ 8]= (src[ 8]+src[ 9])*20 - (src[ 7]+src[10])*6 + (src[ 6]+src[11])*3 - (src[ 5]+src[12]);\
        temp[ 9]= (src[ 9]+src[10])*20 - (src[ 8]+src[11])*6 + (src[ 7]+src[12])*3 - (src[ 6]+src[13]);\
        temp[10]= (src[10]+src[11])*20 - (src[ 9]+src[12])*6 + (src[ 8]+src[13])*3 - (src[ 7]+src[14]);\
        temp[11]= (src[11]+src[12])*20 - (src[10]+src[13])*6 + (src[ 9]+src[14])*3 - (src[ 8]+src[15]);\
        temp[12]= (src[12]+src[13])*20 - (src[11]+src[14])*6 + (src[10]+src[15])*3 - (src[ 9]+src[16]);\
        temp[13]= (src[13]+src[14])*20 - (src[12]+src[15])*6 + (src[11]+src[16])*3 - (src[10]+src[16]);\
        temp[14]= (src[14]+src[15])*20 - (src[13]+src[16])*6 + (src[12]+src[16])*3 - (src[11]+src[15]);\
        temp[15]= (src[15]+src[16])*20 - (src[14]+src[16])*6 + (src[13]+src[15])*3 - (src[12]+src[14]);\
        __asm__ volatile(\
            "movq (%0), %%mm0               \n\t"\
            "movq 8(%0), %%mm1              \n\t"\
            "paddw %2, %%mm0                \n\t"\
            "paddw %2, %%mm1                \n\t"\
            "psraw $5, %%mm0                \n\t"\
            "psraw $5, %%mm1                \n\t"\
            "packuswb %%mm1, %%mm0          \n\t"\
            OP_3DNOW(%%mm0, (%1), %%mm1, q)\
            "movq 16(%0), %%mm0             \n\t"\
            "movq 24(%0), %%mm1             \n\t"\
            "paddw %2, %%mm0                \n\t"\
            "paddw %2, %%mm1                \n\t"\
            "psraw $5, %%mm0                \n\t"\
            "psraw $5, %%mm1                \n\t"\
            "packuswb %%mm1, %%mm0          \n\t"\
            OP_3DNOW(%%mm0, 8(%1), %%mm1, q)\
            :: "r"(temp), "r"(dst), "m"(ROUNDER)\
            : "memory"\
        );\
        dst+=dstStride;\
        src+=srcStride;\
    }\
}\
\
static void OPNAME ## mpeg4_qpel8_h_lowpass_mmx2(uint8_t *dst, uint8_t *src, int dstStride, int srcStride, int h){\
    __asm__ volatile(\
        "pxor %%mm7, %%mm7                \n\t"\
        "1:                               \n\t"\
        "movq  (%0), %%mm0                \n\t" /* ABCDEFGH */\
        "movq %%mm0, %%mm1                \n\t" /* ABCDEFGH */\
        "movq %%mm0, %%mm2                \n\t" /* ABCDEFGH */\
        "punpcklbw %%mm7, %%mm0           \n\t" /* 0A0B0C0D */\
        "punpckhbw %%mm7, %%mm1           \n\t" /* 0E0F0G0H */\
        "pshufw $0x90, %%mm0, %%mm5       \n\t" /* 0A0A0B0C */\
        "pshufw $0x41, %%mm0, %%mm6       \n\t" /* 0B0A0A0B */\
        "movq %%mm2, %%mm3                \n\t" /* ABCDEFGH */\
        "movq %%mm2, %%mm4                \n\t" /* ABCDEFGH */\
        "psllq $8, %%mm2                  \n\t" /* 0ABCDEFG */\
        "psllq $16, %%mm3                 \n\t" /* 00ABCDEF */\
        "psllq $24, %%mm4                 \n\t" /* 000ABCDE */\
        "punpckhbw %%mm7, %%mm2           \n\t" /* 0D0E0F0G */\
        "punpckhbw %%mm7, %%mm3           \n\t" /* 0C0D0E0F */\
        "punpckhbw %%mm7, %%mm4           \n\t" /* 0B0C0D0E */\
        "paddw %%mm3, %%mm5               \n\t" /* b */\
        "paddw %%mm2, %%mm6               \n\t" /* c */\
        "paddw %%mm5, %%mm5               \n\t" /* 2b */\
        "psubw %%mm5, %%mm6               \n\t" /* c - 2b */\
        "pshufw $0x06, %%mm0, %%mm5       \n\t" /* 0C0B0A0A */\
        "pmullw "MANGLE(ff_pw_3)", %%mm6  \n\t" /* 3c - 6b */\
        "paddw %%mm4, %%mm0               \n\t" /* a */\
        "paddw %%mm1, %%mm5               \n\t" /* d */\
        "pmullw "MANGLE(ff_pw_20)", %%mm0 \n\t" /* 20a */\
        "psubw %%mm5, %%mm0               \n\t" /* 20a - d */\
        "paddw %5, %%mm6                  \n\t"\
        "paddw %%mm6, %%mm0               \n\t" /* 20a - 6b + 3c - d */\
        "psraw $5, %%mm0                  \n\t"\
        /* mm1=EFGH, mm2=DEFG, mm3=CDEF, mm4=BCDE, mm7=0 */\
        \
        "movd 5(%0), %%mm5                \n\t" /* FGHI */\
        "punpcklbw %%mm7, %%mm5           \n\t" /* 0F0G0H0I */\
        "pshufw $0xF9, %%mm5, %%mm6       \n\t" /* 0G0H0I0I */\
        "paddw %%mm5, %%mm1               \n\t" /* a */\
        "paddw %%mm6, %%mm2               \n\t" /* b */\
        "pshufw $0xBE, %%mm5, %%mm6       \n\t" /* 0H0I0I0H */\
        "pshufw $0x6F, %%mm5, %%mm5       \n\t" /* 0I0I0H0G */\
        "paddw %%mm6, %%mm3               \n\t" /* c */\
        "paddw %%mm5, %%mm4               \n\t" /* d */\
        "paddw %%mm2, %%mm2               \n\t" /* 2b */\
        "psubw %%mm2, %%mm3               \n\t" /* c - 2b */\
        "pmullw "MANGLE(ff_pw_20)", %%mm1 \n\t" /* 20a */\
        "pmullw "MANGLE(ff_pw_3)", %%mm3  \n\t" /* 3c - 6b */\
        "psubw %%mm4, %%mm3               \n\t" /* -6b + 3c - d */\
        "paddw %5, %%mm1                  \n\t"\
        "paddw %%mm1, %%mm3               \n\t" /* 20a - 6b + 3c - d */\
        "psraw $5, %%mm3                  \n\t"\
        "packuswb %%mm3, %%mm0            \n\t"\
        OP_MMX2(%%mm0, (%1), %%mm4, q)\
        \
        "add %3, %0                       \n\t"\
        "add %4, %1                       \n\t"\
        "decl %2                          \n\t"\
        " jnz 1b                          \n\t"\
        : "+a"(src), "+c"(dst), "+d"(h)\
        : "S"((x86_reg)srcStride), "D"((x86_reg)dstStride), /*"m"(ff_pw_20), "m"(ff_pw_3),*/ "m"(ROUNDER)\
        : "memory"\
    );\
}\
\
static void OPNAME ## mpeg4_qpel8_h_lowpass_3dnow(uint8_t *dst, uint8_t *src, int dstStride, int srcStride, int h){\
    int i;\
    int16_t temp[8];\
    /* quick HACK, XXX FIXME MUST be optimized */\
    for(i=0; i<h; i++)\
    {\
        temp[ 0]= (src[ 0]+src[ 1])*20 - (src[ 0]+src[ 2])*6 + (src[ 1]+src[ 3])*3 - (src[ 2]+src[ 4]);\
        temp[ 1]= (src[ 1]+src[ 2])*20 - (src[ 0]+src[ 3])*6 + (src[ 0]+src[ 4])*3 - (src[ 1]+src[ 5]);\
        temp[ 2]= (src[ 2]+src[ 3])*20 - (src[ 1]+src[ 4])*6 + (src[ 0]+src[ 5])*3 - (src[ 0]+src[ 6]);\
        temp[ 3]= (src[ 3]+src[ 4])*20 - (src[ 2]+src[ 5])*6 + (src[ 1]+src[ 6])*3 - (src[ 0]+src[ 7]);\
        temp[ 4]= (src[ 4]+src[ 5])*20 - (src[ 3]+src[ 6])*6 + (src[ 2]+src[ 7])*3 - (src[ 1]+src[ 8]);\
        temp[ 5]= (src[ 5]+src[ 6])*20 - (src[ 4]+src[ 7])*6 + (src[ 3]+src[ 8])*3 - (src[ 2]+src[ 8]);\
        temp[ 6]= (src[ 6]+src[ 7])*20 - (src[ 5]+src[ 8])*6 + (src[ 4]+src[ 8])*3 - (src[ 3]+src[ 7]);\
        temp[ 7]= (src[ 7]+src[ 8])*20 - (src[ 6]+src[ 8])*6 + (src[ 5]+src[ 7])*3 - (src[ 4]+src[ 6]);\
        __asm__ volatile(\
            "movq (%0), %%mm0           \n\t"\
            "movq 8(%0), %%mm1          \n\t"\
            "paddw %2, %%mm0            \n\t"\
            "paddw %2, %%mm1            \n\t"\
            "psraw $5, %%mm0            \n\t"\
            "psraw $5, %%mm1            \n\t"\
            "packuswb %%mm1, %%mm0      \n\t"\
            OP_3DNOW(%%mm0, (%1), %%mm1, q)\
            :: "r"(temp), "r"(dst), "m"(ROUNDER)\
            :"memory"\
        );\
        dst+=dstStride;\
        src+=srcStride;\
    }\
}

#define QPEL_OP(OPNAME, ROUNDER, RND, OP, MMX)\
\
static void OPNAME ## mpeg4_qpel16_v_lowpass_ ## MMX(uint8_t *dst, uint8_t *src, int dstStride, int srcStride){\
    uint64_t temp[17*4];\
    uint64_t *temp_ptr= temp;\
    int count= 17;\
\
    /*FIXME unroll */\
    __asm__ volatile(\
        "pxor %%mm7, %%mm7              \n\t"\
        "1:                             \n\t"\
        "movq (%0), %%mm0               \n\t"\
        "movq (%0), %%mm1               \n\t"\
        "movq 8(%0), %%mm2              \n\t"\
        "movq 8(%0), %%mm3              \n\t"\
        "punpcklbw %%mm7, %%mm0         \n\t"\
        "punpckhbw %%mm7, %%mm1         \n\t"\
        "punpcklbw %%mm7, %%mm2         \n\t"\
        "punpckhbw %%mm7, %%mm3         \n\t"\
        "movq %%mm0, (%1)               \n\t"\
        "movq %%mm1, 17*8(%1)           \n\t"\
        "movq %%mm2, 2*17*8(%1)         \n\t"\
        "movq %%mm3, 3*17*8(%1)         \n\t"\
        "add $8, %1                     \n\t"\
        "add %3, %0                     \n\t"\
        "decl %2                        \n\t"\
        " jnz 1b                        \n\t"\
        : "+r" (src), "+r" (temp_ptr), "+r"(count)\
        : "r" ((x86_reg)srcStride)\
        : "memory"\
    );\
    \
    temp_ptr= temp;\
    count=4;\
    \
/*FIXME reorder for speed */\
    __asm__ volatile(\
        /*"pxor %%mm7, %%mm7              \n\t"*/\
        "1:                             \n\t"\
        "movq (%0), %%mm0               \n\t"\
        "movq 8(%0), %%mm1              \n\t"\
        "movq 16(%0), %%mm2             \n\t"\
        "movq 24(%0), %%mm3             \n\t"\
        QPEL_V_LOW(%%mm0, %%mm1, %%mm2, %%mm3, %5, %6, %5, 16(%0),  8(%0),   (%0), 32(%0), (%1), OP)\
        QPEL_V_LOW(%%mm1, %%mm2, %%mm3, %%mm0, %5, %6, %5,  8(%0),   (%0),   (%0), 40(%0), (%1, %3), OP)\
        "add %4, %1                     \n\t"\
        QPEL_V_LOW(%%mm2, %%mm3, %%mm0, %%mm1, %5, %6, %5,   (%0),   (%0),  8(%0), 48(%0), (%1), OP)\
        \
        QPEL_V_LOW(%%mm3, %%mm0, %%mm1, %%mm2, %5, %6, %5,   (%0),  8(%0), 16(%0), 56(%0), (%1, %3), OP)\
        "add %4, %1                     \n\t"\
        QPEL_V_LOW(%%mm0, %%mm1, %%mm2, %%mm3, %5, %6, %5,  8(%0), 16(%0), 24(%0), 64(%0), (%1), OP)\
        QPEL_V_LOW(%%mm1, %%mm2, %%mm3, %%mm0, %5, %6, %5, 16(%0), 24(%0), 32(%0), 72(%0), (%1, %3), OP)\
        "add %4, %1                     \n\t"\
        QPEL_V_LOW(%%mm2, %%mm3, %%mm0, %%mm1, %5, %6, %5, 24(%0), 32(%0), 40(%0), 80(%0), (%1), OP)\
        QPEL_V_LOW(%%mm3, %%mm0, %%mm1, %%mm2, %5, %6, %5, 32(%0), 40(%0), 48(%0), 88(%0), (%1, %3), OP)\
        "add %4, %1                     \n\t"\
        QPEL_V_LOW(%%mm0, %%mm1, %%mm2, %%mm3, %5, %6, %5, 40(%0), 48(%0), 56(%0), 96(%0), (%1), OP)\
        QPEL_V_LOW(%%mm1, %%mm2, %%mm3, %%mm0, %5, %6, %5, 48(%0), 56(%0), 64(%0),104(%0), (%1, %3), OP)\
        "add %4, %1                     \n\t"\
        QPEL_V_LOW(%%mm2, %%mm3, %%mm0, %%mm1, %5, %6, %5, 56(%0), 64(%0), 72(%0),112(%0), (%1), OP)\
        QPEL_V_LOW(%%mm3, %%mm0, %%mm1, %%mm2, %5, %6, %5, 64(%0), 72(%0), 80(%0),120(%0), (%1, %3), OP)\
        "add %4, %1                     \n\t"\
        QPEL_V_LOW(%%mm0, %%mm1, %%mm2, %%mm3, %5, %6, %5, 72(%0), 80(%0), 88(%0),128(%0), (%1), OP)\
        \
        QPEL_V_LOW(%%mm1, %%mm2, %%mm3, %%mm0, %5, %6, %5, 80(%0), 88(%0), 96(%0),128(%0), (%1, %3), OP)\
        "add %4, %1                     \n\t"  \
        QPEL_V_LOW(%%mm2, %%mm3, %%mm0, %%mm1, %5, %6, %5, 88(%0), 96(%0),104(%0),120(%0), (%1), OP)\
        QPEL_V_LOW(%%mm3, %%mm0, %%mm1, %%mm2, %5, %6, %5, 96(%0),104(%0),112(%0),112(%0), (%1, %3), OP)\
        \
        "add $136, %0                   \n\t"\
        "add %6, %1                     \n\t"\
        "decl %2                        \n\t"\
        " jnz 1b                        \n\t"\
        \
        : "+r"(temp_ptr), "+r"(dst), "+g"(count)\
        : "r"((x86_reg)dstStride), "r"(2*(x86_reg)dstStride), /*"m"(ff_pw_20), "m"(ff_pw_3),*/ "m"(ROUNDER), "g"(4-14*(x86_reg)dstStride)\
        :"memory"\
    );\
}\
\
static void OPNAME ## mpeg4_qpel8_v_lowpass_ ## MMX(uint8_t *dst, uint8_t *src, int dstStride, int srcStride){\
    uint64_t temp[9*2];\
    uint64_t *temp_ptr= temp;\
    int count= 9;\
\
    /*FIXME unroll */\
    __asm__ volatile(\
        "pxor %%mm7, %%mm7              \n\t"\
        "1:                             \n\t"\
        "movq (%0), %%mm0               \n\t"\
        "movq (%0), %%mm1               \n\t"\
        "punpcklbw %%mm7, %%mm0         \n\t"\
        "punpckhbw %%mm7, %%mm1         \n\t"\
        "movq %%mm0, (%1)               \n\t"\
        "movq %%mm1, 9*8(%1)            \n\t"\
        "add $8, %1                     \n\t"\
        "add %3, %0                     \n\t"\
        "decl %2                        \n\t"\
        " jnz 1b                        \n\t"\
        : "+r" (src), "+r" (temp_ptr), "+r"(count)\
        : "r" ((x86_reg)srcStride)\
        : "memory"\
    );\
    \
    temp_ptr= temp;\
    count=2;\
    \
/*FIXME reorder for speed */\
    __asm__ volatile(\
        /*"pxor %%mm7, %%mm7              \n\t"*/\
        "1:                             \n\t"\
        "movq (%0), %%mm0               \n\t"\
        "movq 8(%0), %%mm1              \n\t"\
        "movq 16(%0), %%mm2             \n\t"\
        "movq 24(%0), %%mm3             \n\t"\
        QPEL_V_LOW(%%mm0, %%mm1, %%mm2, %%mm3, %5, %6, %5, 16(%0),  8(%0),   (%0), 32(%0), (%1), OP)\
        QPEL_V_LOW(%%mm1, %%mm2, %%mm3, %%mm0, %5, %6, %5,  8(%0),   (%0),   (%0), 40(%0), (%1, %3), OP)\
        "add %4, %1                     \n\t"\
        QPEL_V_LOW(%%mm2, %%mm3, %%mm0, %%mm1, %5, %6, %5,   (%0),   (%0),  8(%0), 48(%0), (%1), OP)\
        \
        QPEL_V_LOW(%%mm3, %%mm0, %%mm1, %%mm2, %5, %6, %5,   (%0),  8(%0), 16(%0), 56(%0), (%1, %3), OP)\
        "add %4, %1                     \n\t"\
        QPEL_V_LOW(%%mm0, %%mm1, %%mm2, %%mm3, %5, %6, %5,  8(%0), 16(%0), 24(%0), 64(%0), (%1), OP)\
        \
        QPEL_V_LOW(%%mm1, %%mm2, %%mm3, %%mm0, %5, %6, %5, 16(%0), 24(%0), 32(%0), 64(%0), (%1, %3), OP)\
        "add %4, %1                     \n\t"\
        QPEL_V_LOW(%%mm2, %%mm3, %%mm0, %%mm1, %5, %6, %5, 24(%0), 32(%0), 40(%0), 56(%0), (%1), OP)\
        QPEL_V_LOW(%%mm3, %%mm0, %%mm1, %%mm2, %5, %6, %5, 32(%0), 40(%0), 48(%0), 48(%0), (%1, %3), OP)\
                \
        "add $72, %0                    \n\t"\
        "add %6, %1                     \n\t"\
        "decl %2                        \n\t"\
        " jnz 1b                        \n\t"\
         \
        : "+r"(temp_ptr), "+r"(dst), "+g"(count)\
        : "r"((x86_reg)dstStride), "r"(2*(x86_reg)dstStride), /*"m"(ff_pw_20), "m"(ff_pw_3),*/ "m"(ROUNDER), "g"(4-6*(x86_reg)dstStride)\
        : "memory"\
   );\
}\
\
static void OPNAME ## qpel8_mc00_ ## MMX (uint8_t *dst, uint8_t *src, int stride){\
    OPNAME ## pixels8_ ## MMX(dst, src, stride, 8);\
}\
\
static void OPNAME ## qpel8_mc10_ ## MMX(uint8_t *dst, uint8_t *src, int stride){\
    uint64_t temp[8];\
    uint8_t * const half= (uint8_t*)temp;\
    put ## RND ## mpeg4_qpel8_h_lowpass_ ## MMX(half, src, 8, stride, 8);\
    OPNAME ## pixels8_l2_ ## MMX(dst, src, half, stride, stride, 8);\
}\
\
static void OPNAME ## qpel8_mc20_ ## MMX(uint8_t *dst, uint8_t *src, int stride){\
    OPNAME ## mpeg4_qpel8_h_lowpass_ ## MMX(dst, src, stride, stride, 8);\
}\
\
static void OPNAME ## qpel8_mc30_ ## MMX(uint8_t *dst, uint8_t *src, int stride){\
    uint64_t temp[8];\
    uint8_t * const half= (uint8_t*)temp;\
    put ## RND ## mpeg4_qpel8_h_lowpass_ ## MMX(half, src, 8, stride, 8);\
    OPNAME ## pixels8_l2_ ## MMX(dst, src+1, half, stride, stride, 8);\
}\
\
static void OPNAME ## qpel8_mc01_ ## MMX(uint8_t *dst, uint8_t *src, int stride){\
    uint64_t temp[8];\
    uint8_t * const half= (uint8_t*)temp;\
    put ## RND ## mpeg4_qpel8_v_lowpass_ ## MMX(half, src, 8, stride);\
    OPNAME ## pixels8_l2_ ## MMX(dst, src, half, stride, stride, 8);\
}\
\
static void OPNAME ## qpel8_mc02_ ## MMX(uint8_t *dst, uint8_t *src, int stride){\
    OPNAME ## mpeg4_qpel8_v_lowpass_ ## MMX(dst, src, stride, stride);\
}\
\
static void OPNAME ## qpel8_mc03_ ## MMX(uint8_t *dst, uint8_t *src, int stride){\
    uint64_t temp[8];\
    uint8_t * const half= (uint8_t*)temp;\
    put ## RND ## mpeg4_qpel8_v_lowpass_ ## MMX(half, src, 8, stride);\
    OPNAME ## pixels8_l2_ ## MMX(dst, src+stride, half, stride, stride, 8);\
}\
static void OPNAME ## qpel8_mc11_ ## MMX(uint8_t *dst, uint8_t *src, int stride){\
    uint64_t half[8 + 9];\
    uint8_t * const halfH= ((uint8_t*)half) + 64;\
    uint8_t * const halfHV= ((uint8_t*)half);\
    put ## RND ## mpeg4_qpel8_h_lowpass_ ## MMX(halfH, src, 8, stride, 9);\
    put ## RND ## pixels8_l2_ ## MMX(halfH, src, halfH, 8, stride, 9);\
    put ## RND ## mpeg4_qpel8_v_lowpass_ ## MMX(halfHV, halfH, 8, 8);\
    OPNAME ## pixels8_l2_ ## MMX(dst, halfH, halfHV, stride, 8, 8);\
}\
static void OPNAME ## qpel8_mc31_ ## MMX(uint8_t *dst, uint8_t *src, int stride){\
    uint64_t half[8 + 9];\
    uint8_t * const halfH= ((uint8_t*)half) + 64;\
    uint8_t * const halfHV= ((uint8_t*)half);\
    put ## RND ## mpeg4_qpel8_h_lowpass_ ## MMX(halfH, src, 8, stride, 9);\
    put ## RND ## pixels8_l2_ ## MMX(halfH, src+1, halfH, 8, stride, 9);\
    put ## RND ## mpeg4_qpel8_v_lowpass_ ## MMX(halfHV, halfH, 8, 8);\
    OPNAME ## pixels8_l2_ ## MMX(dst, halfH, halfHV, stride, 8, 8);\
}\
static void OPNAME ## qpel8_mc13_ ## MMX(uint8_t *dst, uint8_t *src, int stride){\
    uint64_t half[8 + 9];\
    uint8_t * const halfH= ((uint8_t*)half) + 64;\
    uint8_t * const halfHV= ((uint8_t*)half);\
    put ## RND ## mpeg4_qpel8_h_lowpass_ ## MMX(halfH, src, 8, stride, 9);\
    put ## RND ## pixels8_l2_ ## MMX(halfH, src, halfH, 8, stride, 9);\
    put ## RND ## mpeg4_qpel8_v_lowpass_ ## MMX(halfHV, halfH, 8, 8);\
    OPNAME ## pixels8_l2_ ## MMX(dst, halfH+8, halfHV, stride, 8, 8);\
}\
static void OPNAME ## qpel8_mc33_ ## MMX(uint8_t *dst, uint8_t *src, int stride){\
    uint64_t half[8 + 9];\
    uint8_t * const halfH= ((uint8_t*)half) + 64;\
    uint8_t * const halfHV= ((uint8_t*)half);\
    put ## RND ## mpeg4_qpel8_h_lowpass_ ## MMX(halfH, src, 8, stride, 9);\
    put ## RND ## pixels8_l2_ ## MMX(halfH, src+1, halfH, 8, stride, 9);\
    put ## RND ## mpeg4_qpel8_v_lowpass_ ## MMX(halfHV, halfH, 8, 8);\
    OPNAME ## pixels8_l2_ ## MMX(dst, halfH+8, halfHV, stride, 8, 8);\
}\
static void OPNAME ## qpel8_mc21_ ## MMX(uint8_t *dst, uint8_t *src, int stride){\
    uint64_t half[8 + 9];\
    uint8_t * const halfH= ((uint8_t*)half) + 64;\
    uint8_t * const halfHV= ((uint8_t*)half);\
    put ## RND ## mpeg4_qpel8_h_lowpass_ ## MMX(halfH, src, 8, stride, 9);\
    put ## RND ## mpeg4_qpel8_v_lowpass_ ## MMX(halfHV, halfH, 8, 8);\
    OPNAME ## pixels8_l2_ ## MMX(dst, halfH, halfHV, stride, 8, 8);\
}\
static void OPNAME ## qpel8_mc23_ ## MMX(uint8_t *dst, uint8_t *src, int stride){\
    uint64_t half[8 + 9];\
    uint8_t * const halfH= ((uint8_t*)half) + 64;\
    uint8_t * const halfHV= ((uint8_t*)half);\
    put ## RND ## mpeg4_qpel8_h_lowpass_ ## MMX(halfH, src, 8, stride, 9);\
    put ## RND ## mpeg4_qpel8_v_lowpass_ ## MMX(halfHV, halfH, 8, 8);\
    OPNAME ## pixels8_l2_ ## MMX(dst, halfH+8, halfHV, stride, 8, 8);\
}\
static void OPNAME ## qpel8_mc12_ ## MMX(uint8_t *dst, uint8_t *src, int stride){\
    uint64_t half[8 + 9];\
    uint8_t * const halfH= ((uint8_t*)half);\
    put ## RND ## mpeg4_qpel8_h_lowpass_ ## MMX(halfH, src, 8, stride, 9);\
    put ## RND ## pixels8_l2_ ## MMX(halfH, src, halfH, 8, stride, 9);\
    OPNAME ## mpeg4_qpel8_v_lowpass_ ## MMX(dst, halfH, stride, 8);\
}\
static void OPNAME ## qpel8_mc32_ ## MMX(uint8_t *dst, uint8_t *src, int stride){\
    uint64_t half[8 + 9];\
    uint8_t * const halfH= ((uint8_t*)half);\
    put ## RND ## mpeg4_qpel8_h_lowpass_ ## MMX(halfH, src, 8, stride, 9);\
    put ## RND ## pixels8_l2_ ## MMX(halfH, src+1, halfH, 8, stride, 9);\
    OPNAME ## mpeg4_qpel8_v_lowpass_ ## MMX(dst, halfH, stride, 8);\
}\
static void OPNAME ## qpel8_mc22_ ## MMX(uint8_t *dst, uint8_t *src, int stride){\
    uint64_t half[9];\
    uint8_t * const halfH= ((uint8_t*)half);\
    put ## RND ## mpeg4_qpel8_h_lowpass_ ## MMX(halfH, src, 8, stride, 9);\
    OPNAME ## mpeg4_qpel8_v_lowpass_ ## MMX(dst, halfH, stride, 8);\
}\
static void OPNAME ## qpel16_mc00_ ## MMX (uint8_t *dst, uint8_t *src, int stride){\
    OPNAME ## pixels16_ ## MMX(dst, src, stride, 16);\
}\
\
static void OPNAME ## qpel16_mc10_ ## MMX(uint8_t *dst, uint8_t *src, int stride){\
    uint64_t temp[32];\
    uint8_t * const half= (uint8_t*)temp;\
    put ## RND ## mpeg4_qpel16_h_lowpass_ ## MMX(half, src, 16, stride, 16);\
    OPNAME ## pixels16_l2_ ## MMX(dst, src, half, stride, stride, 16);\
}\
\
static void OPNAME ## qpel16_mc20_ ## MMX(uint8_t *dst, uint8_t *src, int stride){\
    OPNAME ## mpeg4_qpel16_h_lowpass_ ## MMX(dst, src, stride, stride, 16);\
}\
\
static void OPNAME ## qpel16_mc30_ ## MMX(uint8_t *dst, uint8_t *src, int stride){\
    uint64_t temp[32];\
    uint8_t * const half= (uint8_t*)temp;\
    put ## RND ## mpeg4_qpel16_h_lowpass_ ## MMX(half, src, 16, stride, 16);\
    OPNAME ## pixels16_l2_ ## MMX(dst, src+1, half, stride, stride, 16);\
}\
\
static void OPNAME ## qpel16_mc01_ ## MMX(uint8_t *dst, uint8_t *src, int stride){\
    uint64_t temp[32];\
    uint8_t * const half= (uint8_t*)temp;\
    put ## RND ## mpeg4_qpel16_v_lowpass_ ## MMX(half, src, 16, stride);\
    OPNAME ## pixels16_l2_ ## MMX(dst, src, half, stride, stride, 16);\
}\
\
static void OPNAME ## qpel16_mc02_ ## MMX(uint8_t *dst, uint8_t *src, int stride){\
    OPNAME ## mpeg4_qpel16_v_lowpass_ ## MMX(dst, src, stride, stride);\
}\
\
static void OPNAME ## qpel16_mc03_ ## MMX(uint8_t *dst, uint8_t *src, int stride){\
    uint64_t temp[32];\
    uint8_t * const half= (uint8_t*)temp;\
    put ## RND ## mpeg4_qpel16_v_lowpass_ ## MMX(half, src, 16, stride);\
    OPNAME ## pixels16_l2_ ## MMX(dst, src+stride, half, stride, stride, 16);\
}\
static void OPNAME ## qpel16_mc11_ ## MMX(uint8_t *dst, uint8_t *src, int stride){\
    uint64_t half[16*2 + 17*2];\
    uint8_t * const halfH= ((uint8_t*)half) + 256;\
    uint8_t * const halfHV= ((uint8_t*)half);\
    put ## RND ## mpeg4_qpel16_h_lowpass_ ## MMX(halfH, src, 16, stride, 17);\
    put ## RND ## pixels16_l2_ ## MMX(halfH, src, halfH, 16, stride, 17);\
    put ## RND ## mpeg4_qpel16_v_lowpass_ ## MMX(halfHV, halfH, 16, 16);\
    OPNAME ## pixels16_l2_ ## MMX(dst, halfH, halfHV, stride, 16, 16);\
}\
static void OPNAME ## qpel16_mc31_ ## MMX(uint8_t *dst, uint8_t *src, int stride){\
    uint64_t half[16*2 + 17*2];\
    uint8_t * const halfH= ((uint8_t*)half) + 256;\
    uint8_t * const halfHV= ((uint8_t*)half);\
    put ## RND ## mpeg4_qpel16_h_lowpass_ ## MMX(halfH, src, 16, stride, 17);\
    put ## RND ## pixels16_l2_ ## MMX(halfH, src+1, halfH, 16, stride, 17);\
    put ## RND ## mpeg4_qpel16_v_lowpass_ ## MMX(halfHV, halfH, 16, 16);\
    OPNAME ## pixels16_l2_ ## MMX(dst, halfH, halfHV, stride, 16, 16);\
}\
static void OPNAME ## qpel16_mc13_ ## MMX(uint8_t *dst, uint8_t *src, int stride){\
    uint64_t half[16*2 + 17*2];\
    uint8_t * const halfH= ((uint8_t*)half) + 256;\
    uint8_t * const halfHV= ((uint8_t*)half);\
    put ## RND ## mpeg4_qpel16_h_lowpass_ ## MMX(halfH, src, 16, stride, 17);\
    put ## RND ## pixels16_l2_ ## MMX(halfH, src, halfH, 16, stride, 17);\
    put ## RND ## mpeg4_qpel16_v_lowpass_ ## MMX(halfHV, halfH, 16, 16);\
    OPNAME ## pixels16_l2_ ## MMX(dst, halfH+16, halfHV, stride, 16, 16);\
}\
static void OPNAME ## qpel16_mc33_ ## MMX(uint8_t *dst, uint8_t *src, int stride){\
    uint64_t half[16*2 + 17*2];\
    uint8_t * const halfH= ((uint8_t*)half) + 256;\
    uint8_t * const halfHV= ((uint8_t*)half);\
    put ## RND ## mpeg4_qpel16_h_lowpass_ ## MMX(halfH, src, 16, stride, 17);\
    put ## RND ## pixels16_l2_ ## MMX(halfH, src+1, halfH, 16, stride, 17);\
    put ## RND ## mpeg4_qpel16_v_lowpass_ ## MMX(halfHV, halfH, 16, 16);\
    OPNAME ## pixels16_l2_ ## MMX(dst, halfH+16, halfHV, stride, 16, 16);\
}\
static void OPNAME ## qpel16_mc21_ ## MMX(uint8_t *dst, uint8_t *src, int stride){\
    uint64_t half[16*2 + 17*2];\
    uint8_t * const halfH= ((uint8_t*)half) + 256;\
    uint8_t * const halfHV= ((uint8_t*)half);\
    put ## RND ## mpeg4_qpel16_h_lowpass_ ## MMX(halfH, src, 16, stride, 17);\
    put ## RND ## mpeg4_qpel16_v_lowpass_ ## MMX(halfHV, halfH, 16, 16);\
    OPNAME ## pixels16_l2_ ## MMX(dst, halfH, halfHV, stride, 16, 16);\
}\
static void OPNAME ## qpel16_mc23_ ## MMX(uint8_t *dst, uint8_t *src, int stride){\
    uint64_t half[16*2 + 17*2];\
    uint8_t * const halfH= ((uint8_t*)half) + 256;\
    uint8_t * const halfHV= ((uint8_t*)half);\
    put ## RND ## mpeg4_qpel16_h_lowpass_ ## MMX(halfH, src, 16, stride, 17);\
    put ## RND ## mpeg4_qpel16_v_lowpass_ ## MMX(halfHV, halfH, 16, 16);\
    OPNAME ## pixels16_l2_ ## MMX(dst, halfH+16, halfHV, stride, 16, 16);\
}\
static void OPNAME ## qpel16_mc12_ ## MMX(uint8_t *dst, uint8_t *src, int stride){\
    uint64_t half[17*2];\
    uint8_t * const halfH= ((uint8_t*)half);\
    put ## RND ## mpeg4_qpel16_h_lowpass_ ## MMX(halfH, src, 16, stride, 17);\
    put ## RND ## pixels16_l2_ ## MMX(halfH, src, halfH, 16, stride, 17);\
    OPNAME ## mpeg4_qpel16_v_lowpass_ ## MMX(dst, halfH, stride, 16);\
}\
static void OPNAME ## qpel16_mc32_ ## MMX(uint8_t *dst, uint8_t *src, int stride){\
    uint64_t half[17*2];\
    uint8_t * const halfH= ((uint8_t*)half);\
    put ## RND ## mpeg4_qpel16_h_lowpass_ ## MMX(halfH, src, 16, stride, 17);\
    put ## RND ## pixels16_l2_ ## MMX(halfH, src+1, halfH, 16, stride, 17);\
    OPNAME ## mpeg4_qpel16_v_lowpass_ ## MMX(dst, halfH, stride, 16);\
}\
static void OPNAME ## qpel16_mc22_ ## MMX(uint8_t *dst, uint8_t *src, int stride){\
    uint64_t half[17*2];\
    uint8_t * const halfH= ((uint8_t*)half);\
    put ## RND ## mpeg4_qpel16_h_lowpass_ ## MMX(halfH, src, 16, stride, 17);\
    OPNAME ## mpeg4_qpel16_v_lowpass_ ## MMX(dst, halfH, stride, 16);\
}

#define PUT_OP(a,b,temp, size) "mov" #size " " #a ", " #b "        \n\t"
#define AVG_3DNOW_OP(a,b,temp, size) \
"mov" #size " " #b ", " #temp "   \n\t"\
"pavgusb " #temp ", " #a "        \n\t"\
"mov" #size " " #a ", " #b "      \n\t"
#define AVG_MMX2_OP(a,b,temp, size) \
"mov" #size " " #b ", " #temp "   \n\t"\
"pavgb " #temp ", " #a "          \n\t"\
"mov" #size " " #a ", " #b "      \n\t"

QPEL_BASE(put_       , ff_pw_16, _       , PUT_OP, PUT_OP)
QPEL_BASE(avg_       , ff_pw_16, _       , AVG_MMX2_OP, AVG_3DNOW_OP)
QPEL_BASE(put_no_rnd_, ff_pw_15, _no_rnd_, PUT_OP, PUT_OP)
QPEL_OP(put_       , ff_pw_16, _       , PUT_OP, 3dnow)
QPEL_OP(avg_       , ff_pw_16, _       , AVG_3DNOW_OP, 3dnow)
QPEL_OP(put_no_rnd_, ff_pw_15, _no_rnd_, PUT_OP, 3dnow)
QPEL_OP(put_       , ff_pw_16, _       , PUT_OP, mmx2)
QPEL_OP(avg_       , ff_pw_16, _       , AVG_MMX2_OP, mmx2)
QPEL_OP(put_no_rnd_, ff_pw_15, _no_rnd_, PUT_OP, mmx2)

/***********************************/
/* bilinear qpel: not compliant to any spec, only for -lavdopts fast */

#define QPEL_2TAP_XY(OPNAME, SIZE, MMX, XY, HPEL)\
static void OPNAME ## 2tap_qpel ## SIZE ## _mc ## XY ## _ ## MMX(uint8_t *dst, uint8_t *src, int stride){\
    OPNAME ## pixels ## SIZE ## HPEL(dst, src, stride, SIZE);\
}
#define QPEL_2TAP_L3(OPNAME, SIZE, MMX, XY, S0, S1, S2)\
static void OPNAME ## 2tap_qpel ## SIZE ## _mc ## XY ## _ ## MMX(uint8_t *dst, uint8_t *src, int stride){\
    OPNAME ## 2tap_qpel ## SIZE ## _l3_ ## MMX(dst, src+S0, stride, SIZE, S1, S2);\
}

#define QPEL_2TAP(OPNAME, SIZE, MMX)\
QPEL_2TAP_XY(OPNAME, SIZE, MMX, 20, _x2_ ## MMX)\
QPEL_2TAP_XY(OPNAME, SIZE, MMX, 02, _y2_ ## MMX)\
QPEL_2TAP_XY(OPNAME, SIZE, MMX, 22, _xy2_mmx)\
static const qpel_mc_func OPNAME ## 2tap_qpel ## SIZE ## _mc00_ ## MMX =\
                          OPNAME ## qpel ## SIZE ## _mc00_ ## MMX;\
static const qpel_mc_func OPNAME ## 2tap_qpel ## SIZE ## _mc21_ ## MMX =\
                          OPNAME ## 2tap_qpel ## SIZE ## _mc20_ ## MMX;\
static const qpel_mc_func OPNAME ## 2tap_qpel ## SIZE ## _mc12_ ## MMX =\
                          OPNAME ## 2tap_qpel ## SIZE ## _mc02_ ## MMX;\
static void OPNAME ## 2tap_qpel ## SIZE ## _mc32_ ## MMX(uint8_t *dst, uint8_t *src, int stride){\
    OPNAME ## pixels ## SIZE ## _y2_ ## MMX(dst, src+1, stride, SIZE);\
}\
static void OPNAME ## 2tap_qpel ## SIZE ## _mc23_ ## MMX(uint8_t *dst, uint8_t *src, int stride){\
    OPNAME ## pixels ## SIZE ## _x2_ ## MMX(dst, src+stride, stride, SIZE);\
}\
QPEL_2TAP_L3(OPNAME, SIZE, MMX, 10, 0,         1,       0)\
QPEL_2TAP_L3(OPNAME, SIZE, MMX, 30, 1,        -1,       0)\
QPEL_2TAP_L3(OPNAME, SIZE, MMX, 01, 0,         stride,  0)\
QPEL_2TAP_L3(OPNAME, SIZE, MMX, 03, stride,   -stride,  0)\
QPEL_2TAP_L3(OPNAME, SIZE, MMX, 11, 0,         stride,  1)\
QPEL_2TAP_L3(OPNAME, SIZE, MMX, 31, 1,         stride, -1)\
QPEL_2TAP_L3(OPNAME, SIZE, MMX, 13, stride,   -stride,  1)\
QPEL_2TAP_L3(OPNAME, SIZE, MMX, 33, stride+1, -stride, -1)\

QPEL_2TAP(put_, 16, mmx2)
QPEL_2TAP(avg_, 16, mmx2)
QPEL_2TAP(put_,  8, mmx2)
QPEL_2TAP(avg_,  8, mmx2)
QPEL_2TAP(put_, 16, 3dnow)
QPEL_2TAP(avg_, 16, 3dnow)
QPEL_2TAP(put_,  8, 3dnow)
QPEL_2TAP(avg_,  8, 3dnow)


#if 0
static void just_return(void) { return; }
#endif

static void gmc_mmx(uint8_t *dst, uint8_t *src, int stride, int h, int ox, int oy,
                    int dxx, int dxy, int dyx, int dyy, int shift, int r, int width, int height){
    const int w = 8;
    const int ix = ox>>(16+shift);
    const int iy = oy>>(16+shift);
    const int oxs = ox>>4;
    const int oys = oy>>4;
    const int dxxs = dxx>>4;
    const int dxys = dxy>>4;
    const int dyxs = dyx>>4;
    const int dyys = dyy>>4;
    const uint16_t r4[4] = {r,r,r,r};
    const uint16_t dxy4[4] = {dxys,dxys,dxys,dxys};
    const uint16_t dyy4[4] = {dyys,dyys,dyys,dyys};
    const uint64_t shift2 = 2*shift;
    uint8_t edge_buf[(h+1)*stride];
    int x, y;

    const int dxw = (dxx-(1<<(16+shift)))*(w-1);
    const int dyh = (dyy-(1<<(16+shift)))*(h-1);
    const int dxh = dxy*(h-1);
    const int dyw = dyx*(w-1);
    if( // non-constant fullpel offset (3% of blocks)
        ((ox^(ox+dxw)) | (ox^(ox+dxh)) | (ox^(ox+dxw+dxh)) |
         (oy^(oy+dyw)) | (oy^(oy+dyh)) | (oy^(oy+dyw+dyh))) >> (16+shift)
        // uses more than 16 bits of subpel mv (only at huge resolution)
        || (dxx|dxy|dyx|dyy)&15 )
    {
        //FIXME could still use mmx for some of the rows
        ff_gmc_c(dst, src, stride, h, ox, oy, dxx, dxy, dyx, dyy, shift, r, width, height);
        return;
    }

    src += ix + iy*stride;
    if( (unsigned)ix >= width-w ||
        (unsigned)iy >= height-h )
    {
        ff_emulated_edge_mc(edge_buf, src, stride, w+1, h+1, ix, iy, width, height);
        src = edge_buf;
    }

    __asm__ volatile(
        "movd         %0, %%mm6 \n\t"
        "pxor      %%mm7, %%mm7 \n\t"
        "punpcklwd %%mm6, %%mm6 \n\t"
        "punpcklwd %%mm6, %%mm6 \n\t"
        :: "r"(1<<shift)
    );

    for(x=0; x<w; x+=4){
        uint16_t dx4[4] = { oxs - dxys + dxxs*(x+0),
                            oxs - dxys + dxxs*(x+1),
                            oxs - dxys + dxxs*(x+2),
                            oxs - dxys + dxxs*(x+3) };
        uint16_t dy4[4] = { oys - dyys + dyxs*(x+0),
                            oys - dyys + dyxs*(x+1),
                            oys - dyys + dyxs*(x+2),
                            oys - dyys + dyxs*(x+3) };

        for(y=0; y<h; y++){
            __asm__ volatile(
                "movq   %0,  %%mm4 \n\t"
                "movq   %1,  %%mm5 \n\t"
                "paddw  %2,  %%mm4 \n\t"
                "paddw  %3,  %%mm5 \n\t"
                "movq   %%mm4, %0  \n\t"
                "movq   %%mm5, %1  \n\t"
                "psrlw  $12, %%mm4 \n\t"
                "psrlw  $12, %%mm5 \n\t"
                : "+m"(*dx4), "+m"(*dy4)
                : "m"(*dxy4), "m"(*dyy4)
            );

            __asm__ volatile(
                "movq   %%mm6, %%mm2 \n\t"
                "movq   %%mm6, %%mm1 \n\t"
                "psubw  %%mm4, %%mm2 \n\t"
                "psubw  %%mm5, %%mm1 \n\t"
                "movq   %%mm2, %%mm0 \n\t"
                "movq   %%mm4, %%mm3 \n\t"
                "pmullw %%mm1, %%mm0 \n\t" // (s-dx)*(s-dy)
                "pmullw %%mm5, %%mm3 \n\t" // dx*dy
                "pmullw %%mm5, %%mm2 \n\t" // (s-dx)*dy
                "pmullw %%mm4, %%mm1 \n\t" // dx*(s-dy)

                "movd   %4,    %%mm5 \n\t"
                "movd   %3,    %%mm4 \n\t"
                "punpcklbw %%mm7, %%mm5 \n\t"
                "punpcklbw %%mm7, %%mm4 \n\t"
                "pmullw %%mm5, %%mm3 \n\t" // src[1,1] * dx*dy
                "pmullw %%mm4, %%mm2 \n\t" // src[0,1] * (s-dx)*dy

                "movd   %2,    %%mm5 \n\t"
                "movd   %1,    %%mm4 \n\t"
                "punpcklbw %%mm7, %%mm5 \n\t"
                "punpcklbw %%mm7, %%mm4 \n\t"
                "pmullw %%mm5, %%mm1 \n\t" // src[1,0] * dx*(s-dy)
                "pmullw %%mm4, %%mm0 \n\t" // src[0,0] * (s-dx)*(s-dy)
                "paddw  %5,    %%mm1 \n\t"
                "paddw  %%mm3, %%mm2 \n\t"
                "paddw  %%mm1, %%mm0 \n\t"
                "paddw  %%mm2, %%mm0 \n\t"

                "psrlw    %6,    %%mm0 \n\t"
                "packuswb %%mm0, %%mm0 \n\t"
                "movd     %%mm0, %0    \n\t"

                : "=m"(dst[x+y*stride])
                : "m"(src[0]), "m"(src[1]),
                  "m"(src[stride]), "m"(src[stride+1]),
                  "m"(*r4), "m"(shift2)
            );
            src += stride;
        }
        src += 4-h*stride;
    }
}

#define PREFETCH(name, op) \
static void name(void *mem, int stride, int h){\
    const uint8_t *p= mem;\
    do{\
        __asm__ volatile(#op" %0" :: "m"(*p));\
        p+= stride;\
    }while(--h);\
}
PREFETCH(prefetch_mmx2,  prefetcht0)
PREFETCH(prefetch_3dnow, prefetch)
#undef PREFETCH

#include "h264dsp_mmx.c"
#include "rv40dsp_mmx.c"

/* CAVS specific */
void ff_put_cavs_qpel8_mc00_mmx2(uint8_t *dst, uint8_t *src, int stride) {
    put_pixels8_mmx(dst, src, stride, 8);
}
void ff_avg_cavs_qpel8_mc00_mmx2(uint8_t *dst, uint8_t *src, int stride) {
    avg_pixels8_mmx(dst, src, stride, 8);
}
void ff_put_cavs_qpel16_mc00_mmx2(uint8_t *dst, uint8_t *src, int stride) {
    put_pixels16_mmx(dst, src, stride, 16);
}
void ff_avg_cavs_qpel16_mc00_mmx2(uint8_t *dst, uint8_t *src, int stride) {
    avg_pixels16_mmx(dst, src, stride, 16);
}

/* VC1 specific */
void ff_put_vc1_mspel_mc00_mmx(uint8_t *dst, const uint8_t *src, int stride, int rnd) {
    put_pixels8_mmx(dst, src, stride, 8);
}
void ff_avg_vc1_mspel_mc00_mmx2(uint8_t *dst, const uint8_t *src, int stride, int rnd) {
    avg_pixels8_mmx2(dst, src, stride, 8);
}

/* XXX: those functions should be suppressed ASAP when all IDCTs are
   converted */
#if CONFIG_GPL
static void ff_libmpeg2mmx_idct_put(uint8_t *dest, int line_size, DCTELEM *block)
{
    ff_mmx_idct (block);
    put_pixels_clamped_mmx(block, dest, line_size);
}
static void ff_libmpeg2mmx_idct_add(uint8_t *dest, int line_size, DCTELEM *block)
{
    ff_mmx_idct (block);
    add_pixels_clamped_mmx(block, dest, line_size);
}
static void ff_libmpeg2mmx2_idct_put(uint8_t *dest, int line_size, DCTELEM *block)
{
    ff_mmxext_idct (block);
    put_pixels_clamped_mmx(block, dest, line_size);
}
static void ff_libmpeg2mmx2_idct_add(uint8_t *dest, int line_size, DCTELEM *block)
{
    ff_mmxext_idct (block);
    add_pixels_clamped_mmx(block, dest, line_size);
}
#endif
static void ff_idct_xvid_mmx_put(uint8_t *dest, int line_size, DCTELEM *block)
{
    ff_idct_xvid_mmx (block);
    put_pixels_clamped_mmx(block, dest, line_size);
}
static void ff_idct_xvid_mmx_add(uint8_t *dest, int line_size, DCTELEM *block)
{
    ff_idct_xvid_mmx (block);
    add_pixels_clamped_mmx(block, dest, line_size);
}
static void ff_idct_xvid_mmx2_put(uint8_t *dest, int line_size, DCTELEM *block)
{
    ff_idct_xvid_mmx2 (block);
    put_pixels_clamped_mmx(block, dest, line_size);
}
static void ff_idct_xvid_mmx2_add(uint8_t *dest, int line_size, DCTELEM *block)
{
    ff_idct_xvid_mmx2 (block);
    add_pixels_clamped_mmx(block, dest, line_size);
}

static void vorbis_inverse_coupling_3dnow(float *mag, float *ang, int blocksize)
{
    int i;
    __asm__ volatile("pxor %%mm7, %%mm7":);
    for(i=0; i<blocksize; i+=2) {
        __asm__ volatile(
            "movq    %0,    %%mm0 \n\t"
            "movq    %1,    %%mm1 \n\t"
            "movq    %%mm0, %%mm2 \n\t"
            "movq    %%mm1, %%mm3 \n\t"
            "pfcmpge %%mm7, %%mm2 \n\t" // m <= 0.0
            "pfcmpge %%mm7, %%mm3 \n\t" // a <= 0.0
            "pslld   $31,   %%mm2 \n\t" // keep only the sign bit
            "pxor    %%mm2, %%mm1 \n\t"
            "movq    %%mm3, %%mm4 \n\t"
            "pand    %%mm1, %%mm3 \n\t"
            "pandn   %%mm1, %%mm4 \n\t"
            "pfadd   %%mm0, %%mm3 \n\t" // a = m + ((a<0) & (a ^ sign(m)))
            "pfsub   %%mm4, %%mm0 \n\t" // m = m + ((a>0) & (a ^ sign(m)))
            "movq    %%mm3, %1    \n\t"
            "movq    %%mm0, %0    \n\t"
            :"+m"(mag[i]), "+m"(ang[i])
            ::"memory"
        );
    }
    __asm__ volatile("femms");
}
static void vorbis_inverse_coupling_sse(float *mag, float *ang, int blocksize)
{
    int i;

    __asm__ volatile(
            "movaps  %0,     %%xmm5 \n\t"
        ::"m"(ff_pdw_80000000[0])
    );
    for(i=0; i<blocksize; i+=4) {
        __asm__ volatile(
            "movaps  %0,     %%xmm0 \n\t"
            "movaps  %1,     %%xmm1 \n\t"
            "xorps   %%xmm2, %%xmm2 \n\t"
            "xorps   %%xmm3, %%xmm3 \n\t"
            "cmpleps %%xmm0, %%xmm2 \n\t" // m <= 0.0
            "cmpleps %%xmm1, %%xmm3 \n\t" // a <= 0.0
            "andps   %%xmm5, %%xmm2 \n\t" // keep only the sign bit
            "xorps   %%xmm2, %%xmm1 \n\t"
            "movaps  %%xmm3, %%xmm4 \n\t"
            "andps   %%xmm1, %%xmm3 \n\t"
            "andnps  %%xmm1, %%xmm4 \n\t"
            "addps   %%xmm0, %%xmm3 \n\t" // a = m + ((a<0) & (a ^ sign(m)))
            "subps   %%xmm4, %%xmm0 \n\t" // m = m + ((a>0) & (a ^ sign(m)))
            "movaps  %%xmm3, %1     \n\t"
            "movaps  %%xmm0, %0     \n\t"
            :"+m"(mag[i]), "+m"(ang[i])
            ::"memory"
        );
    }
}

#define IF1(x) x
#define IF0(x)

#define MIX5(mono,stereo)\
    __asm__ volatile(\
        "movss          0(%2), %%xmm5 \n"\
        "movss          8(%2), %%xmm6 \n"\
        "movss         24(%2), %%xmm7 \n"\
        "shufps    $0, %%xmm5, %%xmm5 \n"\
        "shufps    $0, %%xmm6, %%xmm6 \n"\
        "shufps    $0, %%xmm7, %%xmm7 \n"\
        "1: \n"\
        "movaps       (%0,%1), %%xmm0 \n"\
        "movaps  0x400(%0,%1), %%xmm1 \n"\
        "movaps  0x800(%0,%1), %%xmm2 \n"\
        "movaps  0xc00(%0,%1), %%xmm3 \n"\
        "movaps 0x1000(%0,%1), %%xmm4 \n"\
        "mulps         %%xmm5, %%xmm0 \n"\
        "mulps         %%xmm6, %%xmm1 \n"\
        "mulps         %%xmm5, %%xmm2 \n"\
        "mulps         %%xmm7, %%xmm3 \n"\
        "mulps         %%xmm7, %%xmm4 \n"\
 stereo("addps         %%xmm1, %%xmm0 \n")\
        "addps         %%xmm1, %%xmm2 \n"\
        "addps         %%xmm3, %%xmm0 \n"\
        "addps         %%xmm4, %%xmm2 \n"\
   mono("addps         %%xmm2, %%xmm0 \n")\
        "movaps  %%xmm0,      (%0,%1) \n"\
 stereo("movaps  %%xmm2, 0x400(%0,%1) \n")\
        "add $16, %0 \n"\
        "jl 1b \n"\
        :"+&r"(i)\
        :"r"(samples[0]+len), "r"(matrix)\
        :"memory"\
    );

#define MIX_MISC(stereo)\
    __asm__ volatile(\
        "1: \n"\
        "movaps  (%3,%0), %%xmm0 \n"\
 stereo("movaps   %%xmm0, %%xmm1 \n")\
        "mulps    %%xmm6, %%xmm0 \n"\
 stereo("mulps    %%xmm7, %%xmm1 \n")\
        "lea 1024(%3,%0), %1 \n"\
        "mov %5, %2 \n"\
        "2: \n"\
        "movaps   (%1),   %%xmm2 \n"\
 stereo("movaps   %%xmm2, %%xmm3 \n")\
        "mulps   (%4,%2), %%xmm2 \n"\
 stereo("mulps 16(%4,%2), %%xmm3 \n")\
        "addps    %%xmm2, %%xmm0 \n"\
 stereo("addps    %%xmm3, %%xmm1 \n")\
        "add $1024, %1 \n"\
        "add $32, %2 \n"\
        "jl 2b \n"\
        "movaps   %%xmm0,     (%3,%0) \n"\
 stereo("movaps   %%xmm1, 1024(%3,%0) \n")\
        "add $16, %0 \n"\
        "jl 1b \n"\
        :"+&r"(i), "=&r"(j), "=&r"(k)\
        :"r"(samples[0]+len), "r"(matrix_simd+in_ch), "g"((intptr_t)-32*(in_ch-1))\
        :"memory"\
    );

static void ac3_downmix_sse(float (*samples)[256], float (*matrix)[2], int out_ch, int in_ch, int len)
{
    int (*matrix_cmp)[2] = (int(*)[2])matrix;
    intptr_t i,j,k;

    i = -len*sizeof(float);
    if(in_ch == 5 && out_ch == 2 && !(matrix_cmp[0][1]|matrix_cmp[2][0]|matrix_cmp[3][1]|matrix_cmp[4][0]|(matrix_cmp[1][0]^matrix_cmp[1][1])|(matrix_cmp[0][0]^matrix_cmp[2][1]))) {
        MIX5(IF0,IF1);
    } else if(in_ch == 5 && out_ch == 1 && matrix_cmp[0][0]==matrix_cmp[2][0] && matrix_cmp[3][0]==matrix_cmp[4][0]) {
        MIX5(IF1,IF0);
    } else {
        DECLARE_ALIGNED(16, float, matrix_simd)[in_ch][2][4];
        j = 2*in_ch*sizeof(float);
        __asm__ volatile(
            "1: \n"
            "sub $8, %0 \n"
            "movss     (%2,%0), %%xmm6 \n"
            "movss    4(%2,%0), %%xmm7 \n"
            "shufps $0, %%xmm6, %%xmm6 \n"
            "shufps $0, %%xmm7, %%xmm7 \n"
            "movaps %%xmm6,   (%1,%0,4) \n"
            "movaps %%xmm7, 16(%1,%0,4) \n"
            "jg 1b \n"
            :"+&r"(j)
            :"r"(matrix_simd), "r"(matrix)
            :"memory"
        );
        if(out_ch == 2) {
            MIX_MISC(IF1);
        } else {
            MIX_MISC(IF0);
        }
    }
}

static void vector_fmul_3dnow(float *dst, const float *src, int len){
    x86_reg i = (len-4)*4;
    __asm__ volatile(
        "1: \n\t"
        "movq    (%1,%0), %%mm0 \n\t"
        "movq   8(%1,%0), %%mm1 \n\t"
        "pfmul   (%2,%0), %%mm0 \n\t"
        "pfmul  8(%2,%0), %%mm1 \n\t"
        "movq   %%mm0,  (%1,%0) \n\t"
        "movq   %%mm1, 8(%1,%0) \n\t"
        "sub  $16, %0 \n\t"
        "jge 1b \n\t"
        "femms  \n\t"
        :"+r"(i)
        :"r"(dst), "r"(src)
        :"memory"
    );
}
static void vector_fmul_sse(float *dst, const float *src, int len){
    x86_reg i = (len-8)*4;
    __asm__ volatile(
        "1: \n\t"
        "movaps    (%1,%0), %%xmm0 \n\t"
        "movaps  16(%1,%0), %%xmm1 \n\t"
        "mulps     (%2,%0), %%xmm0 \n\t"
        "mulps   16(%2,%0), %%xmm1 \n\t"
        "movaps  %%xmm0,   (%1,%0) \n\t"
        "movaps  %%xmm1, 16(%1,%0) \n\t"
        "sub  $32, %0 \n\t"
        "jge 1b \n\t"
        :"+r"(i)
        :"r"(dst), "r"(src)
        :"memory"
    );
}

static void vector_fmul_reverse_3dnow2(float *dst, const float *src0, const float *src1, int len){
    x86_reg i = len*4-16;
    __asm__ volatile(
        "1: \n\t"
        "pswapd   8(%1), %%mm0 \n\t"
        "pswapd    (%1), %%mm1 \n\t"
        "pfmul  (%3,%0), %%mm0 \n\t"
        "pfmul 8(%3,%0), %%mm1 \n\t"
        "movq  %%mm0,  (%2,%0) \n\t"
        "movq  %%mm1, 8(%2,%0) \n\t"
        "add   $16, %1 \n\t"
        "sub   $16, %0 \n\t"
        "jge   1b \n\t"
        :"+r"(i), "+r"(src1)
        :"r"(dst), "r"(src0)
    );
    __asm__ volatile("femms");
}
static void vector_fmul_reverse_sse(float *dst, const float *src0, const float *src1, int len){
    x86_reg i = len*4-32;
    __asm__ volatile(
        "1: \n\t"
        "movaps        16(%1), %%xmm0 \n\t"
        "movaps          (%1), %%xmm1 \n\t"
        "shufps $0x1b, %%xmm0, %%xmm0 \n\t"
        "shufps $0x1b, %%xmm1, %%xmm1 \n\t"
        "mulps        (%3,%0), %%xmm0 \n\t"
        "mulps      16(%3,%0), %%xmm1 \n\t"
        "movaps     %%xmm0,   (%2,%0) \n\t"
        "movaps     %%xmm1, 16(%2,%0) \n\t"
        "add    $32, %1 \n\t"
        "sub    $32, %0 \n\t"
        "jge    1b \n\t"
        :"+r"(i), "+r"(src1)
        :"r"(dst), "r"(src0)
    );
}

static void vector_fmul_add_3dnow(float *dst, const float *src0, const float *src1,
                                  const float *src2, int len){
    x86_reg i = (len-4)*4;
    __asm__ volatile(
        "1: \n\t"
        "movq    (%2,%0), %%mm0 \n\t"
        "movq   8(%2,%0), %%mm1 \n\t"
        "pfmul   (%3,%0), %%mm0 \n\t"
        "pfmul  8(%3,%0), %%mm1 \n\t"
        "pfadd   (%4,%0), %%mm0 \n\t"
        "pfadd  8(%4,%0), %%mm1 \n\t"
        "movq  %%mm0,   (%1,%0) \n\t"
        "movq  %%mm1,  8(%1,%0) \n\t"
        "sub  $16, %0 \n\t"
        "jge  1b \n\t"
        :"+r"(i)
        :"r"(dst), "r"(src0), "r"(src1), "r"(src2)
        :"memory"
    );
    __asm__ volatile("femms");
}
static void vector_fmul_add_sse(float *dst, const float *src0, const float *src1,
                                const float *src2, int len){
    x86_reg i = (len-8)*4;
    __asm__ volatile(
        "1: \n\t"
        "movaps   (%2,%0), %%xmm0 \n\t"
        "movaps 16(%2,%0), %%xmm1 \n\t"
        "mulps    (%3,%0), %%xmm0 \n\t"
        "mulps  16(%3,%0), %%xmm1 \n\t"
        "addps    (%4,%0), %%xmm0 \n\t"
        "addps  16(%4,%0), %%xmm1 \n\t"
        "movaps %%xmm0,   (%1,%0) \n\t"
        "movaps %%xmm1, 16(%1,%0) \n\t"
        "sub  $32, %0 \n\t"
        "jge  1b \n\t"
        :"+r"(i)
        :"r"(dst), "r"(src0), "r"(src1), "r"(src2)
        :"memory"
    );
}

static void vector_fmul_window_3dnow2(float *dst, const float *src0, const float *src1,
                                      const float *win, float add_bias, int len){
#if HAVE_6REGS
    if(add_bias == 0){
        x86_reg i = -len*4;
        x86_reg j = len*4-8;
        __asm__ volatile(
            "1: \n"
            "pswapd  (%5,%1), %%mm1 \n"
            "movq    (%5,%0), %%mm0 \n"
            "pswapd  (%4,%1), %%mm5 \n"
            "movq    (%3,%0), %%mm4 \n"
            "movq      %%mm0, %%mm2 \n"
            "movq      %%mm1, %%mm3 \n"
            "pfmul     %%mm4, %%mm2 \n" // src0[len+i]*win[len+i]
            "pfmul     %%mm5, %%mm3 \n" // src1[    j]*win[len+j]
            "pfmul     %%mm4, %%mm1 \n" // src0[len+i]*win[len+j]
            "pfmul     %%mm5, %%mm0 \n" // src1[    j]*win[len+i]
            "pfadd     %%mm3, %%mm2 \n"
            "pfsub     %%mm0, %%mm1 \n"
            "pswapd    %%mm2, %%mm2 \n"
            "movq      %%mm1, (%2,%0) \n"
            "movq      %%mm2, (%2,%1) \n"
            "sub $8, %1 \n"
            "add $8, %0 \n"
            "jl 1b \n"
            "femms \n"
            :"+r"(i), "+r"(j)
            :"r"(dst+len), "r"(src0+len), "r"(src1), "r"(win+len)
        );
    }else
#endif
        ff_vector_fmul_window_c(dst, src0, src1, win, add_bias, len);
}

static void vector_fmul_window_sse(float *dst, const float *src0, const float *src1,
                                   const float *win, float add_bias, int len){
#if HAVE_6REGS
    if(add_bias == 0){
        x86_reg i = -len*4;
        x86_reg j = len*4-16;
        __asm__ volatile(
            "1: \n"
            "movaps       (%5,%1), %%xmm1 \n"
            "movaps       (%5,%0), %%xmm0 \n"
            "movaps       (%4,%1), %%xmm5 \n"
            "movaps       (%3,%0), %%xmm4 \n"
            "shufps $0x1b, %%xmm1, %%xmm1 \n"
            "shufps $0x1b, %%xmm5, %%xmm5 \n"
            "movaps        %%xmm0, %%xmm2 \n"
            "movaps        %%xmm1, %%xmm3 \n"
            "mulps         %%xmm4, %%xmm2 \n" // src0[len+i]*win[len+i]
            "mulps         %%xmm5, %%xmm3 \n" // src1[    j]*win[len+j]
            "mulps         %%xmm4, %%xmm1 \n" // src0[len+i]*win[len+j]
            "mulps         %%xmm5, %%xmm0 \n" // src1[    j]*win[len+i]
            "addps         %%xmm3, %%xmm2 \n"
            "subps         %%xmm0, %%xmm1 \n"
            "shufps $0x1b, %%xmm2, %%xmm2 \n"
            "movaps        %%xmm1, (%2,%0) \n"
            "movaps        %%xmm2, (%2,%1) \n"
            "sub $16, %1 \n"
            "add $16, %0 \n"
            "jl 1b \n"
            :"+r"(i), "+r"(j)
            :"r"(dst+len), "r"(src0+len), "r"(src1), "r"(win+len)
        );
    }else
#endif
        ff_vector_fmul_window_c(dst, src0, src1, win, add_bias, len);
}

static void int32_to_float_fmul_scalar_sse(float *dst, const int *src, float mul, int len)
{
    x86_reg i = -4*len;
    __asm__ volatile(
        "movss  %3, %%xmm4 \n"
        "shufps $0, %%xmm4, %%xmm4 \n"
        "1: \n"
        "cvtpi2ps   (%2,%0), %%xmm0 \n"
        "cvtpi2ps  8(%2,%0), %%xmm1 \n"
        "cvtpi2ps 16(%2,%0), %%xmm2 \n"
        "cvtpi2ps 24(%2,%0), %%xmm3 \n"
        "movlhps  %%xmm1,    %%xmm0 \n"
        "movlhps  %%xmm3,    %%xmm2 \n"
        "mulps    %%xmm4,    %%xmm0 \n"
        "mulps    %%xmm4,    %%xmm2 \n"
        "movaps   %%xmm0,   (%1,%0) \n"
        "movaps   %%xmm2, 16(%1,%0) \n"
        "add $32, %0 \n"
        "jl 1b \n"
        :"+r"(i)
        :"r"(dst+len), "r"(src+len), "m"(mul)
    );
}

static void int32_to_float_fmul_scalar_sse2(float *dst, const int *src, float mul, int len)
{
    x86_reg i = -4*len;
    __asm__ volatile(
        "movss  %3, %%xmm4 \n"
        "shufps $0, %%xmm4, %%xmm4 \n"
        "1: \n"
        "cvtdq2ps   (%2,%0), %%xmm0 \n"
        "cvtdq2ps 16(%2,%0), %%xmm1 \n"
        "mulps    %%xmm4,    %%xmm0 \n"
        "mulps    %%xmm4,    %%xmm1 \n"
        "movaps   %%xmm0,   (%1,%0) \n"
        "movaps   %%xmm1, 16(%1,%0) \n"
        "add $32, %0 \n"
        "jl 1b \n"
        :"+r"(i)
        :"r"(dst+len), "r"(src+len), "m"(mul)
    );
}

static void vector_clipf_sse(float *dst, const float *src, float min, float max,
                             int len)
{
    x86_reg i = (len-16)*4;
    __asm__ volatile(
        "movss  %3, %%xmm4 \n"
        "movss  %4, %%xmm5 \n"
        "shufps $0, %%xmm4, %%xmm4 \n"
        "shufps $0, %%xmm5, %%xmm5 \n"
        "1: \n\t"
        "movaps    (%2,%0), %%xmm0 \n\t" // 3/1 on intel
        "movaps  16(%2,%0), %%xmm1 \n\t"
        "movaps  32(%2,%0), %%xmm2 \n\t"
        "movaps  48(%2,%0), %%xmm3 \n\t"
        "maxps      %%xmm4, %%xmm0 \n\t"
        "maxps      %%xmm4, %%xmm1 \n\t"
        "maxps      %%xmm4, %%xmm2 \n\t"
        "maxps      %%xmm4, %%xmm3 \n\t"
        "minps      %%xmm5, %%xmm0 \n\t"
        "minps      %%xmm5, %%xmm1 \n\t"
        "minps      %%xmm5, %%xmm2 \n\t"
        "minps      %%xmm5, %%xmm3 \n\t"
        "movaps  %%xmm0,   (%1,%0) \n\t"
        "movaps  %%xmm1, 16(%1,%0) \n\t"
        "movaps  %%xmm2, 32(%1,%0) \n\t"
        "movaps  %%xmm3, 48(%1,%0) \n\t"
        "sub  $64, %0 \n\t"
        "jge 1b \n\t"
        :"+&r"(i)
        :"r"(dst), "r"(src), "m"(min), "m"(max)
        :"memory"
    );
}

static void float_to_int16_3dnow(int16_t *dst, const float *src, long len){
    x86_reg reglen = len;
    // not bit-exact: pf2id uses different rounding than C and SSE
    __asm__ volatile(
        "add        %0          , %0        \n\t"
        "lea         (%2,%0,2)  , %2        \n\t"
        "add        %0          , %1        \n\t"
        "neg        %0                      \n\t"
        "1:                                 \n\t"
        "pf2id       (%2,%0,2)  , %%mm0     \n\t"
        "pf2id      8(%2,%0,2)  , %%mm1     \n\t"
        "pf2id     16(%2,%0,2)  , %%mm2     \n\t"
        "pf2id     24(%2,%0,2)  , %%mm3     \n\t"
        "packssdw   %%mm1       , %%mm0     \n\t"
        "packssdw   %%mm3       , %%mm2     \n\t"
        "movq       %%mm0       ,  (%1,%0)  \n\t"
        "movq       %%mm2       , 8(%1,%0)  \n\t"
        "add        $16         , %0        \n\t"
        " js 1b                             \n\t"
        "femms                              \n\t"
        :"+r"(reglen), "+r"(dst), "+r"(src)
    );
}
static void float_to_int16_sse(int16_t *dst, const float *src, long len){
    x86_reg reglen = len;
    __asm__ volatile(
        "add        %0          , %0        \n\t"
        "lea         (%2,%0,2)  , %2        \n\t"
        "add        %0          , %1        \n\t"
        "neg        %0                      \n\t"
        "1:                                 \n\t"
        "cvtps2pi    (%2,%0,2)  , %%mm0     \n\t"
        "cvtps2pi   8(%2,%0,2)  , %%mm1     \n\t"
        "cvtps2pi  16(%2,%0,2)  , %%mm2     \n\t"
        "cvtps2pi  24(%2,%0,2)  , %%mm3     \n\t"
        "packssdw   %%mm1       , %%mm0     \n\t"
        "packssdw   %%mm3       , %%mm2     \n\t"
        "movq       %%mm0       ,  (%1,%0)  \n\t"
        "movq       %%mm2       , 8(%1,%0)  \n\t"
        "add        $16         , %0        \n\t"
        " js 1b                             \n\t"
        "emms                               \n\t"
        :"+r"(reglen), "+r"(dst), "+r"(src)
    );
}

static void float_to_int16_sse2(int16_t *dst, const float *src, long len){
    x86_reg reglen = len;
    __asm__ volatile(
        "add        %0          , %0        \n\t"
        "lea         (%2,%0,2)  , %2        \n\t"
        "add        %0          , %1        \n\t"
        "neg        %0                      \n\t"
        "1:                                 \n\t"
        "cvtps2dq    (%2,%0,2)  , %%xmm0    \n\t"
        "cvtps2dq  16(%2,%0,2)  , %%xmm1    \n\t"
        "packssdw   %%xmm1      , %%xmm0    \n\t"
        "movdqa     %%xmm0      ,  (%1,%0)  \n\t"
        "add        $16         , %0        \n\t"
        " js 1b                             \n\t"
        :"+r"(reglen), "+r"(dst), "+r"(src)
    );
}

void ff_float_to_int16_interleave6_sse(int16_t *dst, const float **src, int len);
void ff_float_to_int16_interleave6_3dnow(int16_t *dst, const float **src, int len);
void ff_float_to_int16_interleave6_3dn2(int16_t *dst, const float **src, int len);
int32_t ff_scalarproduct_int16_mmx2(int16_t *v1, int16_t *v2, int order, int shift);
int32_t ff_scalarproduct_int16_sse2(int16_t *v1, int16_t *v2, int order, int shift);
int32_t ff_scalarproduct_and_madd_int16_mmx2(int16_t *v1, int16_t *v2, int16_t *v3, int order, int mul);
int32_t ff_scalarproduct_and_madd_int16_sse2(int16_t *v1, int16_t *v2, int16_t *v3, int order, int mul);
int32_t ff_scalarproduct_and_madd_int16_ssse3(int16_t *v1, int16_t *v2, int16_t *v3, int order, int mul);
void ff_add_hfyu_median_prediction_mmx2(uint8_t *dst, const uint8_t *top, const uint8_t *diff, int w, int *left, int *left_top);
int  ff_add_hfyu_left_prediction_ssse3(uint8_t *dst, const uint8_t *src, int w, int left);
int  ff_add_hfyu_left_prediction_sse4(uint8_t *dst, const uint8_t *src, int w, int left);
void ff_x264_deblock_v_luma_sse2(uint8_t *pix, int stride, int alpha, int beta, int8_t *tc0);
void ff_x264_deblock_h_luma_sse2(uint8_t *pix, int stride, int alpha, int beta, int8_t *tc0);
void ff_x264_deblock_h_luma_intra_mmxext(uint8_t *pix, int stride, int alpha, int beta);
void ff_x264_deblock_v_luma_intra_sse2(uint8_t *pix, int stride, int alpha, int beta);
void ff_x264_deblock_h_luma_intra_sse2(uint8_t *pix, int stride, int alpha, int beta);

#if HAVE_YASM && ARCH_X86_32
void ff_x264_deblock_v8_luma_intra_mmxext(uint8_t *pix, int stride, int alpha, int beta);
static void ff_x264_deblock_v_luma_intra_mmxext(uint8_t *pix, int stride, int alpha, int beta)
{
    ff_x264_deblock_v8_luma_intra_mmxext(pix+0, stride, alpha, beta);
    ff_x264_deblock_v8_luma_intra_mmxext(pix+8, stride, alpha, beta);
}
#elif !HAVE_YASM
#define ff_float_to_int16_interleave6_sse(a,b,c)   float_to_int16_interleave_misc_sse(a,b,c,6)
#define ff_float_to_int16_interleave6_3dnow(a,b,c) float_to_int16_interleave_misc_3dnow(a,b,c,6)
#define ff_float_to_int16_interleave6_3dn2(a,b,c)  float_to_int16_interleave_misc_3dnow(a,b,c,6)
#endif
#define ff_float_to_int16_interleave6_sse2 ff_float_to_int16_interleave6_sse

#define FLOAT_TO_INT16_INTERLEAVE(cpu, body) \
/* gcc pessimizes register allocation if this is in the same function as float_to_int16_interleave_sse2*/\
static av_noinline void float_to_int16_interleave_misc_##cpu(int16_t *dst, const float **src, long len, int channels){\
    DECLARE_ALIGNED(16, int16_t, tmp)[len];\
    int i,j,c;\
    for(c=0; c<channels; c++){\
        float_to_int16_##cpu(tmp, src[c], len);\
        for(i=0, j=c; i<len; i++, j+=channels)\
            dst[j] = tmp[i];\
    }\
}\
\
static void float_to_int16_interleave_##cpu(int16_t *dst, const float **src, long len, int channels){\
    if(channels==1)\
        float_to_int16_##cpu(dst, src[0], len);\
    else if(channels==2){\
        x86_reg reglen = len; \
        const float *src0 = src[0];\
        const float *src1 = src[1];\
        __asm__ volatile(\
            "shl $2, %0 \n"\
            "add %0, %1 \n"\
            "add %0, %2 \n"\
            "add %0, %3 \n"\
            "neg %0 \n"\
            body\
            :"+r"(reglen), "+r"(dst), "+r"(src0), "+r"(src1)\
        );\
    }else if(channels==6){\
        ff_float_to_int16_interleave6_##cpu(dst, src, len);\
    }else\
        float_to_int16_interleave_misc_##cpu(dst, src, len, channels);\
}

FLOAT_TO_INT16_INTERLEAVE(3dnow,
    "1:                         \n"
    "pf2id     (%2,%0), %%mm0   \n"
    "pf2id    8(%2,%0), %%mm1   \n"
    "pf2id     (%3,%0), %%mm2   \n"
    "pf2id    8(%3,%0), %%mm3   \n"
    "packssdw    %%mm1, %%mm0   \n"
    "packssdw    %%mm3, %%mm2   \n"
    "movq        %%mm0, %%mm1   \n"
    "punpcklwd   %%mm2, %%mm0   \n"
    "punpckhwd   %%mm2, %%mm1   \n"
    "movq        %%mm0,  (%1,%0)\n"
    "movq        %%mm1, 8(%1,%0)\n"
    "add $16, %0                \n"
    "js 1b                      \n"
    "femms                      \n"
)

FLOAT_TO_INT16_INTERLEAVE(sse,
    "1:                         \n"
    "cvtps2pi  (%2,%0), %%mm0   \n"
    "cvtps2pi 8(%2,%0), %%mm1   \n"
    "cvtps2pi  (%3,%0), %%mm2   \n"
    "cvtps2pi 8(%3,%0), %%mm3   \n"
    "packssdw    %%mm1, %%mm0   \n"
    "packssdw    %%mm3, %%mm2   \n"
    "movq        %%mm0, %%mm1   \n"
    "punpcklwd   %%mm2, %%mm0   \n"
    "punpckhwd   %%mm2, %%mm1   \n"
    "movq        %%mm0,  (%1,%0)\n"
    "movq        %%mm1, 8(%1,%0)\n"
    "add $16, %0                \n"
    "js 1b                      \n"
    "emms                       \n"
)

FLOAT_TO_INT16_INTERLEAVE(sse2,
    "1:                         \n"
    "cvtps2dq  (%2,%0), %%xmm0  \n"
    "cvtps2dq  (%3,%0), %%xmm1  \n"
    "packssdw   %%xmm1, %%xmm0  \n"
    "movhlps    %%xmm0, %%xmm1  \n"
    "punpcklwd  %%xmm1, %%xmm0  \n"
    "movdqa     %%xmm0, (%1,%0) \n"
    "add $16, %0                \n"
    "js 1b                      \n"
)

static void float_to_int16_interleave_3dn2(int16_t *dst, const float **src, long len, int channels){
    if(channels==6)
        ff_float_to_int16_interleave6_3dn2(dst, src, len);
    else
        float_to_int16_interleave_3dnow(dst, src, len, channels);
}

float ff_scalarproduct_float_sse(const float *v1, const float *v2, int order);

void dsputil_init_mmx(DSPContext* c, AVCodecContext *avctx)
{
    mm_flags = mm_support();

    if (avctx->dsp_mask) {
        if (avctx->dsp_mask & FF_MM_FORCE)
            mm_flags |= (avctx->dsp_mask & 0xffff);
        else
            mm_flags &= ~(avctx->dsp_mask & 0xffff);
    }

#if 0
    av_log(avctx, AV_LOG_INFO, "libavcodec: CPU flags:");
    if (mm_flags & FF_MM_MMX)
        av_log(avctx, AV_LOG_INFO, " mmx");
    if (mm_flags & FF_MM_MMX2)
        av_log(avctx, AV_LOG_INFO, " mmx2");
    if (mm_flags & FF_MM_3DNOW)
        av_log(avctx, AV_LOG_INFO, " 3dnow");
    if (mm_flags & FF_MM_SSE)
        av_log(avctx, AV_LOG_INFO, " sse");
    if (mm_flags & FF_MM_SSE2)
        av_log(avctx, AV_LOG_INFO, " sse2");
    av_log(avctx, AV_LOG_INFO, "\n");
#endif

    if (mm_flags & FF_MM_MMX) {
        const int idct_algo= avctx->idct_algo;

        if(avctx->lowres==0){
            if(idct_algo==FF_IDCT_AUTO || idct_algo==FF_IDCT_SIMPLEMMX){
                c->idct_put= ff_simple_idct_put_mmx;
                c->idct_add= ff_simple_idct_add_mmx;
                c->idct    = ff_simple_idct_mmx;
                c->idct_permutation_type= FF_SIMPLE_IDCT_PERM;
#if CONFIG_GPL
            }else if(idct_algo==FF_IDCT_LIBMPEG2MMX){
                if(mm_flags & FF_MM_MMX2){
                    c->idct_put= ff_libmpeg2mmx2_idct_put;
                    c->idct_add= ff_libmpeg2mmx2_idct_add;
                    c->idct    = ff_mmxext_idct;
                }else{
                    c->idct_put= ff_libmpeg2mmx_idct_put;
                    c->idct_add= ff_libmpeg2mmx_idct_add;
                    c->idct    = ff_mmx_idct;
                }
                c->idct_permutation_type= FF_LIBMPEG2_IDCT_PERM;
#endif
            }else if((CONFIG_VP3_DECODER || CONFIG_VP5_DECODER || CONFIG_VP6_DECODER) &&
                     idct_algo==FF_IDCT_VP3){
                if(mm_flags & FF_MM_SSE2){
                    c->idct_put= ff_vp3_idct_put_sse2;
                    c->idct_add= ff_vp3_idct_add_sse2;
                    c->idct    = ff_vp3_idct_sse2;
                    c->idct_permutation_type= FF_TRANSPOSE_IDCT_PERM;
                }else{
                    c->idct_put= ff_vp3_idct_put_mmx;
                    c->idct_add= ff_vp3_idct_add_mmx;
                    c->idct    = ff_vp3_idct_mmx;
                    c->idct_permutation_type= FF_PARTTRANS_IDCT_PERM;
                }
            }else if(idct_algo==FF_IDCT_CAVS){
                    c->idct_permutation_type= FF_TRANSPOSE_IDCT_PERM;
            }else if(idct_algo==FF_IDCT_XVIDMMX){
                if(mm_flags & FF_MM_SSE2){
                    c->idct_put= ff_idct_xvid_sse2_put;
                    c->idct_add= ff_idct_xvid_sse2_add;
                    c->idct    = ff_idct_xvid_sse2;
                    c->idct_permutation_type= FF_SSE2_IDCT_PERM;
                }else if(mm_flags & FF_MM_MMX2){
                    c->idct_put= ff_idct_xvid_mmx2_put;
                    c->idct_add= ff_idct_xvid_mmx2_add;
                    c->idct    = ff_idct_xvid_mmx2;
                }else{
                    c->idct_put= ff_idct_xvid_mmx_put;
                    c->idct_add= ff_idct_xvid_mmx_add;
                    c->idct    = ff_idct_xvid_mmx;
                }
            }
        }

        c->put_pixels_clamped = put_pixels_clamped_mmx;
        c->put_signed_pixels_clamped = put_signed_pixels_clamped_mmx;
        c->add_pixels_clamped = add_pixels_clamped_mmx;
        c->clear_block  = clear_block_mmx;
        c->clear_blocks = clear_blocks_mmx;
        if ((mm_flags & FF_MM_SSE) &&
            !(CONFIG_MPEG_XVMC_DECODER && avctx->xvmc_acceleration > 1)){
            /* XvMCCreateBlocks() may not allocate 16-byte aligned blocks */
            c->clear_block  = clear_block_sse;
            c->clear_blocks = clear_blocks_sse;
        }

#define SET_HPEL_FUNCS(PFX, IDX, SIZE, CPU) \
        c->PFX ## _pixels_tab[IDX][0] = PFX ## _pixels ## SIZE ## _ ## CPU; \
        c->PFX ## _pixels_tab[IDX][1] = PFX ## _pixels ## SIZE ## _x2_ ## CPU; \
        c->PFX ## _pixels_tab[IDX][2] = PFX ## _pixels ## SIZE ## _y2_ ## CPU; \
        c->PFX ## _pixels_tab[IDX][3] = PFX ## _pixels ## SIZE ## _xy2_ ## CPU

        SET_HPEL_FUNCS(put, 0, 16, mmx);
        SET_HPEL_FUNCS(put_no_rnd, 0, 16, mmx);
        SET_HPEL_FUNCS(avg, 0, 16, mmx);
        SET_HPEL_FUNCS(avg_no_rnd, 0, 16, mmx);
        SET_HPEL_FUNCS(put, 1, 8, mmx);
        SET_HPEL_FUNCS(put_no_rnd, 1, 8, mmx);
        SET_HPEL_FUNCS(avg, 1, 8, mmx);
        SET_HPEL_FUNCS(avg_no_rnd, 1, 8, mmx);

        c->gmc= gmc_mmx;

        c->add_bytes= add_bytes_mmx;
        c->add_bytes_l2= add_bytes_l2_mmx;

        c->draw_edges = draw_edges_mmx;

        if (CONFIG_H263_DECODER || CONFIG_H263_ENCODER) {
            c->h263_v_loop_filter= h263_v_loop_filter_mmx;
            c->h263_h_loop_filter= h263_h_loop_filter_mmx;
        }
        c->put_h264_chroma_pixels_tab[0]= put_h264_chroma_mc8_mmx_rnd;
        c->put_h264_chroma_pixels_tab[1]= put_h264_chroma_mc4_mmx;
        c->put_no_rnd_vc1_chroma_pixels_tab[0]= put_vc1_chroma_mc8_mmx_nornd;

        c->put_rv40_chroma_pixels_tab[0]= put_rv40_chroma_mc8_mmx;
        c->put_rv40_chroma_pixels_tab[1]= put_rv40_chroma_mc4_mmx;

        if (CONFIG_VP6_DECODER) {
            c->vp6_filter_diag4 = ff_vp6_filter_diag4_mmx;
        }

        if (mm_flags & FF_MM_MMX2) {
            c->prefetch = prefetch_mmx2;

            c->put_pixels_tab[0][1] = put_pixels16_x2_mmx2;
            c->put_pixels_tab[0][2] = put_pixels16_y2_mmx2;

            c->avg_pixels_tab[0][0] = avg_pixels16_mmx2;
            c->avg_pixels_tab[0][1] = avg_pixels16_x2_mmx2;
            c->avg_pixels_tab[0][2] = avg_pixels16_y2_mmx2;

            c->put_pixels_tab[1][1] = put_pixels8_x2_mmx2;
            c->put_pixels_tab[1][2] = put_pixels8_y2_mmx2;

            c->avg_pixels_tab[1][0] = avg_pixels8_mmx2;
            c->avg_pixels_tab[1][1] = avg_pixels8_x2_mmx2;
            c->avg_pixels_tab[1][2] = avg_pixels8_y2_mmx2;

            if(!(avctx->flags & CODEC_FLAG_BITEXACT)){
                c->put_no_rnd_pixels_tab[0][1] = put_no_rnd_pixels16_x2_mmx2;
                c->put_no_rnd_pixels_tab[0][2] = put_no_rnd_pixels16_y2_mmx2;
                c->put_no_rnd_pixels_tab[1][1] = put_no_rnd_pixels8_x2_mmx2;
                c->put_no_rnd_pixels_tab[1][2] = put_no_rnd_pixels8_y2_mmx2;
                c->avg_pixels_tab[0][3] = avg_pixels16_xy2_mmx2;
                c->avg_pixels_tab[1][3] = avg_pixels8_xy2_mmx2;

                if (CONFIG_VP3_DECODER) {
                    c->vp3_v_loop_filter= ff_vp3_v_loop_filter_mmx2;
                    c->vp3_h_loop_filter= ff_vp3_h_loop_filter_mmx2;
                }
            }
            if (CONFIG_VP3_DECODER) {
                c->vp3_idct_dc_add = ff_vp3_idct_dc_add_mmx2;
            }

#define SET_QPEL_FUNCS(PFX, IDX, SIZE, CPU) \
            c->PFX ## _pixels_tab[IDX][ 0] = PFX ## SIZE ## _mc00_ ## CPU; \
            c->PFX ## _pixels_tab[IDX][ 1] = PFX ## SIZE ## _mc10_ ## CPU; \
            c->PFX ## _pixels_tab[IDX][ 2] = PFX ## SIZE ## _mc20_ ## CPU; \
            c->PFX ## _pixels_tab[IDX][ 3] = PFX ## SIZE ## _mc30_ ## CPU; \
            c->PFX ## _pixels_tab[IDX][ 4] = PFX ## SIZE ## _mc01_ ## CPU; \
            c->PFX ## _pixels_tab[IDX][ 5] = PFX ## SIZE ## _mc11_ ## CPU; \
            c->PFX ## _pixels_tab[IDX][ 6] = PFX ## SIZE ## _mc21_ ## CPU; \
            c->PFX ## _pixels_tab[IDX][ 7] = PFX ## SIZE ## _mc31_ ## CPU; \
            c->PFX ## _pixels_tab[IDX][ 8] = PFX ## SIZE ## _mc02_ ## CPU; \
            c->PFX ## _pixels_tab[IDX][ 9] = PFX ## SIZE ## _mc12_ ## CPU; \
            c->PFX ## _pixels_tab[IDX][10] = PFX ## SIZE ## _mc22_ ## CPU; \
            c->PFX ## _pixels_tab[IDX][11] = PFX ## SIZE ## _mc32_ ## CPU; \
            c->PFX ## _pixels_tab[IDX][12] = PFX ## SIZE ## _mc03_ ## CPU; \
            c->PFX ## _pixels_tab[IDX][13] = PFX ## SIZE ## _mc13_ ## CPU; \
            c->PFX ## _pixels_tab[IDX][14] = PFX ## SIZE ## _mc23_ ## CPU; \
            c->PFX ## _pixels_tab[IDX][15] = PFX ## SIZE ## _mc33_ ## CPU

            SET_QPEL_FUNCS(put_qpel, 0, 16, mmx2);
            SET_QPEL_FUNCS(put_qpel, 1, 8, mmx2);
            SET_QPEL_FUNCS(put_no_rnd_qpel, 0, 16, mmx2);
            SET_QPEL_FUNCS(put_no_rnd_qpel, 1, 8, mmx2);
            SET_QPEL_FUNCS(avg_qpel, 0, 16, mmx2);
            SET_QPEL_FUNCS(avg_qpel, 1, 8, mmx2);

            SET_QPEL_FUNCS(put_h264_qpel, 0, 16, mmx2);
            SET_QPEL_FUNCS(put_h264_qpel, 1, 8, mmx2);
            SET_QPEL_FUNCS(put_h264_qpel, 2, 4, mmx2);
            SET_QPEL_FUNCS(avg_h264_qpel, 0, 16, mmx2);
            SET_QPEL_FUNCS(avg_h264_qpel, 1, 8, mmx2);
            SET_QPEL_FUNCS(avg_h264_qpel, 2, 4, mmx2);

            SET_QPEL_FUNCS(put_2tap_qpel, 0, 16, mmx2);
            SET_QPEL_FUNCS(put_2tap_qpel, 1, 8, mmx2);
            SET_QPEL_FUNCS(avg_2tap_qpel, 0, 16, mmx2);
            SET_QPEL_FUNCS(avg_2tap_qpel, 1, 8, mmx2);

            c->avg_rv40_chroma_pixels_tab[0]= avg_rv40_chroma_mc8_mmx2;
            c->avg_rv40_chroma_pixels_tab[1]= avg_rv40_chroma_mc4_mmx2;

            c->avg_no_rnd_vc1_chroma_pixels_tab[0]= avg_vc1_chroma_mc8_mmx2_nornd;

            c->avg_h264_chroma_pixels_tab[0]= avg_h264_chroma_mc8_mmx2_rnd;
            c->avg_h264_chroma_pixels_tab[1]= avg_h264_chroma_mc4_mmx2;
            c->avg_h264_chroma_pixels_tab[2]= avg_h264_chroma_mc2_mmx2;
            c->put_h264_chroma_pixels_tab[2]= put_h264_chroma_mc2_mmx2;

#if HAVE_YASM
            c->add_hfyu_median_prediction = ff_add_hfyu_median_prediction_mmx2;
#endif
#if HAVE_7REGS && HAVE_TEN_OPERANDS
            if( mm_flags&FF_MM_3DNOW )
                c->add_hfyu_median_prediction = add_hfyu_median_prediction_cmov;
#endif

            if (CONFIG_CAVS_DECODER)
                ff_cavsdsp_init_mmx2(c, avctx);

            if (CONFIG_VC1_DECODER)
                ff_vc1dsp_init_mmx(c, avctx);

            c->add_png_paeth_prediction= add_png_paeth_prediction_mmx2;
        } else if (mm_flags & FF_MM_3DNOW) {
            c->prefetch = prefetch_3dnow;

            c->put_pixels_tab[0][1] = put_pixels16_x2_3dnow;
            c->put_pixels_tab[0][2] = put_pixels16_y2_3dnow;

            c->avg_pixels_tab[0][0] = avg_pixels16_3dnow;
            c->avg_pixels_tab[0][1] = avg_pixels16_x2_3dnow;
            c->avg_pixels_tab[0][2] = avg_pixels16_y2_3dnow;

            c->put_pixels_tab[1][1] = put_pixels8_x2_3dnow;
            c->put_pixels_tab[1][2] = put_pixels8_y2_3dnow;

            c->avg_pixels_tab[1][0] = avg_pixels8_3dnow;
            c->avg_pixels_tab[1][1] = avg_pixels8_x2_3dnow;
            c->avg_pixels_tab[1][2] = avg_pixels8_y2_3dnow;

            if(!(avctx->flags & CODEC_FLAG_BITEXACT)){
                c->put_no_rnd_pixels_tab[0][1] = put_no_rnd_pixels16_x2_3dnow;
                c->put_no_rnd_pixels_tab[0][2] = put_no_rnd_pixels16_y2_3dnow;
                c->put_no_rnd_pixels_tab[1][1] = put_no_rnd_pixels8_x2_3dnow;
                c->put_no_rnd_pixels_tab[1][2] = put_no_rnd_pixels8_y2_3dnow;
                c->avg_pixels_tab[0][3] = avg_pixels16_xy2_3dnow;
                c->avg_pixels_tab[1][3] = avg_pixels8_xy2_3dnow;
            }

            SET_QPEL_FUNCS(put_qpel, 0, 16, 3dnow);
            SET_QPEL_FUNCS(put_qpel, 1, 8, 3dnow);
            SET_QPEL_FUNCS(put_no_rnd_qpel, 0, 16, 3dnow);
            SET_QPEL_FUNCS(put_no_rnd_qpel, 1, 8, 3dnow);
            SET_QPEL_FUNCS(avg_qpel, 0, 16, 3dnow);
            SET_QPEL_FUNCS(avg_qpel, 1, 8, 3dnow);

            SET_QPEL_FUNCS(put_h264_qpel, 0, 16, 3dnow);
            SET_QPEL_FUNCS(put_h264_qpel, 1, 8, 3dnow);
            SET_QPEL_FUNCS(put_h264_qpel, 2, 4, 3dnow);
            SET_QPEL_FUNCS(avg_h264_qpel, 0, 16, 3dnow);
            SET_QPEL_FUNCS(avg_h264_qpel, 1, 8, 3dnow);
            SET_QPEL_FUNCS(avg_h264_qpel, 2, 4, 3dnow);

            SET_QPEL_FUNCS(put_2tap_qpel, 0, 16, 3dnow);
            SET_QPEL_FUNCS(put_2tap_qpel, 1, 8, 3dnow);
            SET_QPEL_FUNCS(avg_2tap_qpel, 0, 16, 3dnow);
            SET_QPEL_FUNCS(avg_2tap_qpel, 1, 8, 3dnow);

            c->avg_h264_chroma_pixels_tab[0]= avg_h264_chroma_mc8_3dnow_rnd;
            c->avg_h264_chroma_pixels_tab[1]= avg_h264_chroma_mc4_3dnow;

            c->avg_rv40_chroma_pixels_tab[0]= avg_rv40_chroma_mc8_3dnow;
            c->avg_rv40_chroma_pixels_tab[1]= avg_rv40_chroma_mc4_3dnow;

            if (CONFIG_CAVS_DECODER)
                ff_cavsdsp_init_3dnow(c, avctx);
        }


#define H264_QPEL_FUNCS(x, y, CPU)\
            c->put_h264_qpel_pixels_tab[0][x+y*4] = put_h264_qpel16_mc##x##y##_##CPU;\
            c->put_h264_qpel_pixels_tab[1][x+y*4] = put_h264_qpel8_mc##x##y##_##CPU;\
            c->avg_h264_qpel_pixels_tab[0][x+y*4] = avg_h264_qpel16_mc##x##y##_##CPU;\
            c->avg_h264_qpel_pixels_tab[1][x+y*4] = avg_h264_qpel8_mc##x##y##_##CPU;
        if((mm_flags & FF_MM_SSE2) && !(mm_flags & FF_MM_3DNOW)){
            // these functions are slower than mmx on AMD, but faster on Intel
            c->put_pixels_tab[0][0] = put_pixels16_sse2;
            c->avg_pixels_tab[0][0] = avg_pixels16_sse2;
            H264_QPEL_FUNCS(0, 0, sse2);
        }
        if(mm_flags & FF_MM_SSE2){
            H264_QPEL_FUNCS(0, 1, sse2);
            H264_QPEL_FUNCS(0, 2, sse2);
            H264_QPEL_FUNCS(0, 3, sse2);
            H264_QPEL_FUNCS(1, 1, sse2);
            H264_QPEL_FUNCS(1, 2, sse2);
            H264_QPEL_FUNCS(1, 3, sse2);
            H264_QPEL_FUNCS(2, 1, sse2);
            H264_QPEL_FUNCS(2, 2, sse2);
            H264_QPEL_FUNCS(2, 3, sse2);
            H264_QPEL_FUNCS(3, 1, sse2);
            H264_QPEL_FUNCS(3, 2, sse2);
            H264_QPEL_FUNCS(3, 3, sse2);

            if (CONFIG_VP6_DECODER) {
                c->vp6_filter_diag4 = ff_vp6_filter_diag4_sse2;
            }
        }
#if HAVE_SSSE3
        if(mm_flags & FF_MM_SSSE3){
            H264_QPEL_FUNCS(1, 0, ssse3);
            H264_QPEL_FUNCS(1, 1, ssse3);
            H264_QPEL_FUNCS(1, 2, ssse3);
            H264_QPEL_FUNCS(1, 3, ssse3);
            H264_QPEL_FUNCS(2, 0, ssse3);
            H264_QPEL_FUNCS(2, 1, ssse3);
            H264_QPEL_FUNCS(2, 2, ssse3);
            H264_QPEL_FUNCS(2, 3, ssse3);
            H264_QPEL_FUNCS(3, 0, ssse3);
            H264_QPEL_FUNCS(3, 1, ssse3);
            H264_QPEL_FUNCS(3, 2, ssse3);
            H264_QPEL_FUNCS(3, 3, ssse3);
            c->put_no_rnd_vc1_chroma_pixels_tab[0]= put_vc1_chroma_mc8_ssse3_nornd;
            c->avg_no_rnd_vc1_chroma_pixels_tab[0]= avg_vc1_chroma_mc8_ssse3_nornd;
            c->put_h264_chroma_pixels_tab[0]= put_h264_chroma_mc8_ssse3_rnd;
            c->avg_h264_chroma_pixels_tab[0]= avg_h264_chroma_mc8_ssse3_rnd;
            c->put_h264_chroma_pixels_tab[1]= put_h264_chroma_mc4_ssse3;
            c->avg_h264_chroma_pixels_tab[1]= avg_h264_chroma_mc4_ssse3;
            c->add_png_paeth_prediction= add_png_paeth_prediction_ssse3;
#if HAVE_YASM
            c->add_hfyu_left_prediction = ff_add_hfyu_left_prediction_ssse3;
            if (mm_flags & FF_MM_SSE4) // not really sse4, just slow on Conroe
                c->add_hfyu_left_prediction = ff_add_hfyu_left_prediction_sse4;
#endif
        }
#endif

        if(mm_flags & FF_MM_3DNOW){
            c->vorbis_inverse_coupling = vorbis_inverse_coupling_3dnow;
            c->vector_fmul = vector_fmul_3dnow;
            if(!(avctx->flags & CODEC_FLAG_BITEXACT)){
                c->float_to_int16 = float_to_int16_3dnow;
                c->float_to_int16_interleave = float_to_int16_interleave_3dnow;
            }
        }
        if(mm_flags & FF_MM_3DNOWEXT){
            c->vector_fmul_reverse = vector_fmul_reverse_3dnow2;
            c->vector_fmul_window = vector_fmul_window_3dnow2;
            if(!(avctx->flags & CODEC_FLAG_BITEXACT)){
                c->float_to_int16_interleave = float_to_int16_interleave_3dn2;
            }
        }
        if(mm_flags & FF_MM_MMX2){
#if HAVE_YASM
            c->scalarproduct_int16 = ff_scalarproduct_int16_mmx2;
            c->scalarproduct_and_madd_int16 = ff_scalarproduct_and_madd_int16_mmx2;
#endif
        }
        if(mm_flags & FF_MM_SSE){
            c->vorbis_inverse_coupling = vorbis_inverse_coupling_sse;
            c->ac3_downmix = ac3_downmix_sse;
            c->vector_fmul = vector_fmul_sse;
            c->vector_fmul_reverse = vector_fmul_reverse_sse;
            c->vector_fmul_add = vector_fmul_add_sse;
            c->vector_fmul_window = vector_fmul_window_sse;
            c->int32_to_float_fmul_scalar = int32_to_float_fmul_scalar_sse;
            c->vector_clipf = vector_clipf_sse;
            c->float_to_int16 = float_to_int16_sse;
            c->float_to_int16_interleave = float_to_int16_interleave_sse;
#if HAVE_YASM
            c->scalarproduct_float = ff_scalarproduct_float_sse;
#endif
        }
        if(mm_flags & FF_MM_3DNOW)
            c->vector_fmul_add = vector_fmul_add_3dnow; // faster than sse
        if(mm_flags & FF_MM_SSE2){
            c->int32_to_float_fmul_scalar = int32_to_float_fmul_scalar_sse2;
            c->float_to_int16 = float_to_int16_sse2;
            c->float_to_int16_interleave = float_to_int16_interleave_sse2;
#if HAVE_YASM
            c->scalarproduct_int16 = ff_scalarproduct_int16_sse2;
            c->scalarproduct_and_madd_int16 = ff_scalarproduct_and_madd_int16_sse2;
#endif
        }
        if((mm_flags & FF_MM_SSSE3) && !(mm_flags & (FF_MM_SSE42|FF_MM_3DNOW)) && HAVE_YASM) // cachesplit
            c->scalarproduct_and_madd_int16 = ff_scalarproduct_and_madd_int16_ssse3;
    }

    if (CONFIG_ENCODERS)
        dsputilenc_init_mmx(c, avctx);

#if 0
    // for speed testing
    get_pixels = just_return;
    put_pixels_clamped = just_return;
    add_pixels_clamped = just_return;

    pix_abs16x16 = just_return;
    pix_abs16x16_x2 = just_return;
    pix_abs16x16_y2 = just_return;
    pix_abs16x16_xy2 = just_return;

    put_pixels_tab[0] = just_return;
    put_pixels_tab[1] = just_return;
    put_pixels_tab[2] = just_return;
    put_pixels_tab[3] = just_return;

    put_no_rnd_pixels_tab[0] = just_return;
    put_no_rnd_pixels_tab[1] = just_return;
    put_no_rnd_pixels_tab[2] = just_return;
    put_no_rnd_pixels_tab[3] = just_return;

    avg_pixels_tab[0] = just_return;
    avg_pixels_tab[1] = just_return;
    avg_pixels_tab[2] = just_return;
    avg_pixels_tab[3] = just_return;

    avg_no_rnd_pixels_tab[0] = just_return;
    avg_no_rnd_pixels_tab[1] = just_return;
    avg_no_rnd_pixels_tab[2] = just_return;
    avg_no_rnd_pixels_tab[3] = just_return;

    //av_fdct = just_return;
    //ff_idct = just_return;
#endif
}

#if CONFIG_H264DSP
void ff_h264dsp_init_x86(H264DSPContext *c)
{
    mm_flags = mm_support();

    if (mm_flags & FF_MM_MMX) {
        c->h264_idct_dc_add=
        c->h264_idct_add= ff_h264_idct_add_mmx;
        c->h264_idct8_dc_add=
        c->h264_idct8_add= ff_h264_idct8_add_mmx;

        c->h264_idct_add16     = ff_h264_idct_add16_mmx;
        c->h264_idct8_add4     = ff_h264_idct8_add4_mmx;
        c->h264_idct_add8      = ff_h264_idct_add8_mmx;
        c->h264_idct_add16intra= ff_h264_idct_add16intra_mmx;

        if (mm_flags & FF_MM_MMX2) {
            c->h264_idct_dc_add= ff_h264_idct_dc_add_mmx2;
            c->h264_idct8_dc_add= ff_h264_idct8_dc_add_mmx2;
            c->h264_idct_add16     = ff_h264_idct_add16_mmx2;
            c->h264_idct8_add4     = ff_h264_idct8_add4_mmx2;
            c->h264_idct_add8      = ff_h264_idct_add8_mmx2;
            c->h264_idct_add16intra= ff_h264_idct_add16intra_mmx2;

            c->h264_v_loop_filter_luma= h264_v_loop_filter_luma_mmx2;
            c->h264_h_loop_filter_luma= h264_h_loop_filter_luma_mmx2;
            c->h264_v_loop_filter_chroma= h264_v_loop_filter_chroma_mmx2;
            c->h264_h_loop_filter_chroma= h264_h_loop_filter_chroma_mmx2;
            c->h264_v_loop_filter_chroma_intra= h264_v_loop_filter_chroma_intra_mmx2;
            c->h264_h_loop_filter_chroma_intra= h264_h_loop_filter_chroma_intra_mmx2;
            c->h264_loop_filter_strength= h264_loop_filter_strength_mmx2;

            c->weight_h264_pixels_tab[0]= ff_h264_weight_16x16_mmx2;
            c->weight_h264_pixels_tab[1]= ff_h264_weight_16x8_mmx2;
            c->weight_h264_pixels_tab[2]= ff_h264_weight_8x16_mmx2;
            c->weight_h264_pixels_tab[3]= ff_h264_weight_8x8_mmx2;
            c->weight_h264_pixels_tab[4]= ff_h264_weight_8x4_mmx2;
            c->weight_h264_pixels_tab[5]= ff_h264_weight_4x8_mmx2;
            c->weight_h264_pixels_tab[6]= ff_h264_weight_4x4_mmx2;
            c->weight_h264_pixels_tab[7]= ff_h264_weight_4x2_mmx2;

            c->biweight_h264_pixels_tab[0]= ff_h264_biweight_16x16_mmx2;
            c->biweight_h264_pixels_tab[1]= ff_h264_biweight_16x8_mmx2;
            c->biweight_h264_pixels_tab[2]= ff_h264_biweight_8x16_mmx2;
            c->biweight_h264_pixels_tab[3]= ff_h264_biweight_8x8_mmx2;
            c->biweight_h264_pixels_tab[4]= ff_h264_biweight_8x4_mmx2;
            c->biweight_h264_pixels_tab[5]= ff_h264_biweight_4x8_mmx2;
            c->biweight_h264_pixels_tab[6]= ff_h264_biweight_4x4_mmx2;
            c->biweight_h264_pixels_tab[7]= ff_h264_biweight_4x2_mmx2;
        }
        if(mm_flags & FF_MM_SSE2){
            c->h264_idct8_add = ff_h264_idct8_add_sse2;
            c->h264_idct8_add4= ff_h264_idct8_add4_sse2;
        }

#if CONFIG_GPL && HAVE_YASM
        if (mm_flags & FF_MM_MMX2){
#if ARCH_X86_32
            c->h264_v_loop_filter_luma_intra = ff_x264_deblock_v_luma_intra_mmxext;
            c->h264_h_loop_filter_luma_intra = ff_x264_deblock_h_luma_intra_mmxext;
#endif
            if( mm_flags&FF_MM_SSE2 ){
#if ARCH_X86_64 || !defined(__ICC) || __ICC > 1110
                c->h264_v_loop_filter_luma = ff_x264_deblock_v_luma_sse2;
                c->h264_h_loop_filter_luma = ff_x264_deblock_h_luma_sse2;
                c->h264_v_loop_filter_luma_intra = ff_x264_deblock_v_luma_intra_sse2;
                c->h264_h_loop_filter_luma_intra = ff_x264_deblock_h_luma_intra_sse2;
#endif
                c->h264_idct_add16 = ff_h264_idct_add16_sse2;
                c->h264_idct_add8  = ff_h264_idct_add8_sse2;
                c->h264_idct_add16intra = ff_h264_idct_add16intra_sse2;
            }
        }
#endif
    }
}
#endif /* CONFIG_H264DSP */
