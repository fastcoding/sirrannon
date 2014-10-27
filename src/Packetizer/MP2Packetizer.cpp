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
#include "MP2Packetizer.h"
#include "Frame.h"

/**
 * MP2 PACKETIZER
 * @component MP2-packetizer
 * @type packetizer
 * @param mtu, int, 1500, in bytes, the maximum size of a network packet
 * @info Packetizes MPEG1&2 audio and video frames into packets suitable for the network as defined in RFC 2250.
 **/

REGISTER_CLASSES( MP2Packetizer, "MP2-packetizer", 1 );
REGISTER_CLASSES( MP2Packetizer, "M2A-packetizer" , 2);
REGISTER_CLASSES( MP2Packetizer, "M2V-packetizer", 3 );
REGISTER_CLASSES( MP2Packetizer, "M1A-packetizer", 4 );
REGISTER_CLASSES( MP2Packetizer, "M1V-packetizer", 5 );

static const uint32_t PICTURE_START  = 0x00000100;
static const uint32_t SEQUENCE_START = 0x000001B3;
static const uint32_t GROUP_START    = 0x000001B8;

MP2Packetizer::MP2Packetizer( const string& sName, ProcessorManager* pScope )
	: Packetizer(sName, pScope), oHeader(4), iPos(-1), iType(0),
	  iTime(0)
{ }

void MP2Packetizer::receive( MediaPacketPtr& pckt )
{
	/* Check if this packet has a valid codec */
	switch( pckt->codec )
	{
	case codec_t::mp1v:
	case codec_t::mp2v:
	case codec_t::mp1a:
	case codec_t::mp2a:
		break;

	default:
		RuntimeError( this, "Invalid codec(%s) expected(mp1v, mp1a, mp2v, mp2a): %s", CodecToString(pckt->codec), pckt->c_str() );
	}
	pckt->mux = mux_t::RTP;

	/* Do we have to fragment the frame? */
	int iOffset = 0;
	if( pckt->size() > (uint32_t)iMTU - 4 )
	{
		/* Trace */
		int iNumPackets = pckt->size() / (iMTU - 4) + 1;
		trace( pckt, iNumPackets );

		/* Fragment the frame */
		while( pckt->size() )
		{
			/* Create a fragment */
			int iPayload = MIN( pckt->size(), (uint32_t)iMTU-4 );
			MediaPacketPtr pNext ( new MediaPacket( packet_t::media, pckt->content, iPayload + 4 ) );

			/* Add the payload */
			pNext->push_back( pckt->data(), iPayload );
			pckt->pop_front( iPayload );

			/* Meta data */
			pNext->set_metadata( pckt.get() );
			pNext->unitnumber = iUnit++;
			pNext->framestart = ( iOffset == 0 );
			pNext->frameend = ( pckt->size() == 0 );

			/* Insert the header */
			if( header( pNext, iOffset ) < 0 )
				break;
			iOffset += iPayload;

			/* Done */
			route( pNext );
		}
	}
	else
	{
		pckt->unitnumber = iUnit++;
		if( header( pckt, 0 ) >= 0 )
		{
			/* Log */
			trace( pckt, 1 );

			/* Send */
			route( pckt );
		}
	}
	/* Done */
	iPos = -1;
}

int MP2Packetizer::header( MediaPacketPtr& pckt, int iOffset )
{
	/* Construct the header */
	oHeader.clear();

	if( pckt->content == content_t::video )
	{
		/* Find the picture header */
		bool bSequence = false;
		if( iPos < 0 )
		{
			/* Find the picture start code */
			iPos = find_next_start_code( pckt->data(), pckt->size(), PICTURE_START );
			if( iPos < 0 )
				return SirannonWarning( this,  "could not find picture start code" );

			/* Parse the picture header */
			IBits oParser ( pckt->data() + iPos, 6 );
			oParser.read( 32 );
			iTime = oParser.read( 10 );
			iType = oParser.read( 3 );

			/* Sequence header? */
			if( find_next_start_code( pckt->data(), iPos, SEQUENCE_START ) >= 0 )
				bSequence = true;
		}
		/* Construct the video header */
		oHeader.write( 5, 0 ); /* MBZ */
		oHeader.write( 1, 0 ); /*T: No extension */
		oHeader.write( 10, iTime ); /* Temporal Reference */
		oHeader.write( 1, 0 ); /* AN */
		oHeader.write( 1, 0 ); /* N */
		oHeader.write( 1, bSequence ); /* Sequence header */
		oHeader.write( 1, pckt->framestart ); /* Slice header */
		oHeader.write( 1, pckt->frameend ); /* End of slice */
		oHeader.write( 3, iType ); /* Picture Type */
		oHeader.write( 8, 0 );
	}
	else if( pckt->content == content_t::audio )
	{
		/* Construct the audio header */
		oHeader.write( 16, 0 ); /* MBZ */
		oHeader.write( 16, iOffset ); /* Offset */
	}
	else
		RuntimeError( this, "unexpected content in %s", pckt->c_str() );

	/* Add the header */
	pckt->push_front( oHeader.data(), oHeader.size() );
	return 0;
}
