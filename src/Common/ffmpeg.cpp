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
#include "ffmpeg.h"
#include "Frame.h"

/* Loader class */
static const ffmpeg_loader ffmpeg;
mutex oFFMpegMutex;

/* Container formats */
AVInputFormat* pMovContainer = av_find_input_format( "mov,mp4,m4a,3gp,3g2,mj2" );
AVInputFormat* pFlvContainer = av_find_input_format( "flv" );
AVInputFormat* pMPEG2TSContainer = av_find_input_format( "mpegts" );
AVInputFormat* pMPEG2PSContainer = av_find_input_format( "mpeg" );
AVInputFormat* pMKVContainer = av_find_input_format( "matroska" );

container_t FFFormatToSirannonFormat( const AVInputFormat* pFormat )
{
	if( pFormat == pMovContainer )
		return container_t(mux_t::MOV, codec_t::NO);
	else if( pFormat == pFlvContainer )
		return container_t(mux_t::FLV, codec_t::NO);
	else if( pFormat == pMPEG2PSContainer )
		return container_t(mux_t::PS, codec_t::NO);
	else if( pFormat == pMPEG2TSContainer )
		return container_t(mux_t::TS, codec_t::NO);
	else if( pFormat == pMKVContainer )
		return container_t(mux_t::WEBM, codec_t::NO);
	else
		return container_t(mux_t::ES, codec_t::NO);
}

AVInputFormat* SirannonFormatToFFFormat( container_t oFormat )
{
	using namespace mux_t;
	switch( oFormat.first )
	{
	case TS:
		return pMPEG2TSContainer;
	case MOV:
		return pMovContainer;
	case FLV:
		return pFlvContainer;
	case WEBM:
		return pMKVContainer;
	case ES:
		return av_find_input_format( CodecToString( oFormat.second ) );
	default:
		return av_find_input_format( MuxToString( oFormat.first ) );
	}
}

ffmpeg_loader::ffmpeg_loader( void )
{
	Lock_t oLock( oFFMpegMutex );
	av_register_all();
	//av_log_set_level( iVerbose );
}

enum CodecID SirannonCodecToFFCodec( codec_t::type iCodec )
{
	switch( iCodec )
	{
	case codec_t::mp1a:
		return CODEC_ID_MP3;
	case codec_t::mp2a:
		return CODEC_ID_MP2;
	case codec_t::mp4a:
		return CODEC_ID_AAC;
	case codec_t::ac3:
		return CODEC_ID_AC3;
	case codec_t::mp1v:
		return CODEC_ID_MPEG1VIDEO;
	case codec_t::mp2v:
		return CODEC_ID_MPEG2VIDEO;
	case codec_t::mp4v:
		return CODEC_ID_MPEG4;
	case codec_t::avc:
		return CODEC_ID_H264;
	case codec_t::wma1:
		return CODEC_ID_WMAV1;
	case codec_t::wma2:
		return CODEC_ID_WMAV2;
	case codec_t::wmv1:
		return CODEC_ID_WMV1;
	case codec_t::wmv2:
		return CODEC_ID_WMV2;
	case codec_t::wmv3:
		return CODEC_ID_WMV3;
	case codec_t::wmvc:
		return CODEC_ID_VC1;
	case codec_t::vp6:
		return CODEC_ID_VP6;
	case codec_t::vp6f:
		return CODEC_ID_VP6F;
	case codec_t::vp8:
		return CODEC_ID_VP8;
	case codec_t::theora:
		return CODEC_ID_THEORA;
	case codec_t::vorbis:
		return CODEC_ID_VORBIS;
	case codec_t::yuv:
		return CODEC_ID_RAWVIDEO;
	case codec_t::anb:
		return CODEC_ID_AMR_NB;
	case codec_t::awb:
		return CODEC_ID_AMR_WB;
	case codec_t::flv1:
		return CODEC_ID_FLV1;
	case codec_t::swfa:
		return CODEC_ID_ADPCM_SWF;
	default:
		return CODEC_ID_NONE;
	}
}

codec_t::type FFCodecToSirannonCodec( enum CodecID x )
{
	switch( x )
	{
		case CODEC_ID_MP3:
			return codec_t::mp1a;
		case CODEC_ID_MP2:
			return codec_t::mp2a;
		case CODEC_ID_AAC:
			return codec_t::mp4a;
		case CODEC_ID_AC3:
			return codec_t::ac3;
		case CODEC_ID_MPEG1VIDEO:
			return codec_t::mp1v;
		case CODEC_ID_MPEG2VIDEO:
			return codec_t::mp2v;
		case CODEC_ID_MPEG4:
			return codec_t::mp4v;
		case CODEC_ID_H264:
			return codec_t::avc;
		case CODEC_ID_WMAV1:
			return codec_t::wma1;
		case CODEC_ID_WMAV2:
			return codec_t::wma2;
		case CODEC_ID_WMV1:
			return codec_t::wmv1;
		case CODEC_ID_WMV2:
			return codec_t::wmv2;
		case CODEC_ID_WMV3:
			return codec_t::wmv3;
		case CODEC_ID_FLV1:
			return codec_t::flv1;
		case CODEC_ID_VC1:
			return codec_t::wmvc;
		case CODEC_ID_VP6:
			return codec_t::vp6f;
		case CODEC_ID_VP6F:
			return codec_t::vp6f;
		case CODEC_ID_VP8:
			return codec_t::vp8;
		case CODEC_ID_THEORA:
			return codec_t::theora;
		case CODEC_ID_VORBIS:
			return codec_t::vorbis;
		case CODEC_ID_RAWVIDEO:
			return codec_t::yuv;
		case CODEC_ID_AMR_NB:
			return codec_t::anb;
		case CODEC_ID_AMR_WB:
			return codec_t::awb;
		case CODEC_ID_ADPCM_SWF:
			return codec_t::swfa;
		default:
			return codec_t::NO;
	}
}
