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
#include "PESPacketizer.h"
#include "Frame.h"
#include "h264_avc.h"

/**
 * PES PACKETIZER
 * @component PES-packetizer
 * @type packetizer
 * @param delta, int, 0, in ms, the amount of time to add to DTS/PTS to make sure is substractable by a PCR
 * @param insert-AUD, bool, false, if true, add AUDs before each H.264 frame
 * @param audio-per-PES, int, 1, number of audio frames per PES-packet
 * @param zero-length, bool, true, video only, if true, set 0 as PES-packet-length and generate a single PES-packet per frame
 * @info PES packetization is the required format for transport streams TS-multiplexer.
 * All packets belonging to one frame are aggregated into one PES-packet.
 * If the total size of the frame is larger than 65500 bytes two or more PES-packets
 * are generated unless the option zero-length is set.
 **/
REGISTER_CLASSES( PESPacketizer, "PES-packer", 1 );
REGISTER_CLASSES( PESPacketizer, "PES-packetizer", 2 );

PESPacketizer::PESPacketizer( const string& sName, ProcessorManager* pScope )
	: Packetizer(sName, pScope), iMaxSize(65500), iSize(0), oHeader(50),
	  iDelta(0), bAUD(false), iAudio(-1), bZero(false), iFirst(-1)
{
	mInt["delta"] = 0;
	mBool["insert-AUD"] = false;
	mBool["zero-length"] = true;
	mInt["audio-per-PES"] = 1;
}

PESPacketizer::~PESPacketizer()
{
	while( not oBuffer.empty() )
	{
		delete oBuffer.front();
		oBuffer.pop_front();
	}
	while( not oSend.empty() )
	{
		delete oSend.front();
		oSend.pop_front();
	}
}

void PESPacketizer::init( void )
{
	Packetizer::init();

	iDelta = mInt["delta"] * 90;
	bAUD = mBool["insert-AUD"];
	bZero =	mBool["zero-length"];
	iAudio = mInt["audio-per-PES"];

	if( bZero )
		iMaxSize = -1;
}

void PESPacketizer::receive_reset( MediaPacketPtr& pInputPckt )
{
	receive_end( pInputPckt );
}

void PESPacketizer::receive_end( MediaPacketPtr& pInputPckt )
{
	if( iDelta > 0 )
	{
		pInputPckt->dts += iDelta;
		pInputPckt->pts += iDelta;
	}
	if( iAudio > 1 and oBuffer.size() > 0 )
		receive( pInputPckt );

	debug( 1, "first(%d) last(%d) last+inc(%d) delta(%d)", iFirst, iLast, iLast+pInputPckt->inc, iLast - iFirst + pInputPckt->inc );
	route( pInputPckt );
}

const static uint8_t oAUD [] = { 0x00, 0x00, 0x00, 0x01, 0x09, 0xE0 };

void PESPacketizer::receive( MediaPacketPtr& pInputPckt )
{
	/* Requirements */
	if( pInputPckt->mux != mux_t::ES )
		TypeError( "Invalid packetization mode(%s) expected(ES): %s", MuxToString(pInputPckt->mux), pInputPckt->c_str() );

	/* Trace */
	trace( pInputPckt, -1 );

	/* Aggregate audio */
	MediaPacketPtr pMerged( NULL );
	if( iAudio > 1 and pInputPckt->content == content_t::audio )
	{
		if( pInputPckt->type != packet_t::media )
		{
			int iCount = oBuffer.size();
			debug( 1, "processing audio tail: %d packets remaining", iCount );
			MediaPacketPtr pDirty( oBuffer.back() );
			oBuffer.pop_back();
			pMerged = mergeFrameParts( oBuffer, pDirty, false );
			pMerged->inc *= iCount;
		}
		else if( oBuffer.size() < iAudio )
		{
			oBuffer.push_back( pInputPckt.release() );
		}
		else
		{
			pMerged = mergeFrameParts( oBuffer, pInputPckt, false );
			pMerged->inc *= iAudio;
		}
	}
	/* Aggregate video */
	else
	{
		pMerged = mergeFrameParts( oBuffer, pInputPckt, false );
	}
	/* Merged complete? */
	if( not pMerged.get() )
		return;

	/* Acces unit delimitter */
	if( bAUD and pMerged->codec & codec_t::H264 )
	{
		/* Detect if the packet already contains an AUD */
		const uint8_t* pData = pMerged->data();
		int iSize = pMerged->size();
		while( iSize > 0 )
		{
			int iNAL;
			switch( H264_NAL_type( pData ) )
			{
			case NAL_UNIT_TYPE_SEI:
				iNAL = H264_find_NAL( pData, iSize );
				pData += iNAL;
				iSize -= iNAL;
				continue;

			case NAL_UNIT_TYPE_ACCESS_UNIT_DELIM:
				debug( 3, "AUD already present" );
				iSize = 0;
				break;

			default:
				debug( 3, "inserted AUD" );
				pMerged->push_front( oAUD, sizeof(oAUD) );
				iSize = 0;
			}
		}
	}
	if( iDelta > 0 )
	{
		pMerged->dts += iDelta;
		pMerged->pts += iDelta;
	}
	/* Keep the existing packet if it is small enough and singular */
	if( ( bZero and pMerged->content == content_t::video ) or pMerged->size() <= iMaxSize )
	{
		createHeader( pMerged.get(), pMerged->size() );
		oSend.push_back( pMerged.release() );
	}
	/* Split into several PES */
	else
	{
		while( pMerged->size() > 0 )
		{
			/* New pes */
			MediaPacketPtr pPES ( new MediaPacket( packet_t::media, pMerged->content, iMaxSize ) );
			pPES->set_metadata( pMerged.get() );

			/* Add header */
			int iPayload = MIN( pMerged->size(), iMaxSize );
			int iHeader = createHeader( pPES.get(), iPayload );

			pPES->push_back( pMerged->data(), iPayload );
			pMerged->pop_front( iPayload );

			/* Save */
			oSend.push_back( pPES.release() );
		}
	}
	/* Output packets */
	const timestamp_t iINC = oSend.front()->inc / oSend.size();
	timestamp_t iDTS = oSend.front()->dts;
	while( not oSend.empty() )
	{
		MediaPacketPtr pPckt( oSend.front() );
		oSend.pop_front();

		pPckt->dts = iDTS;
		pPckt->unitnumber = iUnit++;
		pPckt->mux = mux_t::PES;
		iDTS += iINC;

		debug( 1, "packed %s", pPckt->c_str() );
		route( pPckt );
	}
}

size_t PESPacketizer::createHeader( MediaPacket* pPckt, size_t iPES )
{
	uint64_t iDTS = pPckt->dts; /* unsigned cast for correct right shift */
	uint64_t iPTS = pPckt->pts; /* unsigned cast for correct right shift */

	/* Write heading */
	oHeader.clear();
	oHeader.write( 24, 0x01 );
	oHeader.write( 8, GetPesID(pPckt->codec) );

	/* Write size */
	int iHeader;
	if( iDTS != iPTS and pPckt->pts >= 0 )
		iHeader = 10;
	else
		iHeader = 5;
	oHeader.write( 16, bZero ? 0 : iPES + iHeader + 3 );

	/* Info */
	uint8_t iFlag = 0x80;
	oHeader.write( 8, iFlag );

	/* Write DTS/PTS */
	if( iFirst < 0 )
		iFirst = iDTS;
	iLast = iDTS;
	if( iDTS != iPTS and pPckt->pts >= 0 )
	{
		oHeader.write( 8, 0xC0 ); /* PTS & DTS present */
		oHeader.write( 8, 0x0A ); /* 10 bytes extension */

		/* Presentation timestamp */
		oHeader.write( 8, (((iPTS << 0 ) >> 0 ) >> 30) << 1 | 0x31 );
		oHeader.write( 8, (((iPTS << 2 ) >> 2 ) >> 22) );
		oHeader.write( 8, (((iPTS << 10) >> 10) >> 15) << 1 | 0x1 );
		oHeader.write( 8, (((iPTS << 17) >> 17) >> 7 ) );
		oHeader.write( 8, (((iPTS << 25) >> 25) >> 0 ) << 1 | 0x1 );

		/* Decoding timestamp */
		oHeader.write( 8, (((iDTS << 0 ) >> 0 ) >> 30) << 1 | 0x11 );
		oHeader.write( 8, (((iDTS << 2 ) >> 2 ) >> 22) );
		oHeader.write( 8, (((iDTS << 10) >> 10) >> 15) << 1 | 0x1 );
		oHeader.write( 8, (((iDTS << 17) >> 17) >> 7 ) );
		oHeader.write( 8, (((iDTS << 25) >> 25) >> 0 ) << 1 | 0x1 );
	}
	else
	{
		oHeader.write( 8, 0x80 ); /* PTS present */
		oHeader.write( 8, 0x05 ); /* 5 bytes extension */

		/* Presentation timestamp */
		oHeader.write( 8, (((iPTS << 0 ) >> 0 ) >> 30) << 1 | 0x21 );
		oHeader.write( 8, (((iPTS << 2 ) >> 2 ) >> 22) );
		oHeader.write( 8, (((iPTS << 10) >> 10) >> 15) << 1 | 0x1 );
		oHeader.write( 8, (((iPTS << 17) >> 17) >> 7 ) );
		oHeader.write( 8, (((iPTS << 25) >> 25) >> 0 ) << 1 | 0x1 );
	}
	/* Add to packet */
	pPckt->push_front( oHeader.data(), oHeader.size() );

	return oHeader.size();
}
