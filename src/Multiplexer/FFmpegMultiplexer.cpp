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
#include "FFmpegMultiplexer.h"
#include "Frame.h"
#include "h264_avc.h"
#include "OSSupport.h"

/**
 * FFMPEG MULTIPLEXER
 * @component FFMPEG-multiplexer
 * @type multiplexer
 * @param chunk-size, int, 262144, in bytes, the maximum size of each chunk
 * @param format, string, flv, format of the container given as file extension (eg. flv, webm, mov, mp4, avi)
 * @param streamed, bool, false, if true, disables seeking backwards into the generated chunks
 * @info Joins packets from different source into a container (no file!) supported by
 * FFMPEG (eg. FLV, WEBM, MP4). The container is released in a stream of MediaPackets
 * containing chunks of the container.
 **/
extern const int ff_mpeg4audio_sample_rates[16];
extern const uint8_t ff_mpeg4audio_channels[8];

REGISTER_CLASS( FFmpegMultiplexer, "ffmpeg-multiplexer" );

AVRational FFmpegMultiplexer::x_time_base = AVRational();

FFmpegMultiplexer::FFmpegMultiplexer( const string& sName, ProcessorManager* pScope )
: Multiplexer(sName, pScope), maxID(0), pCtx(NULL), pFmt(NULL), iDts(0), iBaseDts(0),
  iMux(mux_t::ES), iBuffer(0), pBuffer(NULL), iPos(0LL), iDelta(0LL),
  pCurrent(NULL), bIgnore(false), bBackwards(false), bFragmented(false)
{
	/* Defaults */
	x_time_base.num = 1;
	x_time_base.den = 90000;
	mString["format"] = "flv";
	mInt["format"] = mux_t::NO;
	mInt["codec"] = codec_t::NO;
	mInt["chunk-size"] = 256 * KIBI;
	mBool["streamed"] = false;
	mBool["fragmented"] = false;
}

FFmpegMultiplexer::~FFmpegMultiplexer( )
{
	closeContext();

	/* Clear the other structures */
	delete [] pBuffer;
	delete pCurrent;
	while( not oBuffer.empty() )
	{
		delete oBuffer.front();
		oBuffer.pop_front();
	}
}

void FFmpegMultiplexer::init( void )
{
	/* Base class */
	Multiplexer::init();

	/* Buffer size */
	iBuffer = mInt["chunk-size"];
	pBuffer = new uint8_t [iBuffer];
	bFragmented = mBool["fragmented"];

	/* Format must be set already */
	setFormat();

	/* Marker for mov container */
	if( not strcmp( pFmt->name, "mp4" ) or
		not strcmp( pFmt->name, "mov" ) or
		not strcmp( pFmt->name, "3gp" )    )
		iMux = mux_t::MOV;
	else if( not strcmp( pFmt->name, "flv" ) )
		iMux = mux_t::FLV;
	else if( not strcmp( pFmt->name, "webm" ) )
		iMux = mux_t::WEBM;
	debug( 1, "container format(%s) format(%s)", pFmt->name, MuxToString(iMux) );
}

void FFmpegMultiplexer::setFormat( void )
{
	mux_t::type iFormat = (mux_t::type) mInt["format"];

	if( iFormat != mux_t::NO )
	{
		codec_t::type iCodec = (codec_t::type) mInt["codec"];

		if( iFormat != mux_t::ES )
			pFmt = av_guess_format( MuxToString(iFormat), MuxToString(iFormat), NULL );
		else
			pFmt = av_guess_format( CodecToString(iCodec), CodecToString(iCodec), NULL );

		if( not pFmt )
			   RuntimeError( this, "Could not find suitable output format(%s/%s)", MuxToString(iFormat), CodecToString(iCodec) );
	}
	else
	{
		pFmt = av_guess_format( mString["format"].c_str(), mString["format"].c_str(), NULL );
		if( not pFmt )
			RuntimeError( this, "Could not find suitable output format(%s)", mString["format"].c_str() );
	}
}

void FFmpegMultiplexer::openContext( MediaPacket* pPckt )
{
	/* Allocate the output media cotainer */
	if( pCtx )
		RuntimeError( this, "Context already allocated" );
	pCtx = avformat_alloc_context();
	if( not pCtx )
		RuntimeError( this, "Could not allocate container" );

	/* Set format */
	pCtx->oformat = pFmt;

	/* Add streams */
	maxID = 0;
	for( map<int,muxx_t>::iterator i = mMux.begin(); i != mMux.end(); i++ )
	{
		/* Find the desc */
		const MediaDescriptor* pDesc = NULL;
		if( i->second.vQueue.size() )
			pDesc = i->second.vQueue.front()->desc;
		else
			pDesc = pPckt->desc;

		/* Add the streamer */
		if( pDesc->content == content_t::video )
			add_video_stream( maxID, pDesc );
		else if( pDesc->content == content_t::audio )
			add_audio_stream( maxID, pDesc );
		else
			RuntimeError( this, "invalid content" );

		dStreams[i->first].id = maxID++;
		dStreams[i->first].last = -9999999;
	}
	/* CAVEAT some formats don't like a num != 1 */
	if( pCtx->streams[0]->time_base.num != 1 )
	{
		int num = pCtx->streams[0]->time_base.num, den = pCtx->streams[0]->time_base.den;
		pCtx->streams[0]->time_base.den /= pCtx->streams[0]->time_base.num;
		pCtx->streams[0]->time_base.num = 1;
		SirannonWarning( this,  "changing time base %d/%d to equivalent time base %d/%d for compatibility",
				num, den, pCtx->streams[0]->time_base.num, pCtx->streams[0]->time_base.den );
	}
    /* Set the output parameters (must be done even if no parameters). */
    if( av_set_parameters( pCtx, NULL ) < 0)
    	RuntimeError( this, "invalid output parameters" );

    /* Use delta dts */
    iBaseDts = pPckt->dts;

    /* Create buffer */
    openBuffer();

	/* Write the stream header, if any */
    int iVal = av_write_header( pCtx );
    if( iVal < 0 )
    	FFmpegError( this, "Could not write container header: %s[%d]", strerror(-iVal), iVal );
}

void FFmpegMultiplexer::openBuffer( void )
{
	/* Allocate the ffmpeg buffer */
	pCtx->pb = (ByteIOContext*) av_mallocz(sizeof(ByteIOContext));
	if( init_put_byte( 	pCtx->pb, pBuffer, mInt["chunk-size"], URL_WRONLY, this,
						NULL,
						&FFmpegMultiplexer::processBuffer0,
						&FFmpegMultiplexer::seekBuffer0 ) < 0 )
		av_freep( &pCtx->pb );
	pCtx->pb->is_streamed = mBool["streamed"];
	debug( 1, "Created packet buffers" );
}

int FFmpegMultiplexer::processBuffer0( void* pVoid, uint8_t* pBuffer, int iSize )
{
	FFmpegMultiplexer* pMultiplexer = (FFmpegMultiplexer*) pVoid;
	return pMultiplexer->processBuffer( pBuffer, iSize );
}

inline int FFmpegMultiplexer::processBuffer( uint8_t* pBuffer, int iSize )
{
	/* Normal or backward mode? */
	if( bIgnore )
	{
		debug( 2, "ignoring write: size(%d)", iSize );
	}
	else if( bBackwards )
	{
		/* Sanity */
		if( not pCurrent )
			RuntimeError( this, "Backward write before first write" );

		/* Check bonds */
		if( iSize > iDelta )
			RuntimeError( this, "Writing beyond buffer limit: write(%d) > delta(%"LL"d) limit(%d)",
					iSize, iDelta, pCurrent->size() );

		/* Insert data into the packet */
		memcpy( pCurrent->data() + pCurrent->size() - iDelta, pBuffer, iSize );
		debug( 2, "write backwards: size(%d) delta(%"LL"d)", iSize, iDelta );
	}
	else
	{
		/* Send current one away */
		if( pCurrent )
		{
			MediaPacketPtr pPckt( pCurrent );
			pCurrent = NULL;
			route( pPckt );
		}
		/* Create a new packet */
		iPos += iSize;
		pCurrent = new MediaPacket( iSize );
		pCurrent->push_back( pBuffer, iSize );
		pCurrent->unitnumber = iUnit++;
		pCurrent->dts = iDts;
		debug( 2, "write: size(%d) pos(%"LL"d)", iSize, iPos );
	}
	return 0;
}

int64_t FFmpegMultiplexer::seekBuffer0( void *opaque, int64_t offset, int whence )
{
	FFmpegMultiplexer* pMultiplexer = (FFmpegMultiplexer*) opaque;
	return pMultiplexer->seekBuffer( offset, whence );
}

int64_t FFmpegMultiplexer::seekBuffer( int64_t iOffset, int whence )
{
	/* Is the seek within our limit? */
	iDelta = iPos - iOffset;

	if( whence == SEEK_SET )
	{
		if( iDelta > 0 )
		{
			/* Seeking backwards */
			bBackwards = true;
			if( not pCurrent )
			{
				RuntimeError( this, "Seek before first write" );
			}
			else if( iDelta > pCurrent->size() )
			{
				debug( 1, "seeking backward beyond buffer limit: seek(%"LL"d) < pos(%"LL"d - %u): delta(%"LL"d)",
							iOffset, iPos, pCurrent->size(), iDelta );
				bIgnore = true;
			}
			else
				debug( 1, "seek backward: delta(%"LL"d) offset(%"LL"d) current(%"LL"d)", iDelta, iOffset, iPos );
		}
		else if( iDelta == 0 )
		{
			/* Returning to normal mode */
			bBackwards = false;
			bIgnore = false;
			debug( 1, "seek restore: current(%"LL"d)", iPos );
		}
		else
		{
			/* Seeking forward */
			RuntimeError( this, "Seeking forward beyond buffer limit: seek(%"LL"d) > pos(%"LL"d)",
						iOffset, iPos, pCurrent->size() );
		}
		return iOffset;
	}
	else if( whence == SEEK_CUR )
	{
		if( iOffset != 0 )
			SirannonWarning( this, "Unexpected offset(%"LL"d)", iOffset );
		debug( 1, "ftell: %"LL"d %d", iOffset, whence );
		return iPos;
	}
	return 0;
}

void FFmpegMultiplexer::closeContext( void )
{
	/* Finish the container and close the file */
	if( pCtx )
	{
		/* Close the container */
		if( pCtx->oformat )
			av_write_trailer( pCtx );

		/* Close the buffer */
		if( pCtx->pb )
		{
			if( pCtx->pb->opaque and pCtx->pb->opaque != this )
			{
				debug( 1, "container(%s) closed", pCtx->filename );
				url_fclose( pCtx->pb );
			}
			else
			{
				debug( 1, "container-format(%s) closed", mString["format"].c_str() );
				av_freep( &pCtx->pb );
			}
		}
		/* Close the streams */
		for( int j = 0; j < pCtx->nb_streams; j++ )
		{
			av_metadata_free( &pCtx->streams[j]->metadata );
			av_free( pCtx->streams[j]->codec );
			av_free( pCtx->streams[j] );
		}
		for( int j = 0 ; j < pCtx->nb_programs; j++ )
		   av_metadata_free( &pCtx->programs[j]->metadata );

		for( int j = 0; j < pCtx->nb_chapters; j++ )
		   av_metadata_free( &pCtx->chapters[j]->metadata );

		av_metadata_free( &pCtx->metadata );

		/* Close the context */
		av_freep( &pCtx );
	}
}

void FFmpegMultiplexer::add_audio_stream( int streamID, const MediaDescriptor* desc )
{
    /* Create an additional stream */
    AVStream* pStream = av_new_stream( pCtx, streamID );
    if( pStream == NULL )
		RuntimeError( this, "couldn't allocate audiostream" );

    /* Set codec */
    AVCodecContext* c = pStream->codec;
    c->codec_id = SirannonCodecToFFCodec( desc->codec );
    c->codec_type = AVMEDIA_TYPE_AUDIO;

    /* Set parameters */
  	if( iMux == mux_t::MOV or iMux == mux_t::FLV or iMux == mux_t::WEBM )
   		c->flags |= CODEC_FLAG_GLOBAL_HEADER;
    c->bit_rate = desc->bitrate;
    c->sample_rate = desc->samplerate;
    c->channels = desc->channels;
    c->frame_size = desc->framesize;
    c->time_base.num = desc->timebase.num;
    c->time_base.den = desc->timebase.den;
    pStream->time_base.num = desc->timebase.num;
    pStream->time_base.den = desc->timebase.den;
    if( desc->getExtraSize() and c->flags & CODEC_FLAG_GLOBAL_HEADER )
	{
		/* Copy attrs */
		c->profile = desc->profile;
		c->channels = desc->channels;
		c->sample_rate = desc->samplerate;
		c->frame_size = desc->framesize;
		c->bit_rate = desc->bitrate;

		/* Set it as extra data */
		c->extradata = (uint8_t*) desc->getExtraData();
		c->extradata_size = desc->getExtraSize();
	}
    debug( 0, "added audio stream ID(%d) codec(%s/%d) channels(%d) samplereate(%d) framesize(%d) extra(%d: %s)",
    		streamID, CodecToString(desc->codec), c->codec_id, c->channels, c->sample_rate, c->frame_size, c->extradata_size, strArray(c->extradata, c->extradata_size).c_str() );
}

void  FFmpegMultiplexer::add_video_stream( int streamID, const MediaDescriptor* desc )
{
    /* Create a new stream */
    AVStream* pStream = av_new_stream( pCtx, streamID );
    if( pStream == NULL )
		RuntimeError( this, "couldn't allocate videostream" );

    /* Default settings */
    avcodec_get_context_defaults2( pStream->codec, AVMEDIA_TYPE_VIDEO );
    pStream->stream_copy = 1;

    /* Set codec */
    AVCodecContext* pCodec = pStream->codec;
    pStream->duration = desc->duration;
    pCodec->codec_id = SirannonCodecToFFCodec( desc->codec );
    pCodec->codec_type = AVMEDIA_TYPE_VIDEO;

    /* Set some parameters */
    if( iMux == mux_t::MOV or iMux == mux_t::FLV or iMux == mux_t::WEBM )
    	pCodec->flags |= CODEC_FLAG_GLOBAL_HEADER;
    pCodec->bit_rate = desc->bitrate;
    pCodec->width = desc->width;
    pCodec->height = desc->height;
    pCodec->gop_size = desc->gopsize;
    pCodec->pix_fmt = PIX_FMT_YUV420P;
    pCodec->has_b_frames = 1;
    if( desc->codec == codec_t::yuv )
    {
    	pCodec->bits_per_coded_sample = av_get_bits_per_pixel( &av_pix_fmt_descriptors[pCodec->pix_fmt] );
    	pCodec->codec_tag = avcodec_pix_fmt_to_codec_tag( pCodec->pix_fmt );
    }
    /* Calculate the frequency */
    pCodec->time_base.num = desc->timebase.num;
    pCodec->time_base.den = desc->timebase.den;
    pStream->r_frame_rate.num = 90000;
    pStream->r_frame_rate.den = desc->inc;
    pStream->time_base.num = desc->timebase.num;
    pStream->time_base.den = desc->timebase.den;

    if( desc->getExtraSize() and pCodec->flags & CODEC_FLAG_GLOBAL_HEADER )
	{
		/* Set as extra data */
		pCodec->extradata = (uint8_t*) desc->getExtraData();
		pCodec->extradata_size = desc->getExtraSize();
	}
	debug( 0, "added video stream ID(%d) codec(%s/%d/%4.4s) dimensions(%dx%d) timebase(%d/%d) extra(%d)",
			streamID, CodecToString(desc->codec), pCodec->codec_id, (char*) &pCodec->codec_tag, pCodec->width, pCodec->height,
			pCodec->time_base.num, pCodec->time_base.den, pCodec->extradata_size );
}

void FFmpegMultiplexer::mux( MediaPacketPtr& pPckt )
{
	/* End/reset? */
	if( pPckt->type != packet_t::media )
	{
		debug( 2, "mux: %s", pPckt->c_str_long() );
		if( pPckt->type == packet_t::end or bFragmented )
			if( pCtx )
				closeContext();
		route( pPckt );
		return;
	}
	/* Ready the context */
	if( not pCtx )
		openContext( pPckt.get() );
	/* Make a new packet */
	AVPacket av_pckt;
	av_init_packet( &av_pckt );

	/* Stream */
	stream_t& oStream = dStreams[pPckt->xstream];
	AVStream* pStream = pCtx->streams[oStream.id];

	/* Timestamps */
	av_pckt.pts = av_rescale_q( pPckt->pts - iBaseDts, x_time_base, pStream->time_base );
	av_pckt.dts = av_rescale_q( pPckt->dts - iBaseDts, x_time_base, pStream->time_base );
	av_pckt.duration = av_rescale_q( pPckt->desc->inc, x_time_base, pStream->time_base );

	/* Ensure sane timestamps even if they are wrong, just make sure ffmpeg accepts them FIXME */
	av_pckt.dts = MAX(av_pckt.dts, oStream.last + 1);
	av_pckt.pts = MAX(av_pckt.pts, av_pckt.dts);
	oStream.last = av_pckt.dts;

	/* Set data & parameters */
	av_pckt.size = pPckt->size();
	av_pckt.data = pPckt->data();
	av_pckt.stream_index = oStream.id;

	/* Flags */
	av_pckt.flags = 0;
	if( pPckt->frame == frame_t::IDR or pPckt->content == content_t::audio )
		av_pckt.flags = AV_PKT_FLAG_KEY;

	/* Debug */
	debug( 2, "mux: %s", pPckt->c_str_long() );

	/* Write */
	if( av_write_frame( pCtx, &av_pckt ) != 0 )
		FFmpegError( this, "Could not write frame" );
	pStream->codec->frame_number++;
	iDts = pPckt->dts;
}

void FFmpegMultiplexer::receive( MediaPacketPtr& pPckt )
{
	/* Convert if needed */
	//debug( 2, "received: %s", pPckt->c_str_long() );
	if( iMux == mux_t::MOV or iMux == mux_t::FLV )
	{
		if( pPckt->codec & codec_t::H264 or pPckt->codec == codec_t::mp4a )
			if( pPckt->mux == mux_t::ES )
				pPckt = oMP4Convertor.convertMP4( pPckt );
	}
	else if( pPckt->codec & codec_t::H264 )
	{
		pPckt = mergeFrameParts( oBuffer, pPckt, false );
	}
	/* Send it to the Multiplexer receive function */
	if( pPckt.get() )
		Multiplexer::receive( pPckt );
}
