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
#include "AVCUnpacketizer.h"
#include "Frame.h"

/**
 * AVC UNPACKETIZER
 * @component AVC-unpacketizer
 * @type unpacketizer
 * @param startcodes, bool, true, if true, add startcodes to the unpacked NALs
 * @param strict-annex-b, bool, true, if false, all startcodes will be 4 bytes
 * @info The fragmented or aggregated frames are transformed again into their original
 * NAL units, as defined in RFC 3984. It performs the reverse operation of the
 * AVC-packetizer.
 **/

static const uint8_t FLAG_FU	= 0x1C;
static const uint8_t FLAG_BEGIN	= 0x80;
static const uint8_t FLAG_END	= 0x40;
static const uint8_t MASK_TYPE  = 0x1F;
static const uint8_t FLAG_STAPA	= 0x18;
static const uint8_t START_CODE_LONG [] = { 0, 0, 0, 1 };
static const uint8_t START_CODE_SHORT [] = { 0, 0, 1 };

REGISTER_CLASS( AVCUnpacketizer, "AVC-unpacketizer" );

AVCUnpacketizer::AVCUnpacketizer( const string& sName, ProcessorManager* pScope )
	: Unpacketizer(sName, pScope), bConformAnnexB(false), bStartcodes(true)
{
	mBool["startcodes"] = true;
	mBool["strict-annex-b"] = true;
}

AVCUnpacketizer::~AVCUnpacketizer( void )
{ }

void AVCUnpacketizer::init( void )
{
	/* Base class */
	Unpacketizer::init();

	bStartcodes = mBool["startcodes"];
	bConformAnnexB = mBool["strict-annex-b"];
}

void AVCUnpacketizer::startcodes( MediaPacketPtr& pPckt )
{
	/* Default use the long startcode */
	bool bLong = true;

	/* More eloborate check to determine when to use short startcodes */
	if( bConformAnnexB )
	{
		/* Parse to find out the slice start */
		H264_parse_NAL( pPckt->data(), pPckt->size(), &oNAL );

		/* Long startcode for SPS, PPS and first slice of the frame */
		bLong = false;
		switch( oNAL.nal_unit_type )
		{
		case NAL_UNIT_TYPE_SPS:
		case NAL_UNIT_TYPE_PPS:
			bLong = true;
			break;

		case NAL_UNIT_TYPE_SLICE_SVC:
		case NAL_UNIT_TYPE_SLICE_NON_IDR:
		case NAL_UNIT_TYPE_SLICE_DP_A:
		case NAL_UNIT_TYPE_SLICE_IDR:
			if( oNAL.first_mb_in_slice == 0 )
				bLong = true;
			break;
		}
	}
	/* Parameter sets type for streams who dont know their types */
	using namespace frame_t;
	if( pPckt->frame == no_frame )
	{
		uint8_t iType = pPckt->data()[0];
		switch( iType & MASK_TYPE )
		{
		case NAL_UNIT_TYPE_PPS:
			pPckt->frame = PPS;
			break;
		case NAL_UNIT_TYPE_SPS:
			pPckt->frame = SPS;
			break;
		case NAL_UNIT_TYPE_SLICE_IDR:
			pPckt->frame = IDR;
			break;
		default:
			pPckt->frame = P;
		}
	}
	/* Add startcodes */
	if( pPckt->codec == codec_t::NO )
		pPckt->codec = codec_t::avc;
	if( bLong )
		pPckt->push_front( START_CODE_LONG, sizeof(START_CODE_LONG) );
	else
		pPckt->push_front( START_CODE_SHORT, sizeof(START_CODE_SHORT) );
}

void AVCUnpacketizer::receive( MediaPacketPtr& pPckt )
{
	/* Base class */
	Unpacketizer::receive( pPckt );

	/* Preemptive unpack is possible in most cases save FU  */
	MediaPacket* pBack = vBuffer.back();
	bool bUnpack = true;
	if( (pBack->data()[0] & MASK_TYPE) == FLAG_FU and not(pBack->data()[1] & FLAG_END) )
		bUnpack = false;
	if( bUnpack )
		unpack();
}

void AVCUnpacketizer::unpack( void )
{
	/* Verify */
	if( vBuffer.empty() )
		return;

	/* Which type of packtization we are dealing with? */
	const uint8_t indicator = vBuffer.front()->data()[0];

	switch( indicator & MASK_TYPE )
	{
	case FLAG_FU:
		unpackFU( indicator );
		break;

	case FLAG_STAPA:
		unpackSTAPA();
		break;

	default:
		/* Add the startcodes again */
		MediaPacketPtr pckt ( vBuffer.front() );
		vBuffer.pop_front();

		/* Route */
		startcodes( pckt );
		pckt->unitnumber = iUnit++;
		pckt->mux = mux_t::ES;
		debug( 1, "unpacked %s", pckt->c_str() );
		route( pckt );
		break;
	}

	/* Sanity */
	if( vBuffer.size() )
	{
		SirannonWarning( this,  "corrupted unPacketizer stack, dropping %d packets", vBuffer.size() );
		drop();
	}
}

void AVCUnpacketizer::unpackFU( uint32_t indicator )
{
	/* Reconstruct the original header */
	uint8_t header_first = vBuffer.front()->data()[1];
	uint8_t header_last  = vBuffer.back()->data()[1];
	uint8_t new_header   = ( indicator & ~MASK_TYPE ) | ( header_first & MASK_TYPE );

	/* Is our nal-unit complete? */
	bool complete = true;
	if( !( header_first & FLAG_BEGIN ) )
	{
		debug( 2, "no start" );
		complete = false;
	}
	if( !( header_last & FLAG_END ) )
	{
		debug( 2, "no end" );
		complete = false;
	}
	if( (uint32_t)( vBuffer.back()->unitnumber - vBuffer.front()->unitnumber + 1 ) != vBuffer.size() )
	{
		debug( 2, "not all" );
		complete = false;
	}
	if( !complete )
	{
		debug( 1, "dropping frame %04d.%d, damaged beyond repair",
				vBuffer.front()->framenumber, vBuffer.front()->subframenumber );
		drop();
		iUnit++;
		return;
	}
	/* Size of the packet */
	uint32_t size = 0;
	for( deque_it i = vBuffer.begin(); i != vBuffer.end(); i++ )
		size += (*i)->size() - 2;

	/* Create a new pckt */
	MediaPacketPtr pckt ( new MediaPacket( packet_t::media, content_t::video, size ) );

	/* Push in the new header */
	pckt->push1_back( new_header );

	/* Set the stamps */
	pckt->set_metadata( vBuffer.front() );
	pckt->mux = mux_t::ES;
	pckt->unitnumber = iUnit++;

	/* Frame start and end */
	pckt->framestart = vBuffer.front()->framestart;
	pckt->frameend = vBuffer.back()->frameend;

	/* Push in the data */
	while( !vBuffer.empty() )
	{
		pckt->push_back( vBuffer.front()->data() + 2, vBuffer.front()->size() - 2 );
		delete vBuffer.front();
		vBuffer.pop_front();
	}
	/* Push in sync bytes */
	startcodes( pckt );

	/* Route */
	debug( 1, "unpacked %s", pckt->c_str() );
	route( pckt );
}

void AVCUnpacketizer::unpackSTAPA( void )
{
	/* The aggregation packet */
	MediaPacketPtr pStap ( vBuffer.front() );
	vBuffer.pop_front();
	uint32_t size  = pStap->size();
	uint8_t* data = pStap->data();
	uint8_t* pEnd  = data + size;
	int iSubFrame = pStap->subframenumber;

	/* Skip the header byte */
	data += 1;

	int iCount = 0;
	while( true )
	{
		/* Read length info */
		uint32_t next_size = (( (uint32_t) data[0] ) << 8 ) + data[1];
		data += 2;

		/* Sanity */
		if( data + next_size > pEnd )
			RuntimeError( this, "corrupted STAP-A packet" );

		/* Reconstruct the packet */
		MediaPacketPtr pckt ( new MediaPacket( packet_t::media, content_t::video, next_size + 4 ) );

		/* Add data */
		pckt->push_back( data, next_size );
		data += next_size;
		startcodes( pckt );

		/* Add metadata */
		pckt->set_metadata( pStap.get() );
		pckt->unitnumber = iUnit++;
		pckt->subframenumber = iSubFrame++;
		pckt->mux = mux_t::ES;
		pckt->framestart = ( iCount == 0 ) ? pStap->framestart : false;
		pckt->frameend = ( data == pEnd ) ? pStap->frameend : false;

		/* Route */
		debug( 1, "unpacked %s", pckt->c_str() );
		route( pckt );

		/* Done? */
		if( data == pEnd )
			break;
		iCount++;
	}
}
