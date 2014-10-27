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
#include "Multiplexer.h"
#include "Frame.h"

/**
 * MULTIPLEXER
 * @component multiplexer
 * @properties abstract, buffered, scheduled
 * @type core
 * @param delay, int, 1000, in ms, the minimal amount of data present for each stream of the multiplex before the next packet is released
 * @param streams, int, -1, the number of different streams required before multiplexing, -1 omits this requirement
 * @info Multiplexers buffer MediaPackets coming from different sources and multiplex
 * those based on the DTS of each MediaPacket. Unmultiplexers perform the reverse
 * operation and restore the original streams.
 **/

/**
 * DEMULTIPLEXER
 * @component demultiplexer
 * @properties abstract, buffered, scheduled
 * @type core
 **/

/**
 * STD MULTIPLEXER
 * @component std-multiplexer
 * @type multiplexer
 * @info Joins packets from different sources by only ensuring that the DTS and other
 * timing information rise monotonely. This is a requirement for many protocols and
 * containers.
 **/

/**
 * UNIT MULTIPLEXER
 * @component unit-multiplexer
 * @type multiplexer
 * @info Joins packets from different sources by only ensuring that the unitnumber
 * rises monotonely.
 **/

REGISTER_CLASSES( Multiplexer, "std-multiplexer", 1 );
REGISTER_CLASSES( Multiplexer, "std-muxer", 2 );
REGISTER_CLASS( UnitMultiplexer, "unit-multiplexer" );

Multiplexer::Multiplexer( const string& sName, ProcessorManager* pScope )
	: MediaProcessor(sName, pScope),
	  streamID(nextStreamID()), iUnit(0), iLast(-1), iDelay(1000), iMaxPID(0x100), iStreams(-1),
	  oStart(0)
{
	mInt["delay"] = 1000;
	mInt["streams"] = -1;
	mInt["initial-delay"] = 0;
}

UnitMultiplexer::UnitMultiplexer( const string& sName, ProcessorManager* pScope )
	: Multiplexer(sName, pScope)
{
	mInt["delay"] = 1000;
	mInt["streams"] = -1;
	mInt["intial-delay"] = 0;
}

Multiplexer::~Multiplexer()
{
	for( map<int,muxx_t>::iterator q = mMux.begin(); q != mMux.end(); q++ )
	{
		while( q->second.vQueue.size() )
		{
			delete q->second.vQueue.front();
			q->second.vQueue.pop_front();
		}
	}
}

void Multiplexer::init( void )
{
	/* Base class */
	MediaProcessor::init();

	iDelay = mInt["delay"] * 90;
	iStreams = mInt["streams"];
}

void Multiplexer::receive( MediaPacketPtr& pckt )
{
	/* New ? */
	if( not mMux.count( pckt->xstream ) )
	{
		muxx_t* pMux = &mMux[pckt->xstream];
		pMux->iMarker = 0;
		pMux->iPID  = iMaxPID++;
		pMux->iCC = 0;
		pMux->codec = codec_t::NO;
		pMux->iOrigSize = -1;
	}
	/* Count the number of resets or ends on the queue */
	if( pckt->type != packet_t::media )
		mMux[pckt->xstream].iMarker++;

	/* Add it to its own content queue */
	muxx_t* pMux = &mMux[pckt->xstream];
	pMux->vQueue.push_back( pckt.release() );

	/* Activate */
	bSchedule = true;
}

void Multiplexer::receive_end( MediaPacketPtr& pckt )
{
	Multiplexer::receive( pckt );
}

void Multiplexer::receive_reset( MediaPacketPtr& pckt )
{
	Multiplexer::receive( pckt );
}

void Multiplexer::mux( MediaPacketPtr& pckt )
{
	/* Force to a common unit stream */
	pckt->unitnumber = iUnit++;
	pckt->xstream = streamID;

	/* Done */
	debug( 1, "muxed %s", pckt->c_str() );
	route( pckt );
}

void Multiplexer::process( void )
{
	/* Wait an intial delay */
	SirannonTime oCurrent = SirannonTime::getCurrentTime();
	if( oCurrent < oStart )
		return;

	bool bStop = false;
	while( checkMux() )
	{
		/* Select the lowest */
		int iStream = selectMux();

		/* Selected a normal packet or reset packet */
		MediaPacketPtr pckt;
		if( iStream < 0 )
		{
			pckt = resetMux();
		}
		else
		{
			pckt = MediaPacketPtr( mMux[iStream].vQueue.front() );
			mMux[iStream].vQueue.pop_front();
		}
		/* Mux a sufficient amount of packets */
		if( iLast < 0 )
			iLast = pckt->dts;

		/* Set it on route */
		mux( pckt );
	}
}

bool Multiplexer::checkMux( void )
{
	/* There must be at least one buffer */
	if( mMux.empty() )
		return false;

	/* Required amount of streams */
	if( iStreams > 0 )
		if( (int32_t) mMux.size() < iStreams )
			return false;

	/* Collect untill we have enough data */
	for( map<int,muxx_t>::iterator i = mMux.begin(); i != mMux.end(); i++ )
	{
		/* Data structure */
		muxx_t* pMux = &i->second;
		deque_t* pQueue = &pMux->vQueue;

		/* An empty queue means it is not ready */
		if( pQueue->empty() )
			return false;

		/* We wait for at least iMin seconds of data */
		timestamp_t iDelta = pQueue->back()->dts - pQueue->front()->dts;
		if(	iDelta < iDelay and pMux->iMarker == 0 )
			return false;
	}
	/* Queues are ready and able */
	return true;
}

int Multiplexer::selectMux( void )
{
	/* Iterate over the multiplex to find the queue with the lowest dts */
	int iStream = -1;
	timestamp_t iMin = 0x7FFFFFFF;
	for( map<int,muxx_t>::iterator i = mMux.begin(); i != mMux.end(); i++ )
	{
		/* Data structure */
		muxx_t* pMux = &i->second;
		deque_t* pQueue = &pMux->vQueue;

		/* Skip queue with a reset */
		if( pQueue->empty() )
		{
			printMux();
			RuntimeError( this, "queue should not be empty at this point!"  );
		}
		if( pQueue->front()->type != packet_t::media )
			continue;

		/* Lowest DTS? */
		if( pQueue->front()->dts < iMin )
		{
			iMin = pQueue->front()->dts;
			iStream = i->first;
		}
	}
	/* Send a pointer to the info struct */
	return iStream;
}

int UnitMultiplexer::selectMux( void )
{
	/* Iterate over the multiplex to find the queue with the lowest dts */
	int iStream = -1, iMin = 0x7FFFFFFF;
	for( map<int,muxx_t>::iterator i = mMux.begin(); i != mMux.end(); i++ )
	{
		/* Data structure */
		muxx_t* pMux = &i->second;
		deque_t* pQueue = &pMux->vQueue;

		/* Skip queue with a reset */
		if( pQueue->empty() )
		{
			printMux();
			RuntimeError( this, "queue should not be empty at this point!"  );
		}
		if( pQueue->front()->type != packet_t::media )
			continue;

		/* Lowest DTS? */
		if( pQueue->front()->unitnumber < iMin )
		{
			iMin = pQueue->front()->unitnumber;
			iStream = i->first;
		}
	}
	/* Send a pointer to the info struct */
	return iStream;
}

MediaPacketPtrRef Multiplexer::resetMux( void )
{
	/* Find the reset or end packet with the highest DTS */
	MediaPacket* pckt = NULL;
	int iMax = 0;
	packet_t::type iType = packet_t::reset;
	for( map<int,muxx_t>::iterator i = mMux.begin(); i != mMux.end(); i++ )
	{
		/* Data structure */
		muxx_t* pMux = &i->second;
		deque_t* pQueue = &pMux->vQueue;

		/* Verify */
		if( pQueue->empty() )
		{
			printMux();
			RuntimeError( this, "Multiplexer corrupted 1" );
		}
		else if( pQueue->front()->type == packet_t::media )
		{
			printMux();
			RuntimeError( this, "Multiplexer corrupted 2" );
		}
		/* Normal */
		if( pQueue->front()->dts >= iMax )
		{
			pckt = pQueue->front();
			iMax = pckt->dts;
			iType = pckt->type;
		}
	}
	/* Sanity */
	if( not pckt )
	{
		printMux();
		RuntimeError( this, "Multiplexer corrupted 3" );
	}
	/* Create a unified packet */
	MediaPacketPtr pCtrl ( new MediaPacket( 0 ) );
	pCtrl->set_metadata( pckt );
	pCtrl->type = iType;
	pCtrl->content = content_t::mixed;

	/* Delete the previous control packets */
	vector<int> vKeys;
	get_keys<int,muxx_t>( mMux, vKeys );
	for( uint32_t i = 0; i < vKeys.size(); i++ )
	{
		/* Data structure */
		muxx_t* pMux = &mMux[vKeys[i]];
		deque_t* pQueue = &pMux->vQueue;

		/* Is it a reset or the end? */
		if( pQueue->front()->type == packet_t::end )
		{
			/* Delete all packets in this queue */
			if( pQueue->size() > 1 )
				SirannonWarning( this,  "packets in queue after end-packet" );
			while( pQueue->size() )
			{
				delete pQueue->front();
				pQueue->pop_front();
			}

			/* Delete the struct */
			mMux.erase( vKeys[i] );
		}
		else
		{
			/* Delete the reset-packet */
			delete pQueue->front();
			pQueue->pop_front();
			pMux->iMarker--;
		}
	}
	return pCtrl;
}

void Multiplexer::printMux( void )
{
	for( map<int,muxx_t>::iterator i = mMux.begin(); i != mMux.end(); i++ )
	{
		if( i->second.vQueue.size() )
			print( 0, "stream(%d) codec(%s) size(%d)", i->first, CodecToString(i->second.vQueue.front()->codec),
					i->second.vQueue.size() );
		else
			print( 0, "stream(%d) codec(?) size(%d)", i->first, i->second.vQueue.size() );
	}
}

int Multiplexer::flush( void ) synchronized
{
	/* Super */
	MediaProcessor::flush();

	/* Purge the queues */
	for( map<int,muxx_t>::iterator q = mMux.begin(); q != mMux.end(); q++ )
	{
		while( q->second.vQueue.size() )
		{
			delete q->second.vQueue.front();
			q->second.vQueue.pop_front();
		}
	}
	mMux.clear();

	/* Reset */
	oStart = SirannonTime( 0 );
	iLast = -1;
	return 0;
} end_synchronized
