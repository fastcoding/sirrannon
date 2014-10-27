#include "MP2Unpacketizer.h"
#include "Mpeg.h"
#include "Frame.h"
using namespace codec_t;

/**
 * MP2 UNPACKETIZER
 * @component MP2-unpacketizer
 * @type unpacketizer
 * @info Unpacketizes a stream fragmented with MP2 packetization for RTP (RFC 2250)
 * by for example (MP2-packetizer).
 **/

REGISTER_CLASS( MP2Unpacketizer, "MP2-unpacketizer" );

MP2Unpacketizer::MP2Unpacketizer( const string& sName, ProcessorManager* pScope )
	: Unpacketizer(sName, pScope)
{ }

MP2Unpacketizer::~MP2Unpacketizer()
{ }

void MP2Unpacketizer::receive( MediaPacketPtr& pPckt )
{
	/* Codec check */
	switch( pPckt->codec )
	{
	case mp1a:
	case mp2a:
	case mp1v:
	case mp2v:
		break;

	default:
		ValueError( this, "Invalid codec(%s)", CodecToString(pPckt->codec) );
	}
	Unpacketizer::receive( pPckt );
}

void MP2Unpacketizer::unpack( void )
{
	/* General case of multiple packets */
	if( vBuffer.size() > 1 )
	{
		/* Aggregate size */
		int iSize = 0;
		for( deque_it i = vBuffer.begin(); i != vBuffer.end(); ++i )
			iSize += (*i)->size();

		/* Create the merged packet */
		MediaPacketPtr pUnpack( new MediaPacket( iSize ) );
		pUnpack->set_metadata( vBuffer.front() );
		pUnpack->framestart = pUnpack->frameend = true;
		pUnpack->mux = mux_t::ES;
		pUnpack->unitnumber = iUnit;

		/* Add all parts */
		bool bEnd = false;
		while( vBuffer.size() and not bEnd )
		{
			MediaPacketPtr pPckt( vBuffer.front() );
			vBuffer.pop_front();

			/* Deduce some info */

			if( pUnpack->content == content_t::video )
			{
				oHeader.assign( pPckt->data(), pPckt->size() );
				oHeader.read( 20 );
				bEnd = oHeader.read( 1 );
				pUnpack->frame = MpegToSirannon( oHeader.read( 3 ) );
			}
			/* Strip the packetization header */
			pPckt->pop_front( 4 );

			/* Add fragment */
			pUnpack->push_back( pPckt->data(), pPckt->size() );
		}
		/* Finish packet */
		debug( 1, "Unpacked: %s", pUnpack->c_str() );
		route( pUnpack );
	}
	else
	{
		/* Case for a single packet */
		MediaPacketPtr pPckt( vBuffer.front() );
		vBuffer.clear();

		/* Strip the packetization header */
		pPckt->pop_front( 4 );

		/* Fields */
		pPckt->framestart = pPckt->frameend = true;
		pPckt->mux = mux_t::ES;
		pPckt->unitnumber = iUnit;

		/* Done */
		debug( 1, "Unpacked: %s", pPckt->c_str() );
		route( pPckt );
	}
	/* Check */
	if( vBuffer.size() )
		RuntimeError( this, "Corrupted unpacketizer stack (%d remaining packets)", vBuffer.size() );
}
