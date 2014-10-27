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
#include "TranscoderAudio.h"
#include "OSSupport.h"
#include "Frame.h"

/** TRANSCODER
 * @component transcoder-audio
 * @type transformer
 * @properties threaded, buffered, scheduled
 * @param output-codec, string, mp4a, the target codec
 * @param bitrate, int, -1, the target bitrate, -1 implies maintaining the same bitrate
 * @param target, string, , if defined, use specific encoding settings for this target, values: iphone, ipad, youtube
 * @param route, int, 200, xroute assigned to the transcoded audio frames
 * @info Decodes received packets and reencodes them using the specified settings. This component runs
 * the transcoding in a seperate thread and may consume all CPU.
 **/
REGISTER_CLASS( TranscoderAudio, "transcoder-audio" );

static int getMaxAACBitrate( int iSampleRate )
{
     return (int)(6144.0 * iSampleRate/ 1024 + .5) - 1000;
}

TranscoderAudio::TranscoderAudio( const string& sName, ProcessorManager* pProc )
	: MediaProcessor(sName, pProc), iUnit(0), iFrame(0),
	  pEncoderCtx(NULL), pDecoderCtx(NULL), iStream(nextStreamID()),
	  iDecoded(0), iEncoded(0),
	  oEncoderStart(0), oStart(0),
	  pDesc(NULL), bMovFrame(false),
	  bBypass(false), iSamplesEnd(0), iSamplesStart(0), iSamplesTotal(0), iStartDts(-1)
{
	pSamplesAligned = (int16_t*) mem_align( (uint8_t*) pSamplesAlignedBuffer, 16 );
	iMaxSamples = sizeof(pSamplesNonAligned) / 2 - 16;

	mString["output-codec"] = "mp4a";
	mInt["output-codec-id"] = codec_t::NO;
	mInt["bitrate"] = -1;
	mInt["route"] = 200;
	mBool["youtube"] = false;
	mPrivate["desc"] = NULL;
	mInt["min-ts"] = -1;
	mInt["max-ts"] = -1;
}

TranscoderAudio::~TranscoderAudio()
{
	decode_reset();
	encode_reset();
}

bool TranscoderAudio::ready( void ) const
{
	return true;
}

bool TranscoderAudio::bufferFull( void ) const
{
	if( getQueueSize() > 9000 )
		return true;
	return false;
}

void TranscoderAudio::init( void )
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
	/* Annex B or mov NAL format? */
	bMovFrame = mBool["mov-frame"];

	/* Already generate a descriptor when a descriptor field has been delivered */
	MediaDescriptor* pSourceDesc = (MediaDescriptor*) mPrivate["desc"];
	if( pSourceDesc )
	{
		if( pDesc->codec != pSourceDesc->codec )
		{
			decoder_init( pSourceDesc );
			encoder_init( pSourceDesc );
		}
		else
		{
			bBypass = true;
			*pDesc = *pSourceDesc;
		}
	}
	/* Time constraints */
	iMaxDts = mInt["max-ts"];
	if( iMaxDts > 0 )
		iMaxDts = iMaxDts * 90 + TIMESTAMP_MIN;
	iMinDts = mInt["min-ts"];
	if( iMinDts > 0 )
		iMinDts = iMinDts * 90 + TIMESTAMP_MIN;
	debug( 1, "min(%d) max(%d)", iMinDts, iMaxDts );

	/* Start time */
	oStart = SirannonTime::getCurrentTime();
	if( not bBypass )
		forcePriviligedThread( oQuantum.convertNsecs() );
}

void TranscoderAudio::receive_end( MediaPacketPtr& pPckt )
{
	receive( pPckt );
}
void TranscoderAudio::receive_reset( MediaPacketPtr& pPckt )
{
	receive( pPckt );
}

int TranscoderAudio::flush( void ) synchronized
{
	MediaProcessor::flush();

	decode_reset();
	encode_reset();
	bSchedule = true;
	debug( 1, "flushing complete" );
	return 0;
} end_synchronized

void TranscoderAudio::decoder_init( const MediaDescriptor* pSourceDesc )
{
	/* Save timing info */
	iFrame = 0;
	oEncoderStart = SirannonTime::getCurrentTime();

	/* Ready the codec context */
	if( not pDecoderCtx )
	{
		/* Set up the decoder */
		enum CodecID iCodec = SirannonCodecToFFCodec( pSourceDesc->codec );
		pDecoder = ffmpeg_avcodec_find_decoder( iCodec );
		if( not pDecoder )
			RuntimeError( this, "Decoder not found for codec(%s[%d])", CodecToString(pSourceDesc->codec), iCodec );
		pDecoderCtx = avcodec_alloc_context3( pDecoder );

		/* Basic parameters */
		pDecoderCtx->time_base.num = pSourceDesc->inc;
		pDecoderCtx->time_base.den = 90000;
		pDecoderCtx->channels = pSourceDesc->channels;
		pDecoderCtx->sample_rate = pSourceDesc->samplerate;
		pDecoderCtx->profile = pSourceDesc->profile;
		if( pSourceDesc->codec != codec_t::mp4a )
		{
			pDecoderCtx->extradata = (uint8_t*) pSourceDesc->getExtraData();
			pDecoderCtx->extradata_size = pSourceDesc->getExtraSize();
		}
		/* Open codec */
		if( ffmpeg_avcodec_open( pDecoderCtx, pDecoder ) < 0 )
		   RuntimeError( this, "Could not setup decoder for codec(%s[%d])", CodecToString(pSourceDesc->codec), iCodec );
	}
	iSamplesEnd = 0;
	iStartDts = -1;
	iSamplesTotal = 0;
	iDecoded = iEncoded = 0;

	/* Timing info */
	debug( 1, "decoder: codec(%s->%s) timebase(%d/%d) profile(%d) samplerate(%d) channels(%d) framesize(%d) bitrate(%d) extra(%s)",
			CodecToString(pSourceDesc->codec), CodecToString(pDesc->codec),
			pDecoderCtx->time_base.num, pDecoderCtx->time_base.den, pDecoderCtx->profile,
			pDecoderCtx->sample_rate, pDecoderCtx->channels, pDecoderCtx->frame_size,
			pDecoderCtx->bit_rate, strArray( pDecoderCtx->extradata, pDecoderCtx->extradata_size ).c_str() );
}

void TranscoderAudio::decode_reset( void )
{
	/* Run init again to reset the decoder partially */
	debug( 2, "decoder reset" );
	if( pDecoderCtx )
	{
		ffmpeg_avcodec_close( pDecoderCtx );
		av_freep( &pDecoderCtx );
	}
}

void TranscoderAudio::receive( MediaPacketPtr& pPckt )
{
	/* Decoding loop */
	if( pPckt->type == packet_t::media )
	{
		/* Bypass system */
		if( bBypass )
		{
			/* Time constraints */
			if( pPckt->dts < iMinDts )
				return;

			pPckt->xstream = iStream;
			if( bMovFrame )
				pPckt = oConverter.convertMP4( pPckt );
			route( pPckt );
			return;
		}
		/* Decoder ready? */
		if( not pDecoderCtx )
			decoder_init( pPckt->desc );

		/* Decode */
		AVPacket oFFmpegPckt;
		av_init_packet( &oFFmpegPckt );
		oFFmpegPckt.data = pPckt->data();
		oFFmpegPckt.size = pPckt->size();

		/* Decode */
		int iSamples = iMaxSamples;
		int iStatus = avcodec_decode_audio3( pDecoderCtx, pSamplesAligned, &iSamples, &oFFmpegPckt );
		if( iStatus < 0 )
		{
			SirannonWarning( this, "Decoding failed: %s", pPckt->c_str() );
			return;
		}
		else if( iStatus == 0 )
			return;
		else if( iStatus != oFFmpegPckt.size )
			FFmpegError( this, "Decoder did not consume entire frame: (%d) != (%d)", iStatus, oFFmpegPckt.size );

		/* Copy */
		memcpy( pSamplesNonAligned + iSamplesEnd, pSamplesAligned, iSamples );
		iSamplesEnd += iSamples / sizeof(int16_t);
		debug( 2, "decoded: samples(%d->%d:%d), bytes(%d): %s",
				iSamplesEnd - iSamples / sizeof(int16_t), iSamplesEnd, iSamples / sizeof(int16_t), iSamples, pPckt->c_str() );

		/* Time constraints */
		if( pPckt->dts < iMinDts )
			return;
		else if( iStartDts < 0 )
			iStartDts = pPckt->dts;

		/* Encode */
		iDecoded++;
		for( int i = 0; iSamplesEnd - iSamplesStart > iSamplesPerStep; ++i )
		{
			debug( 1, "buffer (%d->%d:%d)", iSamplesStart, iSamplesEnd, iSamplesEnd - iSamplesStart );
			if( i > 0 )
				debug( 1, "EXTRA FRAME" );
			encode( pPckt.get() );

			/* Move back to start */
			memmove( pSamplesNonAligned, pSamplesNonAligned + iSamplesStart, (iSamplesEnd - iSamplesStart) * sizeof(int16_t) );
			iSamplesEnd -= iSamplesStart;
			iSamplesStart = 0;
			debug( 4, "buffer (%d->%d:%d)", iSamplesStart, iSamplesEnd, iSamplesEnd - iSamplesStart );
		}
	}
	else
	{
		/* Manage encoder delay */
		debug( 1, "decoded(%d) encoded(%d) diff(%d)", iDecoded, iEncoded, iDecoded - iEncoded );
		while( iEncoded < iDecoded )
			encode( pPckt.get() );

		/* Reset the encoder/decoder */
		decode_reset();
		encode_reset();

		/* Send it */
		encode_ctrl( pPckt );
	}
}

void TranscoderAudio::encoder_init( const MediaDescriptor* pSourceDesc )
{
	/* Create the codec context */
	if( not pEncoderCtx )
	{
		/* Bitrate */
		pDesc->bitrate = mInt["bitrate"];
		if( pDesc->bitrate <= 0 )
			pDesc->bitrate = pSourceDesc->bitrate;
		if( pDesc->bitrate <= 0 )
			pDesc->bitrate = 256000;

		/* Find the encoder */
		enum CodecID iCodec = SirannonCodecToFFCodec( pDesc->codec );
		pEncoder = ffmpeg_avcodec_find_encoder( iCodec );
		if( not pEncoder )
			pEncoder = avcodec_find_encoder_by_name( CodecToString(pDesc->codec) );
		if( not pEncoder )
			RuntimeError( this, "Encoder not found for codec (%s[%d])", CodecToString(pDesc->codec), iCodec  );
		pEncoderCtx = avcodec_alloc_context3( pEncoder );

		/* Additional encoder parameters */
		pEncoderCtx->time_base.num = pDecoderCtx->time_base.num;
		pEncoderCtx->time_base.den = pDecoderCtx->time_base.den;
		pEncoderCtx->sample_rate = pDecoderCtx->sample_rate;
		pEncoderCtx->channels = pDecoderCtx->channels;
		pEncoderCtx->bit_rate = MIN(pDesc->bitrate, getMaxAACBitrate(pDecoderCtx->sample_rate));
		pEncoderCtx->channel_layout = pDecoderCtx->channel_layout;
		pEncoderCtx->frame_size = pDecoderCtx->frame_size;
		pEncoderCtx->profile = FF_PROFILE_AAC_MAIN;
		pEncoderCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;
		pEncoderCtx->sample_fmt = pDecoderCtx->sample_fmt;

		debug( 1, "encoder: codec(%s->%s) timebase(%d/%d) profile(%d) samplerate(%d) channels(%d) framesize(%d) bitrate(%d) extra(%s)",
				CodecToString(pSourceDesc->codec), CodecToString(pDesc->codec),
				pEncoderCtx->time_base.num, pEncoderCtx->time_base.den, pEncoderCtx->profile,
				pEncoderCtx->sample_rate, pEncoderCtx->channels, pEncoderCtx->frame_size,
				pEncoderCtx->bit_rate, strArray( pEncoderCtx->extradata, pEncoderCtx->extradata_size ).c_str() );

		/* Open codec */
		if( ffmpeg_avcodec_open( pEncoderCtx, pEncoder ) < 0 )
		   RuntimeError( this, "Could not setup encoder" );

		/* Fill out the Sirannon descriptor */
		if( not pDesc )
			pDesc = addMedia();
		pDesc->content = content_t::audio;
		pDesc->channels = pEncoderCtx->channels;
		pDesc->samplerate = pEncoderCtx->sample_rate;
		pDesc->framesize = pEncoderCtx->frame_size;
		pDesc->setExtraData( pEncoderCtx->extradata, pEncoderCtx->extradata_size );
		pDesc->profile = pEncoderCtx->profile;
		pDesc->route = pSourceDesc->route;
		pDesc->inc = pDesc->framesize * 90000 / pDesc->samplerate;
		iSamplesPerStep = pDesc->channels * pDesc->framesize;

		debug( 1, "encoder: codec(%s->%s) timebase(%d/%d) profile(%d) samplerate(%d) channels(%d) framesize(%d) bitrate(%d) extra(%s)",
				CodecToString(pSourceDesc->codec), CodecToString(pDesc->codec),
				pEncoderCtx->time_base.num, pEncoderCtx->time_base.den, pEncoderCtx->profile,
				pEncoderCtx->sample_rate, pEncoderCtx->channels, pEncoderCtx->frame_size,
				pEncoderCtx->bit_rate, strArray( pEncoderCtx->extradata, pEncoderCtx->extradata_size ).c_str() );
		iSamplesStart = 0;
	}
}

void TranscoderAudio::encode( MediaPacket* pSourcePckt )
{
	/* Initialize encoder if needed */
	MediaDescriptor* pSourceDesc = pSourcePckt->desc;
	if( not pEncoderCtx )
		encoder_init( pSourcePckt->desc );

	/* Encoding loop */
	int iBytes = avcodec_encode_audio( pEncoderCtx, pEncoderBuffer, sizeof(pEncoderBuffer), pSamplesNonAligned + iSamplesStart );
	iSamplesStart += iSamplesPerStep;
	iSamplesTotal += pDesc->framesize;
	if( iBytes > 0 )
		iEncoded++;
	else if( iBytes == 0 )
	{
		debug( 4, "encoder delay: buffer(%d->%d:%d)", iSamplesStart, iSamplesEnd, iSamplesEnd - iSamplesStart );
		return;
	}
	else
		RuntimeError( this, "Encoding failed" );

	/* Create a media packet */
	MediaPacketPtr pPckt( NULL );
	if( pDesc->codec == codec_t::mp4a and not bMovFrame )
	{
		oConverter.convertESAudio( pEncoderBuffer, iBytes, pDesc, vBuffer );
		pPckt.reset( vBuffer.front() );
		if( vBuffer.size() > 1 )
			RuntimeError( "Audio packet converted into more than one elementary packet" );
		vBuffer.pop();
	}
	else
	{
		pPckt.reset( new MediaPacket( packet_t::media, content_t::audio, iBytes ) );
		pPckt->push_back( pEncoderBuffer, iBytes );
	}
	/* Set meta-data */
	pPckt->xstream = iStream;
	pPckt->xroute = pDesc->route;
	pPckt->dts = iStartDts + iSamplesTotal * 90000 / pDesc->samplerate;
	pPckt->pts = pPckt->dts;
	pPckt->inc = pDesc->inc;
	pPckt->desc = pDesc;
	pPckt->framestart = pPckt->frameend = true;
	pPckt->codec = pDesc->codec;
	pPckt->unitnumber = iUnit++;
	pPckt->framenumber = iFrame++;
	pPckt->subframenumber = 0;
	pPckt->frame = frame_t::AUD;
	pPckt->mux = bMovFrame ? mux_t::MOV : mux_t::ES;

	/* Estimate the encoding rate */
	int iDiff = ( SirannonTime::getCurrentTime() - oEncoderStart ).convertMsecs();
	double fFps = iEncoded * 1000. / iDiff;
	debug( 1, "encoded %s, delay: %d ms, rate: %02.2f fps, decoded %d, encoded: %d, buffers: %d->%d|%d",
			pPckt->c_str_long(), iDiff, fFps, iDecoded, iEncoded, iSamplesStart, iSamplesEnd, iSamplesEnd - iSamplesStart );

	route( pPckt );
}

void TranscoderAudio::encode_reset( void )
{
	debug( 2, "encoder reset" );
	if( pEncoderCtx )
	{
		ffmpeg_avcodec_close( pEncoderCtx );
		av_freep( &pEncoderCtx );
	}
}

void TranscoderAudio::encode_ctrl( MediaPacketPtr& pPckt )
{
	/* Set meta-data */
	pPckt->unitnumber = iUnit++;
	pPckt->framenumber = iFrame;
	pPckt->xstream = iStream;
	pPckt->xroute = pDesc->route;
	if( not bBypass )
		pPckt->dts = iStartDts + iSamplesTotal * 90000 / pDesc->samplerate;
	pPckt->pts = pPckt->dts;
	pPckt->inc = pDesc->inc;
	pPckt->codec = pDesc->codec;

	/* Add to output queue */
	debug( 1, "encoded %s", pPckt->c_str() );
	route( pPckt );
}
