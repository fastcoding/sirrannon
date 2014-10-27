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
#ifndef Sirannon_FFMPEG_H_
#define Sirannon_FFMPEG_H_
#include "sirannon.h"
extern "C"
{
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/pixdesc.h"
}
#include "sirannon.h"

class ffmpeg_loader
{
public:
	ffmpeg_loader();
};

/* Container translation */
container_t FFFormatToSirannonFormat( const AVInputFormat* pFormat );
AVInputFormat* SirannonFormatToFFFormat( container_t oFormat );

/* Codec translation */
enum CodecID SirannonCodecToFFCodec( codec_t::type iCodec );
codec_t::type FFCodecToSirannonCodec( enum CodecID x );

/* Thread safe wrappers */
extern mutex oFFMpegMutex;

inline int ffmpeg_avcodec_open( AVCodecContext *avctx, AVCodec *codec )
{
	int iRet;
	Lock_t oLock( oFFMpegMutex );
	iRet = avcodec_open( avctx, codec );
	return iRet;
}
#define avcodec_open ffmpeg_avcodec_open

inline int ffmpeg_avcodec_close( AVCodecContext *avctx )
{
	int iRet;
	Lock_t oLock( oFFMpegMutex );
	iRet = avcodec_close( avctx );
	return iRet;
}
#define avcodec_close ffmpeg_avcodec_close

inline AVCodec* ffmpeg_avcodec_find_decoder( enum CodecID id )
{
	AVCodec* ret;
	Lock_t oLock( oFFMpegMutex );
	ret = avcodec_find_decoder( id );
	return ret;
}
#define avcodec_find_decoder ffmpeg_avcodec_find_decoder

inline AVCodec* ffmpeg_avcodec_find_encoder( enum CodecID id )
{
	AVCodec* ret;
	Lock_t oLock( oFFMpegMutex );
	ret = avcodec_find_encoder( id );
	return ret;
}
#define avcodec_find_encoder ffmpeg_avcodec_find_encoder

//inline int ffmpeg_av_find_stream_info( AVFormatContext *ic )
//{
//	int iRet;
//	Lock_t oLock( oFFMpegMutex );
//	iRet = av_find_stream_info( ic );
//	return iRet;
//}
//#define av_find_stream_info ffmpeg_av_find_stream_info

inline AVRational toAVRational( const rational_t& oRat )
{
	AVRational oRet;
	oRet.num = oRat.num;
	oRet.den = oRat.den;
	return oRet;
}

inline AVRational toAVRational( int32_t num, int32_t den )
{
	AVRational oRet;
	oRet.num = num;
	oRet.den = den;
	return oRet;
}

#endif /*FFMPEG_H_*/
