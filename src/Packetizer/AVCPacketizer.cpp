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
/*
      FRAGMENTED UNIT
	  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      | FU indicator  |   FU header   |                               |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               |
      |                                                               |
      |                         FU payload                            |
      |                                                               |
      |                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                               :...OPTIONAL RTP padding        |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

	  FU indicator
	  +---------------+
      |0|1|2|3|4|5|6|7|
      +-+-+-+-+-+-+-+-+
      |F|NRI|  Type   |
      +---------------+

	  FU header
	  +---------------+
      |0|1|2|3|4|5|6|7|
      +-+-+-+-+-+-+-+-+
      |S|E|R|  Type   |
      +---------------+

	  SINGLE UNIT
       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |F|NRI|  type   |                                               |
      +-+-+-+-+-+-+-+-+                                               |
      |                                                               |
      |               Bytes 2..n of a Single NAL unit                 |
      |                                                               |
      |                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                               :...OPTIONAL RTP padding        |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

 */
#include "AVCPacketizer.h"
#include "Frame.h"
#include "h264_avc.h"

/**
 * AVC PACKETIZER
 * @component AVC-packetizer
 * @type packetizer
 * @param iMTU, int, 1500, in bytes, the maximum size of a network packet
 * @param aggregate, bool, false, if true, aggregate small packets into one network packet
 * @info Packetizes H264 frames into packets suitable for the network as defined in RFC 3984.
 **/

static const uint8_t type_mask	= 0x1F;
static const uint8_t FLAG_BEGIN		= 0x80;
static const uint8_t iEnd		= 0x40;
static const uint8_t fu			= 0x1C;
static const uint8_t stapa		= 0x18;

/* MediaProcessorFactory */
REGISTER_CLASSES( AVCPacketizer, "AVC-packetizer", 1 );
REGISTER_CLASSES( AVCPacketizer, "SVC-packetizer", 2 );
REGISTER_CLASSES( AVCPacketizer, "MVC-packetizer", 3 );
REGISTER_CLASSES( AVCPacketizer, "H264-packetizer", 4 );

AVCPacketizer::AVCPacketizer( const string& sName, ProcessorManager* pScope )
	: Packetizer(sName, pScope), len(0), bAggregate(false)
{
	mBool["aggregate"] = false;
}

AVCPacketizer::~AVCPacketizer()
{
	for( deque_it i = stack.begin(); i != stack.end(); i++ )
		delete *i;
}

void AVCPacketizer::init( void )
{
	/* Base class */
	Packetizer::init();
	bAggregate = mBool["aggregate"];
}

void AVCPacketizer::receive( MediaPacketPtr& pPckt )
{
	/* Strict requirements */
	if( not ( pPckt->codec & codec_t::H264 ) )
		TypeError( this, "Invalid codec(%s): %s", CodecToString(pPckt->codec), pPckt->c_str() );

	if( pPckt->mux != mux_t::ES )
		TypeError( this, "Invalid packetization mode(%s) expected(ES): %s", MuxToString(pPckt->mux), pPckt->c_str() );

	/* Remove start code */
	pPckt->pop_front( H264_is_start_code( pPckt->data() ) );
	uint8_t* data = pPckt->data();

	/* How will we hande the packet? */
	if( not bAggregate )
	{
		pack_FU( pPckt );
	}
	else
	{
		uint32_t size = pPckt->size();
		if( size > iMTU )
		{
			/* Send the current stack away */
			pack();

			/* Send this packet using fragmnetation */
			pack_FU( pPckt );
		}
		else
		{
			/* size of a STAP packet */
			if(  len + size + 1 + ( stack.size() + 1 ) * 2 <= (uint32_t)iMTU and
		       ( not stack.size() or stack.back()->framenumber == pPckt->framenumber ) )
			{
				/* Add to stack */
				push( pPckt );
			}
			else
			{
				/* Send the current frame away */
				pack();

				/* Store this frame or the rare case it is just too small */
				if( size + 3 <= iMTU )
				{
					/* Add to stack */
					push( pPckt );
				}
				else
				{
					/* Send this packet using fragmnetation */
					pack_FU( pPckt );
				}
			}
		}
	}
}

void AVCPacketizer::push( MediaPacketPtr& pPckt )
{
	/* Stack grows larger */
	len += pPckt->size();

	/* Add to stack */
	bool bFrameEnd = pPckt->frameend;
	stack.push_back( pPckt.release() );

	/* Send the packet if the frame ends */
	if( bFrameEnd )
		pack();
}

void AVCPacketizer::pack( void )
{
	if( stack.size() == 1 )
	{
		MediaPacketPtr pPckt( stack.front() );
		pack_FU( pPckt );
		stack.pop_front();
	}
	else if( stack.size() > 1 )
	{
		pack_STAP();
	}
	else
	{
		return;
	}
	/* Clear */
	len = 0;
	if( stack.size() )
		RuntimeError( this, "stack not empty" );
}

void AVCPacketizer::pack_STAP( void )
{
	/* Sanity check */
	if( not len > 0 )
		RuntimeError( this, "assert ( len > 0 )" );
	if( not stack.size() )
		RuntimeError( this, "stack empty" );

	/* Create a STAP-A packet */
	int iSize = 1 /*header*/ + stack.size() * 2 /*header per packet*/ + len /*payload*/;
	MediaPacketPtr stap_pckt ( new MediaPacket( packet_t::media, content_t::video, iSize ) );
	stap_pckt->set_metadata( stack.front() );
	stap_pckt->mux = mux_t::RTP;

	/* Add the STAP-A header */
	const uint8_t nri = ( *stack.front()->data() & ~type_mask ) & 0x7F;
	stap_pckt->push1_back( stapa | nri );

	while( stack.size() )
	{
		/* Get a packet */
		MediaPacketPtr pPckt ( stack.front() );
		stack.pop_front();

		/* Write to trace */
		trace( pPckt, -1 );

		/* Push size & data */
		const uint16_t size = pPckt->size();
		stap_pckt->push1_back( ( size >> 8 ) & 0xFF );
		stap_pckt->push1_back( size & 0xFF );
		stap_pckt->push_back( pPckt->data(), pPckt->size() );

		/* Some flags */
		stap_pckt->frameend |= pPckt->frameend;
		stap_pckt->framestart |= pPckt->framestart;
	}
	/* Send packet */
	stap_pckt->unitnumber = iUnit++;
	debug( 1, "packed %s", stap_pckt->c_str() );
	route( stap_pckt );
}

int AVCPacketizer::pack_FU( MediaPacketPtr& pPckt )
{
	/* Generate packets with size smaller then the iMTU units */
	uint32_t size = pPckt->size();
	uint8_t* data = pPckt->data();

	/* Fragment the NAL unit or not? */
	if( size > iMTU )
	{
		/* Split the NAL unit into iMTU sized packets */
		const uint8_t type = data[0] &  type_mask;
		const uint8_t	nri	 = data[0] & ~type_mask;
		const uint8_t header0 = nri | fu;
		uint8_t header1 = FLAG_BEGIN | type;

		/* Discard first header byte when fragmenting */
		data++;
		size--;

		/* How many packets will we make? */
		int iNumPackets = size / (iMTU - 2) + 1;
		int iPacket = 0;

		/* Cycle over the chunks */
		while( size > 0 )
		{
			/* Determine write size */
			uint32_t write_size;
			if( size <= iMTU - 2 )
			{
				header1 = iEnd | type;
				write_size = size;
			}
			else if( size == pPckt->size() - 1 )
			{
				header1 = FLAG_BEGIN | type;
				write_size = iMTU - 2;
			}
			else
			{
				write_size = iMTU - 2;
			}

			/*  Make a packet */
			MediaPacketPtr new_pckt ( new MediaPacket( packet_t::media, content_t::video, write_size + 2 ) );

			/* Copy data */
			new_pckt->push_back( &header0, 1 );
			new_pckt->push_back( &header1, 1 );
			new_pckt->push_back( data, write_size );

			/* Copy the meta_data */
			new_pckt->set_metadata( pPckt.get() );
			new_pckt->framestart = ( header1 & FLAG_BEGIN ? pPckt->framestart : false );
			new_pckt->frameend = ( header1 & iEnd ? pPckt->frameend   : false );
			new_pckt->unitnumber = iUnit++;
			new_pckt->mux = mux_t::RTP;

			/* Shift the dts a bit */
			//new_pckt->dts += new_pckt->inc * iPacket / iNumPackets;
			//new_pckt->pts += new_pckt->inc * iPacket / iNumPackets;

			/* Route */
			debug( 1, "packed %s", new_pckt->c_str() );
			route( new_pckt );

			/* Indices */
			iPacket++;
			header1  = type;
			data += write_size;
			size -= write_size;
		}
		/* Trace */
		trace( pPckt, iPacket );

		/* Number of packets */
		return iPacket;
	}
	else
	{
		/* Trace */
		trace( pPckt, 1 );

		/*  No need to modify the packet */
		debug( 1, "packed %s", pPckt->c_str() );
		pPckt->mux = mux_t::RTP;
		pPckt->unitnumber = iUnit++;
		route( pPckt );

		/* Number of packets */
		return 1;
	}
}
