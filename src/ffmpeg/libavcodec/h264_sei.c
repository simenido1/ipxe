/*
 * H.26L/H.264/AVC/JVT/14496-10/... SEI decoding
 * Copyright (c) 2003 Michael Niedermayer <michaelni@gmx.at>
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
 * H.264 / AVC / MPEG-4 part10 SEI decoding.
 * @author Michael Niedermayer <michaelni@gmx.at>
 */

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "libavutil/error.h"
#include "libavutil/avutil_log.h"
#include "libavutil/macros.h"
#include "libavutil/mem.h"
#include "atsc_a53.h"
#include "get_bits.h"
#include "golomb.h"
#include "h264_ps.h"
#include "h264_sei.h"
#include "sei.h"

#define AVERROR_PS_NOT_FOUND      FFERRTAG(0xF8,'?','P','S')

static const uint8_t sei_num_clock_ts_table[9] = {
    1, 1, 1, 2, 2, 3, 3, 2, 3
};

void ff_h264_sei_uninit(H264SEIContext *h)
{
    h->recovery_point.recovery_frame_cnt = -1;

    h->picture_timing.dpb_output_delay  = 0;
    h->picture_timing.cpb_removal_delay = -1;

    h->picture_timing.present      = 0;
    h->buffering_period.present    = 0;
    h->frame_packing.present       = 0;
    h->film_grain_characteristics.present = 0;
    h->display_orientation.present = 0;
    h->afd.present                 =  0;

    av_buffer_unref(&h->a53_caption.buf_ref);
    for (int i = 0; i < h->unregistered.nb_buf_ref; i++)
        av_buffer_unref(&h->unregistered.buf_ref[i]);
    h->unregistered.nb_buf_ref = 0;
    av_freep(&h->unregistered.buf_ref);
}

int ff_h264_sei_process_picture_timing(H264SEIPictureTiming *h, const SPS *sps,
                                       void *logctx)
{
    GetBitContext gb;

    init_get_bits(&gb, h->payload, h->payload_size_bits);

    if (sps->nal_hrd_parameters_present_flag ||
        sps->vcl_hrd_parameters_present_flag) {
        h->cpb_removal_delay = get_bits_long(&gb, sps->cpb_removal_delay_length);
        h->dpb_output_delay  = get_bits_long(&gb, sps->dpb_output_delay_length);
    }
    if (sps->pic_struct_present_flag) {
        unsigned int i, num_clock_ts;

        h->pic_struct = get_bits(&gb, 4);
        h->ct_type    = 0;

        if (h->pic_struct > H264_SEI_PIC_STRUCT_FRAME_TRIPLING)
            return AVERROR_INVALIDDATA;

        num_clock_ts = sei_num_clock_ts_table[h->pic_struct];
        h->timecode_cnt = 0;
        for (i = 0; i < num_clock_ts; i++) {
            if (get_bits(&gb, 1)) {                      /* clock_timestamp_flag */
                H264SEITimeCode *tc = &h->timecode[h->timecode_cnt++];
                unsigned int full_timestamp_flag;
                unsigned int counting_type, cnt_dropped_flag;
                h->ct_type |= 1 << get_bits(&gb, 2);
                skip_bits(&gb, 1);                       /* nuit_field_based_flag */
                counting_type = get_bits(&gb, 5);        /* counting_type */
                full_timestamp_flag = get_bits(&gb, 1);
                skip_bits(&gb, 1);                       /* discontinuity_flag */
                cnt_dropped_flag = get_bits(&gb, 1);      /* cnt_dropped_flag */
                if (cnt_dropped_flag && counting_type > 1 && counting_type < 7)
                    tc->dropframe = 1;
                tc->frame = get_bits(&gb, 8);         /* n_frames */
                if (full_timestamp_flag) {
                    tc->full = 1;
                    tc->seconds = get_bits(&gb, 6); /* seconds_value 0..59 */
                    tc->minutes = get_bits(&gb, 6); /* minutes_value 0..59 */
                    tc->hours = get_bits(&gb, 5);   /* hours_value 0..23 */
                } else {
                    tc->seconds = tc->minutes = tc->hours = tc->full = 0;
                    if (get_bits(&gb, 1)) {             /* seconds_flag */
                        tc->seconds = get_bits(&gb, 6);
                        if (get_bits(&gb, 1)) {         /* minutes_flag */
                            tc->minutes = get_bits(&gb, 6);
                            if (get_bits(&gb, 1))       /* hours_flag */
                                tc->hours = get_bits(&gb, 5);
                        }
                    }
                }

                if (sps->time_offset_length > 0)
                    skip_bits(&gb,
                              sps->time_offset_length); /* time_offset */
            }
        }

        av_log(logctx, AV_LOG_DEBUG, "ct_type:%X pic_struct:%d\n",
               h->ct_type, h->pic_struct);
    }

    return 0;
}

static int decode_picture_timing(H264SEIPictureTiming *h, GetBitContext *gb,
                                 void *logctx)
{
    int index     = get_bits_count(gb);
    int size_bits = get_bits_left(gb);
    int size      = (size_bits + 7) / 8;

    if (index & 7) {
        av_log(logctx, AV_LOG_ERROR, "Unaligned SEI payload\n");
        return AVERROR_INVALIDDATA;
    }
    if (size > sizeof(h->payload)) {
        av_log(logctx, AV_LOG_ERROR, "Picture timing SEI payload too large\n");
        return AVERROR_INVALIDDATA;
    }
    memcpy(h->payload, gb->buffer + index / 8, size);

    h->payload_size_bits = size_bits;

    h->present = 1;
    return 0;
}

static int decode_registered_user_data_afd(H264SEIAFD *h, GetBitContext *gb, int size)
{
    int flag;

    if (size-- < 1)
        return AVERROR_INVALIDDATA;
    skip_bits(gb, 1);               // 0
    flag = get_bits(gb, 1);         // active_format_flag
    skip_bits(gb, 6);               // reserved

    if (flag) {
        if (size-- < 1)
            return AVERROR_INVALIDDATA;
        skip_bits(gb, 4);           // reserved
        h->active_format_description = get_bits(gb, 4);
        h->present                   = 1;
    }

    return 0;
}

static int decode_registered_user_data_closed_caption(H264SEIA53Caption *h,
                                                     GetBitContext *gb, void *logctx,
                                                     int size)
{
    if (size < 3)
        return AVERROR(EINVAL);

    return ff_parse_a53_cc(&h->buf_ref, gb->buffer + get_bits_count(gb) / 8, size);
}

static int decode_registered_user_data(H264SEIContext *h, GetBitContext *gb,
                                       void *logctx, int size)
{
    int country_code, provider_code;

    if (size < 3)
        return AVERROR_INVALIDDATA;
    size -= 3;

    country_code = get_bits(gb, 8); // itu_t_t35_country_code
    if (country_code == 0xFF) {
        if (size < 1)
            return AVERROR_INVALIDDATA;

        skip_bits(gb, 8);           // itu_t_t35_country_code_extension_byte
        size--;
    }

    if (country_code != 0xB5) { // usa_country_code
        av_log(logctx, AV_LOG_VERBOSE,
               "Unsupported User Data Registered ITU-T T35 SEI message (country_code = %d)\n",
               country_code);
        return 0;
    }

    /* itu_t_t35_payload_byte follows */
    provider_code = get_bits(gb, 16);

    switch (provider_code) {
    case 0x31: { // atsc_provider_code
        uint32_t user_identifier;

        if (size < 4)
            return AVERROR_INVALIDDATA;
        size -= 4;

        user_identifier = get_bits_long(gb, 32);
        switch (user_identifier) {
        case MKBETAG('D', 'T', 'G', '1'):       // afd_data
            return decode_registered_user_data_afd(&h->afd, gb, size);
        case MKBETAG('G', 'A', '9', '4'):       // closed captions
            return decode_registered_user_data_closed_caption(&h->a53_caption, gb,
                                                              logctx, size);
        default:
            av_log(logctx, AV_LOG_VERBOSE,
                   "Unsupported User Data Registered ITU-T T35 SEI message (atsc user_identifier = 0x%04x)\n",
                   user_identifier);
            break;
        }
        break;
    }
    default:
        av_log(logctx, AV_LOG_VERBOSE,
               "Unsupported User Data Registered ITU-T T35 SEI message (provider_code = %d)\n",
               provider_code);
        break;
    }

    return 0;
}

static int decode_unregistered_user_data(H264SEIUnregistered *h, GetBitContext *gb,
                                         void *logctx, int size)
{
    uint8_t *user_data;
    int e, build, i;
    AVBufferRef *buf_ref, **tmp;

    if (size < 16 || size >= INT_MAX - 1)
        return AVERROR_INVALIDDATA;

    tmp = av_realloc_array(h->buf_ref, h->nb_buf_ref + 1, sizeof(*h->buf_ref));
    if (!tmp)
        return AVERROR(ENOMEM);
    h->buf_ref = tmp;

    buf_ref = av_buffer_alloc(size + 1);
    if (!buf_ref)
        return AVERROR(ENOMEM);
    user_data = buf_ref->data;

    for (i = 0; i < size; i++)
        user_data[i] = get_bits(gb, 8);

    user_data[i] = 0;
    buf_ref->size = size;
    h->buf_ref[h->nb_buf_ref++] = buf_ref;

    e = sscanf(user_data + 16, "x264 - core %d", &build);
    if (e == 1 && build > 0)
        h->x264_build = build;
    if (e == 1 && build == 1 && !strncmp(user_data+16, "x264 - core 0000", 16))
        h->x264_build = 67;

    return 0;
}

static int decode_recovery_point(H264SEIRecoveryPoint *h, GetBitContext *gb, void *logctx)
{
    unsigned recovery_frame_cnt = get_ue_golomb_long(gb);

    if (recovery_frame_cnt >= (1<<MAX_LOG2_MAX_FRAME_NUM)) {
        av_log(logctx, AV_LOG_ERROR, "recovery_frame_cnt %u is out of range\n", recovery_frame_cnt);
        return AVERROR_INVALIDDATA;
    }

    h->recovery_frame_cnt = recovery_frame_cnt;
    /* 1b exact_match_flag,
     * 1b broken_link_flag,
     * 2b changing_slice_group_idc */
    skip_bits(gb, 4);

    return 0;
}

static int decode_buffering_period(H264SEIBufferingPeriod *h, GetBitContext *gb,
                                   const H264ParamSets *ps, void *logctx)
{
    unsigned int sps_id;
    int sched_sel_idx;
    const SPS *sps;

    sps_id = get_ue_golomb_31(gb);
    if (sps_id > 31 || !ps->sps_list[sps_id]) {
        av_log(logctx, AV_LOG_ERROR,
               "non-existing SPS %d referenced in buffering period\n", sps_id);
        return sps_id > 31 ? AVERROR_INVALIDDATA : AVERROR_PS_NOT_FOUND;
    }
    sps = (const SPS*)ps->sps_list[sps_id]->data;

    // NOTE: This is really so duplicated in the standard... See H.264, D.1.1
    if (sps->nal_hrd_parameters_present_flag) {
        for (sched_sel_idx = 0; sched_sel_idx < sps->cpb_cnt; sched_sel_idx++) {
            h->initial_cpb_removal_delay[sched_sel_idx] =
                get_bits_long(gb, sps->initial_cpb_removal_delay_length);
            // initial_cpb_removal_delay_offset
            skip_bits(gb, sps->initial_cpb_removal_delay_length);
        }
    }
    if (sps->vcl_hrd_parameters_present_flag) {
        for (sched_sel_idx = 0; sched_sel_idx < sps->cpb_cnt; sched_sel_idx++) {
            h->initial_cpb_removal_delay[sched_sel_idx] =
                get_bits_long(gb, sps->initial_cpb_removal_delay_length);
            // initial_cpb_removal_delay_offset
            skip_bits(gb, sps->initial_cpb_removal_delay_length);
        }
    }

    h->present = 1;
    return 0;
}

static int decode_frame_packing_arrangement(H264SEIFramePacking *h,
                                            GetBitContext *gb)
{
    h->arrangement_id          = get_ue_golomb_long(gb);
    h->arrangement_cancel_flag = get_bits1(gb);
    h->present = !h->arrangement_cancel_flag;

    if (h->present) {
        h->arrangement_type = get_bits(gb, 7);
        h->quincunx_sampling_flag         = get_bits1(gb);
        h->content_interpretation_type    = get_bits(gb, 6);

        // spatial_flipping_flag, frame0_flipped_flag, field_views_flag
        skip_bits(gb, 3);
        h->current_frame_is_frame0_flag = get_bits1(gb);
        // frame0_self_contained_flag, frame1_self_contained_flag
        skip_bits(gb, 2);

        if (!h->quincunx_sampling_flag && h->arrangement_type != 5)
            skip_bits(gb, 16);      // frame[01]_grid_position_[xy]
        skip_bits(gb, 8);           // frame_packing_arrangement_reserved_byte
        h->arrangement_repetition_period = get_ue_golomb_long(gb);
    }
    skip_bits1(gb);                 // frame_packing_arrangement_extension_flag

    return 0;
}

static int decode_display_orientation(H264SEIDisplayOrientation *h,
                                      GetBitContext *gb)
{
    h->present = !get_bits1(gb);

    if (h->present) {
        h->hflip = get_bits1(gb);     // hor_flip
        h->vflip = get_bits1(gb);     // ver_flip

        h->anticlockwise_rotation = get_bits(gb, 16);
        get_ue_golomb_long(gb);       // display_orientation_repetition_period
        skip_bits1(gb);               // display_orientation_extension_flag
    }

    return 0;
}

static int decode_green_metadata(H264SEIGreenMetaData *h, GetBitContext *gb)
{
    h->green_metadata_type = get_bits(gb, 8);

    if (h->green_metadata_type == 0) {
        h->period_type = get_bits(gb, 8);

        if (h->period_type == 2)
            h->num_seconds = get_bits(gb, 16);
        else if (h->period_type == 3)
            h->num_pictures = get_bits(gb, 16);

        h->percent_non_zero_macroblocks            = get_bits(gb, 8);
        h->percent_intra_coded_macroblocks         = get_bits(gb, 8);
        h->percent_six_tap_filtering               = get_bits(gb, 8);
        h->percent_alpha_point_deblocking_instance = get_bits(gb, 8);

    } else if (h->green_metadata_type == 1) {
        h->xsd_metric_type  = get_bits(gb, 8);
        h->xsd_metric_value = get_bits(gb, 16);
    }

    return 0;
}

static int decode_alternative_transfer(H264SEIAlternativeTransfer *h,
                                       GetBitContext *gb)
{
    h->present = 1;
    h->preferred_transfer_characteristics = get_bits(gb, 8);
    return 0;
}

static int decode_film_grain_characteristics(H264SEIFilmGrainCharacteristics *h,
                                             GetBitContext *gb)
{
    h->present = !get_bits1(gb); // film_grain_characteristics_cancel_flag

    if (h->present) {
        memset(h, 0, sizeof(*h));
        h->model_id = get_bits(gb, 2);
        h->separate_colour_description_present_flag = get_bits1(gb);
        if (h->separate_colour_description_present_flag) {
            h->bit_depth_luma = get_bits(gb, 3) + 8;
            h->bit_depth_chroma = get_bits(gb, 3) + 8;
            h->full_range = get_bits1(gb);
            h->color_primaries = get_bits(gb, 8);
            h->transfer_characteristics = get_bits(gb, 8);
            h->matrix_coeffs = get_bits(gb, 8);
        }
        h->blending_mode_id = get_bits(gb, 2);
        h->log2_scale_factor = get_bits(gb, 4);
        for (int c = 0; c < 3; c++)
            h->comp_model_present_flag[c] = get_bits1(gb);
        for (int c = 0; c < 3; c++) {
            if (h->comp_model_present_flag[c]) {
                h->num_intensity_intervals[c] = get_bits(gb, 8) + 1;
                h->num_model_values[c] = get_bits(gb, 3) + 1;
                if (h->num_model_values[c] > 6)
                    return AVERROR_INVALIDDATA;
                for (int i = 0; i < h->num_intensity_intervals[c]; i++) {
                    h->intensity_interval_lower_bound[c][i] = get_bits(gb, 8);
                    h->intensity_interval_upper_bound[c][i] = get_bits(gb, 8);
                    for (int j = 0; j < h->num_model_values[c]; j++)
                        h->comp_model_value[c][i][j] = get_se_golomb_long(gb);
                }
            }
        }
        h->repetition_period = get_ue_golomb_long(gb);

        h->present = 1;
    }

    return 0;
}

int ff_h264_sei_decode(H264SEIContext *h, GetBitContext *gb,
                       const H264ParamSets *ps, void *logctx)
{
    int master_ret = 0;

    while (get_bits_left(gb) > 16 && show_bits(gb, 16)) {
        GetBitContext gb_payload;
        int type = 0;
        unsigned size = 0;
        int ret  = 0;

        do {
            if (get_bits_left(gb) < 8)
                return AVERROR_INVALIDDATA;
            type += show_bits(gb, 8);
        } while (get_bits(gb, 8) == 255);

        do {
            if (get_bits_left(gb) < 8)
                return AVERROR_INVALIDDATA;
            size += show_bits(gb, 8);
        } while (get_bits(gb, 8) == 255);

        if (size > get_bits_left(gb) / 8) {
            av_log(logctx, AV_LOG_ERROR, "SEI type %d size %d truncated at %d\n",
                   type, 8*size, get_bits_left(gb));
            return AVERROR_INVALIDDATA;
        }

        ret = init_get_bits8(&gb_payload, gb->buffer + get_bits_count(gb) / 8, size);
        if (ret < 0)
            return ret;

        switch (type) {
        case SEI_TYPE_PIC_TIMING: // Picture timing SEI
            ret = decode_picture_timing(&h->picture_timing, &gb_payload, logctx);
            break;
        case SEI_TYPE_USER_DATA_REGISTERED_ITU_T_T35:
            ret = decode_registered_user_data(h, &gb_payload, logctx, size);
            break;
        case SEI_TYPE_USER_DATA_UNREGISTERED:
            ret = decode_unregistered_user_data(&h->unregistered, &gb_payload, logctx, size);
            break;
        case SEI_TYPE_RECOVERY_POINT:
            ret = decode_recovery_point(&h->recovery_point, &gb_payload, logctx);
            break;
        case SEI_TYPE_BUFFERING_PERIOD:
            ret = decode_buffering_period(&h->buffering_period, &gb_payload, ps, logctx);
            break;
        case SEI_TYPE_FRAME_PACKING_ARRANGEMENT:
            ret = decode_frame_packing_arrangement(&h->frame_packing, &gb_payload);
            break;
        case SEI_TYPE_DISPLAY_ORIENTATION:
            ret = decode_display_orientation(&h->display_orientation, &gb_payload);
            break;
        case SEI_TYPE_GREEN_METADATA:
            ret = decode_green_metadata(&h->green_metadata, &gb_payload);
            break;
        case SEI_TYPE_ALTERNATIVE_TRANSFER_CHARACTERISTICS:
            ret = decode_alternative_transfer(&h->alternative_transfer, &gb_payload);
            break;
        case SEI_TYPE_FILM_GRAIN_CHARACTERISTICS:
            ret = decode_film_grain_characteristics(&h->film_grain_characteristics, &gb_payload);
            break;
        default:
            av_log(logctx, AV_LOG_DEBUG, "unknown SEI type %d\n", type);
        }
        if (ret < 0 && ret != AVERROR_PS_NOT_FOUND)
            return ret;
        if (ret < 0)
            master_ret = ret;

        if (get_bits_left(&gb_payload) < 0) {
            av_log(logctx, AV_LOG_WARNING, "SEI type %d overread by %d bits\n",
                   type, -get_bits_left(&gb_payload));
        }

        skip_bits_long(gb, 8 * size);
    }

    return master_ret;
}

const char *ff_h264_sei_stereo_mode(const H264SEIFramePacking *h)
{
    if (h->arrangement_cancel_flag == 0) {
        switch (h->arrangement_type) {
            case H264_SEI_FPA_TYPE_CHECKERBOARD:
                if (h->content_interpretation_type == 2)
                    return "checkerboard_rl";
                else
                    return "checkerboard_lr";
            case H264_SEI_FPA_TYPE_INTERLEAVE_COLUMN:
                if (h->content_interpretation_type == 2)
                    return "col_interleaved_rl";
                else
                    return "col_interleaved_lr";
            case H264_SEI_FPA_TYPE_INTERLEAVE_ROW:
                if (h->content_interpretation_type == 2)
                    return "row_interleaved_rl";
                else
                    return "row_interleaved_lr";
            case H264_SEI_FPA_TYPE_SIDE_BY_SIDE:
                if (h->content_interpretation_type == 2)
                    return "right_left";
                else
                    return "left_right";
            case H264_SEI_FPA_TYPE_TOP_BOTTOM:
                if (h->content_interpretation_type == 2)
                    return "bottom_top";
                else
                    return "top_bottom";
            case H264_SEI_FPA_TYPE_INTERLEAVE_TEMPORAL:
                if (h->content_interpretation_type == 2)
                    return "block_rl";
                else
                    return "block_lr";
            case H264_SEI_FPA_TYPE_2D:
            default:
                return "mono";
        }
    } else if (h->arrangement_cancel_flag == 1) {
        return "mono";
    } else {
        return NULL;
    }
}
