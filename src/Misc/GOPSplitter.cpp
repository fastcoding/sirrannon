#include "GOPSplitter.h"
#include "Frame.h"

REGISTER_CLASS( GOP_Splitter, "GOP-splitter" );

/** GOP SPLIITER
 * @component GOP-splitter
 * @type miscellaneous
 * @param split, int, 1, in how many parts to split the stream
 * @param sync, bool, false, if true, drop all frames until the first PPS/SPS/IDR packet
 * @info Cyclically classifies each GOP by increasing the original xroute, so that xroute cycles through [route,route+split[
 **/

GOP_Splitter::GOP_Splitter( const string& sName, ProcessorManager* pScope )
	: MediaProcessor(sName, pScope), iCount(0), iSplit(3), iFrame(-1)
{
	mInt["split"] = 1;
	mBool["sync"] = false;
}

void GOP_Splitter::init( void )
{
	/* Base class */
	MediaProcessor::init();

	iSplit = mInt["split"];
	bSync = mBool["sync"];
	if( iSplit < 1 )
		RuntimeError( this, "Parameter split(%d) must be larger than 0", iSplit );
}

void GOP_Splitter::split( MediaPacket* pNewPckt )
{
	/* Generate a reset packet */
	MediaPacketPtr pPckt ( new MediaPacket( 0 ) );

	/* Take information from the next packet */
	pPckt->set_metadata( pNewPckt );
	pPckt->type = packet_t::reset;
	pPckt->framestart = pPckt->frameend = true;
	pPckt->frame = frame_t::no_frame;
	pPckt->mux = mux_t::ES;
	pPckt->xroute = pNewPckt->xroute + iCount;
	pPckt->unitnumber =  mUnit[iCount]++;
	pPckt->subframenumber = 0;

	/* Send the created reset packet */
	debug( 1, "split %s", pPckt->c_str() );
	route( pPckt );

	/* Calculate the next route */
	iCount = ( iCount + 1 ) % iSplit;
}

void GOP_Splitter::receive( MediaPacketPtr& pPckt )
{
	/* Detect the start of the next GOP */
	switch( pPckt->codec )
	{
	case codec_t::avc:
	case codec_t::svc:
	case codec_t::mvc:
		switch( pPckt->frame )
		{
		case frame_t::PPS:
		case frame_t::SPS:
		case frame_t::IDR:
			bSync = false;
			if( iFrame > 0 and pPckt->framenumber != iFrame )
				split( pPckt.get() );
		default:
			/* Silently drop the frame */
			if( bSync and not pPckt->key )
			{
				iFrame = pPckt->framenumber;
				return;
			}
			break;
		}
		break;

	case codec_t::mp1v:
	case codec_t::mp2v:
	case codec_t::mp4v:
		if( pPckt->frame == frame_t::I )
			if( iFrame > 0 and pPckt->framenumber != iFrame )
				split( pPckt.get() );
		break;

	default:
		RuntimeError( this, "Codec(%s) not supported", CodecToString(pPckt->codec) );
	}
	iFrame = pPckt->framenumber;

	/* Change route */
	pPckt->xroute += iCount;
	pPckt->unitnumber = mUnit[iCount]++;
	debug( 2, "split %s", pPckt->c_str() );
	route( pPckt );
}

void GOP_Splitter::receive_reset( MediaPacketPtr& pPckt )
{
	/* Change route and send */
	int iRoute = pPckt->xroute;
	pPckt->xroute += iCount;
	pPckt->unitnumber =  mUnit[iCount]++;

	/* Take a copy to emit a global reset */
	MediaPacketPtr pReset ( new MediaPacket( *pPckt ) );

	/* Send the local reset */
	route( pPckt );

	/* Send a global reset */
	for( int i = 0; i < iSplit; i++ )
	{
		/* Take another copy */
		MediaPacketPtr pCpy;
		if( i < iSplit - 1 )
			pCpy = MediaPacketPtr( new MediaPacket( *pReset ) );
		else
			pCpy = pReset;

		pCpy->xroute = iRoute + i;
		pCpy->unitnumber = mUnit[i]++;

		/* Send */
		debug( 1, "split %s", pCpy->c_str() );
		route( pCpy );
	}
	/* Log this GOP and stream end */
	iFrame = -1;
	iCount++;
	iCount = iCount % iSplit;
}

void GOP_Splitter::receive_end( MediaPacketPtr& pPckt )
{
	/* Change route and send */
	int iRoute = pPckt->xroute;
	pPckt->xroute += iCount;
	pPckt->unitnumber =  mUnit[iCount]++;

	/* Take a copy to emit a global reset */
	MediaPacketPtr pReset( new MediaPacket( *pPckt ) );

	/* Send a local reset first */
	pPckt->type = packet_t::reset;
	debug( 1, "split %s", pPckt->c_str() );
	route( pPckt );

	/* Send a global end */
	for( int i = 0; i < iSplit; i++ )
	{
		/* Take another copy */
		MediaPacketPtr pCpy;
		if( i < iSplit - 1 )
			pCpy = MediaPacketPtr( new MediaPacket( *pReset ) );
		else
			pCpy = pReset;

		/* Correct route and unitnumber */
		pCpy->xroute = iRoute + i;
		pCpy->unitnumber = mUnit[i]++;

		/* Send */
		debug( 1, "split %s", pCpy->c_str() );
		route( pCpy );
	}
	/* Log this GOP and stream end */
	iFrame = -1;
	iCount++;
	iCount = iCount % iSplit;
}

REGISTER_CLASS( TimeSplitter, "time-splitter" );

/** TIME SPLITTER
 * @component time-splitter
 * @type miscellaneous
 * @param split, int, 1, in how many parts to split the stream
 * @param interval, int, 10000, in ms, the duration of one part
 * @param key, bool, false, if true, split only if the frame is a keyframe, if false, omit this condition
 * @info Cyclically classifies each time interval by increasing the original xroute, so that xroute cycles through [route,route+split[
 **/

TimeSplitter::TimeSplitter( const string& sName, ProcessorManager* pScope )
	: GOP_Splitter(sName, pScope), iDts(-1), bKey(false)
{
	mInt["interval"] = 10000;
	mBool["key"] = false;
}

void TimeSplitter::init( void )
{
	GOP_Splitter::init();

	iInterval = mInt["interval"] * 90;
	bKey = mBool["key"];
}

void TimeSplitter::receive( MediaPacketPtr& pPckt )
{
	/* Init */
	if( iDts < 0 )
		iDts = pPckt->dts;

	/* Check interval */
	if(		pPckt->dts - iDts > iInterval and
			( not bKey or ( pPckt->key and pPckt->content == content_t::video ) ) )
	{
		iDts += iInterval;
		split( pPckt.get() );
	}
	/* Change route */
	pPckt->xroute += iCount;
	pPckt->unitnumber = mUnit[iCount]++;
	debug( 2, "split %s", pPckt->c_str() );
	route( pPckt );
}
