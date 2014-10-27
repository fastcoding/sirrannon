#include "Unpacketizer.h"

/**
 * UNPACKETIZER
 * @component unpacketizer
 * @properties abstract, buffered
 * @type core
 **/

Unpacketizer::Unpacketizer( const string& sName, ProcessorManager* pScope )
	: MediaProcessor(sName, pScope), iUnit(0)
{ }

Unpacketizer::~Unpacketizer()
{
	drop();
}

void Unpacketizer::drop( void )
{
	while( vBuffer.size() )
	{
		delete vBuffer.front();
		vBuffer.pop_front();
	}
}

void Unpacketizer::receive( MediaPacketPtr& pPckt )
{
	/* New unitnumber? */
	if( !vBuffer.empty() )
		if( vBuffer.back()->subframenumber != pPckt->subframenumber or
			vBuffer.back()->framenumber != pPckt->framenumber )
			unpack();

	/* Store the packet */
	vBuffer.push_back( pPckt.release() );
}

void Unpacketizer::receive_end( MediaPacketPtr& pckt )
{
	unpack();
	pckt->unitnumber = iUnit++;
	route( pckt );
}

void Unpacketizer::receive_reset( MediaPacketPtr& pckt )
{
	unpack();
	pckt->unitnumber = iUnit++;
	route( pckt );
}
