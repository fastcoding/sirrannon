/*
 * Restamp.cpp
 *
 *  Created on: Dec 21, 2011
 *      Author: arombaut
 */

#include "Restamp.h"

REGISTER_CLASS( Restamp, "restamp" );

/**
 * @component restamp
 * @type miscellaneous
 * @param delay, int, 2, the maximal reordering delay in packets
 * @info This components buffers as many packets as the reordering delay and release them we correctly generated DTSs inferred from PTS only streams.
 * This component is only useful when the DTS is corrupted or undefined. CAVEAT: Two packets with the same PTS will cause a RuntuimeError.
 */

Restamp::Restamp( const string& sName, ProcessorManager* pScope )
	: MediaProcessor(sName, pScope), iDelay(2)
{
	mInt["delay"] = 2;
}

Restamp::~Restamp()
{
	while( vPackets.size() )
	{
		delete vPackets.front();
		vPackets.pop();
	}
}

void Restamp::init( void )
{
	MediaProcessor::init();

	iDelay = mInt["delay"];
}

void Restamp::receive( MediaPacketPtr& pPckt )
{
	/* Store */
	timestamp_t iPTS = pPckt->pts;
	vPackets.push( pPckt.release() );
	vTime.insert( iPTS );

	/* Release */
	if( vTime.size() >= iDelay )
		extract_lowest();
}

void Restamp::extract_lowest( void )
{
	/* Extract DTS as the lowest PTS */
	timestamp_t iDts = *vTime.begin();
	vTime.erase( vTime.begin() );

	/* Send all packets with the same DTS */
	int iFrame = vPackets.front()->framenumber;
	while( vPackets.size() and vPackets.front()->framenumber == iFrame )
	{
		MediaPacketPtr pPckt( vPackets.front() );
		vPackets.pop();

		pPckt->dts = iDts;
		route( pPckt );
	}
}

void Restamp::receive_reset( MediaPacketPtr& pPckt )
{
	/* Release the remaining packets */
	while( vTime.size() )
		extract_lowest();
	route( pPckt );
}

void Restamp::receive_end( MediaPacketPtr& pPckt )
{
	receive_reset( pPckt );
}
