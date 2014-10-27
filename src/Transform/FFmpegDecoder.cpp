#include "FFmpegDecoder.h"

REGISTER_CLASS( FFmpegDecoder, "ffmpeg-decoder" );

/** FRAME ANALYZER
 * @component ffmpeg-decoder
 * @type transformer
 * @param reset-on-reset, bool, true, if true, reset the decoder when a reset-packet is received
 * @param frame-copy, bool, false, if true, use frame copy as basic error concealment
 * @info This components decodes a video sequence and generates a stream of YUV packets
 **/

FFmpegDecoder::FFmpegDecoder( const string& sName, ProcessorManager* pScope )
	: MediaProcessor(sName, pScope), iFrame(0), iUnit(0), iDecoded(0), iFrames(0),
	  iStreamID(nextStreamID()), iSize(-1), oStart(0,0), bReset(true),
	  pDecoderCtx(NULL), pDecoder(NULL), pDesc(NULL), pFrame(NULL), iLast(-1), bCopy(false)
{
	mBool["reset-on-reset"] = true;
	mBool["frame-copy"] = false;
}

FFmpegDecoder::~FFmpegDecoder()
{
	if( pDecoderCtx )
	{
		ffmpeg_avcodec_close( pDecoderCtx );
		av_free( pDecoderCtx );
	}
	if( pFrame )
		av_free( pFrame );
}

void FFmpegDecoder::init( void )
{
	MediaProcessor::init();

	pDesc = addMedia();
	pFrame = avcodec_alloc_frame();
	bReset = mBool["reset-on-reset"];
	bCopy = mBool["frame-copy"];

	/* Always threaded */
	forcePriviligedThread( -1 );
	bSchedule = false;
}

void FFmpegDecoder::output( void )
{
	/* Only after the first frame we can load some discription values such as width and height */
	if( pDesc->width == 0 )
	{
		pDesc->width = pDecoderCtx->width;
		pDesc->height = pDecoderCtx->height;
		pDesc->bitrate = pDecoderCtx->bit_rate;
		debug( 1, "descriptor init: dimensions(%dx%d) bitrate(%d kbps)",
			pDesc->width, pDesc->height, pDesc->bitrate/1000 );
	}
	/* Determine the size */
	if( iSize < 0 )
	{
		iSize = avpicture_get_size( pDecoderCtx->pix_fmt, pDecoderCtx->width, pDecoderCtx->height );
		debug( 2, "size of each yuv frame: %d", iSize );
	}
	/* Construct a packet */
	MediaPacketPtr pPckt( new MediaPacket( iSize ) );

	/* Copy the data */
	int iRet = avpicture_layout( (AVPicture*)pFrame, pDecoderCtx->pix_fmt, pDecoderCtx->width,
			pDecoderCtx->height, pPckt->data(), iSize );
	if( iRet != iSize )
	{
		SirannonWarning( this, "Failed to convert YUV frame(%d): %d != %d", iFrame, iRet, iSize );
		return;
	}
	 pPckt->push_back( iSize );

	/* Meta data */
	pPckt->type = packet_t::media;
	pPckt->content = content_t::video;
	pPckt->dts = iFrame * pDesc->inc;
	pPckt->pts = iFrame * pDesc->inc;
	pPckt->inc = pDesc->inc;
	pPckt->framenumber = iFrame++;
	pPckt->unitnumber = iUnit++;
	pPckt->codec = codec_t::yuv;
	pPckt->xstream = iStreamID;
	pPckt->xroute = pDesc->route;
	pPckt->desc = pDesc;

	/* Send */
	const SirannonTime oDiff = SirannonTime::getCurrentTime() - oStart;
	debug( 1, "decoded: %s fps(%.1f)", pPckt->c_str(), iDecoded / oDiff.convertDouble() );
	route( pPckt );
}

void FFmpegDecoder::outputControl( packet_t::type iType )
{
	/* Construct a packet */
	MediaPacketPtr pPckt( new MediaPacket( iType, content_t::video, 0 ) );

	/* Meta data */
	pPckt->dts = iFrame * pDesc->inc;
	pPckt->pts = 0;
	pPckt->inc = pDesc->inc;
	pPckt->framenumber = iFrame;
	pPckt->unitnumber = iUnit++;
	pPckt->codec = codec_t::yuv;
	pPckt->xstream = iStreamID;
	pPckt->xroute = pDesc->route;
	pPckt->desc = pDesc;

	/* Send */
	const SirannonTime oDiff = SirannonTime::getCurrentTime() - oStart;
	debug( 2, "decoded: %s fps(%.1f)", pPckt->c_str(), iDecoded / oDiff.convertDouble() );
	route( pPckt );
}

void FFmpegDecoder::receive( MediaPacketPtr& pPckt )
{
	/* Transform to MP4 format when it is AVC */
	if( pPckt->codec & codec_t::H264 and pPckt->mux != mux_t::MOV )
	{
		pPckt = oMP4.convertMP4( pPckt );
		if( not pPckt.get() )
			return;
	}
	/* Init */
	if( not pDecoderCtx )
		decoder_init( pPckt.get() );

	/* How many frames did we skip */
	if( bCopy and iLast >= 0 and pPckt->framenumber > iLast + 1 )
	{
		int iLost = pPckt->framenumber - iLast - 1;

		/* Insert iLost copies of the last frame */
		debug( 1, "frames lost, copying last frame (%d times)", iLost );
		for( int i = 0; i < iLost; ++i )
			output();
	}
	iLast = pPckt->framenumber;

	/* Decode */
	int iFinished = 0;
	AVPacket oFFmpegPckt;
	av_init_packet( &oFFmpegPckt );
	oFFmpegPckt.data = pPckt->data();
	oFFmpegPckt.size = pPckt->size();
	int iBytes = avcodec_decode_video2( pDecoderCtx, pFrame, &iFinished, &oFFmpegPckt );
	iFrames++;

	/* Succesfull decode? */
	if( iBytes == (int)pPckt->size() )
	{
		if( iFinished )
		{
			iDecoded++;
			output();
		}
	}
	else
	{
		SirannonWarning( this, "Could not decode frame(%d)", iDecoded+1 );
		output();
	}
}

void FFmpegDecoder::receive_reset( MediaPacketPtr& pPckt )
{
	/* Manage the decoder delay */
	if( pDecoderCtx )
	{
		/* Call the decoder "iDecoded - iEncoded" times more at the end */
		debug( 2, "frames: rx %d, decoded %d, output %d, diff %d", iFrames, iDecoded, iFrame, iFrames - iDecoded );

		int iShift1 = iFrames - iDecoded;
		for( int i = 0; i < iShift1; i++ )
		{
			/* Packet to feed the decoder */
			int iFinished = 0;
			AVPacket oFFmpegPckt;
			av_init_packet( &oFFmpegPckt );
			oFFmpegPckt.data = NULL;
			oFFmpegPckt.size = 0;
			int iBytes = avcodec_decode_video2( pDecoderCtx, pFrame, &iFinished, &oFFmpegPckt );
			iDecoded++;
			output();
		}
		/* Only sent a reset if the encoder was active */
		outputControl( pPckt->type );
	}
	/* Reset the decoder */
	decoder_reset();
}

void FFmpegDecoder::receive_end( MediaPacketPtr& pPckt )
{
	receive_reset( pPckt );
}

void FFmpegDecoder::decoder_init( MediaPacket* pPckt )
{
	/* Ready the codec context */
	pDecoderCtx = avcodec_alloc_context();
	pDecoderCtx->codec_type = AVMEDIA_TYPE_VIDEO;
	pDecoderCtx->codec_id = SirannonCodecToFFCodec( pPckt->codec );
	pDecoderCtx->time_base.num = 1;
	pDecoderCtx->time_base.den = 90000;
	iFrame = pPckt->framenumber;

	/* Descriptor */
	pDesc->codec = codec_t::yuv;
	pDesc->content = content_t::video;
	pDesc->framerate = pPckt->desc->framerate;
	pDesc->inc = pPckt->desc->inc;
	pDesc->route = pPckt->desc->route;
	pDesc->track = pPckt->desc->track;

	/* Performance */
	oStart = SirannonTime::getCurrentTime();

	/* Extra data */
	if( not( pPckt->codec & codec_t::H264 ) or pPckt->mux == mux_t::MOV )
	{
		pDecoderCtx->extradata = (uint8_t*) pPckt->desc->getExtraData();
		pDecoderCtx->extradata_size = pPckt->desc->getExtraSize();
	}
	/* Set up the decoder */
	pDecoder = ffmpeg_avcodec_find_decoder( pDecoderCtx->codec_id );
	if( not pDecoder )
		RuntimeError( this, "Decoder not found for codec(%s)", CodecToString(pPckt->codec) );

	/* Open codec */
	if( ffmpeg_avcodec_open( pDecoderCtx, pDecoder ) < 0 )
	   RuntimeError( this, "Could not setup decoder" );

	/* Timing info */
	debug( 1, "decoder init: codec(%s->yuv) inc(%d)",
			CodecToString(pPckt->codec), pDesc->inc );
}

void FFmpegDecoder::decoder_reset( void )
{
	/* Run init again to reset the decoder partially */
	iFrames = 0;
	iDecoded = 0;

	/* Free */
	if( pDecoderCtx )
	{
		debug( 1, "decoder reset" );
		ffmpeg_avcodec_close( pDecoderCtx );
		av_freep( &pDecoderCtx );
	}
}
