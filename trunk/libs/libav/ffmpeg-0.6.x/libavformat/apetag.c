/*
 * APE tag handling
 * Copyright (c) 2007 Benjamin Zores <ben@geexbox.org>
 *  based upon libdemac from Dave Chapman.
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

#include "libavutil/intreadwrite.h"
#include "avformat.h"
#include "apetag.h"

#define ENABLE_DEBUG 0

#define APE_TAG_VERSION               2000
#define APE_TAG_FOOTER_BYTES          32
#define APE_TAG_FLAG_CONTAINS_HEADER  (1 << 31)
#define APE_TAG_FLAG_IS_HEADER        (1 << 29)

static int ape_tag_read_field(AVFormatContext *s)
{
    ByteIOContext *pb = s->pb;
    uint8_t key[1024], *value;
    uint32_t size, flags;
    int i, c;

    size = get_le32(pb);  /* field size */
    flags = get_le32(pb); /* field flags */
    for (i = 0; i < sizeof(key) - 1; i++) {
        c = get_byte(pb);
        if (c < 0x20 || c > 0x7E)
            break;
        else
            key[i] = c;
    }
    key[i] = 0;
    if (c != 0) {
        av_log(s, AV_LOG_WARNING, "Invalid APE tag key '%s'.\n", key);
        return -1;
    }
    if (size >= UINT_MAX)
        return -1;
    value = av_malloc(size+1);
    if (!value)
        return AVERROR(ENOMEM);
    get_buffer(pb, value, size);
    value[size] = 0;
    av_metadata_set2(&s->metadata, key, value, AV_METADATA_DONT_STRDUP_VAL);
    return 0;
}

void ff_ape_parse_tag(AVFormatContext *s)
{
    ByteIOContext *pb = s->pb;
    int file_size = url_fsize(pb);
    uint32_t val, fields, tag_bytes;
    uint8_t buf[8];
    int i;

    if (file_size < APE_TAG_FOOTER_BYTES)
        return;

    url_fseek(pb, file_size - APE_TAG_FOOTER_BYTES, SEEK_SET);

    get_buffer(pb, buf, 8);    /* APETAGEX */
    if (strncmp(buf, "APETAGEX", 8)) {
        return;
    }

    val = get_le32(pb);        /* APE tag version */
    if (val > APE_TAG_VERSION) {
        av_log(s, AV_LOG_ERROR, "Unsupported tag version. (>=%d)\n", APE_TAG_VERSION);
        return;
    }

    tag_bytes = get_le32(pb);  /* tag size */
    if (tag_bytes - APE_TAG_FOOTER_BYTES > (1024 * 1024 * 16)) {
        av_log(s, AV_LOG_ERROR, "Tag size is way too big\n");
        return;
    }

    fields = get_le32(pb);     /* number of fields */
    if (fields > 65536) {
        av_log(s, AV_LOG_ERROR, "Too many tag fields (%d)\n", fields);
        return;
    }

    val = get_le32(pb);        /* flags */
    if (val & APE_TAG_FLAG_IS_HEADER) {
        av_log(s, AV_LOG_ERROR, "APE Tag is a header\n");
        return;
    }

    url_fseek(pb, file_size - tag_bytes, SEEK_SET);

    for (i=0; i<fields; i++)
        if (ape_tag_read_field(s) < 0) break;
}
