/*
 * copyright (c) 2007 Bobby Bingham
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
 * video vertical flip filter
 */

#include "libavutil/pixdesc.h"
#include "avfilter.h"

typedef struct {
    int vsub;   ///< vertical chroma subsampling
} FlipContext;

static int config_input(AVFilterLink *link)
{
    FlipContext *flip = link->dst->priv;

    flip->vsub = av_pix_fmt_descriptors[link->format].log2_chroma_h;

    return 0;
}

static AVFilterPicRef *get_video_buffer(AVFilterLink *link, int perms,
                                        int w, int h)
{
    FlipContext *flip = link->dst->priv;
    int i;

    AVFilterPicRef *picref = avfilter_get_video_buffer(link->dst->outputs[0],
                                                       perms, w, h);

    for (i = 0; i < 4; i ++) {
        int vsub = i == 1 || i == 2 ? flip->vsub : 0;

        if (picref->data[i]) {
            picref->data[i] += ((h >> vsub)-1) * picref->linesize[i];
            picref->linesize[i] = -picref->linesize[i];
        }
    }

    return picref;
}

static void start_frame(AVFilterLink *link, AVFilterPicRef *picref)
{
    FlipContext *flip = link->dst->priv;
    int i;

    for (i = 0; i < 4; i ++) {
        int vsub = i == 1 || i == 2 ? flip->vsub : 0;

        if (picref->data[i]) {
            picref->data[i] += ((link->h >> vsub)-1) * picref->linesize[i];
            picref->linesize[i] = -picref->linesize[i];
        }
    }

    avfilter_start_frame(link->dst->outputs[0], picref);
}

static void draw_slice(AVFilterLink *link, int y, int h, int slice_dir)
{
    AVFilterContext *ctx = link->dst;

    avfilter_draw_slice(ctx->outputs[0], link->h - (y+h), h, -1 * slice_dir);
}

AVFilter avfilter_vf_vflip = {
    .name      = "vflip",
    .description = NULL_IF_CONFIG_SMALL("Flip the input video vertically."),

    .priv_size = sizeof(FlipContext),

    .inputs    = (AVFilterPad[]) {{ .name             = "default",
                                    .type             = AVMEDIA_TYPE_VIDEO,
                                    .get_video_buffer = get_video_buffer,
                                    .start_frame      = start_frame,
                                    .draw_slice       = draw_slice,
                                    .end_frame        = avfilter_null_end_frame,
                                    .config_props     = config_input, },
                                  { .name = NULL}},
    .outputs   = (AVFilterPad[]) {{ .name             = "default",
                                    .type             = AVMEDIA_TYPE_VIDEO, },
                                  { .name = NULL}},
};
