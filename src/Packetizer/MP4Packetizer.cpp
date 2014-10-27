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
#include "MP4Packetizer.h"
#include "Frame.h"

/**
 * MP4 PACKETIZER
 * @component MP4-packetizer
 * @type packetizer
 * @param mtu, int, 1500, in bytes, the maximum size of a network packet
 * @param aggregate, bool, false, if true, aggregate small packets into one network packet
 * @info Packetizes MPEG4 audio and video frames into packets suitable for the network as defined in RFC 3640.
 **/
REGISTER_CLASSES( MP4Packetizer, "MP4-packetizer", 2 );

MP4Packetizer::MP4Packetizer( const string& sName, ProcessorManager* pScope )
	: Packetizer(sName, pScope), bAggregate(false),
	 iLen(0), iMax(3), oHeader(1500)
{
	mBool["aggregate"] = false;
}

MP4Packetizer::~MP4Packetizer()
{
	for( deque_it i = vStack.begin(); i != vStack.end(); i++ )
		delete *i;
}

void MP4Packetizer::init( void )
{
	/* Base class */
	Packetizer::init();
	bAggregate = mBool["aggregate"];
}

void MP4Packetizer::receive( MediaPacketPtr& pPckt )
{
	/* Strict requirements */
	if( not( pPckt->codec & codec_t::MPEG4 ) )
		TypeError( this, "Invalid codec(%s) expected(mp4a, mp4v): %s", CodecToString(pPckt->codec), pPckt->c_str() );
	if( pPckt->mux != mux_t::ES )
		TypeError( this, "Invalid packetization(%s) expected(ES): %s", MuxToString(pPckt->mux), pPckt->c_str() );

	/* Prepare header */
	if( pPckt->codec == codec_t::mp4a )
	{
		/* Strip header in ES format */
		pPckt->pop_front( 7 );
	}
	pPckt->mux = mux_t::RTP;

	/* How will we hande the packet? */
	if( not bAggregate and pPckt->codec == codec_t::mp4a )
	{
		pack_FU( pPckt );
	}
	else
	{
		int iSize = pPckt->size();
		if( iSize + 4 > iMTU )
		{
			/* Send the current stack away */
			pack();

			/* Send this packet using fragmnetation */
			pack_FU( pPckt );
		}
		else
		{
			/* Would the aggregate be larger than the mtu? */
			if(  2 + ( (int)vStack.size() + 1 ) * 2 + iLen + iSize > iMTU )
			{
				/* Send the current stack away */
				pack();

				/* Add to stack */
				push( pPckt );
			}
			else
			{
				/* Add to stack */
				push( pPckt );
			}
		}
	}
}

void MP4Packetizer::push( MediaPacketPtr& pPckt )
{
	/* Stack grows larger */
	iLen += pPckt->size();

	/* Add to stack */
	vStack.push_back( pPckt.release() );

	/* Maximum */
	if( (int32_t)vStack.size() >= iMax )
		pack();
}

void MP4Packetizer::pack( void )
{
	/* How the handle the current stack? */
	if( vStack.size() == 1 )
	{
		MediaPacketPtr pPckt( vStack.front() );
		pack_FU( pPckt );
		vStack.pop_front();
	}
	else if( vStack.size() > 1 )
	{
		pack_STAP();
	}
	else
	{
		return;
	}
	/* Clear */
	iLen = 0;
	if( vStack.size() )
		RuntimeError( this, "stack not empty" );
}

void MP4Packetizer::pack_STAP( void )
{
	/* Number of packets */
	int iCount = vStack.size();
	if( iCount < 2 )
		RuntimeError( this, "cannot aggregate 1 or less packets" );

	/* Aggregated packet */
	MediaPacketPtr pStap ( new MediaPacket( packet_t::media, content_t::audio, iMTU ) );
	pStap->set_metadata( vStack.front() );

	/* Construct headers, only for mp4a */
	if( pStap->codec == codec_t::mp4a )
	{
		/* Construct RFC headers */
		oHeader.clear();
		oHeader.write( 16, iCount*16 );
		for( deque_it i = vStack.begin(); i != vStack.end(); i++ )
		{
			oHeader.write( 13, (*i)->size() );
			oHeader.write( 3, 0 );
		}
		/* Write header */
		pStap->push_back( oHeader.data(), oHeader.size() );
	}
	/* Append each packet */
	while( not vStack.empty() )
	{
		MediaPacketPtr pPckt ( vStack.front() );
		vStack.pop_front();

		trace( pPckt, -1 );

		/* Flags */
		pStap->content = pPckt->content;
		pStap->framestart |= pPckt->framestart;
		pStap->frameend |= pPckt->frameend;

		/* Append data */
		pStap->push_back( pPckt->data(), pPckt->size() );
	}
	pStap->unitnumber = iUnit++;

	/* Done */
	debug( 1, "aggregated (%d): %s", iCount, pStap->c_str() );
	route( pStap );
}

void MP4Packetizer::pack_FU( MediaPacketPtr& pPckt )
{
	/* Original size and fragments */
	const int iSize = pPckt->size();
	int iFrag = 0;

	/* Continue popping, will generate error if the unwinding is wrong */
	while( pPckt->size() )
	{
		/* Size of this fragment */
		int iLocalSize = MIN( iMTU - 4, (int)pPckt->size() );

		/* Create the appropriate packet */
		MediaPacketPtr pFrag ( new MediaPacket( packet_t::media, content_t::audio, iLocalSize + 4 ) );
		pFrag->set_metadata( pPckt.get() );

		/* Only mp4a uses the RFC header, mp4v is raw */
		if( pPckt->codec == codec_t::mp4a )
		{
			/* Header is different for each fragment */
			oHeader.clear();
			oHeader.write( 16, 0x0010 );
			oHeader.write( 13, iSize );
			oHeader.write( 3, iUnit );

			pFrag->push_back( oHeader.data(), oHeader.size() );
		}
		pFrag->push_back( pPckt->data(), iLocalSize );
		pPckt->pop_front( iLocalSize );

		/* Numbers & flags */
		pFrag->content = pPckt->content;
		pFrag->framestart = ( iFrag == 0 ? pPckt->framestart : false );
		pFrag->frameend   = ( pPckt->size() == 0 ? pPckt->frameend : false );
		pFrag->unitnumber = iUnit++;
		iFrag++;

		/* Route */
		debug( 1, "packed: %s", pFrag->c_str() );
		route( pFrag );
	}
	/* Trace */
	trace( pPckt, iFrag );
}
