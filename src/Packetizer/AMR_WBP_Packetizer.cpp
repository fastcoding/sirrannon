#include "AMR_WBP_Packetizer.h"
#include "Bits.h"
#include "Frame.h"

REGISTER_CLASS( AMR_WBP_Packetizer, "AMR-WBP-packetizer" );

/**
 * @component AMR-WBP-packetizer
 * @type packetizer
 * @properties private, beta
 * @param mtu, int, 1500, in bytes, the maximum size of a network packet
 * @info AMR-WB+ packets are aggregated into packets of (near) MTU size according to RFC 4352
 */

AMR_WBP_Packetizer::AMR_WBP_Packetizer( const string& sName, ProcessorManager* pScope )
	: Packetizer(sName, pScope), iPack(1), iInternalSampleRate(-1)
{ }

AMR_WBP_Packetizer::~AMR_WBP_Packetizer()
{
	for( int i = 0; i < vBuffer.size(); ++i )
		delete vBuffer[i];
}

void AMR_WBP_Packetizer::receive( MediaPacketPtr& pPckt )
{
	/* Strict requirements */
	if( pPckt->codec != codec_t::awbp )
		TypeError( this, "Invalid codec(%s) expected(AMR-WB+): %s", CodecToString(pPckt->codec), pPckt->c_str() );
	if( pPckt->mux != mux_t::ES )
		TypeError( this, "Invalid packetization(%s) expected(ES): %s", MuxToString(pPckt->mux), pPckt->c_str() );

	/* Check if the ISF changes */
	int iNextInternalSampleRate = pPckt->data()[1] & 0x1F;
	if( iInternalSampleRate < 0 )
		iInternalSampleRate = iNextInternalSampleRate;
	if( iNextInternalSampleRate != iInternalSampleRate )
	{
		pack();
	}
	/* Check if the new packet would exceed the limit */
	else if( iPack + pPckt->size() > iMTU )
	{
		pack();
	}
	/* Store */
	iInternalSampleRate = iNextInternalSampleRate;
	iPack += pPckt->size();
	vBuffer.push_back( pPckt.release() );
}

void AMR_WBP_Packetizer::pack( void )
{
	/* Sanity */
	if( not vBuffer.size() )
		return;

	/* Create the new packet */
	MediaPacketPtr pPacked( new MediaPacket( iMTU ) );
	OBits oPayload( pPacked->data(), iMTU );

	/* Set meta-data */
	pPacked->set_metadata( vBuffer[0] );
	pPacked->framestart = pPacked->frameend = true;

	/* Write the common header header */
	IBits oHeader( vBuffer[0]->data() + 1, 1 );
	uint8_t iTFI = oHeader.read( 2 );
	oHeader.read( 1 );
	uint8_t iISF = oHeader.read( 5 );

	/* Why on earth is the order different than in the second byte of the Transport Header...
	 * we could have just copied it otherwise... */
	oPayload.write( 5, iISF );
	oPayload.write( 2, iTFI );
	oPayload.write( 1, 0 );

	/* Write the payload header */
	for( int i = 0; i < vBuffer.size(); ++i )
	{
		oPayload.write( 1, ((i+1) != vBuffer.size()) ); // Next ToC
		oPayload.write( 7, vBuffer[i]->data()[0] & 0x7F ); // FrameType
		oPayload.write( 8, 1 ); // 1 frame
	}
	/* Write the payload data */
	for( int i = 0; i < vBuffer.size(); ++i )
	{
		MediaPacket* pPckt = vBuffer[i];
		oPayload.write_buffer( pPckt->data() + 2, pPckt->size() - 2 );
	}
	/* Add payload to the packet */
	pPacked->push_back( oPayload.size() );

	/* Clean */
	for( int i = 0; i < vBuffer.size(); ++i )
		delete vBuffer[i];
	vBuffer.clear();

	/* Send the new packet */
	send( pPacked );
	iPack = 1; /* New size is 1 bytes */
}
