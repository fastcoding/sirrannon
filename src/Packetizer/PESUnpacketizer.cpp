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
#include "PESUnpacketizer.h"
#include "Bits.h"
#include "Endianness.h"

/**
 * PES UNPACKETIZER
 * @component PES-unpacketizer
 * @type unpacketizer
 * @info One or more PES-packets belonging to one frame are split again into the original
 * parts. It performs the reverse operation of the PES-packetizer.
 **/

REGISTER_CLASS( PESUnpacketizer, "PES-unpacketizer" );

PESUnpacketizer::PESUnpacketizer( const string& sName, ProcessorManager* pScope )
	: MediaProcessor(sName, pScope), iFrame(0), iUnit(0), bSync(true), iLastDTS(-1),
	iBaseDTS(90000), iFirstDTS(-1)
{
}

PESUnpacketizer::~PESUnpacketizer()
{
	while( vQueue.size() )
	{
		delete vQueue.front();
		vQueue.pop_front();
	}
}

void PESUnpacketizer::receive( MediaPacketPtr& pckt )
{
	/* Verify minimum size */
	if( pckt->size() < 9 )
		RuntimeError( this, "PES, packet too small: %d < 9", pckt->size() );
	pckt->mux = mux_t::RTP;

	/* Try to synchronize */
	uint8_t* pData = pckt->data();
	bool bStart = ( pData[6] & 0x08 ) or ( pData[7] & 0xC0 ); /* payload_start_indicator or PTS/DTS present */
	if( bSync )
	{
		if( bStart )
		{
			/* Found sync again */
			SirannonWarning( this,  "resyncing succeeded" );
			bSync = false;
		}
		else
		{
			/* Drop packet */
			return;
		}
	}
	/* Unpack the previous PES if a frame starts in this PES */
	if( bStart )
		unpack();

	/* Discontinuity check */
	if( pckt->error )
	{
		SirannonWarning( this,  "PES truncated and/or next PES('s) lost, resyncing" );
		bSync = true;
	}
	/* Add this packet */
	vQueue.push_back( pckt.release() );
}

void PESUnpacketizer::receive_end( MediaPacketPtr& pckt )
{
	unpack();
	debug( 1, "unpacked %s", pckt->c_str() );
	route( pckt );
}

void PESUnpacketizer::receive_reset( MediaPacketPtr& pckt )
{
	unpack();
	debug( 1, "unpacked %s", pckt->c_str() );
	route( pckt );
}

int PESUnpacketizer::unpack( void )
{
	/* Verify queue */
	if( not vQueue.size() )
		return -1;

	/* Packet to create */
	MediaPacketPtr pckt;

	/* Find out the total size */
	int iFrameSize = 0;
	for( deque_t::iterator i = vQueue.begin(); i != vQueue.end(); i++ )
		iFrameSize += ntoh16( *(uint16_t*) ((*i)->data() + 4) );

	/* Merge the split PES packets */
	while( vQueue.size() )
	{
		/* PES packet */
		MediaPacketPtr pPES ( vQueue.front() );
		vQueue.pop_front();

		/* Header */
		uint8_t* pData = pPES->data();
		int iSize = pPES->size();

		/* Start parsing the PES */
		IBits oStream( pData, iSize );

		/* Verify startcode */
		int iSyntax = oStream.read( 24 );
		if( iSyntax != 0x000001 )
			return SirannonWarning( this,  "PES, wrong startcode: 0x000001 != 0x%06X", iSyntax );

		/* Register stream type */
		int iStream = oStream.read( 8 );

		/* Register length */
		int iLength = oStream.read( 16 );

		/* Verify syntax */
		iSyntax = oStream.read( 2 );
		if( iSyntax != 0x2 )
			return SirannonWarning( this,  "PES, wrong syntax: 0x2 != 0x%X", iSyntax );

		/* Skip unimportant flags */
		oStream.read( 6 );

		/* Register timing info */
		int iTime = oStream.read( 2 );

		/* Skip unimportant flags */
		oStream.read( 6 );

		/* Register remaning header length */
		int iHeader = oStream.read( 8 );

		/* Register timing info */
		timestamp_t iPTS = 0, iDTS = 0;
		if( iTime == 0x2 )
		{
			/* Verify syntax */
			iSyntax = oStream.read( 4 );
			if( iSyntax != 0x2 )
				return SirannonWarning( this,  "PES, wrong syntax 2: 0x2 != 0x%X", iSyntax );

			/* Extract PTS, such a kludge! */
			iPTS += oStream.read( 3 ) << 30;
			oStream.read( 1 );
			iPTS += oStream.read( 15 ) << 15;
			oStream.read( 1 );
			iPTS += oStream.read( 15 );
			oStream.read( 1 );

			/* DTS & PTS equal */
			iDTS = iPTS;
		}
		else if( iTime == 0x3 )
		{
			/* Verify syntax */
			iSyntax = oStream.read( 4 );
			if( iSyntax != 0x3 )
				return SirannonWarning( this,  "PES, wrong syntax: 0x3 != 0x%X", iSyntax );

			/* Extract PTS, such a kludge! */
			iPTS += oStream.read( 3 ) << 30;
			oStream.read( 1 );
			iPTS += oStream.read( 15 ) << 15;
			oStream.read( 1 );
			iPTS += oStream.read( 15 );
			oStream.read( 1 );

			/* Verify syntax */
			iSyntax = oStream.read( 4 );
			if( iSyntax != 0x1 )
				return SirannonWarning( this,  "PES, wrong syntax: 0x01 != 0x%X", iSyntax );

			/* Extract DTS, such a kludge! */
			iDTS += oStream.read( 3 ) << 30;
			oStream.read( 1 );
			iDTS += oStream.read( 15 ) << 15;
			oStream.read( 1 );
			iDTS += oStream.read( 15 );
			oStream.read( 1 );
		}
		else if( not pckt.get() )
		{
			SirannonWarning( this,  "PES, required PTS/DTS missing for reconstruction, skipping" );
			continue;
		}
		/* Relative time */
		if( iFirstDTS == -1 )
			iFirstDTS = iDTS;

		/* Payload start point */
		uint8_t* pPayload = pData + 6 + 3 + iHeader;
		int iPayload1 = iSize - 9 - iHeader;
		int iPayload2 = iLength - 3 - iHeader;

		/* Create the new packet */
		if( not pckt.get() )
		{
			/* Create the new packet */
			pckt = MediaPacketPtr( new MediaPacket( packet_t::media, pPES->content, iFrameSize ) );
			pckt->set_metadata( pPES.get() );

			/* Set timing info */
			pckt->dts = iDTS - iFirstDTS + iBaseDTS;
			pckt->pts = iPTS - iFirstDTS + iBaseDTS;
			pckt->inc = iLastDTS < 0 ? 0 : pckt->dts - iLastDTS;
			pckt->unitnumber = iUnit++;
			pckt->framenumber = iFrame++;
			pckt->framestart = pckt->frameend = true;
			iLastDTS = pckt->dts;
		}

		/* Check if the packet is not truncated */
		if( iPayload1 != iPayload2 )
			SirannonWarning( this,  "PES payload is truncated from %d B to %d B", iPayload2, iPayload1 );

		/* Copy over the data */
		pckt->push_back( pPayload, iPayload1 );
	}
	/* Send the reconstructed packet */
	if( pckt.get() )
	{
		debug( 1, "unpacked %s", pckt->c_str() );
		route( pckt );
	}
	return 0;
}
