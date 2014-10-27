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
#include "sirannon.h"
#include "Frame.h"
#include "ffmpeg.h"
#include "FileManager.h"
#include "h264_avc.h"
#include "SirannonPrint.h"
#include "Utils.h"
using namespace codec_t;
using namespace frame_t;

extern const int ff_mpeg4audio_sample_rates[16];
extern const uint8_t ff_mpeg4audio_channels[8];

//
//rational_t getFraction( int iWidth, int iHeight )
//{
//	int c = gcd( iWidth, iHeight );
//	rational_t ret;
//	ret.num = iWidth / c;
//	ret.den = iHeight / c;
//	return ret;
//}

codec_t::type StringToCodec( const char* sCodec )
{
	if( not strcasecmp( sCodec, "vp8") )
		return vp8;
	if( not strcasecmp( sCodec, "vorbis") or not strcasecmp( sCodec, "libvorbis") )
		return vorbis;
	if( not strcasecmp( sCodec, "h264") or not strcasecmp( sCodec, "264" ) or not strcasecmp( sCodec, "avc") or not strcasecmp( sCodec, "avc1") )
		return avc;
	if( not strcasecmp( sCodec, "m1v") )
		return mp1v;
	if( not strcasecmp( sCodec, "m2v") )
		return mp2v;
	if( not strcasecmp( sCodec, "m4v") )
		return mp4v;
	if( not strcasecmp( sCodec, "m1a") )
		return mp1a;
	if( not strcasecmp( sCodec, "m2a") )
		return mp2a;
	if( not strcasecmp( sCodec, "m4a" ) or not strcasecmp( sCodec, "mp4a" ) or not strcasecmp( sCodec, "aac" ) )
		return mp4a;
	if( not strcasecmp( sCodec, "ac3") )
		return ac3;
	if( not strcasecmp( sCodec, "wmv1") )
		return wmv1;
	if( not strcasecmp( sCodec, "wmv2") )
		return wmv2;
	if( not strcasecmp( sCodec, "wmv3") )
		return wmv3;
	if( not strcasecmp( sCodec, "vmvc") )
		return wmvc;
	if( not strcasecmp( sCodec, "wma1") )
		return wma1;
	if( not strcasecmp( sCodec, "wma2") )
		return wma2;
	if( not strcasecmp( sCodec, "vp6") )
		return vp6;
	if( not strcasecmp( sCodec, "vp6f") )
		return vp6f;
	if( not strcasecmp( sCodec, "theora") )
		return theora;
	if( not strcasecmp( sCodec, "yuv") )
		return yuv;
	if( not strcasecmp( sCodec, "amr-wb") )
		return awb;
	if( not strcasecmp( sCodec, "amr-nb" ) )
		return anb;
	if( not strcasecmp( sCodec, "amr-wb+" ) )
		return awbp;
	if( not strcasecmp( sCodec, "yuv" ) )
		return yuv;
	if( not strcasecmp( sCodec, "pcm" ) )
		return pcm;
	if( not strcasecmp( sCodec, "H.263" ) )
		return h263;
	if( not strcasecmp( sCodec, "FLV1" ) )
		return flv1;
	if( not strcasecmp( sCodec, "SWFa" ) )
		return swfa;
	return NO;
}

const char* CodecToString( codec_t::type codec )
{
	switch( codec )
	{
		case mp1v:
			return "m1v";
		case mp2v:
			return "m2v";
		case mp4v:
			return "m4v";
		case mp1a:
			return "m1a";
		case mp2a:
			return "m2a";
		case mp4a:
			return "aac";
		case ac3:
			return "ac3";
		case avc:
			return "h264";
		case svc:
			return "svc";
		case wmv1:
			return "wmv1";
		case wmv2:
			return "wmv2";
		case wmv3:
			return "WMV3";
		case wmvc:
			return "wmvc";
		case wma1:
			return "wma1";
		case wma2:
			return "wma2";
		case vp6:
			return "vp6";
		case vp6f:
			return "vp6f";
		case vp8:
			return "vp8";
		case theora:
			return "theora";
		case vorbis:
			return "vorbis";
		case yuv:
			return "yuv";
		case awb:
			return "arm-wb";
		case anb:
			return "arm-nb";
		case awbp:
			return "amr-wb+";
		case pcm:
			return "pcm";
		case h263:
			return "H.263";
		case flv1:
			return "FLV1";
		case swfa:
			return "SWFa";
		case NO:
			return "NO";
		default:
			return "unknown";
	}
}

uint8_t GetPesID( codec_t::type codec )
{
	switch( codec )
	{
		case mp1v:
		case mp2v:
		case mp4v:
		case avc:
		case svc:
			return 0xE0;

		case mp1a:
		case mp2a:
		case mp4a:
			return 0xC0;

		case ac3:
			return 0xBD;

		case wmv1:
		case wmv2:
		case wmv3:
		case wmvc:
			return 0xA0; /* non standard case supported by vlc */

		default:
			return 0x00;
	}
}

codec_t::type GetPmtCodec( int iPMT )
{
	switch( iPMT )
	{
		case 0x01:
			return mp1v;

		case 0x02:
			return mp2v;

		case 0x10:
			return mp4v;

		case 0x1B:
			return avc;

		case 0x03:
			return mp1a;

		case 0x04:
			return mp2a;

		case 0x11:
		case 0x0F:
			return mp4a;

		case 0xA0:
			return wmvc; /* non standard case supported by vlc */

		default:
			return NO;
	}
}

uint8_t GetPmtID( codec_t::type codec )
{
	switch( codec )
	{
		case mp1v:
			return 0x01;

		case mp2v:
			return 0x02;

		case mp4v:
			return 0x10;

		case avc:
		case svc:
			return 0x1B;

		case mp1a:
			return 0x03;

		case mp2a:
			return 0x04;

		case mp4a:
			return 0x0F; // 0x11;

		case ac3:
			return 0x81; /* non standard case supported by vlc */

		case wmv1:
		case wmv2:
		case wmv3:
		case wmvc:
			return 0xA0; /* non standard case supported by vlc */

		default:
			return 0x00;
	}
}

const char* FrameToString( frame_t::type frame )
{
	switch( frame )
	{
		case I:
			return "I";
		case IDR:
			return "IDR";
		case B:
			return "B";
		case P:
			return "P";
		case EI:
			return "EI";
		case EB:
			return "EB";
		case EP:
			return "EP";
		case SP:
			return "SP";
		case SI:
			return "SI";
		case SEI:
			return "SEI";
		case PPS:
			return "PPS";
		case SPS:
			return "SPS";
		case ESPS:
			return "ESPS";
		case E:
			return "E";
		case AUD:
			return "AUD";
		case AUDH:
			return "AH";
		case no_frame:
			return "N/A";
		case BI:
			return "I(B)";
		case BP:
			return "P(B)";
		case BB:
			return "B(B)";
		case CI:
			return "I(C)";
		case CP:
			return "P(C)";
		case CB:
			return "B(C)";
		case frame_t::other:
		default:
			return "other";
	}
}

char FrameToChar( frame_t::type frame )
{
	switch( frame )
	{
		case I:
		case IDR:
		case EI:
		case SI:
		case BI:
		case CI:
			return 'I';
		case B:
		case EB:
		case BP:
		case BB:
		case CB:
			return 'B';
		case P:
		case EP:
		case SP:
		case CP:
			return 'P';
		case SEI:
		case PPS:
		case SPS:
		case ESPS:
		case E:
		case no_frame:
		case frame_t::other:
		case AUD:
		default:
			return 'H';
	}
}

content_t::type CodecToContent( codec_t::type oCodec )
{
	switch( oCodec )
	{
	case avc:
	case svc:
	case mp1v:
	case mp2v:
	case mp4v:
	case wmv1:
	case wmv2:
	case wmv3:
	case wmvc:
	case vp6:
	case vp6f:
	case vp8:
	case yuv:
	case h263:
	case theora:
	case flv1:
		return content_t::video;

	case mp1a:
	case mp2a:
	case mp4a:
	case ac3:
	case wma1:
	case wma2:
	case vorbis:
	case anb:
	case awb:
	case awbp:
	case pcm:
	case swfa:
		return content_t::audio;

	default:
		return content_t::mixed;
	}
}

const char* ContentToString( content_t::type content )
{
	switch( content )
	{
	case content_t::audio:
		return "audio";
	case content_t::video:
		return "video";
	case content_t::mixed:
	default:
		return "mixed";
	}
}

const char* TargetToString( target_t::type iTarget )
{
	switch( iTarget )
	{
		using namespace target_t;
		case target_t::NO:
			return "NO";
		case IPAD1:
			return "IPAD1";
		case IPAD2:
			return "IPAD2";
		case IPHONE3:
			return "IPHONE3";
		case IPHONE4:
			return "IPHONE4";
		case IPHONE5:
			return "IPHONE5";
		case ANDROID:
			return "ANDROID";
		case YOUTUBE:
			return "YouTube";
		case SDTV:
			return "SDTV";
		case HDTV:
			return "HDTV";
		default:
			return "NO";
	}
}

const char* ProfileToString( int iProfile )
{
	switch( iProfile )
	{
	case H264_PROFILE_STR_BASELINE:
		return "Baseline";
	case H264_PROFILE_STR_MAIN:
		return "Main";
	case H264_PROFILE_STR_EXTENDED:
		return "Extended";
	default:
		return "N/A";
	}
}

const char* LevelToString( int iLevel )
{
	switch( iLevel )
	{
	case H264_LEVEL_STR_1:
		return "1.0";
	case H264_LEVEL_STR_1_1:
		return "1.1";
	case H264_LEVEL_STR_1_2:
		return "1.2";
	case H264_LEVEL_STR_1_3:
		return "1.3";
	case H264_LEVEL_STR_1_b:
		return "1.b";
	case H264_LEVEL_STR_2:
		return "2.0";
	case H264_LEVEL_STR_2_1:
		return "2.1";
	case H264_LEVEL_STR_2_2:
		return "2.2";
	case H264_LEVEL_STR_3:
		return "3.0";
	case H264_LEVEL_STR_3_1:
		return "3.1";
	case H264_LEVEL_STR_3_2:
		return "3.2";
	case H264_LEVEL_STR_4:
		return "4.0";
	case H264_LEVEL_STR_4_1:
		return "4.1";
	case H264_LEVEL_STR_4_2:
		return "4.2";
	case H264_LEVEL_STR_5:
		return "5.0";
	case H264_LEVEL_STR_5_1:
		return "5.1";
	default:
		return "N/A";
	}
}

mux_t::type StringToMux( const char* sMux )
{
	using namespace mux_t;
	if( strcasecmp( sMux, "mp4" ) == 0 or  strcasecmp( sMux, "mov" ) == 0 or strcasecmp( sMux, "3gp" ) == 0 )
		return MOV;
	if( strcasecmp( sMux, "flv" ) == 0 )
		return FLV;
	if( strcasecmp( sMux, "webm" ) == 0 )
		return mux_t::WEBM;
	if( strcasecmp( sMux, "ts") == 0 )
		return TS;
	if( strcasecmp( sMux, "m3u" ) == 0 or strcasecmp( sMux, "m3u8" ) == 0 )
		return M3U;
	if( strcasecmp( sMux, "mpeg" ) == 0 or strcasecmp( sMux, "mpg" ) == 0 )
		return PS;
	if( strcasecmp( sMux, "avi" ) == 0 )
		return AVI;
	if( strcasecmp( sMux, "wmv" ) == 0 )
		return mux_t::WMV;
	if( strcasecmp( sMux, "mkv" ) == 0 )
		return MKV;
	if( strcasecmp( sMux, "ogg" ) == 0 )
		return OGG;
	return mux_t::NO;
}

container_t StringToContainer( const char* sExtension )
{
	container_t oVal( mux_t::NO, codec_t::NO );
	oVal.first = StringToMux( sExtension );
	oVal.second = StringToCodec( sExtension );
	if( oVal.first == mux_t::NO and oVal.second != codec_t::NO )
		oVal.first = mux_t::ES;
	if( oVal.first != mux_t::ES )
		oVal.second = codec_t::NO;
	return oVal;
}

const char* MuxToString( mux_t::type iMux )
{
	using namespace mux_t;
	switch( iMux )
	{
	case ES:
		return "ES";
	case MOV:
		return "mov";
	case RTP:
		return "RTP";
	case RTMP:
		return "RTMP";
	case FLV:
		return "flv";
	case TS:
		return "TS";
	case PS:
		return "MPEG";
	case PES:
		return "PES";
	case mux_t::WEBM:
		return "webm";
	case M3U:
		return "m3u";
	case AVI:
		return "avi";
	case MKV:
		return "mvk";
	case mux_t::WMV:
		return "wmv";
	case OGG:
		return "ogg";
	default:
		return "N0";
	}
}

const char* ContainerToMime( container_t oFormat )
{
	using namespace mux_t;
	using namespace codec_t;
	switch( oFormat.first )
	{
	case TS:
		return "video/MP2T";

	case M3U:
		return "application/vnd.apple.mpegurl";

	case MOV:
		return "video/mp4";

	case mux_t::WEBM:
		return "video/webm";

	case FLV:
		return "video/x-flv";

	case PS:
		return "video/x-mpeg";

	case MKV:
		return "video/x-matroska";

	case AVI:
		return "video/avi";

	case mux_t::WMV:
		return "video/x-ms-wmv";

	case OGG:
		return "video/ogg";

	case ES:
		switch( oFormat.second )
		{
		case mp1v:
		case mp2v:
			return "video/x-mpeg";

		case mp4v:
			return "video/mpeg4-generic";

		case avc:
			return "video/H264";

		case ac3:
			return "audio/ac3";

		case vorbis:
			return "audio/vorbis";

		case mp4a:
			return "audio/mpeg4-generic";

		case mp2a:
			return "audio/x-mpeg";

		case mp1a:
			return "audio/mpeg3";

		default:
			break;
		}
	default:
		break;
	}
	ValueError( "Unknown container type(%s/%s)", MuxToString(oFormat.first), CodecToString(oFormat.second) );
	return "";
}

container_t MimeToContainer( const char* sMime )
{
	using namespace mux_t;
	using namespace codec_t;
	container_t iContainer;
	iContainer.first = ES;

	if( strcasecmp( sMime, "video/mp4" ) == 0 or
		strcasecmp( sMime, "video/3gpp" ) == 0 or
		strcasecmp( sMime, "video/3gp" ) == 0 or
		strcasecmp( sMime, "video/quicktime" ) == 0 )
	{
		iContainer.first = MOV;
	}
	else if( 	strcasecmp( sMime, "video/mpeg" ) == 0 or
				strcasecmp( sMime, "video/x-mpeg" ) == 0 )
	{
		iContainer.first = PS;
	}
	else if( strcasecmp( sMime, "video/avi" ) == 0 or
			 strcasecmp( sMime, "video/x-msvideo" ) == 0 or
			 strcasecmp( sMime, "video/msvideo" ) == 0 )
	{
		iContainer.first = AVI;
	}
	else if( strcasecmp( sMime, "video/x-flv" ) == 0 )
	{
		iContainer.first = FLV;
	}
	else if( strcasecmp( sMime, "video/webm" ) == 0 )
	{
		iContainer.first = WEBM;
	}
	else if( strcasecmp( sMime, "video/MP2T" ) == 0 )
	{
		iContainer.first = TS;
	}
	else if( strcasecmp( sMime, "application/vnd.apple.mpegurl" ) == 0 )
	{
		iContainer.first = M3U;
	}
	else if( strcasecmp( sMime, "video/x-matroska" ) == 0 )
	{
		iContainer.first = MKV;
	}
	else if( strcasecmp( sMime, "video/ogg" ) == 0 or
			 strcasecmp( sMime, "audio/ogg" ) == 0 or
			 strcasecmp( sMime, "application/ogg" ) == 0 )
	{
		iContainer.first = OGG;
	}
	else if( strcasecmp( sMime, "video/x-ms-wmv" ) == 0 )
	{
		iContainer.first = mux_t::WMV;
	}
	/* Raw formats */
	else if( strcasecmp( sMime, "video/x-mpeg" ) == 0 )
	{
		iContainer.second = mp2v;
	}
	else if( strcasecmp( sMime, "video/mpeg4-generic" ) == 0 )
	{
		iContainer.second = mp4v;
	}
	else if( strcasecmp( sMime, "video/H264" ) == 0 )
	{
		iContainer.second = avc;
	}
	else if( strcasecmp( sMime, "audio/ac3" ) == 0 )
	{
		iContainer.second = ac3;
	}
	else if( strcasecmp( sMime, "audio/vorbis" ) == 0 )
	{
		iContainer.second = vorbis;
	}
	else if( strcasecmp( sMime, "audio/mpeg4-generic" ) == 0 )
	{
		iContainer.second = mp4a;
	}
	else if( strcasecmp( sMime, "audio/x-mpeg" ) == 0 )
	{
		iContainer.second = mp2a;
	}
	else if( strcasecmp( sMime, "audio/mpeg3" ) == 0 )
	{
		iContainer.second = mp1a;
	}
	else
		ValueError( "Unknown MIME type(%s)", sMime );
	return iContainer;
}

bool IsIFrame( const MediaPacket* pPckt )
{
	switch( pPckt->frame )
	{
	case I:
	case IDR:
	case SI:
	case EI:
		return true;
	default:
		return false;
	}
}

bool IsSlice( const MediaPacket* pPckt )
{
	switch( pPckt->frame )
	{
	case IDR:
	case I:
	case SI:
	case EI:
	case P:
	case SP:
	case EP:
	case B:
	case EB:
	case BI:
	case BP:
	case BB:
	case CI:
	case CP:
	case CB:
		return true;
	default:
		return false;
	}
}

frame_t::type H264FrameToXFrame( int iNALType, int iSliceType )
{
	/* What other kind of NAL unit is it? */
	switch( iNALType )
	{
	case NAL_UNIT_TYPE_SEI:
		return SEI;

	case NAL_UNIT_TYPE_SPS:
		return SPS;

	case NAL_UNIT_TYPE_SPS_SVC:
	case NAL_UNIT_TYPE_SPS_EXT:
		return ESPS;

	case NAL_UNIT_TYPE_PPS:
		return PPS;

	case NAL_UNIT_TYPE_PREFIX:
		return E;

	case NAL_UNIT_TYPE_ACCESS_UNIT_DELIM:
		return AUD;

	case NAL_UNIT_TYPE_END_OF_SEQ:
	case NAL_UNIT_TYPE_END_OF_STREAM:
	case NAL_UNIT_TYPE_FILLER_DATA:
		return frame_t::other;

	case NAL_UNIT_TYPE_SLICE_IDR:
		return IDR;

	case NAL_UNIT_TYPE_SLICE_DP_B:
	case NAL_UNIT_TYPE_SLICE_DP_C:
		return frame_t::other;

	case NAL_UNIT_TYPE_SLICE_SVC:
		switch( iSliceType )
		{
		case SLICE_TYPE_EP:
			return EP;

		case SLICE_TYPE_EB:
			return EB;

		case SLICE_TYPE_EI:
			return EI;

		default:
			return no_frame;
		}

	case NAL_UNIT_TYPE_SLICE_DP_A:
	case NAL_UNIT_TYPE_SLICE_NON_IDR:
		switch( iSliceType )
		{
		case SLICE_TYPE_I:
		case SLICE_TYPE_I2:
			return I;

		case SLICE_TYPE_SI:
		case SLICE_TYPE_SI2:
			return SI;

		case SLICE_TYPE_P:
		case SLICE_TYPE_P2:
			return P;

		case SLICE_TYPE_SP:
		case SLICE_TYPE_SP2:
			return SP;

		case SLICE_TYPE_B:
		case SLICE_TYPE_B2:
			return B;

		default:
			return no_frame;
		}

	default:
		return no_frame;
	}
}

void setH264FrameType( MediaPacket* pPckt )
{
	/* Which NAL type? */
	const uint8_t* pData = pPckt->data();
	int iSize = pPckt->size();
	int iNAL = H264_NAL_type( pData );

	/* Find the slice type if it is present */
	int iSlice = -1;
	if( iNAL == NAL_UNIT_TYPE_SLICE_DP_A or
		iNAL == NAL_UNIT_TYPE_SLICE_NON_IDR or
		iNAL == NAL_UNIT_TYPE_SLICE_SVC )
	{
		/* Skip the header */
		IBits oFrame( pData, iSize );
		oFrame.seek( H264_header_size( pData ) );

		/* Read the slice type */
		oFrame.read_uev();
		iSlice = oFrame.read_uev();
	}
	/* Now we can deduce the frame type */
	frame_t::type iFrame = H264FrameToXFrame( iNAL, iSlice );
	pPckt->frame = iFrame;
}

container_t ExtensionToContainer( const string& sMedia )
{
	int iPos = sMedia.rfind( '.' );
	if( iPos != string::npos )
		return StringToContainer( sMedia.c_str() + iPos + 1 );
	return container_t( mux_t::NO, codec_t::NO );
}

codec_t::type VerifyCodecForContainer( container_t iContainer, codec_t::type iCodec )
{
	using namespace mux_t;
	using namespace codec_t;

	int iContent = CodecToContent( iCodec );

	switch( iContainer.first )
	{
	case ES:
		return iCodec;

	case FLV:
	case RTMP:
		if( iCodec == mp1a or iCodec == vp6 or iCodec == vp6f or iCodec == avc or
			iCodec == mp4a or iCodec == flv1 or iCodec == swfa )
			return iCodec;
		if( iContent == content_t::video )
			return avc;
		if( iContent == content_t::audio )
			return mp4a;
		return codec_t::NO;

	case MOV:
		if( iContent == content_t::video )
			return avc;
		if( iContent == content_t::audio )
			return mp4a;
		return codec_t::NO;

	case WEBM:
		if( iContent == content_t::video )
			return vp8;
		if( iContent == content_t::audio )
			return vorbis;
		return codec_t::NO;

	case TS:
	case RTP:
		switch( iCodec )
		{
		case mp1a:
		case mp1v:
		case mp2a:
		case mp2v:
		case mp4a:
		case mp4v:
		case awbp:
		case awb:
		case anb:
		case avc:
		case svc:
		case ac3:
			return iCodec;

		default:
			if( iContent == content_t::video )
				return avc;
			if( iContent == content_t::audio )
				return mp4a;
			return codec_t::NO;
		}

	case PS:
		if( iCodec == mp1a or iCodec == mp2a )
			return iCodec;
		if( iCodec == mp1v or iCodec == mp2v )
			return iCodec;
		return codec_t::NO;

	default:
		return codec_t::NO;
	}
}

void H264_global_header( deque_t& vSPS, deque_t& vPPS, uint8_t* pExtraData, uint32_t* iExtraSize )
{ }

void convert_header_MP4( MediaPacket* pPckt )
{
	if( pPckt->codec == avc or pPckt->codec == svc  )
	{
		pPckt->pop_front( H264_is_start_code( pPckt->data() ) );
		pPckt->push_front( 4 );
		OBits oHeader( pPckt->data(), 4 );
		oHeader.write( 32, pPckt->size() - 4 );
	}
	else if( pPckt->codec == mp4a )
	{
		IBits oHeader( pPckt->data(), pPckt->size() );
		if( oHeader.peek( 12 ) == 0xFFF )
			pPckt->pop_front( 7 );
	}
	else
		TypeError( "Invalid codec(%s): %s", CodecToString(pPckt->codec), pPckt->c_str_long() );
}

int convert_header_ES( MediaPacket* pPckt )
{
	if( pPckt->codec == avc or pPckt->codec == svc  )
	{
		IBits oHeader( pPckt->data(), 4 );
		uint32_t iSize = oHeader.read( 8 * pPckt->desc->lengthSize );
		pPckt->pop_front( pPckt->desc->lengthSize );
		pPckt->push_front( H264_START_CODE, 4 );
		return iSize + 4;
	}
	else if( pPckt->codec == mp4a )
	{
		RuntimeError( "Audio conversion not yet supported" );
	}
	else
		TypeError( "Invalid codec(%s): %s", CodecToString(pPckt->codec), pPckt->c_str_long() );
	return 0;
}

MediaPacketPtrRef mergeFrameParts( deque_t& vBuffer, MediaPacketPtr& pWrappedPckt, bool bConvertMp4 )
{
	/* Add part */
	MediaPacket* pPckt = pWrappedPckt.release();
	if( pPckt )
		vBuffer.push_back( pPckt );
	else
		pPckt = vBuffer.back();

	/* Collect an entire frame */
	MediaPacketPtr pMerged;
	if( pPckt->frameend )
	{
		/* Only create the aggregated packet if needed */
		if( vBuffer.size() > 1 )
		{
			/* Aggregate size */
			int iSize = 0;
			for( deque_it i = vBuffer.begin(); i != vBuffer.end(); i++ )
				iSize += (*i)->size();

			/* Create an aggregated packet */
			pMerged = MediaPacketPtr( new MediaPacket( packet_t::media, content_t::video, iSize ) );
			pMerged->set_metadata( vBuffer.front() );
			pMerged->framestart = pMerged->frameend = true;
			pMerged->subframenumber = 0;

			/* Aggregate the NAL's into one frame */
			while( not vBuffer.empty() )
			{
				MediaPacketPtr pWrappedPckt( vBuffer.front() );
				vBuffer.pop_front();

				/* Change header */
				if( bConvertMp4 )
					convert_header_MP4( pWrappedPckt.get() );
				pMerged->push_back( pWrappedPckt->data(), pWrappedPckt->size() );
			}
		}
		else
		{
			/* Change header */
			if( bConvertMp4 )
				convert_header_MP4( pPckt );

			pMerged = MediaPacketPtr( pPckt );
			vBuffer.clear();
		}
	}
	return pMerged;
}

/* Sirannon conventions:
 * MediaDescriptor profile is the profile values as written in the esds header of MP4 containers. ADTS headers
 * will contain as profile field the value "profile-1".
 *
 *
 * Much AAC data is encapsulated in MPEG-4 files which is an extension of the QuickTime container format.
 * The MPEG-4 file will have an audio 'trak' atom which will contain a 'stsd' description atom which will
 * contain an 'mp4a' atom which will contain an 'esds' atom. Part of the esds atom contains the setup data
 *
	5 bits: object type
	if (object type == 31) [IGNORED]
		6 bits + 32: object type
	4 bits: frequency index
	if (frequency index == 15) [IGNORED]
		24 bits: frequency
	4 bits: channel configuration
	var bits: AOT Specific Config
 *
 * http://wiki.multimedia.cx/index.php?title=MPEG-4_Audio*
*/
MP4MediaConverter::MP4MediaConverter( bool bFixPTS, bool bSkipSEI, bool bSkipAUD )
	: iUnit(0), oVideoExtra(64000), oAudioExtra(32), iSub(0), bFirstVideo(true),
	  iAudioHeaderVersion(-1), iVideoHeaderVersion(-1), iVideoParamVersion(-2),
	  bSkipSEI(bSkipSEI), bSkipAUD(bSkipAUD), bFixPTS(bFixPTS), oAudioHeader(4096)
{
	memset( &oNAL, 0, sizeof(oNAL) );
}

MP4MediaConverter::~MP4MediaConverter()
{
	clearParameterSets();
	while( not vVideo.empty() )
	{
		delete vVideo.front();
		vVideo.pop_front();
	}
}

void MP4MediaConverter::skipNAL( bool bSkipSEI, bool bSkipAUD )
{
	this->bSkipSEI = bSkipSEI;
	this->bSkipAUD = bSkipSEI;
}

MediaPacketPtrRef MP4MediaConverter::convertMP4( MediaPacketPtr& pPckt )
{
	/* Easy when if it is already mp4 */
	if( pPckt->mux != mux_t::ES )
		TypeError( "MP4Converter: Packet is not in ES format: %s", pPckt->c_str_long() );

	/* Convert the raw frames */
	MediaPacketPtr pMerged;
	pPckt->mux = mux_t::MOV;
	if( pPckt->codec == avc or pPckt->codec == svc )
	{
		/* Filter out SPS & PPS */
		if( pPckt->frame == frame_t::SPS or pPckt->frame == frame_t::PPS )
		{
			/* Repeat of parameter sets? */
			if( pPckt->framenumber != iVideoParamVersion )
				clearParameterSets();
			iVideoParamVersion = pPckt->framenumber;

			/* Store */
			if( pPckt->frame == frame_t::SPS )
				vSPS.push_back( pPckt.release() );
			else
				vPPS.push_back( pPckt.release() );
		}
		else
		{
			/* Merge it into one frame */
			pMerged = mergeFrameParts( vVideo, pPckt, true );
			if( pMerged.get() )
			{
				pMerged->unitnumber = iUnit++;

				/* Generate extra data */
				if( iVideoParamVersion != iVideoHeaderVersion )
				{
					if( ANNEXB2AVCC( pMerged->desc ) < 0 )
					{
						/* Only error if no earlier extra data exists */
						if( not pMerged->desc->getExtraSize() )
							RuntimeError( "MP4Converter: Could not create video global header: %s", pMerged->c_str() );
					}
					iVideoHeaderVersion = iVideoParamVersion;
				}
			}
		}
	}
	else if( pPckt->codec == mp4a )
	{
		/* Store if needed */
		if( not oAudioHeader.size() )
			memcpy( oAudioHeader.data(), pPckt->data(), 7 );

		/* Copy the header */
		if( iAudioHeaderVersion < 0 )
		{
			/* Copy the header */
			if( ADTS2ESDS( pPckt->desc ) < 0 )
			{
				/* Only error if no earlier extra data exists */
				if( not pPckt->desc->getExtraSize() )
					RuntimeError( "MP4Converter: could not create audio global header" );
			}
			iAudioHeaderVersion = pPckt->framenumber;
		}
		/* Strip header */
		pPckt->pop_front( 7 );

		/* No need for a new packet */
		pMerged = pPckt;
	}
	else
		TypeError( "MP4Converter: invalid codec(%s), %s", CodecToString(pPckt->codec), pPckt->c_str_short() );

	return pMerged;
}

MediaPacketPtrRef MP4MediaConverter::convertMP4Video( const uint8_t* pBuffer, int iSize, MediaDescriptor* pDesc )
{
	MediaPacketPtr pPckt( new MediaPacket( packet_t::media, content_t::video, iSize ) );
	pPckt->mux = mux_t::MOV;

	/* Make parameter sets if this block contains an SPS */
	bool bExtract = false;
	clearParameterSets();

	/* Read each NAL and write to packet */
	int iPos = 0;
	while( iPos < iSize )
	{
		int iNAL = H264_find_NAL( pBuffer + iPos, iSize - iPos );
		int iType = H264_NAL_type( pBuffer + iPos );
		int iHeader = H264_is_start_code( pBuffer + iPos );

		/* Skip paramter sets */
		MediaPacket* pParam = NULL;
		switch( iType )
		{
		case NAL_UNIT_TYPE_SPS:
		case NAL_UNIT_TYPE_SPS_EXT:
		case NAL_UNIT_TYPE_SPS_SVC:
		case NAL_UNIT_TYPE_PPS:
			if( not bExtract )
				clearParameterSets();
			bExtract = true;
			pParam = new MediaPacket( packet_t::media, content_t::video, iNAL );
			pParam->push_back( pBuffer + iPos, iNAL );
			if( iType == NAL_UNIT_TYPE_PPS )
				vPPS.push_back( pParam );
			else
				vSPS.push_back( pParam );
			break;

		case NAL_UNIT_TYPE_ACCESS_UNIT_DELIM:
		case NAL_UNIT_TYPE_SEI:
			if( bSkipSEI and iType == NAL_UNIT_TYPE_SEI )
				break;
			if( bSkipAUD and iType == NAL_UNIT_TYPE_ACCESS_UNIT_DELIM )
				break;

		default:
			OBits oHeader( pPckt->data() + pPckt->size(), 4 );
			oHeader.write( 32, iNAL - iHeader );

			//pPckt->push_back( oHeader.data(), oHeader.size() );
			pPckt->push_back( oHeader.size() );
			pPckt->push_back( pBuffer + iPos + iHeader, iNAL - iHeader );

			if( bExtract )
			{
				ANNEXB2AVCC( pDesc );
				bExtract = false;
			}
		}
		iPos += iNAL;
	}
	/* Send the MP4 packet */
	return pPckt;
}

#ifdef ZENON
MediaPacketPtrRef MP4MediaConverter::convertMP4Video( RTP_DataFrameList& output, MediaDescriptor* pDesc )
{
	int iSize = 0;
	for( RTP_DataFrameList::iterator it = output.begin(); it != output.end(); ++it )
		iSize += it->GetPayloadSize() + 4;

	MediaPacketPtr pPckt( new MediaPacket( packet_t::media, content_t::video, iSize ) );
	pPckt->mux = mux_t::MOV;

	/* Make parameter sets if this block contains an SPS */
	bool bExtract = false;
	clearParameterSets();

	/* Read each NAL and write to packet */
	for( RTP_DataFrameList::iterator it = output.begin(); it != output.end(); ++it )
	{
		const uint8_t* pBuffer = it->GetPayloadPtr();
		const int iNAL = it->GetPayloadSize();
		int iType = H264_NAL_type( pBuffer );

		/* Skip parameter sets */
		MediaPacket* pParam = NULL;
		switch( iType )
		{
		case NAL_UNIT_TYPE_SPS:
		case NAL_UNIT_TYPE_SPS_EXT:
		case NAL_UNIT_TYPE_SPS_SVC:
		case NAL_UNIT_TYPE_PPS:
			if( not bExtract )
				clearParameterSets();
			bExtract = true;
			pParam = new MediaPacket( packet_t::media, content_t::video, iNAL );
			pParam->push_back( pBuffer, iNAL );
			if( iType == NAL_UNIT_TYPE_PPS )
				vPPS.push_back( pParam );
			else
				vSPS.push_back( pParam );
			break;

		case NAL_UNIT_TYPE_ACCESS_UNIT_DELIM:
		case NAL_UNIT_TYPE_SEI:
			if( bSkipSEI and iType == NAL_UNIT_TYPE_SEI )
				break;
			if( bSkipAUD and iType == NAL_UNIT_TYPE_ACCESS_UNIT_DELIM )
				break;

		default:
			OBits oHeader( pPckt->data() + pPckt->size(), 4 );
			oHeader.write( 32, iNAL );

			pPckt->push_back( oHeader.size() );
			pPckt->push_back( pBuffer, iNAL);

			if( bExtract )
			{
				if( pDesc )
					ANNEXB2AVCC( pDesc );
				bExtract = false;
			}
		}
	}
	/* Send the MP4 packet */
	return pPckt;
}
#endif


MediaPacketPtrRef MP4MediaConverter::convertMP4Audio( const uint8_t* pBuffer, int iSize, MediaDescriptor* pDesc )
{
	/* Simply create a packet without the 7 start bytes */
	MediaPacketPtr pPckt( new MediaPacket( packet_t::media, content_t::audio, iSize - 7 ) );
	pPckt->mux = mux_t::MOV;
	pPckt->push_back( pBuffer + 7, iSize - 7 );

	/* Remember header */
	if( not oAudioExtra.size() )
	{
		memcpy( oAudioHeader.data(), pBuffer, 7 );
		ADTS2ESDS( pDesc );
	}
	return pPckt;
}


int MP4MediaConverter::buildVideoExtraData( const uint8_t* pBuffer, int iSize, MediaDescriptor* pDesc )
{
	/* Go to the first SPS packet */
	while( H264_NAL_type( pBuffer ) != NAL_UNIT_TYPE_SPS )
	{
		int iNAL = H264_find_NAL( pBuffer, iSize );
		pBuffer += iNAL;
		iSize -= iNAL;
	}
	/* Profile information in first SPS packet */
	int iHeader = H264_header_size( pBuffer );

	/* Start header */
	oVideoExtra.clear();
	oVideoExtra.write( 8, 0x01 ); /* version */
	oVideoExtra.write( 8, pBuffer[iHeader] ); /* profile */
	oVideoExtra.write( 8, pBuffer[iHeader+1] ); /* profile compat */
	oVideoExtra.write( 8, pBuffer[iHeader+2] ); /* level */
	oVideoExtra.write( 8, 0xFF ); /* 6 bits reserved (111111) + 2 bits nal size length - 1 (11) */

	/* Write SPSs */
	int iSPSIdx = oVideoExtra.size();
	uint8_t iSPS = 0;
	oVideoExtra.seek( 1, SEEK_CUR );

	while( iSize > 0 and H264_NAL_type( pBuffer ) != NAL_UNIT_TYPE_PPS )
	{
		int iNAL = H264_find_NAL( pBuffer, iSize );
		int iHeader = H264_is_start_code( pBuffer );

		oVideoExtra.write( 16, iNAL - iHeader );
		oVideoExtra.write_buffer( pBuffer + iHeader, iNAL - iHeader );

		pBuffer += iNAL;
		iSize -= iNAL;
		iSPS++;
	}

	/* Write PPSs */
	int iPPSIdx = oVideoExtra.size();
	uint8_t iPPS = 0;
	oVideoExtra.seek( 1, SEEK_CUR );

	while( iSize > 0 and H264_NAL_type( pBuffer ) == NAL_UNIT_TYPE_PPS )
	{
		int iNAL = H264_find_NAL( pBuffer, iSize );
		int iHeader = H264_is_start_code( pBuffer );

		oVideoExtra.write( 16, iNAL - iHeader );
		oVideoExtra.write_buffer( pBuffer + iHeader, iNAL - iHeader );

		pBuffer += iNAL;
		iSize -= iNAL;
		iPPS++;
	}
	/* Set some size values */
	int iTotal = oVideoExtra.size();
	oVideoExtra.seek( iSPSIdx, SEEK_SET );
	oVideoExtra.write( 8, 0xE0 | iSPS );
	oVideoExtra.seek( iPPSIdx, SEEK_SET );
	oVideoExtra.write( 8, 0x00 | iPPS );

	/* Copy desc and set data */
	pDesc->setExtraData( oVideoExtra.data(), iTotal );
	return 0;
}

int MP4MediaConverter::ADTS2ESDS( const uint8_t* pADTS, uint32_t iADTSSize, MediaDescriptor* pDesc )
{
	/* Copy over the ADTS header */
	memcpy( oAudioHeader.data(), pADTS, iADTSSize );
	oAudioHeader.clear();

	/* Parse and construct */
	ADTS2ESDS( pDesc );
	return 0;
}

int MP4MediaConverter::META2ESDS( MediaDescriptor* pDesc )
{
	/* Sanity */
	if( pDesc->profile < 1 )
		SirannonWarning( "Invalid MPEG-4 audio profile(%d), guessing(%d)", pDesc->profile, 2 );
	pDesc->profile = 2;

	/* Create the audio extra data */
	oAudioExtra.clear();
	oAudioExtra.write( 5, pDesc->profile );
	oAudioExtra.write( 4, mp4aSampleRateIdx( pDesc->samplerate ) );
	oAudioExtra.write( 4, mp4aChannelsIdx( pDesc->channels ) );
	oAudioExtra.write( 3, 0 );

	/* Copy desc and set data */
	pDesc->setExtraData( oAudioExtra.data(), oAudioExtra.size() );
	return 0;
}

int MP4MediaConverter::ANNEXB2AVCC( MediaDescriptor* pDesc )
{
	/* Check */
	if( not vSPS.size() or not vPPS.size() )
		return -1;

	/* Profile information in first SPS packet */
	const uint8_t* pProfile = vSPS.front()->data();
	pProfile += H264_header_size( pProfile );

	/* Start header */
	oVideoExtra.clear();
	oVideoExtra.write( 8, 0x01 ); /* version */
	oVideoExtra.write( 8, pProfile[0] ); /* profile */
	oVideoExtra.write( 8, pProfile[1] ); /* profile compat */
	oVideoExtra.write( 8, pProfile[2] ); /* level */
	oVideoExtra.write( 8, 0xFF ); /* 6 bits reserved (111111) + 2 bits nal size length - 1 (11) */

	/* Write SPSs */
	oVideoExtra.write( 8, 0xE0 | vSPS.size() );
	for( int i = 0; i < vSPS.size(); ++i )
	{
		MediaPacket* pSPS = vSPS[i];
		pSPS->pop_front( H264_is_start_code(pSPS->data()) );
		oVideoExtra.write( 16, pSPS->size() );
		oVideoExtra.write_buffer( pSPS->data(), pSPS->size() );
	}
	/* Write PPSs */
	oVideoExtra.write( 8, 0x00 | vPPS.size() );
	for( int i = 0; i < vPPS.size(); ++i )
	{
		MediaPacket* pPPS = vPPS[i];
		pPPS->pop_front( H264_is_start_code(pPPS->data()) );
		oVideoExtra.write( 16, pPPS->size() );
		oVideoExtra.write_buffer( pPPS->data(), pPPS->size() );
	}
	/* Copy desc and set data */
	pDesc->setExtraData( oVideoExtra.data(), oVideoExtra.size() );
	return 0;
}

int MP4MediaConverter::ADTS2ESDS( MediaDescriptor* pDesc )
{
	/* Construct the ESDS header based on the ADTS audio header */
	oAudioHeader.clear();
	if( oAudioHeader.read( 12 ) != 0xFFF )
		RuntimeError( "MP4Converteror: no AAC sync in audio header: %s", strArray(oAudioHeader.data(), 7).c_str() );
	oAudioHeader.read( 4 );
	int iProfileIdx = oAudioHeader.read( 2 );
	int iSampleIdx = oAudioHeader.read( 4 );
	oAudioHeader.read( 1 );
	int iChannelIdx = oAudioHeader.read( 3 );
	oAudioHeader.read( 4 );
	int iSize = oAudioHeader.read( 13 );
	oAudioHeader.read( 11 );
	int iSamples = oAudioHeader.read( 2 );

	/* Deduce parameters */
	pDesc->profile = mp4aProfile( iProfileIdx );
	pDesc->channels = mp4aChannels( iChannelIdx );
	pDesc->samplerate = mp4aSampleRate( iSampleIdx );
	pDesc->framesize = ( iSamples + 1 ) * 1024;
	pDesc->bitrate = iSize * 8 * pDesc->samplerate / pDesc->framesize;

	/* Create the audio extra data */
	oAudioExtra.clear();
	oAudioExtra.write( 5, pDesc->profile );
	oAudioExtra.write( 4, iSampleIdx );
	oAudioExtra.write( 4, iChannelIdx );
	oAudioExtra.write( 3, 0 );

	/* Copy desc and set data */
	pDesc->setExtraData( oAudioExtra.data(), oAudioExtra.size() );
	return 0;
}

int MP4MediaConverter::ESDS2META( MediaDescriptor* pAudioDesc )
{
	/* Construct ESDS from meta */
	if( pAudioDesc->getExtraSize() < 2 )
		RuntimeError( "MP4Converter: AAC extra data too short: (%d) < 2", pAudioDesc->getExtraSize() );

	/* Decoding based on the encoding from an above method */
	IBits oExtra( pAudioDesc->getExtraData(), pAudioDesc->getExtraSize() );
	pAudioDesc->profile = oExtra.read( 5 );
	pAudioDesc->samplerate = mp4aSampleRate( oExtra.read( 4 ) );
	pAudioDesc->channels = mp4aChannels( oExtra.read( 4 ) );
	return 0;
}

int MP4MediaConverter::AVCC2META( MediaDescriptor* pDesc )
{
	/* Just need to know the length size */
	IBits oHeader( pDesc->getExtraData(), pDesc->getExtraSize() );
	oHeader.seek( 4 ); // 8 + 24
	pDesc->lengthSize = ( oHeader.read( 8 ) & 0x03 ) + 1;

	/* Parse the SPS to extract the width and height */
	int iSPS = oHeader.read( 8 ) & 0x1F;
	if( iSPS <= 0 )
		RuntimeError( "MP4Converter: No SPS in exta data: (%d: %s)", pDesc->getExtraSize(), strArray( pDesc->getExtraData(), pDesc->getExtraSize() ).c_str() );
	iSPS = oHeader.read( 16 );
	if( iSPS <= 0 )
		RuntimeError( "MP4Converter: No SPS in exta data: (%d: %s)", pDesc->getExtraSize(), strArray( pDesc->getExtraData(), pDesc->getExtraSize() ).c_str() );
	NAL_t oH264Decode;
	memset( &oH264Decode, 0, sizeof(oH264Decode) );
	H264_parse_SPS( oHeader.cur(), iSPS, &oH264Decode );
	if( pDesc->height == 0 and pDesc->width == 0 )
	{
		pDesc->height = (( oH264Decode.pic_height_in_map_units_minus1 + 1 ) << 4);
		pDesc->width = ( oH264Decode.pic_width_in_mbs_minus1 + 1 ) << 4;
	}
	return 0;
}

void MP4MediaConverter::convertESAudio( const uint8_t* pSource, int iSize, const MediaDescriptor* pDesc, queue_t& oBuffer )
{
	/* Make a new packet */
	MediaPacketPtr pPckt ( new MediaPacket( packet_t::media, content_t::audio, iSize+7 ) );
	pPckt->mux = mux_t::ES;

	/* Add data */
	pPckt->push_back( pSource, iSize );

	/* Write directly into the packet */
	pPckt->push_front( 7 );
	OBits oHeader( pPckt->data(), 7 );
	oHeader.write( 12, 0xFFF );  // syncword
	oHeader.write( 1, 0 ); 		 // id
	oHeader.write( 2, 0 );		 // layer
	oHeader.write( 1, 1 );		 // protection_absent
	oHeader.write( 2, mp4aProfileIdx( pDesc->profile ) );
	oHeader.write( 4, mp4aSampleRateIdx( pDesc->samplerate ) );
	oHeader.write( 1, 0 );				// private
	oHeader.write( 3, mp4aChannelsIdx( pDesc->channels) );		// channel_configuration
	oHeader.write( 1, 0 );				// home
	oHeader.write( 1, 0 );				// original
	oHeader.write( 1, 0 );				// copyright_id
	oHeader.write( 1, 0 );				// copyright_id_start
	oHeader.write( 13, iSize + 7 );
	oHeader.write( 11, 0x7FF );
	oHeader.write( 2, 0 );

	/* Add to list */
	oBuffer.push( pPckt.release() );
}

void MP4MediaConverter::convertESVideo( const uint8_t* pData, int iSize, MediaDescriptor* pDesc, queue_t& oBuffer, bool bAddParameterSets )
{
	iSub = 0;
	bool bExtract = false;
	int iSlice = -1;
	while( iSize > 0 )
	{
		/* Handle both annex B and MP4 as input */
		int iFrameSize = 0, iHeader = 4;
		if( pDesc->lengthSize <= 0 )
		{
			/* We MUST construct the extra data */
			iHeader = H264_is_start_code( pData );
			iFrameSize = H264_find_NAL( pData, iSize ) - iHeader;
			pData += iHeader;
			iSize -= iHeader;
		}
		else
		{
			IBits oHeader( pData, pDesc->lengthSize );
			iFrameSize = oHeader.read( pDesc->lengthSize * 8 );
			pData += pDesc->lengthSize;
			iSize -= pDesc->lengthSize;
		}
		/* New packet */
		MediaPacketPtr pPckt ( new MediaPacket( packet_t::media, content_t::video, iFrameSize+4 ) );
		pPckt->mux = mux_t::ES;

		/* Fix the kludge done in .mov files to the nal header */
		if( iHeader == 3 )
			pPckt->push_back( H264_START_CODE_SHORT, 3 );
		else
			pPckt->push_back( H264_START_CODE, 4 );

		/* Add actual nal */
		pPckt->push_back( pData, iFrameSize );

		/* Delta */
		pData += iFrameSize;
		iSize -= iFrameSize;

		/* Which NAL type? */
		int iNAL = H264_NAL_type( pData - iFrameSize );
		pPckt->frame = H264FrameToXFrame( iNAL, iSlice );

		/* Our own parse */
		switch( iNAL )
		{
		case NAL_UNIT_TYPE_SPS:
		case NAL_UNIT_TYPE_SPS_EXT:
		case NAL_UNIT_TYPE_SPS_SVC:
		case NAL_UNIT_TYPE_PPS:
			if( not bExtract )
				clearParameterSets();
			bExtract = true;

			if( iNAL == NAL_UNIT_TYPE_PPS )
			{
				vPPS.push_back( pPckt.release() );
			}
			else
			{
				vSPS.push_back( pPckt.release() );
			}
			continue;

		case NAL_UNIT_TYPE_ACCESS_UNIT_DELIM:
			if( bSkipAUD )
				continue;
			break;

		case NAL_UNIT_TYPE_SEI:
			if( bSkipSEI )
				continue;
			break;

		case NAL_UNIT_TYPE_SLICE_DP_A:
		case NAL_UNIT_TYPE_SLICE_NON_IDR:
		case NAL_UNIT_TYPE_SLICE_SVC:
		case NAL_UNIT_TYPE_SLICE_IDR:
			/* Skip the header */
			IBits oFrame( pData - iFrameSize, iFrameSize );
			oFrame.seek( 1 );

			/* Read the slice type */
			oFrame.read_uev();
			iSlice = oFrame.read_uev();
			break;
		}
		pPckt->frame = H264FrameToXFrame( iNAL, iSlice );

		/* Insert parameter sets if needed */
		if( pPckt->frame == frame_t::IDR and bAddParameterSets )
		{
			if( pDesc->getExtraSize() )
				extractParameterSets( pDesc );
			insertParameterSets( pPckt, oBuffer );
			bAddParameterSets = false;
		}
		/* Parse for PTS */
		if( bFixPTS )
			H264_parse_NAL( pPckt->data(), pPckt->size(), &oNAL );

		/* Store */
		oBuffer.push( pPckt.release() );
	}
}

void MP4MediaConverter::convertES( MediaPacketPtr& pPckt, queue_t& oBuffer, bool bAddParameterSets )
{
	/* Sanity */
	if( pPckt->mux != mux_t::MOV )
		TypeError( "MP4Converter: Packet is not in MP4 format: %s", pPckt->c_str_long() );

	if( pPckt->codec == avc or pPckt->codec == svc )
	{
		/* Insert parameter sets */
		iSub = 0;
		if( bAddParameterSets or bFirstVideo )
		{
			extractParameterSets( pPckt->desc );
			insertParameterSets( pPckt, oBuffer );
			bFirstVideo = false;
		}
		while( pPckt->size() )
		{
			/* Change header to startcode */
			int iSize = convert_header_ES( pPckt.get() );

			/* Create copies if split into multiple parts */
			if( iSize < pPckt->size() )
			{
				/* Create a new packet */
				MediaPacketPtr pNewPacket( new MediaPacket( pPckt->type, pPckt->content, iSize ) );
				pNewPacket->push_back( pPckt->data(), iSize );
				pPckt->pop_front( iSize );

				/* Metadata */
				pNewPacket->set_metadata( pPckt.get() );
				pNewPacket->framestart = ( iSub == 0 );
				pNewPacket->frameend = false;
				pNewPacket->unitnumber = iUnit++;
				pNewPacket->subframenumber = iSub++;
				pNewPacket->mux = mux_t::ES;
				oBuffer.push( pNewPacket.release() );
			}
			else
			{
				pPckt->framestart = ( iSub == 0 );
				pPckt->frameend = true;
				pPckt->unitnumber = iUnit++;
				pPckt->subframenumber = iSub++;
				pPckt->mux = mux_t::ES;
				oBuffer.push( pPckt.release() );
				break;
			}
		}
	}
	else if( pPckt->codec == mp4a )
	{
		/* Add 7 bytes to the start of the packet */
		pPckt->push_front( 7 );
		OBits oHeader( pPckt->data(), 7 );
		pPckt->mux = mux_t::ES;
		pPckt->unitnumber = iUnit++;

		/* Recreate the header each time */
		oHeader.write( 12, 0xFFF );
		oHeader.write( 1, 0 );
		oHeader.write( 2, 0 );
		oHeader.write( 1, 1 );	// 2 byte
		oHeader.write( 2, mp4aProfileIdx( pPckt->desc->profile ) );
		oHeader.write( 4, mp4aSampleRateIdx( pPckt->desc->samplerate ) );
		oHeader.write( 1, 0 );
		oHeader.write( 3, mp4aChannelsIdx( pPckt->desc->channels ) );
		oHeader.write( 1, 0 );
		oHeader.write( 1, 0 );
		oHeader.write( 1, 0 );
		oHeader.write( 1, 0 );
		oHeader.write( 13, pPckt->size() );
		oHeader.write( 11, 0x7FF );
		oHeader.write( 2, 0 );
		oBuffer.push( pPckt.release() );
	}
	else
		TypeError( "MP4Converter: Invalid codec(%s) for convertES", CodecToString(pPckt->codec) );
}

void MP4MediaConverter::extractParameterSets( const MediaDescriptor* pDesc )
{
	/* Parse header */
	clearParameterSets();
	IBits oHeader( pDesc->getExtraData(), pDesc->getExtraSize() );
	oHeader.read( 32 );

	/* What are the sizes of length fields? */
	uint32_t iLengthSize = ( oHeader.read( 8 ) & 0x03 ) + 1;
	//fprintf( stderr, "length-field (%d)\n", iLengthSize );

	/* Construct SPS */
	uint32_t iSPS = oHeader.read( 8 ) & 0x1F;
	//fprintf( stderr, "detected (%d) SPS\n", iSPS );
	for( uint32_t i = 0; i < iSPS; i++ )
	{
		/* Size of this SPS */
		uint32_t iSize = oHeader.read( 16 );
		MediaPacket* pSPS = new MediaPacket( packet_t::media, content_t::video, iSize + 4 ) ;
		vSPS.push_back( pSPS );

		/* Packet data */
		pSPS->push_back( H264_START_CODE, 4 );
		pSPS->push_back( oHeader.cur(), iSize );
		oHeader.seek( iSize );
	}
	/* Construct SPS */
	uint32_t iPPS = oHeader.read( 8 ) & 0x1F;
	//fprintf( stderr, "detected (%d) PPS\n", iPPS );
	for( uint32_t i = 0; i < iPPS; i++ )
	{
		/* Size of this PPS */
		uint32_t iSize = oHeader.read( 16 );
		MediaPacket* pPPS = new MediaPacket( packet_t::media, content_t::video, iSize + 4 );
		vPPS.push_back( pPPS );

		/* Packet data */
		pPPS->push_back( H264_START_CODE, 4 );
		pPPS->push_back( oHeader.cur(), iSize );
		oHeader.seek( iSize );
	}
}

void MP4MediaConverter::clearParameterSets( void )
{
	for( int i = 0; i < vSPS.size(); ++i )
	{
		delete vSPS[i];
		vSPS[i] = NULL;
	}
	vSPS.clear();

	for( int i = 0; i < vPPS.size(); ++i )
	{
		delete vPPS[i];
		vPPS[i] = NULL;
	}
	vPPS.clear();
}

void MP4MediaConverter::insertParameterSets( MediaPacketPtr& pPckt, queue_t& oBuffer )
{
	/* Parse a SPS */
	if( bFixPTS and vSPS.size() > 0 )
		H264_parse_NAL( vSPS[0]->data(), vSPS[0]->size(), &oNAL );

	/* Construct SPS */
	for( uint32_t i = 0; i < vSPS.size(); ++i )
	{
		/* Size of this SPS */
		MediaPacketPtr pSPS( new MediaPacket( *vSPS[i] ) );

		/* Meta data */
		pSPS->set_metadata( pPckt.get() );
		pSPS->framestart = ( iSub == 0 );
		pSPS->frameend = false;
		pSPS->frame = SPS;
		pSPS->unitnumber = iUnit++;
		pSPS->subframenumber = iSub++;
		pSPS->mux = mux_t::ES;

		oBuffer.push( pSPS.release() );
	}
	/* Construct PPS */
	for( uint32_t i = 0; i < vPPS.size(); ++i )
	{
		/* Size of this PPS */
		MediaPacketPtr pPPS( new MediaPacket( *vPPS[i] ) );

		/* Meta data */
		pPPS->set_metadata( pPckt.get() );
		pPPS->framestart = ( iSub == 0 );
		pPPS->frameend = false;
		pPPS->frame = PPS;
		pPPS->unitnumber = iUnit++;
		pPPS->subframenumber = iSub++;
		pPPS->mux = mux_t::ES;

		oBuffer.push( pPPS.release() );
	}
}

int mp4aSampleRate( int iIdx )
{
	/* http://wiki.multimedia.cx/index.php?title=MPEG-4_Audio */
	static const int dTrans [16] = { 96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350 };
	return dTrans[iIdx];
}

int mp4aSampleRateIdx( int iSampleRate )
{
	if( 92017 <= iSampleRate )
		return 0;
	if( 75132 <= iSampleRate )
		return 1;
	if( 55426 <= iSampleRate )
		return 2;
	if( 46009 <= iSampleRate )
		return 3;
	if( 37566 <= iSampleRate )
		return 4;
	if( 27713 <= iSampleRate )
		return 5;
	if( 23004 <= iSampleRate )
		return 6;
	if( 18783 <= iSampleRate )
		return 7;
	if( 13856 <= iSampleRate )
		return 8;
	if( 11502 <= iSampleRate )
		return 9;
	if( 9391 <= iSampleRate )
		return 10;
	return 11;
}

int mp4aProfile( int iIdx )
{
	return iIdx + 1;
}

int mp4aProfileIdx( int iProfile )
{
	return iProfile - 1;
}

int mp4aChannels( int iIdx )
{
	static const int mp4aChannel[8] = { 0, 1, 2, 3, 4, 5, 6, 8 };
	return mp4aChannel[iIdx];
}

int mp4aChannelsIdx( int iChannels )
{
	static const int mp4aChannel[9] = { 0, 1, 2, 3, 4, 5, 6, 7, 7 };
	return mp4aChannel[iChannels];
}

void GetMetadata( const char* sMedia, const container_t* pContainer, MediaDescriptor* pDesc )
{
	AVFormatContext* pFormatCtx;
	if( 	av_open_input_file( &pFormatCtx, sMedia, NULL, 0, NULL ) != 0 or
			av_find_stream_info( pFormatCtx ) < 0 )
	{
		av_close_input_file( pFormatCtx );
		return;
	}
	pDesc->duration = pFormatCtx->duration / 1000;
	pDesc->bitrate = pFormatCtx->bit_rate;
}
