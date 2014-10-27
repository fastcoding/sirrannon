#include "LiveReader.h"
#include "Reader/Reader.h"

/**
 * @component live-reader
 * @type miscellaneous
 * @properties buffered
 * @param url, string, , if defined, create a route from the component out which uses this url
 * @info This component finds a component a runtime and creates a route from that component to this.
 * It also buffers the received packets untill the component is scheduled.
 * Using this technique a live captured stream can be tapped into and routed to this component.
 **/
REGISTER_CLASS( LiveReader, "live-reader" );

LiveReader::LiveReader( const string& sName, ProcessorManager* pScope )
: InLink(sName, pScope), pSource(NULL), bSync(false), iBaseDts(-1), iStartDts(0), bMp4(false)
{
	mInt["start-dts"] = 0;
	mBool["mov-frame"] = false;
}

LiveReader::~LiveReader()
{
	/* Delete the link as soon as possible */
	clearLink();

	/* Reset the scheduler list in the reader */
	if( pSource )
		pSource->findSchedulers();

	/* Now we can safely delete since no more packets can arrive */
	while( not vQueue.empty() )
	{
		delete vQueue.front();
		vQueue.pop();
	}
	/* Free dStreams */
	for( map<int,vector_t>::iterator i = dStreams.begin(); i != dStreams.end(); ++i )
	{
		vector_t& oVector = i->second;
		for( int j = 0; j < oVector.size(); ++j )
			delete oVector[j];
	}
}

void LiveReader::init( void )
{
	InLink::init();

	bMp4 = mBool["mov-frame"];
	iStartDts = mInt["start-dts"];
}

bool LiveReader::ready( void ) const
{
	return bSync;
}

void LiveReader::findSchedulers( void ) synchronized
{
	if( not pSource )
		return;
	pSource->findSchedulers();
} end_synchronized

void LiveReader::link( SourceInterface* _pSource )
{
	/* Copy descriptor */
	pSource = _pSource;
	oContainer = *pSource->getDescriptor();
	oContainer.duration = 0;
}

void LiveReader::receive( MediaPacketPtr& pPckt )
{
	/* Add to queue and release when scheduled */
	if( not pSource )
		link( pPckt->desc->source );
	else if( pSource != pPckt->desc->source )
		ValueError( this, "Descriptor is not the same for each packet" );

	/* Store */
	if( not bSync )
	{
		/* Keep track of synced streams */
		if( pPckt->key and pPckt->framestart )
			dSync.insert( pPckt->xstream );

		/* Before we are synced store all packets in a large multiplex */
		debug( 1, "deferring %s", pPckt->c_str_long() );
		vector_t& oVector = dStreams[pPckt->xstream];
		oVector.push_back( pPckt.release() );

		/* When need as many synced streams as streams in the container whose frames we receive */
		if( dSync.size() == getDescriptor()->size() )
		{
			/* All streams are synced now, only keep the latest(!) key frame from each stream */
			bSync = true;
			for( map<int,vector_t>::iterator i = dStreams.begin(); i != dStreams.end(); ++i )
			{
				vector_t& oVector = i->second;

				/* Find the last key frame in the queue */
				int j;
				for( j = oVector.size() - 1; j >= 0; --j )
				{
					MediaPacket* pSample = oVector[j];
					if( pSample->key and pSample->framestart )
						break;
				}
				/* Delete the unsynced frames in front */
				for( int k = 0; k < j; ++k )
				{
					debug( 1, "rejected (%s)", oVector[k]->c_str_long() );
					delete oVector[k];
					oVector[k] = NULL;
				}
				/* Determine base DTS */
				if( iBaseDts < 0 )
					iBaseDts = oVector[j]->dts;

				/* Route the rest from that point */
				for( int k = j; k < oVector.size(); ++k )
				{
					MediaPacketPtr pElem( oVector[k] );
					enqueue( pElem );
					oVector[k] = NULL;
				}
			}
			dStreams.clear();
		}
	}
	else
	{
		enqueue( pPckt );
	}
}

void LiveReader::enqueue( MediaPacketPtr& pPckt )
{
	/* Reform */
	switch( pPckt->codec )
	{
	case codec_t::avc:
	case codec_t::svc:
	case codec_t::mvc:
	case codec_t::mp4a:
		if( bMp4 and pPckt->mux == mux_t::ES )
		{
			MediaPacketPtr pMerged( oMP4.convertMP4( pPckt ) );
			if( pMerged.get() )
				vQueue.push( pMerged.release() );
		}
		else if( not bMp4 and pPckt->mux == mux_t::MOV )
			oMP4.convertES( pPckt, vQueue, true );
		else
			vQueue.push( pPckt.release() );
		break;

	default:
		vQueue.push( pPckt.release() );
	}
}

void LiveReader::send( MediaPacketPtr& pPckt )
{
	/* Relative timing info */
	if( pPckt->dts > iBaseDts )
		pPckt->dts = pPckt->dts - iBaseDts + iStartDts;
	else
		pPckt->dts = iStartDts;

	if( pPckt->pts > iBaseDts )
		pPckt->pts = pPckt->pts - iBaseDts + iStartDts;
	else
		pPckt->pts = iStartDts;

	/* Simple send */
	debug( 1, "accepted (%s)", pPckt->c_str_long() );
	route( pPckt );
}

void LiveReader::process( void )
{
	/* We must be synced */
	if( not bSync )
		return;

	/* Send all packets in buffer */
	while( not vQueue.empty() )
	{
		MediaPacketPtr pPckt( vQueue.front() );
		vQueue.pop();
		send( pPckt );
	}
}

int LiveReader::play( double fSpeed ) synchronized
{
	bSchedule = true;
	return 0;
} end_synchronized

int LiveReader::pause( void ) synchronized
{
	bSchedule = false;
	return 0;
} end_synchronized
