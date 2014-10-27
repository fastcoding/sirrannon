#include "FrameTransformer.h"
using namespace mux_t;

/**
 * TRANSFORMER
 * @component transformer
 * @properties abstract
 * @type core
 * @info These components transform the received frames for example by transcoding or changing
 * the header format
 **/

/**
 * FRAME TRANSFORMER
 * @component frame-transformer
 * @type transformer
 * @param ES, bool, true, if true, convert the packets to ES format, if false, convert to MP4 format
 * @info Handles the mess caused by the MP4 container that strips and merges MP4A and H.264 frames,
 * while TSs and RTP keep the frames in the original format. Reconstructs the header based on the meta data.
 **/
REGISTER_CLASS( FrameTransformer, "frame-transformer" );

FrameTransformer::FrameTransformer( const string& sName, ProcessorManager* pScope )
	: MediaProcessor(sName, pScope), bToES(true)
{
	mBool["ES"] = true;
}

FrameTransformer::~FrameTransformer()
{
	while( not oBuffer.empty() )
	{
		delete oBuffer.front();
		oBuffer.pop();
	}
}

void FrameTransformer::init( void )
{
	MediaProcessor::init();

	bToES = mBool["ES"];
}

void FrameTransformer::receive( MediaPacketPtr& pPckt )
{
	/* Convert to ES */
	if( bToES )
	{
		/* No work needed if already ES */
		if( pPckt->mux == ES )
		{
			route( pPckt );
		}
		/* Use the convertor function */
		else if( pPckt->mux == MOV )
		{
			oConvertor.convertES( pPckt, oBuffer, true );

			/* Send back all generated packets */
			while( not oBuffer.empty() )
			{
				MediaPacketPtr pNew( oBuffer.front() );
				oBuffer.pop();
				route( pNew );
			}
		}
		/* Sanity */
		else
			TypeError( "Expected packetization(MOV) received(%s)", MuxToString(pPckt->mux) );
	}
	/* Convert to MOV */
	else
	{
		/* No work needed if already MOV */
		if( pPckt->mux == MOV )
		{
			route( pPckt );
		}
		/* Use the convertor function */
		else if( pPckt->mux == ES )
		{
			MediaPacketPtr pMerged( oConvertor.convertMP4( pPckt ) );
			if( pMerged.get() )
				route( pMerged );
		}
		/* Sanity */
		else
			TypeError( "Expected packetization(ES) received(%s)", MuxToString(pPckt->mux) );
	}
}
