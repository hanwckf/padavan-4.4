/*
 * VQF demuxer
 * Copyright (c) 2009 Vitor Sessak
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
#include "libavutil/intreadwrite.h"

typedef struct VqfContext {
    int frame_bit_len;
    uint8_t last_frame_bits;
    int remaining_bits;
} VqfContext;

static int vqf_probe(AVProbeData *probe_packet)
{
    if (AV_RL32(probe_packet->buf) != MKTAG('T','W','I','N'))
        return 0;

    if (!memcmp(probe_packet->buf + 4, "97012000", 8))
        return AVPROBE_SCORE_MAX;

    if (!memcmp(probe_packet->buf + 4, "00052200", 8))
        return AVPROBE_SCORE_MAX;

    return AVPROBE_SCORE_MAX/2;
}

static void add_metadata(AVFormatContext *s, const char *tag,
                         unsigned int tag_len, unsigned int remaining)
{
    int len = FFMIN(tag_len, remaining);
    char *buf;

    if (len == UINT_MAX)
        return;

    buf = av_malloc(len+1);
    if (!buf)
        return;
    get_buffer(s->pb, buf, len);
    buf[len] = 0;
    av_metadata_set2(&s->metadata, tag, buf, AV_METADATA_DONT_STRDUP_VAL);
}

static int vqf_read_header(AVFormatContext *s, AVFormatParameters *ap)
{
    VqfContext *c = s->priv_data;
    AVStream *st  = av_new_stream(s, 0);
    int chunk_tag;
    int rate_flag = -1;
    int header_size;
    int read_bitrate = 0;
    int size;

    if (!st)
        return AVERROR(ENOMEM);

    url_fskip(s->pb, 12);

    header_size = get_be32(s->pb);

    st->codec->codec_type = AVMEDIA_TYPE_AUDIO;
    st->codec->codec_id   = CODEC_ID_TWINVQ;
    st->start_time = 0;

    do {
        int len;
        chunk_tag = get_le32(s->pb);

        if (chunk_tag == MKTAG('D','A','T','A'))
            break;

        len = get_be32(s->pb);

        if ((unsigned) len > INT_MAX/2) {
            av_log(s, AV_LOG_ERROR, "Malformed header\n");
            return -1;
        }

        header_size -= 8;

        switch(chunk_tag){
        case MKTAG('C','O','M','M'):
            st->codec->channels = get_be32(s->pb) + 1;
            read_bitrate        = get_be32(s->pb);
            rate_flag           = get_be32(s->pb);
            url_fskip(s->pb, len-12);

            st->codec->bit_rate              = read_bitrate*1000;
            st->codec->bits_per_coded_sample = 16;
            break;
        case MKTAG('N','A','M','E'):
            add_metadata(s, "title"    , len, header_size);
            break;
        case MKTAG('(','c',')',' '):
            add_metadata(s, "copyright", len, header_size);
            break;
        case MKTAG('A','U','T','H'):
            add_metadata(s, "author"   , len, header_size);
            break;
        case MKTAG('A','L','B','M'):
            add_metadata(s, "album"    , len, header_size);
            break;
        case MKTAG('T','R','C','K'):
            add_metadata(s, "track"    , len, header_size);
            break;
        case MKTAG('C','O','M','T'):
            add_metadata(s, "comment"  , len, header_size);
            break;
        case MKTAG('F','I','L','E'):
            add_metadata(s, "filename" , len, header_size);
            break;
        case MKTAG('D','S','I','Z'):
            add_metadata(s, "size"     , len, header_size);
            break;
        case MKTAG('D','A','T','E'):
            add_metadata(s, "date"     , len, header_size);
            break;
        case MKTAG('G','E','N','R'):
            add_metadata(s, "genre"    , len, header_size);
            break;
        default:
            av_log(s, AV_LOG_ERROR, "Unknown chunk: %c%c%c%c\n",
                   ((char*)&chunk_tag)[0], ((char*)&chunk_tag)[1],
                   ((char*)&chunk_tag)[2], ((char*)&chunk_tag)[3]);
            url_fskip(s->pb, FFMIN(len, header_size));
            break;
        }

        header_size -= len;

    } while (header_size >= 0);

    switch (rate_flag) {
    case -1:
        av_log(s, AV_LOG_ERROR, "COMM tag not found!\n");
        return -1;
    case 44:
        st->codec->sample_rate = 44100;
        break;
    case 22:
        st->codec->sample_rate = 22050;
        break;
    case 11:
        st->codec->sample_rate = 11025;
        break;
    default:
        st->codec->sample_rate = rate_flag*1000;
        break;
    }

    switch (((st->codec->sample_rate/1000) << 8) +
            read_bitrate/st->codec->channels) {
    case (11<<8) + 8 :
    case (8 <<8) + 8 :
    case (11<<8) + 10:
    case (22<<8) + 32:
        size = 512;
        break;
    case (16<<8) + 16:
    case (22<<8) + 20:
    case (22<<8) + 24:
        size = 1024;
        break;
    case (44<<8) + 40:
    case (44<<8) + 48:
        size = 2048;
        break;
    default:
        av_log(s, AV_LOG_ERROR, "Mode not suported: %d Hz, %d kb/s.\n",
               st->codec->sample_rate, st->codec->bit_rate);
        return -1;
    }
    c->frame_bit_len = st->codec->bit_rate*size/st->codec->sample_rate;
    av_set_pts_info(st, 64, 1, st->codec->sample_rate);

    return 0;
}

static int vqf_read_packet(AVFormatContext *s, AVPacket *pkt)
{
    VqfContext *c = s->priv_data;
    int ret;
    int size = (c->frame_bit_len - c->remaining_bits + 7)>>3;

    pkt->pos          = url_ftell(s->pb);
    pkt->stream_index = 0;

    if (av_new_packet(pkt, size+2) < 0)
        return AVERROR(EIO);

    pkt->data[0] = 8 - c->remaining_bits; // Number of bits to skip
    pkt->data[1] = c->last_frame_bits;
    ret = get_buffer(s->pb, pkt->data+2, size);

    if (ret<=0) {
        av_free_packet(pkt);
        return AVERROR(EIO);
    }

    c->last_frame_bits = pkt->data[size+1];
    c->remaining_bits  = (size << 3) - c->frame_bit_len + c->remaining_bits;

    return size+2;
}

static int vqf_read_seek(AVFormatContext *s,
                         int stream_index, int64_t timestamp, int flags)
{
    VqfContext *c = s->priv_data;
    AVStream *st;
    int ret;
    int64_t pos;

    st = s->streams[stream_index];
    pos = av_rescale_rnd(timestamp * st->codec->bit_rate,
                         st->time_base.num,
                         st->time_base.den * (int64_t)c->frame_bit_len,
                         (flags & AVSEEK_FLAG_BACKWARD) ?
                                                   AV_ROUND_DOWN : AV_ROUND_UP);
    pos *= c->frame_bit_len;

    st->cur_dts = av_rescale(pos, st->time_base.den,
                             st->codec->bit_rate * (int64_t)st->time_base.num);

    if ((ret = url_fseek(s->pb, ((pos-7) >> 3) + s->data_offset, SEEK_SET)) < 0)
        return ret;

    c->remaining_bits = -7 - ((pos-7)&7);
    return 0;
}

AVInputFormat vqf_demuxer = {
    "vqf",
    NULL_IF_CONFIG_SMALL("Nippon Telegraph and Telephone Corporation (NTT) TwinVQ"),
    sizeof(VqfContext),
    vqf_probe,
    vqf_read_header,
    vqf_read_packet,
    NULL,
    vqf_read_seek,
    .extensions = "vqf",
};
