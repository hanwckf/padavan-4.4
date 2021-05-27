/*
 * Generate a header file for hardcoded MDCT tables
 *
 * Copyright (c) 2009 Reimar Döffinger <Reimar.Doeffinger@gmx.de>
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

#include <stdlib.h>
#define CONFIG_HARDCODED_TABLES 0
#define SINETABLE_CONST
#define SINETABLE(size) \
    float ff_sine_##size[size]
#define FF_ARRAY_ELEMS(a) (sizeof(a) / sizeof((a)[0]))
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include "mdct_tablegen.h"
#include "tableprint.h"

int main(void)
{
    int i;

    write_fileheader();

    for (i = 5; i <= 12; i++) {
        ff_init_ff_sine_windows(i);
        printf("SINETABLE(%4i) = {\n", 1 << i);
        write_float_array(ff_sine_windows[i], 1 << i);
        printf("};\n");
    }

    return 0;
}
