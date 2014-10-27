/*****************************************************************************
 * (c)2006-2010 Sirannon
 * Authors: Alexis Rombaut <alexis.rombaut@intec.ugent.be>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *****************************************************************************/
#ifndef H264_AVC_H_
#define H264_AVC_H_
#include "Utils.h"

typedef struct
{
	/* NAL Header */
	uint8_t nal_ref_idc;
	uint8_t nal_unit_type;

	/* SVC Header */
	uint8_t svc;
	uint8_t idr_flag;
	uint8_t priority_id;
	uint8_t temporal_id;
	uint8_t dependency_id;
	uint8_t quality_id;
	uint8_t layer_base_flag;
	uint8_t use_base_prediction_flag;
	uint8_t discardable_flag;
	uint8_t output_flag;
	uint8_t no_inter_layer_pred_flag;

	/* SPS */
	uint8_t profile_idc;
	uint8_t level_idc;
	uint32_t seq_parameter_set_id;
	uint32_t chroma_format_idc;
	uint8_t residual_colour_transform_flag;
	uint32_t bit_depth_luma_minus8;
	uint32_t bit_depth_chroma_minus8;
	uint8_t qpprime_y_zero_transform_bypass_flag;
	uint8_t seq_scaling_matrix_present_flag;
	uint32_t log2_max_frame_num_minus4;
	uint32_t pic_order_cnt_type;
	uint32_t log2_max_pic_order_cnt_lsb_minus4;
	uint8_t delta_pic_order_always_zero_flag;
	uint8_t pic_order_present_flag;
	int32_t offset_for_non_ref_pic;
    int32_t offset_for_top_to_bottom_field;
    uint32_t num_ref_frames_in_pic_order_cnt_cycle;
    int16_t offset_for_ref_frame[256];
	uint32_t num_ref_frames;
	uint8_t gaps_in_frame_num_value_allowed_flag;
	uint32_t pic_width_in_mbs_minus1;
	uint32_t pic_height_in_map_units_minus1;
	uint8_t frame_mbs_only_flag;
	uint8_t mb_adpative_frame_field_flag;
	uint8_t direct_8x8_inference_flag;
	uint8_t frame_cropping_flag;
	uint32_t frame_crop_left_offset;
	uint32_t frame_crop_right_offset;
	uint32_t frame_crop_top_offset;
	uint32_t frame_crop_bottom_offset;
	uint8_t vui;

  	/* VUI state */
	uint8_t timing_info_present_flag;
	uint32_t num_units_in_tick;
	uint32_t time_scale;
	uint8_t fixed_frame_rate_flag;

	/* Slice Header */
	uint32_t first_mb_in_slice;
	uint32_t slice_type;
	uint32_t pic_parameter_set;
	uint32_t frame_num;
	uint8_t field_pic_flag;
	uint8_t bottom_field_flag;
	uint32_t idr_pic_id;
	uint32_t pic_order_cnt_lsb;

	/* Own POC fields */
	uint8_t pic_order_cnt_valid;
	int32_t  pic_order_cnt;
	uint32_t  pic_order_cnt_msb;
	uint32_t  pic_order_cnt_msb_prev;
	uint32_t  pic_order_cnt_lsb_prev;
} NAL_t;

/* NAL unit type */
#define NAL_UNIT_TYPE_UNSPECIFIED 0x00
#define NAL_UNIT_TYPE_SLICE_NON_IDR 0x01
#define NAL_UNIT_TYPE_SLICE_DP_A 0x02
#define NAL_UNIT_TYPE_SLICE_DP_B 0x03
#define NAL_UNIT_TYPE_SLICE_DP_C 0x04
#define NAL_UNIT_TYPE_SLICE_IDR 0x05
#define NAL_UNIT_TYPE_SEI 0x06
#define NAL_UNIT_TYPE_SPS 0x07
#define NAL_UNIT_TYPE_PPS 0x08
#define NAL_UNIT_TYPE_ACCESS_UNIT_DELIM 0x09
#define NAL_UNIT_TYPE_END_OF_SEQ 0x0A
#define NAL_UNIT_TYPE_END_OF_STREAM 0x0B
#define NAL_UNIT_TYPE_FILLER_DATA 0x0C
#define NAL_UNIT_TYPE_SPS_EXT 0x0D
#define NAL_UNIT_TYPE_PREFIX 0x0E /* SVC */
#define NAL_UNIT_TYPE_SPS_SVC 0x0F /* SVC */
#define NAL_UNIT_TYPE_SLICE_SVC 0x14  /* SVC */

/* Slice types */
#define SLICE_TYPE_P 0
#define SLICE_TYPE_B 1
#define SLICE_TYPE_I 2
#define SLICE_TYPE_EP 0
#define SLICE_TYPE_EB 1
#define SLICE_TYPE_EI 2
#define SLICE_TYPE_SP 3
#define SLICE_TYPE_SI 4
#define SLICE_TYPE_P2 5
#define SLICE_TYPE_B2 6
#define SLICE_TYPE_I2 7
#define SLICE_TYPE_SP2 8
#define SLICE_TYPE_SI2 9

/* Profiles */
#define H264_PROFILE_STR_BASELINE  		0
#define H264_PROFILE_STR_MAIN 			1
#define H264_PROFILE_STR_EXTENDED 		2

/* Levels */
#define H264_LEVEL_STR_1    0
#define H264_LEVEL_STR_1_b  1
#define H264_LEVEL_STR_1_1  2
#define H264_LEVEL_STR_1_2  3
#define H264_LEVEL_STR_1_3  4
#define H264_LEVEL_STR_2    5
#define H264_LEVEL_STR_2_1  6
#define H264_LEVEL_STR_2_2  7
#define H264_LEVEL_STR_3    8
#define H264_LEVEL_STR_3_1  9
#define H264_LEVEL_STR_3_2  10
#define H264_LEVEL_STR_4    11
#define H264_LEVEL_STR_4_1  12
#define H264_LEVEL_STR_4_2  13
#define H264_LEVEL_STR_5    14
#define H264_LEVEL_STR_5_1  15

/* Basic functions */
bool H264_is_SVC( const uint8_t* pBuffer );
int32_t H264_is_start_code( const uint8_t* pBuffer );
int32_t H264_find_NAL( const uint8_t* pBuffer, int iSize );
uint8_t H264_NAL_type( const uint8_t* pBuffer );
uint8_t H264_NAL_idc( const uint8_t* pBuffer );
int32_t H264_getTDQ( const uint8_t* pBuffer, uint8_t* T, uint8_t* D, uint8_t* Q );

/* Parsing */
int32_t H264_parse_NAL( const uint8_t* pBuffer, uint32_t iSize, NAL_t* pNAL );
int32_t H264_parse_SPS( const uint8_t* pBuffer, uint32_t iSize, NAL_t* pNAL );
int32_t H264_parse_SLICE( const uint8_t* pBuffer, uint32_t iSize, NAL_t* pNAL );
int32_t H264_parse_SVC( const uint8_t* pBuffer, uint32_t iSize, NAL_t* pNAL );

const static uint8_t H264_START_CODE [] = { 0, 0, 0, 1 };
const static uint8_t H264_START_CODE_SHORT [] = { 0, 0, 1 };

#include "h264_avc_priv.h"

#endif /* H264_AVC_H_ */
