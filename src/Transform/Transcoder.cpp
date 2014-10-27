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
#include "Transcoder.h"
#include "Frame.h"
#include "SirannonTime.h"
#include "OSSupport.h"

/** TRANSCODER
 * @component transcoder-video
 * @type transformer
 * @properties threaded, buffered, scheduled
 * @param output-codec, string, h264, the target codec
 * @param bitrate, int, -1, the target bitrate, -1 implies maintaining the same bitrate
 * @param width, int, 0, the target width, 0 implies maintaining the same width
 * @param height, int, 0, the target height, 0 implies maintaining the same height
 * @param framerate, int, -1, the new frame rate which must be less or equal to the current frame rate, -1 implies maintaning the same frame rate
 * @param mov-frame, bool, false, if true, generate H.264 in MOV/MP4 frames
 * @param target, string, , if defined, use specific encoding settings for this target, values: iphone, ipad, youtube
 * @info Decodes received packets and reencodes them using the specified settings. This component runs
 * the transcoding in a seperate thread and may consume all CPU. The component works
 * best effort and can not garantee realtime transcoding.
 **/
REGISTER_CLASS( TranscoderVideo, "transcoder-video" );

TranscoderVideo::TranscoderVideo( const string& sName, ProcessorManager* pProc )
	: MediaProcessor(sName, pProc), iUnit(0), iFrame(0),
	  pEncoderCtx(NULL), pDecoderCtx(NULL), pFrame(NULL), iStream(nextStreamID()),
	  iDecoded(0), iEncoded(0), iEncodedDts(0),
	  oEncoderStart(0),
	  pResampledFrame(NULL), pResampleCtx(NULL), iW(0), oW(0), iH(0), oH(0),
	  iFrames(0),  bMovFrame(false), pDesc(NULL), iTarget(target_t::NO), iMaxDts(-1), iMinDts(-1),
	  oConverter(true),  fAdaptiveFrameRate(-1.), iStartDts(-1), iFrac(0)
{
	mString["output-codec"] = "h264";
	mInt["output-codec-id"] = codec_t::NO;
	mInt["bitrate"] = -1;
	mInt["route"] = 100;
	mInt["width"] = 0;
	mInt["height"] = 0;
	mInt["framerate"] = -1;
	mBool["mov-frame"] = false;
	mString["target"] = "";
	mPrivate["desc"] = NULL;
	mInt["min-ts"] = -1;
	mInt["max-ts"] = -1;
	mString["fps"] = "0/0";
}

TranscoderVideo::~TranscoderVideo()
{
	if( pDecoderCtx )
	{
		ffmpeg_avcodec_close( pDecoderCtx );
		av_free( pDecoderCtx );
	}
	if( pEncoderCtx )
	{
		ffmpeg_avcodec_close( pEncoderCtx );
		av_free( pEncoderCtx );
	}
	if( pFrame )
		av_free( pFrame );

	if( pResampledFrame )
	{
		av_free( pResampledFrame->data[0] );
		av_free( pResampledFrame );
	}
	if( pResampleCtx )
		sws_freeContext( pResampleCtx );

	while( not vBuffer.empty() )
	{
		delete vBuffer.front();
		vBuffer.pop();
	}

	while( not vBuffer2.empty() )
	{
		delete vBuffer2.front();
		vBuffer2.pop_front();
	}
}

bool TranscoderVideo::ready( void ) const
{
	return true;
}

bool TranscoderVideo::bufferFull( void ) const
{
	if( getQueueSize() > 9000 )
		return true;
	return false;
}

void TranscoderVideo::init( void )
{
	/* Base class */
	MediaProcessor::init();

	/* We schedule */
	bSchedule = true;

	/* Translate the selected codecs */
	pDesc = addMedia();
	pDesc->codec = (codec_t::type) mInt["output-codec-id"];
	if( pDesc->codec == codec_t::NO )
	{
		pDesc->codec = StringToCodec( mString["output-codec"].c_str() );
		if( pDesc->codec == codec_t::NO )
			RuntimeError( this, "unknown codec '%s'", mString["output-codec"].c_str() );
	}
	/* Frame rate reduction */
	if( mString["fps"].length() and
				sscanf( mString["fps"].c_str(), "%d_%d", &iReduce.num, &iReduce.den ) == 2 )
	{
		fAdaptiveFrameRate = -1.;
		if( iReduce.den <= 0 or iReduce.num > iReduce.den )
			ValueError( this, "Malformed parameter fps: %s", mString["fps"].c_str() );
	}
	else if( fAdaptiveFrameRate > 0. )
	{
		iReduce.num = 1000;
		iReduce.den = 1000;
	}
	else
	{
		iReduce.num = -1;
		iReduce.den = 1;
	}
	/* Frames */
	pFrame = avcodec_alloc_frame();

	/* Dimensions */
	oW = mInt["width"];
	oH = mInt["height"];

	/* Time constraints */
	iMaxDts = mInt["max-ts"];
	if( iMaxDts > 0 )
		iMaxDts = iMaxDts * 90 + TIMESTAMP_MIN;
	iMinDts = mInt["min-ts"];
	if( iMinDts > 0 )
		iMinDts = iMinDts * 90 + TIMESTAMP_MIN;
	debug( 1, "reduce(%d/%d) min(%d) max(%d)", iReduce.num, iReduce.den, iMinDts, iMaxDts );

	/* Annex B or mov NAL format? */
	bMovFrame = mBool["mov-frame"];

	/* Already init the decoder and encoder when a private descriptor field has been delivered */
	MediaDescriptor* pSourceDesc = (MediaDescriptor*) mPrivate["desc"];
	if( pSourceDesc )
		encoder_init( pSourceDesc );

	 /* Threaded */
	 forcePriviligedThread( -1 );
}

int TranscoderVideo::flush( void ) synchronized
{
	MediaProcessor::flush();

	decode_reset();
	encode_reset();

	if( mString["fps"].length() and mString["fps"] != "disable" and
			sscanf( mString["fps"].c_str(), "%d_%d", &iReduce.num, &iReduce.den ) == 2 )
	{
		fAdaptiveFrameRate = -1.;
		if( iReduce.den <= 0 or iReduce.num > iReduce.den )
			ValueError( this, "Malformed parameter fps: %s", mString["fps"].c_str() );
	}
	else if( fAdaptiveFrameRate > 0. )
	{
		iReduce.num = 1000;
		iReduce.den = 1000;
	}
	else
	{
		iReduce.num = -1;
		iReduce.den = 1;
	}

	iFrames = iEncoded = iDecoded = iEncodedDts = iFrac = 0;
	vEncodeDTS = vDecodeDTS = queue<int>();
	debug( 1, "flushing complete" );
	return 0;
} end_synchronized

void TranscoderVideo::receive_end( MediaPacketPtr& pPckt )
{
	receive( pPckt );
}

void TranscoderVideo::receive_reset( MediaPacketPtr& pPckt )
{
	receive( pPckt );
}

void TranscoderVideo::receive( MediaPacketPtr& pPckt )
{
	/* Decode */
	int iFinished;
	if( pPckt->type == packet_t::media )
	{
		/* Decoder ready? */
		if( not pDecoderCtx )
			decoder_init( pPckt->desc, (pPckt->mux == mux_t::MOV) );

		/* Merge */
		pPckt = mergeFrameParts( vBuffer2, pPckt, false );
		if( not pPckt.get() )
			return;

		/* Remember timing */
		if( iStartDts < 0 )
			iStartDts = pPckt->dts;
		iFrames++;
		vDecodeDTS.push( pPckt->dts );

		/* Decode */
		AVPacket oFFmpegPckt;
		av_init_packet( &oFFmpegPckt );
		oFFmpegPckt.data = pPckt->data();
		oFFmpegPckt.size = pPckt->size();

		/* Decode it */
		int iStatus = avcodec_decode_video2( pDecoderCtx, pFrame, &iFinished, &oFFmpegPckt );
		if( iStatus < 0 )
			FFmpegError( this, "Could not decode frame: %s", pPckt->c_str() );
		else if( iStatus == 0 )
		{
			debug( 3, "wait: %s", pPckt->c_str() );
			return;
		}
		else if( pPckt->dts < iMinDts )
		{
			debug( 3, "pre-start: %s", pPckt->c_str() );
			iFrames--;
			vDecodeDTS.pop();
			return;
		}
		else
		{
			debug( 3, "decoded: %s", pPckt->c_str() );
		}
		/* Fetch a DTS */
		int iDTS = vDecodeDTS.front();
		vDecodeDTS.pop();

		/* Frame rate reduction */
		if( fAdaptiveFrameRate > 0. )
		{
			SirannonTime oDiff( SirannonTime::getCurrentTime() );
			oDiff -= oEncoderStart;

			double fRate = ( iDTS - iStartDts ) / 90000. / oDiff.convertDouble() / fAdaptiveFrameRate;
			if( fRate < 1. )
				iReduce.num -= 50;
			else
				iReduce.num += 50;

			iReduce.num = MIN( iReduce.num, 1000 );
			iReduce.num = MAX( iReduce.num, 200 );
		}
		if( iReduce.num > 0 )
		{
			iFrac += iReduce.num;
			if( iFrac >= iReduce.den )
			{
				/* Keep! */
				iFrac = iFrac % iReduce.den;
			}
			else
			{
				/* Drop! */
				debug( 3, "dropped: %s", pPckt->c_str() );
				iFrames--;
				return;
			}
		}
		/* Put the timestamp in another queue */
		vEncodeDTS.push( iDTS );

		/* Finished the decode? */
		if( iFinished )
		{
			/* Reset some values so that encoder does not keep their settings */
			pFrame->pts = pPckt->dts;
			pFrame->pict_type = AV_PICTURE_TYPE_NONE;

			/* Encode it  */
			iDecoded++;
			encode( pPckt.get() );
		}
	}
	/* Reset encoder */
	else
	{
		/* Call the decoder "iDecoded - iEncoded" times more at the end */
		debug( 1, "frames %d, decoded %d, encoded %d, diff1 %d, diff2 %d",
				iFrames, iDecoded, iEncoded, iFrames - iDecoded, iDecoded - iEncoded );

		/* Manage the decoder delay */
		while( iDecoded < iFrames )
		{
			/* Packet to feed the decoder */
			AVPacket oFFmpegPckt;
			av_init_packet( &oFFmpegPckt );
			oFFmpegPckt.data = NULL;
			oFFmpegPckt.size = 0;
			int iBytes = avcodec_decode_video2( pDecoderCtx, pFrame, &iFinished, &oFFmpegPckt );
			iDecoded++;
			encode( pPckt.get() );
		}
		/* Manage the encoder delay */
		while( iEncoded < iFrames )
			encode( pPckt.get() );
		debug( 2, "frames %d, decoded %d, encoded %d", iFrames, iDecoded, iEncoded );

		/* Reset the encoder/decoder */
		decode_reset();
		encode_reset();
		if( pPckt->type == packet_t::end )
			encode_ctrl( pPckt );
	}
}

void TranscoderVideo::decoder_init( const MediaDescriptor* pSourceDesc, bool bSourceMov )
{
	/* Save info */
	iEncoded = iDecoded = 0;
	oEncoderStart = SirannonTime::getCurrentTime();

	/* Ready the codec context */
	if( not pDecoderCtx )
	{
		/* Find codec */
		enum CodecID iCodec = SirannonCodecToFFCodec( pSourceDesc->codec );
		pDecoder = ffmpeg_avcodec_find_decoder( iCodec );
		if( not pDecoder )
			RuntimeError( this, "Decoder not found for codec(%s)", CodecToString(pSourceDesc->codec ) );

		/* Create context */
		pDecoderCtx = avcodec_alloc_context3( pDecoder );
		pDecoderCtx->codec_type = AVMEDIA_TYPE_VIDEO;
		pDecoderCtx->time_base.num = pSourceDesc->inc;
		pDecoderCtx->time_base.den = 90000;

		/* Extra data */
		if( pSourceDesc->codec != codec_t::avc or bSourceMov )
		{
			pDecoderCtx->extradata = (uint8_t*) pSourceDesc->getExtraData();
			pDecoderCtx->extradata_size = pSourceDesc->getExtraSize();
		}
		/* Open decoder */
		if( ffmpeg_avcodec_open( pDecoderCtx, pDecoder ) < 0 )
			RuntimeError( this, "Could not setup decoder" );
	}
	/* Timing info */
	debug( 1, "decoder: codec(%s->%s) frame rate(%f) dts(%d) inc(%d) rate(%d kbps)",
			CodecToString(pSourceDesc->codec), CodecToString(pDesc->codec),
			pDecoderCtx->time_base.den * 1. / pDecoderCtx->time_base.num,
			iEncodedDts, pSourceDesc->inc, pSourceDesc->bitrate / 1000 );
}

void TranscoderVideo::decode_reset( bool bFinal )
{
	/* Run init again to reset the decoder partially*/
	debug( 1, "decoder reset" );
	iFrames = 0;

	/* Free */
	if( pDecoderCtx )
	{
		ffmpeg_avcodec_close( pDecoderCtx );
		av_freep( &pDecoderCtx );
	}
}

void TranscoderVideo::encoder_init( const MediaDescriptor* pSourceDesc )
{
	/* Create the codec context */
	if( not pEncoderCtx )
	{
		/* Bitrate */
		pDesc->bitrate = mInt["bitrate"];
		if( pDesc->bitrate <= 0 )
			pDesc->bitrate = pSourceDesc->bitrate;
		if( pDesc->bitrate <= 0 )
			pDesc->bitrate = MEGA;

		/* Find the codec */
		enum CodecID iCodec = SirannonCodecToFFCodec( pDesc->codec );
		pEncoder = ffmpeg_avcodec_find_encoder( iCodec );
		if( not pEncoder )
			RuntimeError( this, "Encoder not found(%s)", CodecToString(pDesc->codec) );

		/* Alloc */
		pEncoderCtx = avcodec_alloc_context3( pEncoder );
		oEncoderStart = SirannonTime::getCurrentTime();

		/* Due to the conflict between FFmpeg and X264, I had to load all these settings myself */
		pEncoderCtx->coder_type = 1;
		pEncoderCtx->flags |= CODEC_FLAG_LOOP_FILTER;
		pEncoderCtx->me_cmp = FF_CMP_CHROMA;
		pEncoderCtx->partitions = X264_PART_I8X8 | X264_PART_I4X4 | X264_PART_P8X8 | X264_PART_B8X8;
		pEncoderCtx->me_method = ME_UMH;
		pEncoderCtx->me_subpel_quality = 8;
		pEncoderCtx->me_range = 16;
		pEncoderCtx->scenechange_threshold = 40;
		pEncoderCtx->i_quant_factor = 0.71;
		pEncoderCtx->b_frame_strategy = 2; // 1
		pEncoderCtx->qcompress = 0.6;
		pEncoderCtx->qmin = 0;
		pEncoderCtx->qmax = 30;
		pEncoderCtx->max_qdiff = 4;
		pEncoderCtx->max_b_frames = 0; // 3
		pEncoderCtx->refs = 4;
		pEncoderCtx->directpred = 3;
		pEncoderCtx->trellis = 1;
		pEncoderCtx->flags2 |= CODEC_FLAG2_WPRED | CODEC_FLAG2_MIXED_REFS | CODEC_FLAG2_8X8DCT |CODEC_FLAG2_FASTPSKIP | CODEC_FLAG2_STRICT_GOP;
		pEncoderCtx->gop_size = 250;

		/* Input dimensions */
		iW = pSourceDesc->width, iH = pSourceDesc->height;

		/* Create the resample filter if needed */
		if( oW > 0 and oH > 0 and ( oW != iW or oH != iH ) )
		{
			if( not pResampleCtx )
			{
				/* Resample filter */
				pResampleCtx = sws_alloc_context();
				sws_init_context( pResampleCtx, NULL, NULL );
				pResampleCtx = sws_getContext( iW, iH, PIX_FMT_YUV420P,
											oW, oH, PIX_FMT_YUV420P,
											SWS_BICUBIC, NULL, NULL, NULL);

				/* Create the resample frame */
				pResampledFrame = avcodec_alloc_frame();
				avcodec_get_frame_defaults( pResampledFrame );
				avpicture_alloc( (AVPicture*)pResampledFrame, PIX_FMT_YUV420P, oW, oH );

				/* Print format */
				debug( 1, "dimensions: %dx%d --> %dx%d", iW, iH, oW, oH );
			}
			/* Dimensions */
			pEncoderCtx->width = oW;
			pEncoderCtx->height = oH;
		}
		else
		{
			pEncoderCtx->width = iW;
			pEncoderCtx->height = iH;
		}
		/* Additional encoder parameters */
		pEncoderCtx->time_base.num = pSourceDesc->inc; //pSourceDesc->timebase.num; //pDecoderCtx->time_base.num;
		pEncoderCtx->time_base.den = 90000; //pSourceDesc->timebase.den; //pDecoderCtx->time_base.den;
		pEncoderCtx->keyint_min = -1;
		pEncoderCtx->pix_fmt = PIX_FMT_YUV420P;
		pEncoderCtx->thread_count = 1;
		pEncoderCtx->bit_rate_tolerance = pDesc->bitrate;
		pEncoderCtx->rc_max_rate = pDesc->bitrate;
		pEncoderCtx->rc_buffer_size = pDesc->bitrate;
		pEncoderCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;

		/* Open codec */
		if( ffmpeg_avcodec_open( pEncoderCtx, pEncoder ) < 0 )
		   RuntimeError( this, "Could not setup encoder(%s)", CodecToString(pDesc->codec) );

		/* Fill out the Sirannon discriptor */
		if( not pDesc )
			pDesc = addMedia();
		pDesc->width = pEncoderCtx->width;
		pDesc->height = pEncoderCtx->height;
		pDesc->content = content_t::video;
		pDesc->gopsize = pEncoderCtx->gop_size;
		pDesc->inc = pSourceDesc->inc;
		pDesc->route = pSourceDesc->route;
		if( pEncoderCtx->extradata_size )
		{
			if( pDesc->codec == codec_t::avc )
				oConverter.buildVideoExtraData( pEncoderCtx->extradata, pEncoderCtx->extradata_size, pDesc );
			else
				pDesc->setExtraData( pEncoderCtx->extradata, pEncoderCtx->extradata_size );

			debug( 1, "encoder: bitrate(%d) extra(%d: %s)", pEncoderCtx->bit_rate,
					 pDesc->getExtraSize(), strArray(pDesc->getExtraData(), pDesc->getExtraSize()).c_str() );
		}
	}
}

void TranscoderVideo::encode( MediaPacket* pSourcePckt )
{
	/* Initialize encoder if needed */
	if( not pEncoderCtx )
		encoder_init( pSourcePckt->desc );

	/* Resample the frame if required */
	if( pResampleCtx )
	{
		/* Scale the video */
		sws_scale( pResampleCtx, pFrame->data, pFrame->linesize,
		              0, iH, pResampledFrame->data, pResampledFrame->linesize );
		pResampledFrame->pts = iEncodedDts;
	}
	pFrame->pts = iEncodedDts;

	/* Encoding loop */
	int iBytes = avcodec_encode_video( pEncoderCtx, pEncoderBuffer, iEncoderBuffer, pResampleCtx ? pResampledFrame : pFrame );
	if( iBytes > 0 )
		iEncoded++;
	else if( iBytes == 0 )
		return;
	else
		RuntimeError( this, "encoding failed" );

	/* New DTS */
	iEncodedDts = vEncodeDTS.front();
	vEncodeDTS.pop();

	/* Convert the coded frame to actual frames */
	if( pDesc->codec == codec_t::avc )
	{
		/* Convert H.264 frames in the two different styles */
		if( bMovFrame )
		{
			MediaPacketPtr pPckt( oConverter.convertMP4Video( pEncoderBuffer, iBytes, pDesc ) );
			vBuffer.push( pPckt.release() );
		}
		else
		{
			oConverter.convertESVideo( pEncoderBuffer, iBytes, pDesc, vBuffer, true );
//    		if( vBuffer.back()->frame == frame_t::IDR )
//    			iRefDts = iEncodedDts;
//   			iRefPts = iRefDts + ( oConverter.getNALParse()->num_ref_frames + oConverter.getPOC() / 2 ) * pDesc->inc;
		}
	}
	else
	{
		MediaPacket* pPckt = new MediaPacket( packet_t::media, content_t::video, iBytes );
		pPckt->push_back( pEncoderBuffer, iBytes );
		vBuffer.push( pPckt );
	}
	/* Release the buffer */
	int iSubFrame = 0;
	while( not vBuffer.empty() )
	{
		/* Next subframe */
		MediaPacket* pPckt = vBuffer.front();
		vBuffer.pop();

		/* Set meta-data */
		pPckt->content = content_t::video;
		pPckt->type = packet_t::media;
		pPckt->xstream = iStream;
		pPckt->xroute = pDesc->route;
		pPckt->dts = TIMESTAMP_MIN + (iEncodedDts - TIMESTAMP_MIN ) / 1;
		pPckt->pts = pPckt->dts;
		pPckt->inc = pDesc->inc;
		pPckt->desc = pDesc;
		pPckt->framestart = iSubFrame == 0 ? true : false;
		pPckt->frameend = (vBuffer.size() == 0) ? true : false;
		pPckt->codec = pDesc->codec;
		pPckt->unitnumber = iUnit++;
		pPckt->framenumber = iFrame;
		pPckt->subframenumber = iSubFrame++;
		pPckt->key = pEncoderCtx->coded_frame->key_frame;
		pPckt->mux = bMovFrame ? mux_t::MOV : mux_t::ES;

		/* Estimate the encoding rate */
		int iDiff = ( SirannonTime::getCurrentTime() - oEncoderStart ).convertMsecs();
		debug( 2, "encoded %s, delay: %d ms, rate: %02.2f fps, queue %d ms, frames: %d, dec: %d, enc: %d, fps: %d ~ %d/%d",
				pPckt->c_str(), iDiff, iEncoded * 1000. / iDiff, getQueueSize()/90, iFrames, iDecoded, iEncoded, iFrac, iReduce.num, iReduce.den );

		/* Done */
		MediaPacketPtr pWrapper( pPckt );
		route( pWrapper );
	}
	/* Frame done */
	iFrame++;
}

void TranscoderVideo::encode_reset( bool bFinal )
{
	/* Flag */
	debug( 2, "encoder reset" );

	/* Free */
	if( pEncoderCtx )
	{
		ffmpeg_avcodec_close( pEncoderCtx );
		av_freep( &pEncoderCtx );
	}
}

void TranscoderVideo::encode_ctrl( MediaPacketPtr& pPckt )
{
	/* Set meta-data */
	pPckt->unitnumber = iUnit++;
	pPckt->framenumber = iFrame;
	pPckt->xstream = iStream;
	pPckt->xroute = pDesc->route;
	pPckt->dts = iEncodedDts;
	pPckt->pts = pPckt->dts;
	pPckt->inc = pDesc->inc;
	pPckt->codec = pDesc->codec;

	/* Add to output queue */
	debug( 1, "encoded %s", pPckt->c_str() );
	route( pPckt );
}
