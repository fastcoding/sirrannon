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
#include "RTMPPacketizer.h"
#include "Frame.h"
#include "Interfaces.h"
#include "Flash.h"

/**
 * RTMP PACKETIZER
 * @component RTMP-packetizer
 * @type packetizer
 * @param chunk-ID, int, 5, the identifier for this RTMP chunk stream
 * @param stream-ID, int, 1, the identifier to which global stream this RTMP chunk stream belongs
 * @param chunk-size, int, 128, in bytes, the maximum size of each RTMP chunk
 * @info This packetizer produces an RTMP chunk stream from the ingested MediaPackets.
 * Such a stream can directly be sent to a Flash Media Server. Unpacketization of
 * this stream is done by the component RMTP-client internally.
 **/
REGISTER_CLASS( RTMPPacketizer, "RTMP-packetizer" );

RTMPPacketizer::RTMPPacketizer( const char* sName, ProcessorManager* pProc)
	: Packetizer(sName, pProc), oMessageHeader(2048), oChunkHeader(2048),
	  iOldTimestampDelta(0), iSize(-1), iUnit(0), bFirstChunkEver(true),
	  iStreamID(1), iChunkID(3), iChunkSize(128), iSummedTime(0), bReset(true),
	  oFlag(1)
{
	mInt["chunk-size"] = 128;
	mInt["stream-ID"] = 1;
	mInt["chunk-ID"] = 5;
}

RTMPPacketizer::~RTMPPacketizer()
{
	while( not vBuffer.empty() )
	{
		delete vBuffer.front();
		vBuffer.pop_front();
	}
}

int RTMPPacketizer::flush( void ) synchronized
{
	iSize = iOldTimestampDelta = -1;
	bReset = true;
	return 0;
} end_synchronized

uint8_t RTMPPacketizer::getFlag( const MediaPacketPtr& pPckt )
{
	/* Cached? */
	if( oFlag.size() )
		return *oFlag.data();

	if( pPckt->content == content_t::video )
	{
		/* Ignore key field, switches each packet */
		oFlag.write( 4, 0 );

		/* Write codec */
		int iCodec = SirannonToFlash( pPckt->codec );
		if( not iCodec )
			ValueError( this, "Unsupported video codec(%s)", CodecToString(pPckt->codec) );
		oFlag.write( 4, iCodec );
	}
	else if( pPckt->content == content_t::audio )
	{
		/* Write codec */
		int iCodec = SirannonToFlash( pPckt->codec );
		if( not iCodec )
			ValueError( this, "Unsupported audio codec(%s)", CodecToString(pPckt->codec) );
		oFlag.write( 4, iCodec );

		/* Sample rate field */
		switch( pPckt->desc->samplerate )
		{
		case 48000:
		case 44100:
			oFlag.write( 2, 3 );
			break;
		case 22050:
			oFlag.write( 2, 2 );
			break;
		case 11025:
			oFlag.write( 2, 1 );
			break;
		case 5512:
		case 8000:
			oFlag.write( 2, 0 );
			break;

		default:
			ValueError( this, "Unsupported sample rate(%d)", pPckt->desc->samplerate );
		}
		/* Sound size, ignore */
		oFlag.write( 1, 1 );

		/* Mono or stereo */
		oFlag.write( 1, pPckt->desc->channels > 1 );
	}
	else
		ValueError( this, "Invalid mixed content: %s", pPckt->c_str() );

	return *oFlag.data();
}

void RTMPPacketizer::init( void )
{
	MediaProcessor::init();

	iChunkSize = mInt["chunk-size"];
	iStreamID = mInt["stream-ID"];
	iChunkID = mInt["chunk-ID"];
	if( iChunkID > 63 )
		RuntimeError( this, "ChunkID too large: (%d) > (63)", iChunkID );
}

/**
 * Tag sequence
 * MetaData, Video config, Audio config, remaining audio and video
 *
 * Packet prefixes:
 * 17 00 00 00 00 = Video extra data (first video packet)
 * 17 01 00 00 00 = Video keyframe
 * 27 01 00 00 00 = Video interframe
 * af 00 ...   06 = Audio extra data (first audio packet)
 * af 01          = Audio frame
 *
 * Audio extra data(s):
 * af 00                = Prefix
 * 11 90 4f 14          = AAC Main   = aottype 0
 * 12 10                = AAC LC     = aottype 1
 * 13 90 56 e5 a5 48 00 = HE-AAC SBR = aottype 2
 * 06                   = Suffix
 * af 01 normal
 * Still not absolutely certain about this order or the bytes - need to verify later
 */
void RTMPPacketizer::receive( MediaPacketPtr& pPckt )
{
	using namespace codec_t;
	switch( pPckt->codec )
	{
	case codec_t::mp1a:
	case codec_t::mp2a:
	case codec_t::vp6:
	case codec_t::vp6f:
		pack( pPckt, 0 );
		break;

	case codec_t::mp4a:
	case codec_t::avc:
		if( pPckt->mux != mux_t::MOV )
			TypeError( this, "Frame not in MP4 format: %s", pPckt->c_str_long() );

		/* AVC and AAC require special extra data packets */
		if( not bReset )
		{
			pack( pPckt, 0 );
		}
		else
		{
			if( pPckt->content == content_t::video )
			{
				/* Create a fake SPS */
				MediaPacketPtr pExtraPckt ( new MediaPacket( packet_t::media, pPckt->content, pPckt->desc->getExtraSize() ) );
				pExtraPckt->push_back( pPckt->desc->getExtraData(), pPckt->desc->getExtraSize() );
				pExtraPckt->set_metadata( pPckt.get() );
				pExtraPckt->frame = frame_t::SPS;
				pExtraPckt->framestart = true;
				pExtraPckt->frameend = pPckt->framestart = false;

				/* Pack the global header before the frame */
				pack( pExtraPckt, 0 );
				pack( pPckt, 1 );
			}
			else if( pPckt->content == content_t::audio )
			{
				/* Create a fake audio header */
				MediaPacketPtr pExtraPckt ( new MediaPacket( packet_t::media, pPckt->content, pPckt->desc->getExtraSize() ) );
				pExtraPckt->push_back( pPckt->desc->getExtraData(), pPckt->desc->getExtraSize() );
				pExtraPckt->set_metadata( pPckt.get() );
				pExtraPckt->frame = frame_t::AUDH;
				pExtraPckt->framestart = true;
				pExtraPckt->frameend = pPckt->framestart = false;

				/* Pack the global header before the frame */
				pack( pExtraPckt, 0 );
				pack( pPckt, 1 );
			}
			else
				RuntimeError( this, "content must be either video or audio for RTMP, %s", pPckt->c_str_short() );
			bGlobalHeader = false;
		}
		break;

	default:
		TypeError( this, "Codec type(%s) is not supported for RTMP", CodecToString(pPckt->codec) );
	}
}

void RTMPPacketizer::pack( MediaPacketPtr& pPckt, uint32_t iSub )
{
	/* Types & IDs */
	int iMessageType = 9;
	if( pPckt->content == content_t::video )
		iMessageType = 9;
	else if( pPckt->content == content_t::audio )
		iMessageType = 8;
	else
		RuntimeError( this, "Content must be either video or audio for RTMP, %s", pPckt->c_str_short() );

	/* Did the packet size change? */
	bool bNewSize = false;
	if( iSize != (int) pPckt->size() )
		bNewSize = true;
	iSize = pPckt->size();

	/* Did the framerate change? */
	timestamp_t iTimestamp = pPckt->dts / 90;
	timestamp_t iTimestampDelta = pPckt->dts / 90 - iSummedTime;
	bool bNewTimestampDelta = false;
	if( iTimestampDelta != iOldTimestampDelta )
		bNewTimestampDelta = true;
	iSummedTime += iTimestampDelta;
	iOldTimestampDelta = iTimestampDelta;

	/* Codec flag */
	oMessageHeader.clear();
	uint8_t iFlag = getFlag( pPckt );
	if( pPckt->content == content_t::video )
	{
		if( pPckt->key or pPckt->frame == frame_t::SPS )
			iFlag |= 0x10;
		else
			iFlag |= 0x20;
	}
	oMessageHeader.write( 8, iFlag );

	/* Codec specific additions */
	if( pPckt->codec == codec_t::avc )
	{
		if( pPckt->frame == frame_t::SPS )
			oMessageHeader.write( 32, 0 );
		else
		{
			oMessageHeader.write( 8, 0x01 );
			oMessageHeader.write( 24, ( pPckt->pts - pPckt->dts ) / 90 );
		}
	}
	else if( pPckt->codec == codec_t::mp4a )
	{
		oMessageHeader.write( 8, pPckt->frame != frame_t::AUDH );
	}
	else if( pPckt->codec == codec_t::vp6 )
	{
		oMessageHeader.write( 8, 0 );
	}
	else if( pPckt->codec == codec_t::vp6f )
	{
		oMessageHeader.write( 8, pPckt->desc->getExtraSize() ? pPckt->desc->getExtraData()[0] : 0 );
	}
	/* Insert header */
	pPckt->push_front( oMessageHeader.data(), oMessageHeader.size() );

	/* Generate the chuncks */
	bool bFirstChunk = true;
	int iParts = 0;

	while( pPckt->size() )
	{
		/* Which fmt will we use? */
		bool bFull = false;
		if( pPckt->content == content_t::video )
			bFull = pPckt->framenumber and ( pPckt->framenumber % 8 == 0 );
		else
			bFull = pPckt->framenumber and ( pPckt->framenumber % 11 == 0 );
		if( bReset )
		{
			bFull = true;
			bReset = false;
		}
		/* Basic & Chunk stream headers */
		if( bFirstChunk and bFull )
		{
			/* Chunk Fmt 0 */
			oChunkHeader.write( 2, 0 );
			oChunkHeader.write( 6, iChunkID );
			oChunkHeader.write( 24, iTimestamp );
			oChunkHeader.write( 24, pPckt->size() );
			oChunkHeader.write( 8, iMessageType );
			oChunkHeader.write( 8, iStreamID );
			oChunkHeader.write( 24, 0 );
			iOldTimestampDelta = -1;
		}
		else if( bFirstChunk and bNewSize )
		{
			/* Chunk Fmt 1 */
			oChunkHeader.write( 2, 1 );
			oChunkHeader.write( 6, iChunkID );
			oChunkHeader.write( 24, iTimestampDelta );
			oChunkHeader.write( 24, pPckt->size() );
			oChunkHeader.write( 8, iMessageType );
		}
		else if( bFirstChunk and bNewTimestampDelta )
		{
			/* Chunk Fmt 2 */
			oChunkHeader.write( 2, 2 );
			oChunkHeader.write( 6, iChunkID );
			oChunkHeader.write( 24, iTimestampDelta );
		}
		else
		{
			/* Chunk Fmt 3 */
			oChunkHeader.write( 2, 3 );
			oChunkHeader.write( 6, iChunkID );
		}
		/* Generate chunck */
		int iPayload = MIN( iChunkSize, (int)pPckt->size() );
		MediaPacketPtr pChunk ( new MediaPacket( packet_t::media, pPckt->content, iPayload ) );
		pChunk->push_back( oChunkHeader.data(), oChunkHeader.size() );

		/* Payload */
		pChunk->push_back( pPckt->data(), iPayload );
		pPckt->pop_front( iPayload );

		/* Meta-data */
		pChunk->set_metadata( pPckt.get() );
		pChunk->framestart = bFirstChunk and pPckt->framestart;
		pChunk->frameend = not pPckt->size() and pPckt->frameend;
		pChunk->unitnumber = iUnit++;
		pChunk->subframenumber = iSub++;
		pChunk->mux = mux_t::RTMP;

		/* Debug */
		if( pScope->getVerbose() <= 2)
			debug( 1, "packed %s", pChunk->c_str() );
		else
			debug( 3, "packed %s", pChunk->c_str_full( 50 ) );

		/* Clean */
		iParts++;
		bFirstChunk = false;
		oChunkHeader.clear();

		/* Route */
		route( pChunk );
	}
	/* Tracefile */
	trace( pPckt, iParts );
}
