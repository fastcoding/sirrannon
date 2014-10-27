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
#ifndef FFMPEG_READER_H_
#define FFMPEG_READER_H_
#include "Reader.h"
#include "ffmpeg.h"
#include "h264_avc.h"
#include "Bits.h"
#include "Frame.h"

struct stream_t
{
	timestamp_t dts;
	timestamp_t pts;
	timestamp_t inc;
};


class FFmpegReader : public Reader
{
public:
	FFmpegReader( const string& sName, ProcessorManager* pScope );
	~FFmpegReader();
	virtual void init( void );
	virtual int seek( uint32_t iTimeIndex );
	int getBitrate( content_t::type iType ) const;

protected:
	virtual void openStream( void );
	virtual void openBuffer( void );
	virtual void closeBuffer( void );
	void parseEnd( void );
	bool doStep( void );
	void purgePackets( void );
	bool parse( AVPacket* pckt );
	void parse_ctrl( packet_t::type type );
	void parseAVCFrame( AVPacket*, AVCodecContext* pCodec );
	void parseAVCMovFrame( AVPacket*, AVCodecContext* pCodec );
	void parseAACFrame( AVPacket*, AVCodecContext* pCodec );
	void parseAACMovFrame( AVPacket*, AVCodecContext* pCodec );
	void parseFrame( AVPacket*, AVCodecContext* pCodec );

	typedef FFmpegReader self;
	typedef void (FFmpegReader::*parser_t)( AVPacket*, AVCodecContext* pCodec );

	AVFormatContext* pFormatCtx;
	int  videoStream, audioStream, mainStream, videoMux, audioMux, iAudioRoute, iTarget,
	iVideoRoute;
	AVRational videoScale, audioScale, oMillisecond, oTimeBase;
	codec_t::type videoCodec, audioCodec;
	mux_t::type mediaMux;
	parser_t videoFrameParser, audioFrameParser;
	bool bAddParameterSets, bRepeatParameterSets, bMovFrameStructure, bVideoSeek, bAudioSeek, bFixPTS, bMaxDurationReached;
	uint32_t iUnit;
	timestamp_t iOldVideoDts, iOldVideoPts, iOldAudioDts, iOldAudioPts, iBaseDts, iStartDts, iRefDts, iLastDts, iOldDts, iMaxDts, iMinDts;
	int iPOC, iAudioFrame, iVideoFrame, iErrors;
	NAL_t oH264Decode;
	OBits oHeader;
	queue_t vPackets;
	vector_t vParameterSets, vSEI;
	MediaDescriptor *pAudioDesc, *pVideoDesc;
	AVPacket oAVPckt;
	MP4MediaConverter oConvertor;
	map<int,stream_t> oStream;
};

#endif /*FFMPEG_READER_H_*/
