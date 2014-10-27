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
#include "SirannonPacketizer.h"

/**
 * SIRANNON PACKETIZER
 * @component sirannon-packetizer
 * @type packetizer
 * @param mtu, int, 1500, in bytes, the maximum size of a network packet
 * @info This simple and generic packetizer can handle any content. However,
 * this packetization is internal to Sirannon and is not recognized by other players.
 * Use sirannon-unpacketizer to unpacketize this stream again.
 **/

/**
 * SIRANNON UNPACKETIZER
 * @component sirannon-unpacketizer
 * @type unpacketizer
 * @param recover-frame, bool, false, if true, unpacked parts of a damage frame instead of discarding the entire frame
 * @param error-on-loss, bool, false, if true, throw an exception when packet loss occurs
 * @info Unpacketizes a stream fragmented by the internal packetizer of Sirannon (sirannon-packetizer).
 **/

REGISTER_CLASSES( SirannonPacketizer, "xstream-packetizer", 1 );
REGISTER_CLASSES( SirannonUnpacketizer, "xstream-unpacketizer", 1 );
REGISTER_CLASSES( SirannonPacketizer, "sirannon-packetizer", 2 );
REGISTER_CLASSES( SirannonUnpacketizer, "sirannon-unpacketizer", 2 );

SirannonPacketizer::SirannonPacketizer( const string& sName, ProcessorManager* pScope )
	: Packetizer(sName, pScope)
{ }

SirannonPacketizer::~SirannonPacketizer()
{
}

void SirannonPacketizer::receive( MediaPacketPtr& pckt )
{
	/* New format */
	pckt->mux = mux_t::RTP;

	if( pckt->size() > iMTU )
	{
		/* Fragment the frame */
		int iFirst = 0, iParts = 0;
		while( pckt->size() )
		{
			/* Create a fragment */
			int iPayload = MIN( pckt->size(), iMTU );
			MediaPacketPtr pNext ( new MediaPacket( packet_t::media, pckt->content, iPayload + 4 ) );

			/* Add the payload */
			pNext->push_back( pckt->data(), iPayload );
			pckt->pop_front( iPayload );

			/* Meta data */
			iParts++;
			pNext->set_metadata( pckt.get() );
			pNext->unitnumber = iUnit++;
			pNext->framestart = pckt->framestart and not iFirst++;
			pNext->frameend = pckt->frameend and not pckt->size();

			/* Done */
			debug( 1, "packed %s", pNext->c_str() );
			route( pNext );
		}
		/* Trace */
		trace( pckt, iParts );
	}
	else
	{
		/* Trace */
		trace( pckt, 1 );

		pckt->unitnumber = iUnit++;
		debug( 1, "packed %s", pckt->c_str() );
		route( pckt );
	}
}

SirannonUnpacketizer::SirannonUnpacketizer( const string& sName, ProcessorManager* pScope )
	: MediaProcessor(sName, pScope), iUnit(-1), iFrame(-1), iSubFrame(-1),
	  iNewUnit(0), bFragment(true), bSync(false), bSeverity(false)
{
	mBool["recover-frame"] = true;
	mBool["error-on-loss"] = false;
}

SirannonUnpacketizer::~SirannonUnpacketizer()
{
	while( not vQueue.empty() )
	{
		delete vQueue.front();
		vQueue.pop_front();
	}
}

void SirannonUnpacketizer::init( void )
{
	/* Base class */
	MediaProcessor::init();

	bFragment = mBool["recover-frame"];
	bSeverity = mBool["error-on-loss"];
}

void SirannonUnpacketizer::unpack( void )
{
	int iPackets = vQueue.size();
	if( iPackets == 0 )
	{
		/* No packets */
		return;
	}
	else if( iPackets == 1 )
	{
		/* One fragment, no need to copy it */
		MediaPacketPtr pckt ( vQueue.front() );
		vQueue.clear();

		/* Hardly any action to take */
		pckt->unitnumber = iNewUnit++;
		debug( 1, "unpacked %s", pckt->c_str() );

		/* Route */
		route( pckt );
	}
	else
	{
		/* Many fragments */
		int iSize = 0;
		for( deque_it i = vQueue.begin(); i != vQueue.end(); i++ )
			iSize += (*i)->size();

		/* Create a new pckt */
		MediaPacketPtr pckt ( new MediaPacket( packet_t::media, vQueue.front()->content, iSize ) );

		/* Set the meta data */
		pckt->set_metadata( vQueue.front() );
		pckt->unitnumber = iNewUnit++;

		/* Frame start and end */
		pckt->framestart = vQueue.front()->framestart;
		pckt->frameend = vQueue.back()->frameend;

		/* Push in the data */
		while( not vQueue.empty() )
		{
			pckt->push_back( vQueue.front()->data(), vQueue.front()->size() );
			delete vQueue.front();
			vQueue.pop_front();
		}
		/* Route */
		debug( 1, "unpacked %s", pckt->c_str() );
		route( pckt );
	}
}

void SirannonUnpacketizer::receive( MediaPacketPtr& pckt )
{
	/* Initial packet */
	if( iUnit < 0 )
	{
		if( not pckt->framestart )
		{
			if( bSeverity )
				RuntimeError( this, "no synchronization: first packet (%d) is not framestart", pckt->unitnumber );
			else
				debug( 1, "no synchronization: first packet (%d) is not framestart", pckt->unitnumber );
			pckt.reset();
			return;
		}
		iFrame = pckt->framenumber;
		iSubFrame = pckt->subframenumber;
		iUnit = pckt->unitnumber - 1;
	}
	/* Standard evolution ? */
	if( pckt->unitnumber == iUnit + 1 )
	{
		/* New frame or subframe */
		if( pckt->framenumber != iFrame or pckt->subframenumber != iSubFrame )
			unpack();

		/* Remember numbers */
		iFrame = pckt->framenumber;
		iSubFrame = pckt->subframenumber;
		iUnit = pckt->unitnumber;

		/* Quick handling if there is a frameend */
		bool bFrameEnd = pckt->frameend;

		/* Save */
		vQueue.push_back( pckt.release() );
		if( bFrameEnd )
			unpack();
	}
	/* Error handling */
	else
	{
		/* Purge or save the current stack? */
		if( bFragment )
		{
			/* Send out the partial fragment we have */
			unpack();
		}
		else
		{
			/* Purge */
			while( not vQueue.empty() )
			{
				delete vQueue.front();
				vQueue.pop_front();
			}
		}
		/* Only realign if we see a frame start flag */
		if( not bSync )
		{
			if( bSeverity )
				RuntimeError( this, "synchronization lost: %d != %d + 1", pckt->unitnumber, iUnit );
			else
				SirannonWarning( this,  "synchronization lost: %d != %d + 1", pckt->unitnumber, iUnit );
		}
		bSync = true;
		if( pckt->framestart )
		{
			/* Save */
			SirannonWarning( this,  "resynchronized at %d", pckt->unitnumber );
			bSync = false;

			/* Remember numbers */
			iFrame = pckt->framenumber;
			iSubFrame = pckt->subframenumber;
			iUnit = pckt->unitnumber;

			/* Quick handling if there is a frameend */
			bool bFrameEnd = pckt->frameend;
			vQueue.push_back( pckt.release() );
			if( bFrameEnd )
				unpack();
		}
		else
		{
			/* Delete junk */
			pckt.reset();
		}
	}
}

