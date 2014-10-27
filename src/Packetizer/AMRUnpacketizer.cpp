#include "AMRUnpacketizer.h"
#include "Frame.h"
#include "Bits.h"

REGISTER_CLASS( AMRUnpacketizer, "AMR-unpacketizer" );

/**
 * @component AMR-unpacketizer
 * @type unpacketizer
 * @properties beta
 * @info Unpacketizes an AMR-NB/-WB stream packed according to RFC 3267.
 */

AMRUnpacketizer::AMRUnpacketizer( const string& sName, ProcessorManager* pScope )
	: Unpacketizer(sName, pScope)
{ }

AMRUnpacketizer::~AMRUnpacketizer()
{ }

static int mFrameTypeToSizeBits_NB []  = { 95, 103, 118, 134, 148, 159, 204, 244, 0 };
static int mFrameTypeToSizeBits_WB []  = { 132, 177, 253, 285, 317, 365, 397, 461, 477 };

void AMRUnpacketizer::unpack( void )
{
	/* Sanity check */
	if( vBuffer.size() == 0 )
		return;
	else if( vBuffer.size() > 1 )
		RuntimeError( "Fragmented AMR should not occur" );
	MediaPacket* pPckt = vBuffer.front();

	/* Codec must be set */
	if( pPckt->codec != codec_t::anb and pPckt->codec != codec_t::awb )
		ValueError( "Invalid codec(%s) expected(AMR-NB,AMR-WB): %s", CodecToString(pPckt->codec), pPckt->c_str() );

	/* Packed in two parts: TOC and Data */
	IBits oHeader( pPckt->data(), pPckt->size() );
	IBits oPayload( pPckt->data(), pPckt->size() );

	/* Position the payload bit parser at the data start */
	oPayload.read( 4 );
	while( oPayload.read( 1 ) )
		oPayload.read( 5 );
	oPayload.read( 5 );

	/* Position the header bit parser at the first TOC */
	oHeader.read( 4 );
	bool bCont;
	uint16_t iFrame = 0;
	do
	{
		/* Parse the TOC entry */
		bCont = oHeader.read( 1 );
		uint8_t iFrameType = oHeader.read( 5 ) >> 1;
		uint16_t iFrameSize = (pPckt->codec == codec_t::anb) ?
				mFrameTypeToSizeBits_NB[iFrameType] : mFrameTypeToSizeBits_WB[iFrameType];

		/* A new fragment */
		MediaPacketPtr pUnpack( new MediaPacket( iFrameSize / 8 + 2 ) );
		pUnpack->set_metadata( pPckt );
		OBits oFrame( pUnpack->data(), iFrameSize / 8 + 2 );

		/* Write the header */
		oFrame.write( 1, 0 );
		oFrame.write( 4, iFrameType );
		oFrame.write( 3, 4 );

		/* Write the payload */
		while( iFrameSize > 0 )
		{
			uint8_t iWrite = MIN( iFrameSize, 32 );
			oFrame.write( iWrite, oPayload.read( iWrite ) );
			iFrameSize -= iWrite;
		}
		/* Pad untill byte alinged */
		oFrame.pad( 1, 0 );
		pUnpack->push_back( oFrame.size() );

		/* Meta data */
		pUnpack->mux = mux_t::ES;
		pUnpack->dts += pUnpack->inc * iFrame;
		pUnpack->pts = pUnpack->dts;
		pUnpack->unitnumber = pUnpack->framenumber = iUnit++;

		/* Done */
		debug( 1, "unpacked(%s)", pUnpack->c_str() );
		route( pUnpack );

	} while( bCont );

	/* Done */
	drop();
}
