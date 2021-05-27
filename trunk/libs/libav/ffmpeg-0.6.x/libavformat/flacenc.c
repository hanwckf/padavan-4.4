/*
 * raw FLAC muxer
 * Copyright (c) 2006-2009 Justin Ruggles
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

#include "libavcodec/flac.h"
#include "avformat.h"
#include "flacenc.h"
#include "metadata.h"
#include "vorbiscomment.h"
#include "libavcodec/bytestream.h"


static int flac_write_block_padding(ByteIOContext *pb, unsigned int n_padding_bytes,
                                    int last_block)
{
    put_byte(pb, last_block ? 0x81 : 0x01);
    put_be24(pb, n_padding_bytes);
    while (n_padding_bytes > 0) {
        put_byte(pb, 0);
        n_padding_bytes--;
    }
    return 0;
}

static int flac_write_block_comment(ByteIOContext *pb, AVMetadata *m,
                                    int last_block, int bitexact)
{
    const char *vendor = bitexact ? "ffmpeg" : LIBAVFORMAT_IDENT;
    unsigned int len, count;
    uint8_t *p, *p0;

    len = ff_vorbiscomment_length(m, vendor, &count);
    p0 = av_malloc(len+4);
    if (!p0)
        return AVERROR(ENOMEM);
    p = p0;

    bytestream_put_byte(&p, last_block ? 0x84 : 0x04);
    bytestream_put_be24(&p, len);
    ff_vorbiscomment_write(&p, m, vendor, count);

    put_buffer(pb, p0, len+4);
    av_freep(&p0);
    p = NULL;

    return 0;
}

static int flac_write_header(struct AVFormatContext *s)
{
    int ret;
    AVCodecContext *codec = s->streams[0]->codec;

    ret = ff_flac_write_header(s->pb, codec, 0);
    if (ret)
        return ret;

    ret = flac_write_block_comment(s->pb, s->metadata, 0,
                                   codec->flags & CODEC_FLAG_BITEXACT);
    if (ret)
        return ret;

    /* The command line flac encoder defaults to placing a seekpoint
     * every 10s.  So one might add padding to allow that later
     * but there seems to be no simple way to get the duration here.
     * So let's try the flac default of 8192 bytes */
    flac_write_block_padding(s->pb, 8192, 1);

    return ret;
}

static int flac_write_trailer(struct AVFormatContext *s)
{
    ByteIOContext *pb = s->pb;
    uint8_t *streaminfo;
    enum FLACExtradataFormat format;
    int64_t file_size;

    if (!ff_flac_is_extradata_valid(s->streams[0]->codec, &format, &streaminfo))
        return -1;

    if (!url_is_streamed(pb)) {
        /* rewrite the STREAMINFO header block data */
        file_size = url_ftell(pb);
        url_fseek(pb, 8, SEEK_SET);
        put_buffer(pb, streaminfo, FLAC_STREAMINFO_SIZE);
        url_fseek(pb, file_size, SEEK_SET);
        put_flush_packet(pb);
    } else {
        av_log(s, AV_LOG_WARNING, "unable to rewrite FLAC header.\n");
    }
    return 0;
}

static int flac_write_packet(struct AVFormatContext *s, AVPacket *pkt)
{
    put_buffer(s->pb, pkt->data, pkt->size);
    put_flush_packet(s->pb);
    return 0;
}

AVOutputFormat flac_muxer = {
    "flac",
    NULL_IF_CONFIG_SMALL("raw FLAC"),
    "audio/x-flac",
    "flac",
    0,
    CODEC_ID_FLAC,
    CODEC_ID_NONE,
    flac_write_header,
    flac_write_packet,
    flac_write_trailer,
    .flags= AVFMT_NOTIMESTAMPS,
    .metadata_conv = ff_vorbiscomment_metadata_conv,
};
