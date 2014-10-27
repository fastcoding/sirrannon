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
#include "FFmpegReader.h"
#include "sdp.h"
#include "Frame.h"

/**
 * FFMPEG READER
 * @component ffmpeg-reader
 * @type reader
 * @param video-mode, bool, true, if true, video will be read from the container, if false video will be ignored
 * @param audio-mode, bool, true, if true, audio will be read from the container, if false audio will be ignored
 * @param min-ts, int, -1, in ms, the timestamp to jump to into the stream, -1 implying no seek
 * @param max-ts, int, -1, in ms, the maximum timestamp to read from the file, -1 implying no limit
 * @param dts-start, int, 1000, in ms, specifies the value of the timestamp of the first frame
 * @param add-parameter-sets, bool, true, H.264/AVC only, if true, extract the parameter sets from the container and insert them into the stream
 * @param repeat-parameter-sets, bool, false, H.264/AVC only, if true, repeat the parameter sets before each IDR frame
 * @param mov-frame, bool, false, MOV/MP4/F4V container only, if true, keep frames in the format of the container, as opposed to annex-B H.264/AVC streams with start codes before each NAL unit
 * @param skip-SEI, bool, false, if true, remove SEI NALUs from the stream
 * @param skip-AUD, bool, false, if true, remove AUD NALUs from the stream
 * @param fix-PTS, bool, false, if true, parse H.264 frames to extract the POC from which to calculate the correct PTS
 * @info Reads in a wide variety of containers supported by ffmpeg. Audio and video are
 * put into different MediaPackets. Per cycle, the reader processes one video
 * frame (if present) and associated audio, possibly generating multiple packets.
 * A separate end- or reset-packet is generated for audio & video. Note, ffmpeg-reader
 * can also process audio only files.
 **/
REGISTER_CLASS( FFmpegReader, "ffmpeg-reader" );

// CAVEATS: TSs must start near 0

FFmpegReader::FFmpegReader( const string& sName, ProcessorManager* pScope )
	: Reader(sName, pScope), pFormatCtx(NULL), videoStream(-1), audioStream(-1), mainStream(-1),
	  iUnit(0), iStartDts(1000), iBaseDts(-1), iOldVideoDts(-1), iOldVideoPts(-1),
	  iOldAudioDts(-1), iOldAudioPts(-1), iOldDts(-1), iAudioFrame(-1), iVideoFrame(-1),
	  mediaMux(mux_t::ES), bAddParameterSets (true), bRepeatParameterSets(false),
	  videoFrameParser(NULL),  audioFrameParser(NULL),
	  videoMux(-1), audioMux(-1), iErrors(0),
	  iRefDts(0), iLastDts(-1), iTarget(1000000),
	  oHeader(128), iAudioRoute(200), iVideoRoute(100),
	  videoCodec(codec_t::NO), audioCodec(codec_t::NO),
	  bMovFrameStructure(false), bVideoSeek(false), bAudioSeek(false),
	  pAudioDesc(NULL), pVideoDesc(NULL), oConvertor(true), bFixPTS(false), iMaxDts(-1),
	  bMaxDurationReached(false), iMinDts(-1)
{
	/* Default */
	oTimeBase.num = 1;
	oTimeBase.den = 90000;
	audioScale.num = 1;
	audioScale.den = 90000;
	videoScale.num = 1;
	videoScale.den = 90000;
	oMillisecond.num = 1;
	oMillisecond.den = 1000;
	mInt["dts-start"] = iStartDts;
	mBool["add-parameter-sets"] = true;
	mBool["repeat-parameter-sets"] = false;
	mBool["sdp"] = false;
	mInt["min-ts"] = -1;
	mInt["max-ts"] = -1;
	mBool["mov-frame"] = false;
	mBool["video-mode"] = true;
	mBool["audio-mode"] = true;
	mBool["skip-SEI"] = false;
	mBool["skip-AUD"] = false;
	mBool["fix-PTS"] = false;

	oAVPckt.data = NULL;
}

FFmpegReader::~FFmpegReader()
{
	if( oAVPckt.data != NULL )
		av_free_packet( &oAVPckt );
	if( pFormatCtx )
		av_close_input_file( pFormatCtx );
	for( uint32_t i = 0; i < vParameterSets.size(); i++ )
		delete vParameterSets[i];
	for( uint32_t i = 0; i < vSEI.size(); i++ )
		delete vSEI[i];
	while( not vPackets.empty() )
	{
		delete vPackets.front();
		vPackets.pop();
	}
}

void FFmpegReader::init( void )
{
	/* iStartDts */
	iStartDts = mInt["dts-start"] * 90;
	bAddParameterSets = mBool["add-parameter-sets"];
	bRepeatParameterSets = mBool["repeat-parameter-sets"];
	iVideoRoute = mInt["video-route"];
	iAudioRoute = mInt["audio-route"];
	bMovFrameStructure = mBool["mov-frame"];
	oConvertor.skipNAL( mBool["skip-SEI"], mBool["skip-AUD"] );
	bFixPTS = mBool["fix-PTS"];
	iMaxDts = mInt["max-ts"];
	if( iMaxDts > 0 )
		iMaxDts = iMaxDts * 90 + TIMESTAMP_MIN;
	iMinDts = mInt["min-ts"];

	/* Base class */
	Reader::init();
	debug( 1, "min(%d) max(%d)", iMinDts * 90 + TIMESTAMP_MIN, iMaxDts );
	openStream();
}

void FFmpegReader::openBuffer( void )
{
	/* Open video file */
	if( av_open_input_file( &pFormatCtx, mString["filename"].c_str(), NULL, 0, NULL )!= 0 )
		RuntimeError( this, "Could not open file(%s)", mString["filename"].c_str() );
}

void FFmpegReader::closeBuffer( void )
{
	/* Close the video file */
	av_close_input_file( pFormatCtx );
	pFormatCtx = NULL;
}

void FFmpegReader::openStream( void )
{
	/* Reset some fields */
	iPOC = -1;
	memset( &oH264Decode, 0, sizeof(NAL_t) );

	/* Anything? */
	if( not ( mBool["video-mode"] or mBool["audio-mode"] ) )
		RuntimeError( this, "At least one media-mode should be selected" );

	/* Open the input stream */
	//pFormatCtx->max_analyze_duration = 100000;
	if( av_find_stream_info( pFormatCtx ) < 0 )
    	RuntimeError( this, "Could not find stream information" );

	/* Find the first video stream */
	videoStream = -1;
	for( uint32_t i = 0; i < pFormatCtx->nb_streams; i++ )
	{
	    if( pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO )
	    {
		        videoStream = i;
		        mainStream = i;
		}
	}
	if( not mBool["video-mode"] and videoStream >= 0 )
	{
		videoStream = -1;
		mainStream = -1;
		debug( 1, "ignored video stream" );
	}
	else if( mBool["video-mode"] and videoStream == -1 )
	{
		debug( 1, "no video stream found" );
	}
	else
	{
		debug( 1, "found video stream" );
	}

	/* Find the first audio stream */
	audioStream = -1;
	for( uint32_t i = 0; i < pFormatCtx->nb_streams; i++ )
	{
	    if( pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO )
	    {
	        audioStream = i;
	        if( mainStream < 0 )
	        	mainStream = i;
	    }
	}
	if( not mBool[ "audio-mode" ] and audioStream >= 0 )
	{
		audioStream = -1;
		debug( 1, "ignored audio stream" );
	}
	else if( mBool[ "audio-mode" ] and audioStream == -1 )
	{
		debug( 1, "no audio stream found" );
	}
	else
	{
		debug( 1, "found audio stream" );
	}

	/* Determine mux type */
	mediaMux = FFFormatToSirannonFormat( pFormatCtx->iformat ).first;
	debug( 1, "container type(%s)", MuxToString(mediaMux) );

	/* Fix a bug in ffmpeg for PTS */
	if( mediaMux == mux_t::ES )
		bFixPTS = true;

	/* Anything? */
	if(  videoStream == -1 and  audioStream == -1 )
		RuntimeError( this, "No streams found" );
	else
		debug( 1, "succesfully opened stream(%s)", pFormatCtx->iformat->name );

	/* Global info */
	oContainer.bytesize = pFormatCtx->file_size;
	oContainer.bitrate = pFormatCtx->bit_rate;
	oContainer.duration = pFormatCtx->duration;
	oContainer.mux = mediaMux;

	/* Found out some information about frequecny */
	if( videoStream >= 0 )
	{
		AVStream* pStream = pFormatCtx->streams[videoStream];
		AVCodecContext* pCodec = pStream->codec;
		videoCodec = FFCodecToSirannonCodec( pCodec->codec_id );
		if( not pVideoDesc )
			pVideoDesc = addMedia();
		if( videoMux < 0 )
			videoMux = nextStreamID();

		/* Timing */
		AVRational x = pFormatCtx->streams[videoStream]->time_base;
		videoScale.num = x.num;
		videoScale.den = x.den;
		AVRational f = pFormatCtx->streams[videoStream]->r_frame_rate;
		pVideoDesc->inc = (int64_t) 90000 * (f.den) / f.num;
		if( mediaMux == mux_t::WEBM )  //HACK for Webm
			pVideoDesc->inc *= 2;
		if( pVideoDesc->inc < 300 ) // HACK for Apple Trailers
			pVideoDesc->inc *= 100;

		/* Descriptor */
		pVideoDesc->codec  = videoCodec;
		pVideoDesc->content = content_t::video;
		pVideoDesc->width = pCodec->width;
		pVideoDesc->height = pCodec->height;
		pVideoDesc->bitrate = (pCodec->bit_rate > 0) ? pCodec->bit_rate : oContainer.bitrate;
		pVideoDesc->profile = pCodec->profile;
		pVideoDesc->level = pCodec->level;
		pVideoDesc->framerate = 90000.  / pVideoDesc->inc;
		pVideoDesc->route = iVideoRoute ? iVideoRoute : oContainer.size() * 100;
		pVideoDesc->setExtraData( pCodec->extradata, pCodec->extradata_size );

		debug( 1, "video codec(%s/%d) timebase (%d/%d) fps(%d/%d) inc(%d) dim(%dx%d) rate(%d) extra(%d: %s)",
				CodecToString(pVideoDesc->codec), (int)pCodec->codec_id, x.num, x.den, f.num,
				f.den, pVideoDesc->inc, pCodec->width, pCodec->height, pVideoDesc->bitrate,
				pCodec->extradata_size, strArray(pCodec->extradata,pCodec->extradata_size).c_str() );
	}
	if( audioStream >= 0 )
	{
		AVStream* pStream = pFormatCtx->streams[audioStream];
		AVCodecContext* pCodec = pStream->codec;
		audioCodec = FFCodecToSirannonCodec( pCodec->codec_id );
		if( not pAudioDesc )
			pAudioDesc = addMedia();
		if( audioMux < 0 )
			audioMux = nextStreamID();

		/* Timing */
		AVRational x = pFormatCtx->streams[audioStream]->time_base;
		audioScale.num = x.num;
		audioScale.den = x.den;

		/* Descriptor */
		pAudioDesc->codec = audioCodec;
		pAudioDesc->content = content_t::audio;
		pAudioDesc->bitrate = (pCodec->bit_rate > 0) ? pCodec->bit_rate : oContainer.bitrate;
		pAudioDesc->channels = pCodec->channels;
		pAudioDesc->samplerate = pCodec->sample_rate;
		pAudioDesc->framesize = pCodec->frame_size;
		pAudioDesc->profile = pCodec->profile;
		pAudioDesc->framerate = x.num * 1. / x.den;
		pAudioDesc->route = iAudioRoute ? iAudioRoute : oContainer.size() * 100;

		/* Fixes */
		if( audioCodec == codec_t::mp4a )
			pAudioDesc->profile = mp4aProfile( pCodec->profile );
		if( mediaMux == mux_t::WEBM )
			pAudioDesc->framesize *= 2;
		if( pAudioDesc->samplerate == 0 )
		{
			SirannonWarning( this, "Sample rate undefined, assuming 44100 Hz" );
			pAudioDesc->samplerate = 44100;
		}
		if( pAudioDesc->channels == 0 )
		{
			SirannonWarning( this, "Channel count undefined, assuming 2" );
			pAudioDesc->channels = 2;
		}
		if( pAudioDesc->framesize == 0 )
		{
			if( pAudioDesc->codec == codec_t::anb or pAudioDesc->codec == codec_t::awb )
				pAudioDesc->framesize = 160;
			else
			{
				SirannonWarning( this, "Sample count undefined, assuming 1024" );
				pAudioDesc->framesize = 1024;
			}
		}
		pAudioDesc->inc = 90000 * pAudioDesc->framesize / pAudioDesc->samplerate;
		pAudioDesc->setExtraData( pCodec->extradata, pCodec->extradata_size );

		debug( 1, "audio: codec(%s/%d) timebase (%d/%d) samplerate(%d) samples(%d) inc(%d) profile(%d) channels(%d) extra(%d: %s)",
				CodecToString(pAudioDesc->codec), (int)pCodec->codec_id,
				x.num, x.den, pAudioDesc->samplerate, pAudioDesc->framesize, pAudioDesc->inc,
				pAudioDesc->profile, pAudioDesc->channels,
				pCodec->extradata_size, strArray(pCodec->extradata,pCodec->extradata_size).c_str() );
	}
	/* Reload */
	pAudioDesc = oContainer.getAudioDescriptor();
	pVideoDesc = oContainer.getVideoDescriptor();

	/* Set the video parse */
	if( ( mediaMux == mux_t::MOV or mediaMux == mux_t::FLV ) and videoCodec & codec_t::H264 )
		videoFrameParser = &self::parseAVCMovFrame;
	else if( videoCodec == codec_t::avc )
		videoFrameParser = &self::parseAVCFrame;
	else
		videoFrameParser = &self::parseFrame;

	/* Set audio parse */
	if( ( mediaMux == mux_t::MOV or mediaMux == mux_t::FLV ) and audioCodec == codec_t::mp4a )
		audioFrameParser = &self::parseAACMovFrame;
	else if( audioCodec == codec_t::mp4a )
		audioFrameParser = &self::parseAACFrame;
	else
		audioFrameParser = &self::parseFrame;

	/* Ensure correct extra data */
  	if(	videoCodec == codec_t::avc )
  	{
  		if( mediaMux == mux_t::MOV or mediaMux == mux_t::FLV )
  			oConvertor.AVCC2META( pVideoDesc );
  		else if( mediaMux == mux_t::ES or mediaMux == mux_t::TS )
  		{
  			oConvertor.buildVideoExtraData( pVideoDesc->getExtraData(), pVideoDesc->getExtraSize(), pVideoDesc );
  			debug( 1, "video (reformed): extra data(%d: %s)", pVideoDesc->getExtraSize(),
  					strArray(pVideoDesc->getExtraData(),pVideoDesc->getExtraSize()).c_str() );
  		}
  	}
  	if( audioCodec == codec_t::mp4a )
  	{
  		if( mediaMux == mux_t::MOV or mediaMux == mux_t::FLV )
  			oConvertor.ESDS2META( pAudioDesc );
  		else if( mediaMux == mux_t::ES or mediaMux == mux_t::TS )
  			oConvertor.META2ESDS( pAudioDesc );
  		debug( 1, "audio (reformed): profile(%d) channels(%d) samplerate(%d) extra(%d: %s)",
  		  	pAudioDesc->profile, pAudioDesc->channels, pAudioDesc->samplerate,
  		  	pAudioDesc->getExtraSize(), strArray(pAudioDesc->getExtraData(),pAudioDesc->getExtraSize()).c_str() );
  	}
  	/* Seek */
  	if( iMinDts > 0 )
  		seek( iMinDts );
}

int FFmpegReader::getBitrate( content_t::type iType ) const
{
	if( iType == content_t::video )
		return pVideoDesc->bitrate;
	else if( iType == content_t::audio )
		return pAudioDesc->bitrate;
	else
		return pVideoDesc->bitrate + pAudioDesc->bitrate;
}

bool FFmpegReader::doStep( void )
{
	/* Loop until we find a fitting packet */
	oAVPckt.data = NULL;
	bool bStep = true;

	/* Read new packet*/
	if( av_read_frame( pFormatCtx, &oAVPckt ) < 0 )
	{
		if( mInt["loop"] < 0 or iLoop++ < mInt["loop"] )
		{
			/* Send a reset packet */
			debug( 1, "reset" );
			parse_ctrl( packet_t::reset );

			/* Seek to start (brute force)
			 * Actually not so brute because seek(0) jumps to the first VIDEO frame and
			 * skips any audio frames before it */
			closeBuffer();
			openBuffer();
			openStream();
		}
		else
		{
			/* Send an end packet */
			parseEnd();
			bStep = false;
		}
	}
	/* Do we recognize the packet? */
	else if( oAVPckt.stream_index == videoStream or oAVPckt.stream_index == audioStream )
	{
		/* Parse */
		bStep = parse( &oAVPckt );
	}
	/* Free data */
	if( oAVPckt.data != NULL )
		av_free_packet( &oAVPckt );
	return bStep;
}

void FFmpegReader::parseEnd( void )
{
	/* Send an end packet */
	parse_ctrl( packet_t::end );

	/* Close */
	closeBuffer();

	/* Stop the infinite loop */
	bSchedule = false;
}

int FFmpegReader::seek( uint32_t iSeek ) synchronized
{
	/* Catch the situation where the file is closed */
	if( not pFormatCtx )
	{
		openBuffer();
		openStream();
		bSchedule = true;
	}
	/* Convert from milliseconds to stream time index */
	bVideoSeek = bAudioSeek = true;
	int iTmpSeek;
	iLastDts = -1;

	if( videoStream >= 0 )
	{
		iTmpSeek = av_rescale_q( iSeek + pVideoDesc->inc / 90, oMillisecond, videoScale );
		if( av_seek_frame( pFormatCtx, videoStream, iTmpSeek, AVSEEK_FLAG_BACKWARD ) < 0 )
				return SirannonWarning( this,  "seek(%d ms - %d) failed", iSeek, iTmpSeek );
		debug( 1, "seek(%d ms - %d)", iSeek, iTmpSeek );
	}
	else if( audioStream >= 0 )
	{
		iTmpSeek = av_rescale_q( iSeek, oMillisecond, audioScale );
		if( av_seek_frame( pFormatCtx, videoStream, iTmpSeek, AVSEEK_FLAG_BACKWARD ) < 0 )
				return SirannonWarning( this,  "seek(%d ms - %d) failed", iSeek, iTmpSeek );
		debug( 1, "seek(%d ms - %d)", iSeek, iTmpSeek );
	}
	return 0;
} end_synchronized

bool FFmpegReader::parse( AVPacket* av_pckt )
{
    /* Parse the ffmpeg packet */
	AVFrame* pFrame = (AVFrame*) av_pckt;
	AVRational oLocalTimeBase;
	timestamp_t* pOldDts;
	timestamp_t iInc;
	if( av_pckt->stream_index == videoStream )
	{
		(this->*videoFrameParser)( av_pckt, pFormatCtx->streams[videoStream]->codec );
		oLocalTimeBase = videoScale;
		iVideoFrame++;
		pOldDts = &iOldVideoDts;
		iInc = pVideoDesc->inc;
	}
	else if( av_pckt->stream_index == audioStream )
	{
		(this->*audioFrameParser)( av_pckt, pFormatCtx->streams[audioStream]->codec );
		oLocalTimeBase = audioScale;
		iAudioFrame++;
		pOldDts = &iOldAudioDts;
		iInc = pAudioDesc->inc;
	}
	/* Sanity */
	if( not vPackets.size() )
		RuntimeError( this, "No packets generated by parse function" );

    /* Timestamps */
	timestamp_t iFFmpegDts = av_rescale_q( av_pckt->dts, oLocalTimeBase, oTimeBase );
    timestamp_t iFFmpegPts = av_rescale_q( av_pckt->pts, oLocalTimeBase, oTimeBase );
    iFFmpegDts += TIMESTAMP_MIN;
    iFFmpegPts += TIMESTAMP_MIN;

	/* Special PTS fix for PTS */
	if( bFixPTS and videoCodec == codec_t::avc )
	{
		if( vPackets.back()->frame == frame_t::IDR )
			iRefDts = iFFmpegDts;
		if( oConvertor.getPOC() == 0 )
			iFFmpegPts = iFFmpegDts + 2 * pVideoDesc->inc;
		else
			iFFmpegPts = iRefDts + ( oConvertor.getNALParse()->num_ref_frames + oConvertor.getPOC() / 2 ) * pVideoDesc->inc;
	}

	/* Monotone checks */
	if( iFFmpegDts <= *pOldDts )
		iFFmpegDts = *pOldDts + iInc;
	*pOldDts = iFFmpegDts;

	/* PTS vs DTS checks */
	iFFmpegPts = MAX(iFFmpegDts, iFFmpegPts);

    /* Max duration? */
   if(	iMaxDts > 0 and
		iFFmpegDts >= iMaxDts )
   {
	   if( av_pckt->stream_index == videoStream )
	   {
		   debug( 1, "max duration reached: %d >= %d", iFFmpegDts, iMaxDts );
		   bMaxDurationReached = true;
	   }
	   else if( not bMaxDurationReached )
	   {
		   purgePackets();
		   return true;
	   }
   }
   if( bMaxDurationReached )
   {
	   if( audioStream < 0 )
	   {
		   purgePackets();
		   iOldDts = iMaxDts;
		   parseEnd();
		   return false;
	   }
	   else if( av_pckt->stream_index == audioStream and iFFmpegDts >= iMaxDts )
	   {
		   purgePackets();
		   iOldDts = iMaxDts;
		   parseEnd();
		   return false;
	   }
	   else if( av_pckt->stream_index == videoStream )
	   {
		   purgePackets();
		   return false;
	   }
   }
   iOldDts = iFFmpegDts;

    /* Iterate over the packets */
    int iPackets = 0;
    vPackets.front()->framestart = true;
    vPackets.back()->frameend = true;
    while( not vPackets.empty() )
    {
    	/* The packet */
    	MediaPacket* pPckt = vPackets.front();

    	/* Audio or video? */
	   if( av_pckt->stream_index == videoStream )
	   {
		   pPckt->content = content_t::video;
		   pPckt->xroute = pVideoDesc->route;
		   pPckt->codec = videoCodec;
		   pPckt->xstream = videoMux;
		   pPckt->desc = pVideoDesc;
		   pPckt->framenumber = iVideoFrame;
	   }
	   else if( av_pckt->stream_index == audioStream )
	   {
		   pPckt->content = content_t::audio;
		   pPckt->xroute = pAudioDesc->route;
		   pPckt->codec = audioCodec;
		   pPckt->xstream = audioMux;
		   pPckt->desc = pAudioDesc;
		   pPckt->framenumber = iAudioFrame;
	   }
	    /* Timecounters */
		pPckt->framestart = (iPackets == 0) ? true : false;
	   	pPckt->frameend = (vPackets.size() == 1) ? true : false;
		pPckt->unitnumber  = iUnit++;
		pPckt->subframenumber = iPackets++;
		pPckt->dts = iFFmpegDts;
		pPckt->pts = iFFmpegPts;
		pPckt->inc = pPckt->desc->inc;
		pPckt->mux = bMovFrameStructure ? mux_t::MOV : mux_t::ES;

		/* Key */
		if( av_pckt->flags & AV_PKT_FLAG_KEY )
			pPckt->key = true;

		/* Send */
		debug( 1, "parsed %s", pPckt->c_str_long() );
		vPackets.pop();
		MediaPacketPtr pWrapped( pPckt );
		route( pWrapped );
    }
	return true;
}

void FFmpegReader::parse_ctrl( packet_t::type type )
{
	/* Create a media packet */
    MediaPacketPtr pPckt ( new MediaPacket( type, content_t::mixed, 0 ) );

	/* Meta-data */
	pPckt->dts = iOldDts;
    pPckt->pts = pPckt->dts;
	pPckt->subframenumber   = 0;
    pPckt->framestart  = true;
	pPckt->frameend	  = true;
	pPckt->codec = codec_t::NO;
	pPckt->mux = bMovFrameStructure ? mux_t::MOV : mux_t::ES;

	/* Changes iStartDts */
	//iStartDts += pPckt->dts + pPckt->inc - iStartDts;
	//iBaseDts = -1;

	/* Report errors now */
	if( iErrors )
		SirannonWarning( this,  "Fixed %d DTS/PTS timestamp errors", iErrors );
	iErrors = 0;

	/* Generate and end/reset packet for each stream */
	if( videoStream != -1 )
	{
		MediaPacketPtr next_pckt ( new MediaPacket( *pPckt ) );
		next_pckt->content = content_t::video;
		next_pckt->xroute = pVideoDesc->route;
		next_pckt->unitnumber = iUnit++;
		next_pckt->xstream = videoMux;
		next_pckt->desc = pVideoDesc;
		next_pckt->codec = videoCodec;
		next_pckt->framenumber = iVideoFrame;
		next_pckt->frame = frame_t::no_frame;
		next_pckt->inc = pVideoDesc->inc;
		debug( 1, "parsed %s", next_pckt->c_str() );
		route( next_pckt );
	}
	if( audioStream != -1 )
	{
		MediaPacketPtr next_pckt ( new MediaPacket( *pPckt ) );
		next_pckt->content = content_t::audio;
		next_pckt->xroute = pAudioDesc->route;
		next_pckt->unitnumber = iUnit++;
		next_pckt->xstream = audioMux;
		next_pckt->desc = pAudioDesc;
		next_pckt->codec = audioCodec;
		next_pckt->framenumber = iAudioFrame++;
		next_pckt->inc = pAudioDesc->inc;
		debug( 1, "parsed %s", next_pckt->c_str() );
		route( next_pckt );
	}
}

void FFmpegReader::parseFrame( AVPacket* pAVPckt, AVCodecContext* pCodec )
{
	/* Make a new packet */
	MediaPacketPtr pPckt ( new MediaPacket( packet_t::media, content_t::mixed, pAVPckt->size ) );

	/* Add data */
	pPckt->push_back( pAVPckt->data, pAVPckt->size );

	/* Add to list */
	vPackets.push( pPckt.release() );
}

void FFmpegReader::parseAACMovFrame( AVPacket* pAVPckt, AVCodecContext* pCodec )
{
	if( bMovFrameStructure )
	{
		parseFrame( pAVPckt, pCodec );
	}
	else
	{
		oConvertor.convertESAudio( pAVPckt->data, pAVPckt->size, pAudioDesc, vPackets );
	}
}

void FFmpegReader::parseAACFrame( AVPacket* pAVPckt, AVCodecContext* pCodec )
{
	if( bMovFrameStructure )
	{
		MediaPacketPtr pPckt( oConvertor.convertMP4Audio( pAVPckt->data, pAVPckt->size, pAudioDesc ) );
		vPackets.push( pPckt.release() );
	}
	else
	{
		/* Evil: FFmpeg sometimes does not generate a correct profile (-99) in this case we need
		 * to extract the profile using the first ADTS header */
		if( audioCodec == codec_t::mp4a and pAudioDesc->profile < 1 )
		{
			oConvertor.ADTS2ESDS( pAVPckt->data, pAVPckt->size, pAudioDesc );
			debug( 1, "rereformed extra data(%d: %s)", pAudioDesc->getExtraSize(),
					strArray(pAudioDesc->getExtraData(),pAudioDesc->getExtraSize()).c_str() );
		}

		parseFrame( pAVPckt, pCodec );
	}
}

void FFmpegReader::parseAVCMovFrame( AVPacket* pAVPckt, AVCodecContext* pCodec )
{
	if( bMovFrameStructure )
	{
		parseFrame( pAVPckt, pCodec );
	}
	else
	{
		oConvertor.convertESVideo( pAVPckt->data, pAVPckt->size, pVideoDesc, vPackets, bAddParameterSets or bRepeatParameterSets );
		bAddParameterSets = false;
	}
}

void FFmpegReader::parseAVCFrame( AVPacket* pAVPckt, AVCodecContext* pCodec )
{
	if( bMovFrameStructure )
	{
		MediaPacketPtr pPckt( oConvertor.convertMP4Video( pAVPckt->data, pAVPckt->size, pVideoDesc ) );
		vPackets.push( pPckt.release() );
	}
	else
	{
		oConvertor.convertESVideo( pAVPckt->data, pAVPckt->size, pVideoDesc, vPackets, bAddParameterSets or bRepeatParameterSets );
		bAddParameterSets = false;
	}
}

void FFmpegReader::purgePackets( void )
{
	while( not vPackets.empty() )
	{
		delete vPackets.front();
		vPackets.pop();
	}
}
