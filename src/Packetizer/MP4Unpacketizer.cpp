#include "MP4Unpacketizer.h"
#include "Frame.h"

/**
 * MP4 UNPACKETIZER
 * @component MP4-unpacketizer
 * @type unpacketizer
 * @info Unpacketizes a stream fragmented with MP4 packetization for RTP (RFC 3640) by
 * for example MP4-packetizer.
 **/

REGISTER_CLASS( MP4Unpacketizer, "MP4-unpacketizer" );

MP4Unpacketizer::MP4Unpacketizer( const string& sName, ProcessorManager* pScope )
	: Unpacketizer(sName, pScope)
{ }

MP4Unpacketizer::~MP4Unpacketizer()
{ }

void MP4Unpacketizer::unpack( void )
{
	/* Strict requirements */
	MediaPacket* pPckt = vBuffer.front();
	if( pPckt->codec != codec_t::mp4a and pPckt->codec != codec_t::mp4v )
		TypeError( this, "Invalid codec(%s): %s", CodecToString(pPckt->codec), pPckt->c_str() );

	if( pPckt->mux != mux_t::RTP )
		TypeError( this, "Invalid packetization mode(%s) expected(RTP): %s", MuxToString(pPckt->mux), pPckt->c_str() );

	/* Video is packetized as an elementary stream */
	if( pPckt->codec == codec_t::mp4v )
	{
		/* Size of the packet */
		int iSize = 0;
		for( deque_it i = vBuffer.begin(); i != vBuffer.end(); ++i )
			iSize += (*i)->size();

		/* Create the joined packet */
		MediaPacketPtr pFrag( new MediaPacket( packet_t::media, content_t::video, iSize ) );
		pFrag->set_metadata( pPckt );
		pFrag->mux = mux_t::ES;
		pFrag->unitnumber = iUnit++;
		pFrag->framestart = pFrag->frameend = true;

		/* Add all fragments */
		while( vBuffer.size() )
		{
			MediaPacketPtr pPckt( vBuffer.front() );
			vBuffer.pop_front();

			/* Add data from the fragment */
			pFrag->push_back( pPckt->data(), pPckt->size() );
		}
		/* Done */
		debug( 1, "unpacked %s", pFrag->c_str_long() );
		route( pFrag );
	}
	else
	{
		/* Read the first field in the buffer containing the number of fragments in this packet */
		IBits oHeader( pPckt->data(), pPckt->size() );
		const uint32_t iHeader = oHeader.read( 16 ) / 8;
		const uint32_t iParts = iHeader / 2;

		/* STAP packet when more than one part */
		if( iParts > 1 )
		{
			IBits oPayload( pPckt->data() + 2 + iHeader, pPckt->size() - 2 - iHeader );

			/* Loop over each part */
			for( int i = 0; i < iParts; ++i )
			{
				/* Size of the fragment */
				uint32_t iFrag = oHeader.read( 16 ) / 8;

				/* Construct the new packet */
				MediaPacketPtr pFrag( new MediaPacket( packet_t::media, content_t::audio, iFrag ) );
				pFrag->set_metadata( pPckt );
				pFrag->mux = mux_t::MOV;
				pFrag->unitnumber = iUnit++;
				pFrag->framestart = pFrag->frameend = true;
				pFrag->dts += i * pFrag->inc;
				pFrag->pts = pFrag->dts;

				/* Add data */
				oPayload.read_buffer( pFrag->data(), iFrag );
				pFrag->push_back( iFrag );

				debug( 1, "unpacked %s", pFrag->c_str_long() );
				route( pFrag );
			}
			/* Processed the packet */
			vBuffer.pop_front();
			delete pPckt;
		}
		/* Fragmented Unit if only one part */
		else if( iParts == 1 )
		{
			uint32_t iSize = oHeader.read( 16 ) / 8;

			/* General FU packet */
			if( iSize > pPckt->size() - 4 )
			{
				/* Create the joined packet */
				MediaPacketPtr pFrag( new MediaPacket( packet_t::media, content_t::audio, iSize ) );
				pFrag->set_metadata( pPckt );
				pFrag->mux = mux_t::MOV;
				pFrag->unitnumber = iUnit++;
				pFrag->framestart = pFrag->frameend = true;

				/* Add all fragments */
				uint32_t iTestSize = 0;
				while( vBuffer.size() and iTestSize < iSize )
				{
					/* Sanity check */
					pPckt = vBuffer.front();
					oHeader.assign( pPckt->data(), pPckt->size() );
					if( oHeader.read( 16 ) != 16 or oHeader.read( 16 ) / 8 != iSize )
						RuntimeError( this, "STAP after FU in the same frame is not allowed" );

					/* Add data from the fragment */
					pFrag->push_back( pPckt->data() + 4, pPckt->size() - 4 );
					iTestSize += pPckt->size() - 4;

					/* Fragment is consumed */
					vBuffer.pop_front();
					delete pPckt;
				}
				/* Complete packet? */
				if( iTestSize != iSize )
				{
					SirannonWarning( this, "Dropping frame %d, missing fragments: size(%d) != size(%d)",
							pFrag->framenumber, iTestSize, iSize );
					return drop();
				}
				/* Done */
				debug( 1, "unpacked %s", pFrag->c_str_long() );
				route( pFrag );
			}
			/* More efficient version for one packet */
			else if( iSize == pPckt->size() - 4 )
			{
				/* Simply strip the 4 byte header */
				pPckt->pop_front( 4 );

				/* Meta data */
				pPckt->mux = mux_t::MOV;
				pPckt->unitnumber = iUnit++;
				pPckt->framestart = pPckt->frameend = true;

				/* Done */
				vBuffer.pop_front();
				MediaPacketPtr pFrag( pPckt );
				debug( 1, "unpacked %s", pFrag->c_str_long() );
				route( pFrag );
			}
			else
				ValueError( this, "Invalid fragement size field(%d)", iSize );
		}
		else
			ValueError( this, "Invalid header size(0)" );
	}
	/* If any packets remain there is corruption */
	if( vBuffer.size() )
		RuntimeError( this, "Packets remaining in buffer after unpacking a frame" );
}
