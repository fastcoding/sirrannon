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
#include "h264_avc.h"
#include "SirannonException.h"
#include "Bits.h"

int32_t H264_getTDQ( const uint8_t* pBuffer, uint8_t* T, uint8_t* D, uint8_t* Q  )
{
	/* Make sure it is svc */
	if( not H264_is_SVC( pBuffer ) )
		return -1;

	/* Skip the header */
	pBuffer += H264_is_start_code( pBuffer ) + 1;

	/* Load into a bitstream */
	IBits oBuffer( pBuffer, 3 );

	/* Reserved 1 */
	oBuffer.read( 1 );

	/* Priority */
	oBuffer.read( 1 );
	oBuffer.read( 6 );
	oBuffer.read( 1 );

	/* The scalable parameters */
	*D = oBuffer.read( 3 );
	*Q = oBuffer.read( 4 );
	*T = oBuffer.read( 3 );

	/* Succes */
	return 0;
}

static inline bool H264_is_NAL_slice( int iType )
{
	switch( iType )
	{
	case NAL_UNIT_TYPE_SLICE_NON_IDR:
	case NAL_UNIT_TYPE_SLICE_DP_A:
	case NAL_UNIT_TYPE_SLICE_DP_B:
	case NAL_UNIT_TYPE_SLICE_DP_C:
	case NAL_UNIT_TYPE_SLICE_IDR:
	case NAL_UNIT_TYPE_SLICE_SVC:
		return true;

	default:
		return false;
	}
}

static int32_t H264_unemulate( uint8_t* pBufferDst, const uint8_t* pBufferSrc, const int iSizeSrc )
{
	const uint8_t *pBufferSrc0 = pBufferSrc, *pBufferDst0 = pBufferDst;
	int iVal = 0;

	/* Prologue */
	for( int i = 0; i < 3; i++ )
	{
		iVal <<= 8;
		iVal |= *pBufferSrc++;
	}
	/* Main loop */
	while( pBufferSrc - pBufferSrc0 < iSizeSrc )
	{
		/* Emulation prevention */
		if( ( iVal & 0x00FFFFFF ) == 0x3 )
			iVal = ( iVal & 0xFFFFFF00 ) | *pBufferSrc++;
		iVal <<= 8;
		iVal |= *pBufferSrc++;
		*pBufferDst++ = iVal >> 24;
	}
	/* Epilogue */
	for( int i = 0; i < 3; i++ )
	{
		iVal <<= 8;
		*pBufferDst++ = iVal >> 24;
	}
	return pBufferDst - pBufferDst0;
}

static void H264_scaling_list( IBits& oBuffer, uint32_t sizeOfScalingList )
{
	uint32_t lastScale = 8, nextScale = 8;
	for( uint32_t j = 0; j < sizeOfScalingList; j++ )
	{
		if( nextScale != 0 )
		{
			int32_t deltaScale = oBuffer.read_sev();
			nextScale = ( lastScale + deltaScale + 256 ) % 256;
		}

		if( nextScale == 0 )
			lastScale = lastScale;
		else
			lastScale = nextScale;
	}
}

static int32_t H264_find_poc( NAL_t* pNAL )
{
	/* Check if we support this POC version */
	if( pNAL->pic_order_cnt_type != 0 )
		return -1;

	/* Reset the POC at an IDR picture */
	if( pNAL->nal_unit_type == NAL_UNIT_TYPE_SLICE_IDR )
	{
		pNAL->pic_order_cnt_lsb_prev = 0;
		pNAL->pic_order_cnt_msb_prev = 0;
	}
	/* System to increase/decrease the MSB whenever the LSB over/underflows */
	uint32_t iMax = 1 << ( pNAL->log2_max_pic_order_cnt_lsb_minus4 + 4 );
    if( pNAL->pic_order_cnt_lsb < pNAL->pic_order_cnt_lsb_prev and
        pNAL->pic_order_cnt_lsb_prev - pNAL->pic_order_cnt_lsb >= iMax / 2 )
    	pNAL->pic_order_cnt_msb = pNAL->pic_order_cnt_msb_prev + iMax;
    else if( pNAL->pic_order_cnt_lsb > pNAL->pic_order_cnt_lsb_prev and
    		 pNAL->pic_order_cnt_lsb - pNAL->pic_order_cnt_lsb_prev > iMax / 2 )
    	pNAL->pic_order_cnt_msb = pNAL->pic_order_cnt_msb_prev - iMax;
    else
    	pNAL->pic_order_cnt_msb = pNAL->pic_order_cnt_msb_prev;

    /* Real POC is simply the sum of LSB and MSB */
    pNAL->pic_order_cnt_valid = 1;
    pNAL->pic_order_cnt = pNAL->pic_order_cnt_msb + pNAL->pic_order_cnt_lsb;
    pNAL->pic_order_cnt_lsb_prev = pNAL->pic_order_cnt_lsb;
    pNAL->pic_order_cnt_msb_prev = pNAL->pic_order_cnt_msb;
    return 0;
}

static int32_t H264_next_frame( const NAL_t* pNAL, NAL_t* pNewNAL )
{
	switch( pNewNAL->nal_unit_type )
	{
	case NAL_UNIT_TYPE_END_OF_SEQ:
	case NAL_UNIT_TYPE_END_OF_STREAM:
	case NAL_UNIT_TYPE_FILLER_DATA:
		return 1;

	case NAL_UNIT_TYPE_SLICE_DP_B:
	case NAL_UNIT_TYPE_SLICE_DP_C:
		return 0;

	case NAL_UNIT_TYPE_PREFIX:
	/* We assume a prefix always forms the border because the next unit
	 * is forced to be a base layer unit and we presume a base layer
	 * unit is always sent first */
		if( not H264_is_NAL_slice( pNAL->nal_unit_type ) )
			return 0;
		else
			return 1;

	case NAL_UNIT_TYPE_SLICE_SVC:
		/* An SVC slice can never form a border because it needs to be preceeded by a base layer NAL of the same frame */
		return 0;

	case NAL_UNIT_TYPE_SLICE_NON_IDR:
	case NAL_UNIT_TYPE_SLICE_DP_A:
	case NAL_UNIT_TYPE_SLICE_IDR:
		/* If the previous unit is not a slice, this NAL can never form a border */
		if( not H264_is_NAL_slice( pNAL->nal_unit_type ) )
			return 0;

		if( pNAL->frame_num != pNewNAL->frame_num )
			return 1;
		else if( pNAL->frame_num == pNewNAL->frame_num and pNAL->pic_order_cnt_type == pNewNAL->pic_order_cnt_type )
			if( pNAL->pic_order_cnt_type == 0 )
				if( pNAL->pic_order_cnt_lsb != pNewNAL->pic_order_cnt_lsb )
					return 1;

		if( pNAL->nal_ref_idc != pNewNAL->nal_ref_idc and ( pNAL->nal_ref_idc == 0 or pNewNAL->nal_ref_idc == 0 )  )
			return 1;

		if( pNAL->nal_unit_type == NAL_UNIT_TYPE_SLICE_IDR and pNewNAL->nal_unit_type == NAL_UNIT_TYPE_SLICE_IDR )
			if( pNAL->idr_pic_id != pNewNAL->idr_pic_id )
				return 1;
		return 0;

	case NAL_UNIT_TYPE_SEI:
	case NAL_UNIT_TYPE_ACCESS_UNIT_DELIM:
	case NAL_UNIT_TYPE_SPS:
		switch( pNAL->nal_unit_type )
		{
		case NAL_UNIT_TYPE_SEI:
		case NAL_UNIT_TYPE_ACCESS_UNIT_DELIM:
		case NAL_UNIT_TYPE_SPS:
		case NAL_UNIT_TYPE_PPS:
		case NAL_UNIT_TYPE_SPS_EXT:
		case NAL_UNIT_TYPE_SPS_SVC:
			return 0;

		default:
			return 1;
		}

	case NAL_UNIT_TYPE_PPS:
	case NAL_UNIT_TYPE_SPS_EXT:
	case NAL_UNIT_TYPE_SPS_SVC:
		return 0;

	case 21:
		return 0; /* Legacy solution for MMLAB */

	default:
		return 1;
	}
}

int32_t H264_parse_NAL( const uint8_t *pBuffer, uint32_t iSize, NAL_t* pNAL )
{
  	/* Copy previous values into the new struct */
	NAL_t oNewNAL = *pNAL, *pNewNAL = &oNewNAL;

  	/* Save the NAL type */
   	pNewNAL->nal_unit_type = H264_NAL_type( pBuffer );
 	pNewNAL->nal_ref_idc = H264_NAL_idc( pBuffer );

  	/* SVC extension */
  	pNewNAL->svc = 0;
    if( H264_is_SVC( pBuffer ) )
    {
    	if( H264_parse_SVC( pBuffer, iSize, pNewNAL ) < 0 )
    		return -1;
    }

    /* Parse specific headers */
    switch( pNewNAL->nal_unit_type )
    {
    case NAL_UNIT_TYPE_SPS:
    	if(  H264_parse_SPS(pBuffer, iSize, pNewNAL ) < 0 )
    		return -1;
    	break;

    case NAL_UNIT_TYPE_PREFIX:
    	if( H264_parse_SVC( pBuffer, iSize, pNewNAL ) < 0 )
    		return -1;
    	break;

    case NAL_UNIT_TYPE_SLICE_SVC:
       	if( H264_parse_SVC( pBuffer, iSize, pNewNAL ) < 0 )
    	    return -1;

    case NAL_UNIT_TYPE_SLICE_NON_IDR:
    case NAL_UNIT_TYPE_SLICE_DP_A:
    case NAL_UNIT_TYPE_SLICE_IDR:
    	if( H264_parse_SLICE( pBuffer, iSize, pNewNAL ) < 0 )
    		return -1;
    	break;
    }

    /* Is this NAL part of a new frame? */
  	int iBoundary = H264_next_frame( pNAL, pNewNAL );
  	if( iBoundary < 0 )
  		return -1;
  	else if( iBoundary > 0 )
  		pNewNAL->pic_order_cnt_valid = 0;

  	/* Compute POC once a new slice starts */
	if( not pNewNAL->pic_order_cnt_valid and H264_is_NAL_slice( pNewNAL->nal_unit_type ) )
		H264_find_poc( pNewNAL );

	/* Copy results into the old parse struct */
	*pNAL = *pNewNAL;

	return iBoundary;
}

int32_t H264_parse_SLICE( const uint8_t* pBuffer, uint32_t iSize, NAL_t* pNAL )
{
	/* Remove the emulation bytes, copy the result into the tempoary buffer and initialize the parser */
	uint8_t pEmulation[324];
	int iHeader = H264_header_size( pBuffer );
	int iEmulationSize = H264_unemulate( pEmulation, pBuffer + iHeader, MIN(iSize-iHeader,sizeof(pEmulation)) );
	IBits oBuffer ( pEmulation, iEmulationSize );

	/* Default values */
	pNAL->field_pic_flag = 0;
	pNAL->bottom_field_flag = 0;

	/* Read data from the buffer */
	pNAL->first_mb_in_slice = oBuffer.read_uev();
	pNAL->slice_type = oBuffer.read_uev();
	pNAL->pic_parameter_set = oBuffer.read_uev();
	if( pNAL->residual_colour_transform_flag )
		oBuffer.read( 2 );

	/* Read the frame number, very important in seeing slices apart from the next frame */
	pNAL->frame_num = oBuffer.read( pNAL->log2_max_frame_num_minus4 + 4 );
	if( not pNAL->frame_mbs_only_flag )
	{
		pNAL->field_pic_flag = oBuffer.read( 1 );
		if( pNAL->field_pic_flag )
			pNAL->bottom_field_flag = oBuffer.read( 1 );
		}
	if( pNAL->nal_unit_type == NAL_UNIT_TYPE_SLICE_IDR )
		pNAL->idr_pic_id = oBuffer.read_uev();

	/* POC information */
	if( pNAL->pic_order_cnt_type == 0 )
		pNAL->pic_order_cnt_lsb = oBuffer.read( pNAL->log2_max_pic_order_cnt_lsb_minus4 + 4 );
	return 0;
}

int32_t H264_parse_SVC( const uint8_t* pBuffer, uint32_t iSize, NAL_t* pNAL )
{
	/* Load into a bitstream */
	IBits oBuffer ( pBuffer + H264_header_size( pBuffer ) - 3, 3 );

	/* Parse the svc header */
	pNAL->svc = 1;

	/* Reserved 1 */
	oBuffer.read( 1 );

	/* Priority */
	pNAL->idr_flag = oBuffer.read( 1 );
	pNAL->priority_id = oBuffer.read( 6 );
	pNAL->no_inter_layer_pred_flag = oBuffer.read( 1 );

	/* The scalable parameters */
	pNAL->dependency_id = oBuffer.read( 3 );
	pNAL->quality_id = oBuffer.read( 4 );
	pNAL->temporal_id = oBuffer.read( 3 );

	/* Flags */
	pNAL->use_base_prediction_flag = oBuffer.read( 1 );
	pNAL->discardable_flag = oBuffer.read( 1 );
	pNAL->output_flag = oBuffer.read( 1 );

	/* Reserved 3 */
	oBuffer.read( 2 );

	/* Succes */
	return 0;
}

int32_t H264_parse_VUI( IBits& oBuffer, NAL_t* vui )
{
	uint8_t aspect_ratio_info_present_flag = oBuffer.read(1);
	if( aspect_ratio_info_present_flag )
	{
		uint8_t aspect_ratio_idc = oBuffer.read(8);
		if( aspect_ratio_idc == 0xFF )
		{
			uint16_t sar_width  = oBuffer.read(16);
			uint16_t sar_height = oBuffer.read(16);
		}
	}
	uint8_t overscan_info_present_flag = oBuffer.read(1);
	if( overscan_info_present_flag )
		uint8_t overscan_appropriate_flag = oBuffer.read(1);

	uint8_t video_signal_type_present_flag = oBuffer.read(1);
	if( video_signal_type_present_flag )
	{
		uint8_t video_format = oBuffer.read(3);
		uint8_t video_full_range_flag = oBuffer.read(1);
		uint8_t colour_description_present_flag = oBuffer.read(1);
		if( colour_description_present_flag )
		{
			uint8_t colour_primaries = oBuffer.read(8);
			uint8_t transfer_characteristics = oBuffer.read(8);
			uint8_t matrix_coefficients = oBuffer.read(8);
		}
	}

	uint8_t chroma_loc_info_present_flag = oBuffer.read(1);
	if( chroma_loc_info_present_flag )
	{
		uint32_t chroma_sample_loc_type_top_field = oBuffer.read_uev();
		uint32_t chroma_sample_loc_type_bottom_field = oBuffer.read_uev();
	}

	if( vui->profile_idc == 83 or vui->profile_idc == 86 )
	{
		/* FIXME */
		return 0;
	}
	else
	{
		vui->timing_info_present_flag = oBuffer.read(1);
		if( vui->timing_info_present_flag )
		{
			vui->num_units_in_tick = oBuffer.read(32);
			vui->time_scale = oBuffer.read(32);
			vui->fixed_frame_rate_flag = oBuffer.read(1);
		}
	}
	return 0;
}

int32_t H264_parse_SPS( const uint8_t* pBuffer, uint32_t iSize, NAL_t* pNAL )
{
	/* Remove the emulation bytes, copy the result into the tempoary buffer and initialize the parser */
	uint8_t pEmulation[512];
	int iHeader = H264_header_size( pBuffer );
	int iEmulationSize = H264_unemulate( pEmulation, pBuffer + iHeader, MIN(iSize-iHeader,sizeof(pEmulation)) );
	IBits oBuffer( pEmulation, iEmulationSize );

	/* Set the different values according to the NAL standard */
	pNAL->profile_idc = oBuffer.read( 8 );
	oBuffer.read( 8 );
	pNAL->level_idc = oBuffer.read( 8 );
	pNAL->seq_parameter_set_id = oBuffer.read_uev();

	/* Ugly format for special types of SPS */
	if( pNAL->profile_idc == 100 or pNAL->profile_idc == 110 or pNAL->profile_idc == 122 or
		pNAL->profile_idc == 244 or pNAL->profile_idc == 44 or
		pNAL->profile_idc == 83 or pNAL->profile_idc == 86 )
	{
		pNAL->chroma_format_idc = oBuffer.read_uev();
		if( pNAL->chroma_format_idc == 3 )
			pNAL->residual_colour_transform_flag = oBuffer.read( 1 );
		pNAL->bit_depth_luma_minus8 = oBuffer.read_uev();
		pNAL->bit_depth_chroma_minus8 = oBuffer.read_uev();
		pNAL->qpprime_y_zero_transform_bypass_flag = oBuffer.read( 1 );
		pNAL->seq_scaling_matrix_present_flag = oBuffer.read( 1 );
		if( pNAL->seq_scaling_matrix_present_flag )
			for( int i = 0; i < (pNAL->chroma_format_idc != 3 ? 8 : 12); i++ )
				if( oBuffer.read( 1 ) )
					H264_scaling_list( oBuffer, i < 6 ? 16 : 64 );
	}
	/* Picture Order Count */
	pNAL->log2_max_frame_num_minus4 = oBuffer.read_uev();
	pNAL->pic_order_cnt_type = oBuffer.read_uev();
	if( pNAL->pic_order_cnt_type == 0 )
		pNAL->log2_max_pic_order_cnt_lsb_minus4 = oBuffer.read_uev();
	else if( pNAL->pic_order_cnt_type == 1 )
	{
		pNAL->delta_pic_order_always_zero_flag = oBuffer.read( 1 );
		pNAL->offset_for_non_ref_pic = oBuffer.read_sev();
		pNAL->offset_for_top_to_bottom_field = oBuffer.read_sev();
		pNAL->num_ref_frames_in_pic_order_cnt_cycle = oBuffer.read_uev();
		for( uint32_t i = 0; i < pNAL->num_ref_frames_in_pic_order_cnt_cycle; i++ )
			pNAL->offset_for_ref_frame[MIN(i,255)] = oBuffer.read_sev();
	}
	pNAL->num_ref_frames = oBuffer.read_uev();
	pNAL->gaps_in_frame_num_value_allowed_flag = oBuffer.read( 1 );

	/* Picture dimensions */
	pNAL->pic_width_in_mbs_minus1 = oBuffer.read_uev();
	pNAL->pic_height_in_map_units_minus1 = oBuffer.read_uev();
	pNAL->frame_mbs_only_flag = oBuffer.read( 1 );
	if( not pNAL->frame_mbs_only_flag )
		pNAL->mb_adpative_frame_field_flag = oBuffer.read( 1 );
	pNAL->direct_8x8_inference_flag = oBuffer.read( 1 );
	if( ( pNAL-> frame_cropping_flag = oBuffer.read( 1 ) ) )
	{
		pNAL->frame_crop_left_offset = oBuffer.read_uev();
		pNAL->frame_crop_right_offset = oBuffer.read_uev();
		pNAL->frame_crop_top_offset = oBuffer.read_uev();
		pNAL->frame_crop_bottom_offset = oBuffer.read_uev();
	}

	/* VUI parameters */
	if( ( pNAL->vui = oBuffer.read( 1 ) ) )
		H264_parse_VUI( oBuffer, pNAL );
	return 0;
}
