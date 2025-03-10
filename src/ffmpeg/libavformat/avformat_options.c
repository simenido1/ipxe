/*
 * Copyright (c) 2000, 2001, 2002 Fabrice Bellard
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
#include "avio_internal.h"
#include "internal.h"

#include "libavutil/avassert.h"
#include "libavutil/internal.h"
#include "libavutil/opt.h"

/**
 * @file
 * Options definition for AVFormatContext.
 */

FF_DISABLE_DEPRECATION_WARNINGS
#include "options_table.h"
FF_ENABLE_DEPRECATION_WARNINGS

static const char* format_to_name(void* ptr)
{
    AVFormatContext* fc = (AVFormatContext*) ptr;
    if(fc->iformat) return fc->iformat->name;
    else if(fc->oformat) return fc->oformat->name;
    else return "NULL";
}

static void *format_child_next(void *obj, void *prev)
{
    AVFormatContext *s = obj;
    if (!prev && s->priv_data &&
        ((s->iformat && s->iformat->priv_class) ||
          s->oformat && s->oformat->priv_class))
        return s->priv_data;
    if (s->pb && s->pb->av_class && prev != s->pb)
        return s->pb;
    return NULL;
}

enum {
    CHILD_CLASS_ITER_AVIO = 0,
    CHILD_CLASS_ITER_MUX,
    CHILD_CLASS_ITER_DEMUX,
    CHILD_CLASS_ITER_DONE,

};

#define ITER_STATE_SHIFT 16

static const AVClass *format_child_class_iterate(void **iter)
{
    // we use the low 16 bits of iter as the value to be passed to
    // av_(de)muxer_iterate()
    void *val = (void*)(((uintptr_t)*iter) & ((1 << ITER_STATE_SHIFT) - 1));
    unsigned int state = ((uintptr_t)*iter) >> ITER_STATE_SHIFT;
    const AVClass *ret = NULL;

    if (state == CHILD_CLASS_ITER_AVIO) {
        ret = &ff_avio_class;
        state++;
        goto finish;
    }

    if (state == CHILD_CLASS_ITER_MUX) {
        const AVOutputFormat *ofmt;

        while ((ofmt = av_muxer_iterate(&val))) {
            ret = ofmt->priv_class;
            if (ret)
                goto finish;
        }

        val = NULL;
        state++;
    }

    if (state == CHILD_CLASS_ITER_DEMUX) {
        const AVInputFormat *ifmt;

        while ((ifmt = av_demuxer_iterate(&val))) {
            ret = ifmt->priv_class;
            if (ret)
                goto finish;
        }
        val = NULL;
        state++;
    }

finish:
    // make sure none av_(de)muxer_iterate does not set the high bits of val
    //av_assert0(!((uintptr_t)val >> ITER_STATE_SHIFT));
    *iter = (void*)((uintptr_t)val | (state << ITER_STATE_SHIFT));
    return ret;
}

static AVClassCategory get_category(void *ptr)
{
    AVFormatContext* s = ptr;
    if(s->iformat) return AV_CLASS_CATEGORY_DEMUXER;
    else           return AV_CLASS_CATEGORY_MUXER;
}

static const AVClass av_format_context_class = {
    .class_name     = "AVFormatContext",
    .item_name      = format_to_name,
    .option         = avformat_options,
    .version        = LIBAVUTIL_VERSION_INT,
    .child_next     = format_child_next,
    .child_class_iterate = format_child_class_iterate,
    .category       = AV_CLASS_CATEGORY_MUXER,
    .get_category   = get_category,
};

static int io_open_default(AVFormatContext *s, AVIOContext **pb,
                           const char *url, int flags, AVDictionary **options)
{
    int loglevel;

    if (!strcmp(url, s->url) ||
        s->iformat && !strcmp(s->iformat->name, "image2") ||
        s->oformat && !strcmp(s->oformat->name, "image2")
    ) {
        loglevel = AV_LOG_DEBUG;
    } else
        loglevel = AV_LOG_INFO;

    av_log(s, loglevel, "Opening \'%s\' for %s\n", url, flags & AVIO_FLAG_WRITE ? "writing" : "reading");

    return ffio_open_whitelist(pb, url, flags, &s->interrupt_callback, options, s->protocol_whitelist, s->protocol_blacklist);
}

static int io_close2_default(AVFormatContext *s, AVIOContext *pb)
{
    return avio_close(pb);
}

AVFormatContext *avformat_alloc_context(void)
{
    FFFormatContext *const si = av_mallocz(sizeof(*si));
    AVFormatContext *s;

    if (!si)
        return NULL;

    s = &si->pub;
    s->av_class = &av_format_context_class;
    s->io_open  = io_open_default;
    s->io_close = ff_format_io_close_default;
    s->io_close2= io_close2_default;

    av_opt_set_defaults(s);

    si->pkt = av_packet_alloc();
    si->parse_pkt = av_packet_alloc();
    if (!si->pkt || !si->parse_pkt) {
        avformat_free_context(s);
        return NULL;
    }

    si->shortest_end = AV_NOPTS_VALUE;

    return s;
}

enum AVDurationEstimationMethod av_fmt_ctx_get_duration_estimation_method(const AVFormatContext* ctx)
{
    return ctx->duration_estimation_method;
}

const AVClass *avformat_get_class(void)
{
    return &av_format_context_class;
}
