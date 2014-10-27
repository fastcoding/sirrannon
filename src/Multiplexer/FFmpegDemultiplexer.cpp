#include "FFmpegDemultiplexer.h"
#include "ffmpeg.h"
#include "Frame.h"
#include "OSSupport.h"

/**
 * FFMPEG DEMULTIPLEXER
 * @component FFMPEG-demultiplexer
 * @type demultiplexer
 * @param format, string, flv, container format of the input
 * @param chunk-size, int, 32768, amount of bytes fed to the FFMPEG IOContext each time
 * @param loop, int, 1, the number of times to play the video, -1 being infinite, 0 interpreted as 1
 * @param videoroute, int, 100, the xroute that will be assigned to packets containing video
 * @param audioroute, int, 200, the xroute that will be assigned to packets containing audio
 * @param dts-start, int, 1000, in ms, the timestamp that will be added to the DTS & PTS of each frame
 * @param video-mode, bool, true, if true, video will be read from the container, if false video will be ignored
 * @param audio-mode, bool, true, if true, audio will be read from the container, if false audio will be ignored
 * @param seek, int, -1, in ms, the timestamp to jump to into the stream, -1 implying no seek
 * @param dts-start, int, 1000, in ms, specifies the value of the timestamp of the first frame
 * @param add-parameter-sets, bool, true, H.264/AVC only, if true, extract the parameter sets from the container and insert them into the stream
 * @param repeat-parameter-sets, bool, false, H.264/AVC only, if true, repeat the parameter sets before each IDR frame
 * @param mov-frame, bool, false, MOV/MP4/F4V container only, if true, keep frames in the format of the container, as opposed to annex-B H.264/AVC streams with start codes before each NAL unit
 * @param skip-SEI, bool, false, if true, remove SEI NALUs from the stream
 * @param skip-AUD, bool, false, if true, remove AUD NALUs from the stream
 * @info This component is identical to the component FFMPEG-reader, with the exception
 * that the source is not a file but a stream of chunks from a container.
 **/

REGISTER_CLASS( FFmpegDemultiplexer, "ffmpeg-demultiplexer" );

FFmpegDemultiplexer::FFmpegDemultiplexer( const string& sName, ProcessorManager* pScope )
	: FFmpegReader(sName, pScope), bReady(false), pFFMPEGBuffer(NULL)
{
	mString["format"] = "flv";
	mInt["format"] = mux_t::NO;
	mInt["codec"] = codec_t::NO;
	mInt["chunk-size"] = 32*KIBI;
}

FFmpegDemultiplexer::~FFmpegDemultiplexer()
{
	/* Literal copy of closeBuffer */
	av_freep( &pFFMPEGBuffer );
	if( pFormatCtx )
	{
		av_close_input_stream( pFormatCtx );
		pFormatCtx = NULL;
	}
	/* Clean the buffers */
	while( vBuffer.size() )
	{
		delete vBuffer.front();
		vBuffer.pop();
	}
}

bool FFmpegDemultiplexer::ready( void ) const
{
	return bReady;
}

void FFmpegDemultiplexer::init( void )
{
	/* Base class */
	MediaProcessor::init();

	/* Buffer scheduling */
	findSchedulers();

	/* iStartDts */
	bAddParameterSets = mBool[ "add-parameter-sets"];
	bRepeatParameterSets = mBool[ "repeat-parameter-sets"];
	iVideoRoute = mInt["video-route"];
	iAudioRoute = mInt["audio-route"];
	bMovFrameStructure = mBool["mov-frame"];

	/* Create the process thread */
	createThread( bind( &FFmpegDemultiplexer::process, this ) );

	/* Make sure we dont schedule, because the other thread is calling that */
	bSchedule = false;
}

void FFmpegDemultiplexer::process( void )
{
	/* Open the buffer */
	openBuffer();
	openStream();

	/* Now we are ready */
	bReady = true;

	/* Do these steps in this seperate blocking thread */
	while( pFormatCtx )
	{
		doStep();
		this_thread::interruption_point();
	}
}

void FFmpegDemultiplexer::openBuffer( void )
{
	/* Allocate the ffmpeg buffer */
	pFFMPEGBuffer = avio_alloc_context( pBuffer, MIN(sizeof(pBuffer),mInt["chunk-size"]),
						0, this,
						&FFmpegDemultiplexer::readPacket0,
						NULL,
						&FFmpegDemultiplexer::seekPacket0 );
	if( not pFFMPEGBuffer )
		FFmpegError( this, "Could not create byte buffer" );
	pFFMPEGBuffer->seekable = 0;

	/* Assign the demux format */
	AVInputFormat* pFmt = NULL;
	if( mInt["format"] != mux_t::NO )
	{
		container_t oFormat(  (mux_t::type)mInt["format"], (codec_t::type)mInt["codec"] );
		pFmt = SirannonFormatToFFFormat( oFormat );
		if( not pFmt )
			RuntimeError( this, "Could not find suitable output format(%s/%s)", MuxToString(oFormat.first), CodecToString(oFormat.second)  );
	}
	else
	{
		pFmt = av_find_input_format( mString["format"].c_str() );
		if( not pFmt )
			RuntimeError( this, "Could not find suitable output format(%s)", mString["format"].c_str()  );
	}
	/* Open the context */
	if( av_open_input_stream( &pFormatCtx, pFFMPEGBuffer, "dummy", pFmt, NULL ) < 0 )
		RuntimeError( this, "Could not open buffer with format(%s)", mString["format"].c_str() );
	debug( 1, "opened container format(%s)", pFmt->name );
}

void FFmpegDemultiplexer::closeBuffer( void )
{
	debug( 1, "closed container" );
	av_freep( &pFFMPEGBuffer );
	if( pFormatCtx )
	{
		av_close_input_stream( pFormatCtx );
		pFormatCtx = NULL;
	}
}

int FFmpegDemultiplexer::readPacket0( void* pSelf, uint8_t* pBuffer, int iSize )
{
	FFmpegDemultiplexer* pThis = (FFmpegDemultiplexer*) pSelf;
	return pThis->readPacket( pBuffer, iSize );
}

int FFmpegDemultiplexer::readPacket( uint8_t* pBuffer, int iSize )
{
	debug( 4, "read packet size(%d)", iSize );

	LockUnique_t oLock( oBufferMutex );
	int iWrite = 0;
	while( iWrite < iSize )
	{
		/* While the buffer is empty we must block */
		while( vBuffer.empty() )
			oBufferCondition.wait( oLock );

		/* Check packet */
		MediaPacket* pPckt = vBuffer.front();
		packet_t::type iType = pPckt->type;
		if( iType != packet_t::media )
		{
			vBuffer.pop();
			delete pPckt;

			/* Stop or continue? */
			if( iType == packet_t::end )
				return 0;
			continue;
		}
		/* Copy data from the packet buffer to the ffmpeg buffer */
		int iPayload = MIN( pPckt->size(), iSize - iWrite );
		if( pBuffer )
			memcpy( pBuffer + iWrite, pPckt->data(), iPayload );
		pPckt->pop_front( iPayload );
		iWrite += iPayload;
		debug( 4, "copying packet size(%d) write(%d)", iPayload, iWrite );

		/* Media packet depleted? */
		if( not pPckt->size() )
		{
			vBuffer.pop();
			delete pPckt;
		}
	}
	return iWrite;
}

int64_t FFmpegDemultiplexer::seekPacket0( void* pSelf, int64_t iOffset, int iMode )
{
	FFmpegDemultiplexer* pThis = (FFmpegDemultiplexer*) pSelf;
	return pThis->seekPacket( iOffset, iMode );
}

int64_t FFmpegDemultiplexer::seekPacket( int64_t iOffset, int iMode )
{
	debug( 3, "seek(%"LL"d) mode(%d)", iOffset, iMode );
	return 0;
}

void FFmpegDemultiplexer::receive( MediaPacketPtr& pPckt )
{
	/* Simply add to the queue */
	LockUnique_t oLock( oBufferMutex );
	if( vBuffer.empty() )
		oBufferCondition.notify_one();
	vBuffer.push( pPckt.release() );
}

void FFmpegDemultiplexer::receive_end( MediaPacketPtr& pPckt )
{
	receive( pPckt );
}

void FFmpegDemultiplexer::receive_reset( MediaPacketPtr& pPckt )
{
	receive( pPckt );
}
