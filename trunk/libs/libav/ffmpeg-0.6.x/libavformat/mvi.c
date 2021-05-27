/*
 * Motion Pixels MVI Demuxer
 * Copyright (c) 2008 Gregory Montoir (cyx@users.sourceforge.net)
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

#include "avformat.h"

#define MVI_FRAC_BITS 10

#define MVI_AUDIO_STREAM_INDEX 0
#define MVI_VIDEO_STREAM_INDEX 1

typedef struct MviDemuxContext {
    unsigned int (*get_int)(ByteIOContext *);
    uint32_t audio_data_size;
    uint64_t audio_size_counter;
    uint64_t audio_frame_size;
    int audio_size_left;
    int video_frame_size;
} MviDemuxContext;

static int read_header(AVFormatContext *s, AVFormatParameters *ap)
{
    MviDemuxContext *mvi = s->priv_data;
    ByteIOContext *pb = s->pb;
    AVStream *ast, *vst;
    unsigned int version, frames_count, msecs_per_frame, player_version;

    ast = av_new_stream(s, 0);
    if (!ast)
        return AVERROR(ENOMEM);

    vst = av_new_stream(s, 0);
    if (!vst)
        return AVERROR(ENOMEM);

    vst->codec->extradata_size = 2;
    vst->codec->extradata = av_mallocz(2 + FF_INPUT_BUFFER_PADDING_SIZE);

    version                  = get_byte(pb);
    vst->codec->extradata[0] = get_byte(pb);
    vst->codec->extradata[1] = get_byte(pb);
    frames_count             = get_le32(pb);
    msecs_per_frame          = get_le32(pb);
    vst->codec->width        = get_le16(pb);
    vst->codec->height       = get_le16(pb);
    get_byte(pb);
    ast->codec->sample_rate  = get_le16(pb);
    mvi->audio_data_size     = get_le32(pb);
    get_byte(pb);
    player_version           = get_le32(pb);
    get_le16(pb);
    get_byte(pb);

    if (frames_count == 0 || mvi->audio_data_size == 0)
        return AVERROR_INVALIDDATA;

    if (version != 7 || player_version > 213) {
        av_log(s, AV_LOG_ERROR, "unhandled version (%d,%d)\n", version, player_version);
        return AVERROR_INVALIDDATA;
    }

    av_set_pts_info(ast, 64, 1, ast->codec->sample_rate);
    ast->codec->codec_type      = AVMEDIA_TYPE_AUDIO;
    ast->codec->codec_id        = CODEC_ID_PCM_U8;
    ast->codec->channels        = 1;
    ast->codec->bits_per_coded_sample = 8;
    ast->codec->bit_rate        = ast->codec->sample_rate * 8;

    av_set_pts_info(vst, 64, msecs_per_frame, 1000000);
    vst->codec->codec_type = AVMEDIA_TYPE_VIDEO;
    vst->codec->codec_id   = CODEC_ID_MOTIONPIXELS;

    mvi->get_int = (vst->codec->width * vst->codec->height < (1 << 16)) ? get_le16 : get_le24;

    mvi->audio_frame_size   = ((uint64_t)mvi->audio_data_size << MVI_FRAC_BITS) / frames_count;
    mvi->audio_size_counter = (ast->codec->sample_rate * 830 / mvi->audio_frame_size - 1) * mvi->audio_frame_size;
    mvi->audio_size_left    = mvi->audio_data_size;

    return 0;
}

static int read_packet(AVFormatContext *s, AVPacket *pkt)
{
    int ret, count;
    MviDemuxContext *mvi = s->priv_data;
    ByteIOContext *pb = s->pb;

    if (mvi->video_frame_size == 0) {
        mvi->video_frame_size = (mvi->get_int)(pb);
        if (mvi->audio_size_left == 0)
            return AVERROR(EIO);
        count = (mvi->audio_size_counter + mvi->audio_frame_size + 512) >> MVI_FRAC_BITS;
        if (count > mvi->audio_size_left)
            count = mvi->audio_size_left;
        if ((ret = av_get_packet(pb, pkt, count)) < 0)
            return ret;
        pkt->stream_index = MVI_AUDIO_STREAM_INDEX;
        mvi->audio_size_left -= count;
        mvi->audio_size_counter += mvi->audio_frame_size - (count << MVI_FRAC_BITS);
    } else {
        if ((ret = av_get_packet(pb, pkt, mvi->video_frame_size)) < 0)
            return ret;
        pkt->stream_index = MVI_VIDEO_STREAM_INDEX;
        mvi->video_frame_size = 0;
    }
    return 0;
}

AVInputFormat mvi_demuxer = {
    "mvi",
    NULL_IF_CONFIG_SMALL("Motion Pixels MVI format"),
    sizeof(MviDemuxContext),
    NULL,
    read_header,
    read_packet,
    .extensions = "mvi"
};
