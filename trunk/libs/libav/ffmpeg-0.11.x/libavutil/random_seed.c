/*
 * Copyright (c) 2009 Baptiste Coudurier <baptiste.coudurier@gmail.com>
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

#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>
#include "timer.h"
#include "random_seed.h"

static int read_random(uint32_t *dst, const char *file)
{
    int fd = open(file, O_RDONLY);
    int err = -1;

    if (fd == -1)
        return -1;
    err = read(fd, dst, sizeof(*dst));
    close(fd);

    return err;
}

static uint32_t get_generic_seed(void)
{
    clock_t last_t  = 0;
    int bits        = 0;
    uint64_t random = 0;
    unsigned i;
    float s = 0.000000000001;

    for (i = 0; bits < 64; i++) {
        clock_t t = clock();
        if (last_t && fabs(t - last_t) > s || t == (clock_t) -1) {
            if (i < 10000 && s < (1 << 24)) {
                s += s;
                i = t = 0;
            } else {
                random = 2 * random + (i & 1);
                bits++;
            }
        }
        last_t = t;
    }
#ifdef AV_READ_TIME
    random ^= AV_READ_TIME();
#else
    random ^= clock();
#endif

    random += random >> 32;

    return random;
}

uint32_t av_get_random_seed(void)
{
    uint32_t seed;

    if (read_random(&seed, "/dev/urandom") == sizeof(seed))
        return seed;
    if (read_random(&seed, "/dev/random")  == sizeof(seed))
        return seed;
    return get_generic_seed();
}

#ifdef TEST
#undef printf
#define N 256
#include <stdio.h>

int main(void)
{
    int i, j, retry;
    uint32_t seeds[N];

    for (retry=0; retry<3; retry++){
        for (i=0; i<N; i++){
            seeds[i] = av_get_random_seed();
            for (j=0; j<i; j++)
                if (seeds[j] == seeds[i])
                    goto retry;
        }
        printf("seeds OK\n");
        return 0;
        retry:;
    }
    printf("FAIL at %d with %X\n", j, seeds[j]);
    return 1;
}
#endif
