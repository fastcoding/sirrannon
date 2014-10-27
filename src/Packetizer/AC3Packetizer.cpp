#include "AC3Packetizer.h"
#include "Frame.h"

/**
 * AC3 Packetization header
 *
 * RFC
 *                 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
 *                +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *                |    MBZ    | FT|       NF      |
 *                +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * MBZ 0
 * FT 0 1 2 3
 * NF fragments
 *
 * DRAFT
 *
 *  0 1 2 3 4 5 6 7 8
 *  +-+-+-+-+-+-+-+-+
 *  |      NDU      |
 *  +-+-+-+-+-+-+-+-+
 *
 *  0 1 2 3 4 5 6 7 8
 *  +-+-+-+-+-+-+-+-+
 *  |TYP|F|B| RDT |T|
 *  +-+-+-+-+-+-+-+-+
 *
 */

/** AC3-PACKETIZER
 * @component AC3-packetizer
 * @type packetizer
 * @param mtu, int, 1500, in bytes, the maximum size of a network packet
 * @param draft, bool, true, if true, fill out the header according to the draft version of RFC 4184; this is the version supported by live555/vlc/mplayer
 * @info Packetizes AC3 audio frames into packets suitable for RTP as defined in (draft) RFC 4184.
 **/

REGISTER_CLASS( AC3Packetizer, "AC3-packetizer" );

AC3Packetizer::AC3Packetizer( const string& sName, ProcessorManager* pScope )
	: Packetizer(sName, pScope), bDraft(true)
{
	mBool["draft"] = true;
}

AC3Packetizer::~AC3Packetizer()
{ }

void AC3Packetizer::init( void )
{
	Packetizer::init();

	bDraft = mBool["draft"];
}

void AC3Packetizer::receive( MediaPacketPtr& pPckt )
{
	/* Strict requirements */
	if( pPckt->codec != codec_t::ac3 )
		TypeError( this, "Invalid codec(%s) expected(ac3): %s", CodecToString(pPckt->codec), pPckt->c_str() );
	if( pPckt->mux != mux_t::ES )
		TypeError( this, "Invalid packetization(%s) expected(ES): %s", MuxToString(pPckt->mux), pPckt->c_str() );

	/* Destinguish between FU and Single mode
	 * No support for aggregation */
	const int iFullSize = pPckt->size();
	if( iFullSize + 2 > iMTU )
	{
		/* In each fragment there is room for "MTU - 2" bytes */
		const int iFragments = ( iFullSize - 1 ) / ( iMTU - 2 ) + 1;
		bool bFirst = true;
		const bool bFiveEights = ( iMTU - 2 ) >= ( iFullSize * 2 + iFullSize * 8 );

		while( pPckt->size() )
		{
			/* New fragment */
			MediaPacketPtr pFragment( new MediaPacket( iMTU ) );
			pFragment->set_metadata( pPckt.get() );

			/* Fill out the header */
			fillHeader( pFragment.get(), iFragments, bFirst, bFiveEights );

			/* Insert a fragment of the original data */
			const int iWrite = MIN( iMTU - 2, pPckt->size() );
			pFragment->push_back( pPckt->data(), iWrite );
			pPckt->pop_front( iWrite );

			/* Start & end */
			pFragment->framestart &= bFirst;
			pFragment->frameend &= not pPckt->size();
			bFirst = false;

			/* Send */
			send( pFragment );
		}
		/* Original packet consumed */
		pPckt.reset();
	}
	else
	{
		/* Merely push in two bytes at the front */
		fillHeader( pPckt.get(), 1, true, true );

		/* Send */
		send( pPckt );
	}
}

void AC3Packetizer::fillHeader( MediaPacket* pPckt, int iFrags, bool bFirst, bool bFiveEights )
{
	/* Insert 2 bytes */
	pPckt->push_back( 2 );

	/* Fill out depending on which version */
	if( bDraft )
	{
		OBits oHeader( pPckt->data(), 2 );
		oHeader.write( 8, 1 );
		oHeader.write( 2, 0 ); // TYP=0
		oHeader.write( 1, iFrags > 1 );
		oHeader.write( 1, bFirst and bFiveEights );
		oHeader.write( 4, 0 );
	}
	else
	{
		if( iFrags > 1 )
			pPckt->data()[0] = bFirst ? ( bFiveEights ? 1 : 2 ) : 3;
		else
			pPckt->data()[0] = 0;
		pPckt->data()[1] = iFrags;
	}
}
