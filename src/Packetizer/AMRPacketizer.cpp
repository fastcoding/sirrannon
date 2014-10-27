#include "AMRPacketizer.h"
#include "Bits.h"
#include "Frame.h"

REGISTER_CLASS( AMRPacketizer, "AMR-packetizer" );

/** AMR-packetizer
 * @component AMR-packetizer
 * @properties beta
 * @type packetizer
 * @param mtu, int, 1500, in bytes, the maximum size of a network packet
 * @param maxptime, int, 200, in milliseconds, if >= 0, the maximum duration of an aggregate frame, if < 0, omit this requirement
 * @info AMR and AMR-WB packets are aggregated into packets of (near) MTU size according to RFC 3267
 */

static int mFrameTypeToSizeBits_NB []  = { 95, 103, 118, 134, 148, 159, 204, 244, 0 };
static int mFrameTypeToSizeBits_WB []  = { 132, 177, 253, 285, 317, 365, 397, 461, 477 };

AMRPacketizer::AMRPacketizer( const string& sName, ProcessorManager* pScope )
	: Packetizer(sName, pScope), iPack(4), iMaxDuration(18000)
{
	mInt["maxptime"] = 200;
}

AMRPacketizer::~AMRPacketizer()
{
	for( int i = 0; i < vBuffer.size(); ++i )
		delete vBuffer[i];
}

void AMRPacketizer::init( void )
{
	Packetizer::init();

	iMaxDuration = mInt["maxptime"] * 90;
}

void AMRPacketizer::receive( MediaPacketPtr& pPckt )
{
	/* Strict requirements */
	if( pPckt->codec != codec_t::anb and pPckt->codec != codec_t::awb )
		TypeError( this, "Invalid codec(%s) expected(AMR, AMR-WB): %s", CodecToString(pPckt->codec), pPckt->c_str() );
	if( pPckt->mux != mux_t::ES )
		TypeError( this, "Invalid packetization(%s) expected(ES): %s", MuxToString(pPckt->mux), pPckt->c_str() );

	/* Check if the new packet would exceed the limit */
	int iBits = 6 + encodedBits( pPckt.get() );
	if( iPack + iBits > iMTU * 8 )
		pack();
	else if( vBuffer.size() and iMaxDuration > 0 and vBuffer.back()->dts - vBuffer.front()->dts > iMaxDuration )
		pack();

	/* Store the new packet */
	iPack += iBits;
	vBuffer.push_back( pPckt.release() );
}

void AMRPacketizer::pack( void )
{
	/* Sanity */
	if( not vBuffer.size() )
		return;

	/* Create the new packet */
	MediaPacketPtr pPacked( new MediaPacket( iMTU ) );
	OBits oPayload( pPacked->data(), iMTU );

	/* Set meta-data */
	pPacked->set_metadata( vBuffer[0] );
	pPacked->mux = mux_t::RTP;
	pPacked->unitnumber = iUnit++;
	pPacked->framestart = pPacked->frameend = true;

	/* Write CMR header */
	oPayload.write( 4, 0xF );

	/* Write the payload header */
	for( int i = 0; i < vBuffer.size(); ++i )
	{
		oPayload.write( 1, ((i+1) != vBuffer.size()) ); // Next ToC
		oPayload.write( 4, (vBuffer[i]->data()[0] & 0x78) >> 3 ); // FrameType
		oPayload.write( 1, 1 ); // Error free
	}
	/* Write the payload data */
	for( int i = 0; i < vBuffer.size(); ++i )
	{
		/* Write iBits from the packet into the payload */
		MediaPacket* pPckt = vBuffer[i];
		IBits oData( pPckt->data(), pPckt->size() );
		oData.seek( 1 );

		for( int iBits = encodedBits( pPckt ); iBits > 0; iBits-=32 )
		{
			int iWrite = min( 32, iBits );
			oPayload.write( iWrite, oData.read( iWrite ) );
		}
	}
	/* Pad the data */
	oPayload.pad();

	/* Add payload to the packet */
	pPacked->push_back( oPayload.size() );

	/* Send the new packet */
	debug( 1, "packed (%s)", pPacked->c_str() );
	route( pPacked );
	iPack = 4; /* New size is 4 bits (CMR) */

	/* Clean */
	for( int i = 0; i < vBuffer.size(); ++i )
		delete vBuffer[i];
	vBuffer.clear();
}

int AMRPacketizer::encodedBits( const MediaPacket* pPckt )
{
	int iFrame = (pPckt->data()[0] & 0x78) >> 3;
	if( pPckt->codec == codec_t::anb )
		return mFrameTypeToSizeBits_NB[iFrame];
	else
		return mFrameTypeToSizeBits_WB[iFrame];
}
