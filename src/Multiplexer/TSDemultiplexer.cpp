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
#include "TSDemultiplexer.h"
#include "Frame.h"

/**
 * TS DEMULTIPLEXER
 * @component TS-demultiplexer
 * @type demultiplexer
 * @param channel, int, -1, the selected channel to extract, -1 being all channels
 * @param video-route, int, 100, the xroute that will be assigned to packets containing video
 * @param audio-route, int, 100, the xroute that will be assigned to packets containing audio
 * @info Unmultiplexes an MPEG Transport Stream into the original streams each
 * consisting of series of PES-packets. It performs the reverse operation of
 * ts-multiplexer. The MPEG Transport Stream can be as large as entire multi-channel
 * stream.
 **/

const static int FIRST_SYNC = -3;
const static int LOST_SYNC = -2;

REGISTER_CLASS( TSDemultiplexer, "ts-demultiplexer" );

TSDemultiplexer::TSDemultiplexer( const string& sName, ProcessorManager* pScope )
	: MediaProcessor(sName, pScope), pPAT(&oPAT), iVideoRoute(100), iAudioRoute(200)
{
	pPAT->iID = -1;
	pPAT->iVersion = -1;
	mInt["channel"] = -1;
	mInt["video-route"] = 100;
	mInt["audio-route"] = 200;
}

TSDemultiplexer::~TSDemultiplexer( )
{
	/* Delete the dynamic streams */
	for( map<int,STREAM_t*>::iterator i = pPAT->mStreams.begin(); i != pPAT->mStreams.end(); i++ )
		delete i->second;
}

void TSDemultiplexer::init( void )
{
	/* Base class */
	MediaProcessor::init();

	/* Vars */
	iVideoRoute = mInt["video-route"];
	iAudioRoute = mInt["audio-route"];

	/* Add PID 0 to the allowed list */
	pPAT->mCC[0] = FIRST_SYNC;
}

void TSDemultiplexer::receive( MediaPacketPtr& pckt )
{
	/* Verify size */
	int iSize = pckt->size();
	if( not iSize or iSize % 188 != 0 )
		ValueError( this, "packet must be a multitude of 188 bytes: %s", pckt->c_str() );

	/* Split up and decode */
	uint8_t* pData = pckt->data();
	for( int i = 0; i < iSize; i += 188 )
		parseTS( pckt, pData + i );
}

void TSDemultiplexer::receive_reset( MediaPacketPtr& pckt )
{
	receive_end( pckt );
}

void TSDemultiplexer::receive_end( MediaPacketPtr& pckt )
{
	/* We replace the original end packet with an end packet for each stream */
	packet_t::type oType = pckt->type;

	/* Any streams detected? */
	if( pPAT->mStreams.size() )
	{
		/* Generate an end or reset packet for each stream */
		for( map<int,STREAM_t*>::iterator i = pPAT->mStreams.begin(); i != pPAT->mStreams.end(); i++ )
		{
			/* Finish each stream */
			parsePKT( NULL, 0, i->first, true, true );

			/* Generate an end packet for each stream */
			MediaPacketPtr pckt( new MediaPacket( oType, content_t::mixed, 0 ) );

			/* Meta data */
			STREAM_t* pStream = i->second;
			pckt->unitnumber = pStream->iUnit++;
			pckt->xstream = pStream->iXstream;
			pckt->xroute = pStream->iRoute;
			pckt->codec = pStream->iCodec;
			pckt->content = pStream->iContent;

			/* Send */
			debug( 1, "unmuxed %s", pckt->c_str() );
			route( pckt );
		}
	}
	/* Handle the case of extreme loss or very small streams such that no PAT/PMT
	 * were parsed */
	else
	{
		/* Generate an end packet for all streams */
		MediaPacketPtr pckt( new MediaPacket( oType, content_t::mixed, 0 ) );
		pckt->xroute = 0;

		/* Multicast */
		route( pckt );
	}
}

int TSDemultiplexer::parseTS( MediaPacketPtr& pckt, uint8_t* pData )
{
	/* Parse the fixed 4 byte transport stream header
	 * and variable adaptation field */
	IBits oStream( pData, 188 );

	/* Verify sync byte */
	int iSyntax =  oStream.read( 8 );
	if( iSyntax != 0x47 )
		return SirannonWarning( this,  "invalid sync byte: 0x47 != 0x%02X", iSyntax );

	/* Register PES start flag */
	oStream.read( 1 ); 				/* transport_error_indicator */
	bool bStart = oStream.read( 1 );  /* payload_start */
	oStream.read( 1 ); 				/* transport_priority_indicator */

	/* Verify PID */
	int iPID = oStream.read( 13 );
	oStream.read( 2 ); /* scrambling */
	int iField = oStream.read( 2 ); 	/* adaptation field */

	/* Verify CC */
	bool bSkip = false;
	int iCC = oStream.read( 4 );

	/* Ingore unkown PID */
	if( not pPAT->mCC.count( iPID ) )
	{
		debug( 1, "PID(%d), ingnoring", iPID );
		return -1;
	}
	else
	{
		int iExpected = ( pPAT->mCC[iPID] + 1 ) % 0x10;

		/* First occurence */
		if( pPAT->mCC[iPID] == FIRST_SYNC )
		{
			if( bStart )
			{
				debug( 1, "PID(%d), first packet", iPID );
				pPAT->mCC[iPID] = iCC;
			}
			else
			{
				debug( 1, "PID(%d), first packet", iPID );
				return -1;
			}
		}
		/* Handling in case of already lost sync */
		else if( pPAT->mCC[iPID] == LOST_SYNC )
		{
			if( bStart )
			{
				/* Resync succeeded */
				SirannonWarning( this,  "PID(%d) CC(%d), resyncing succeeded", iPID, iCC );
				pPAT->mCC[iPID] = iCC;
				bSkip = true;
			}
			else
			{
				/* Not resynced yet, silently drop this packet */
				debug( 1, "PID(%d) CC(%d), ingorning unsynced packet ", iPID, iCC );
				return -1;
			}
		}
		/* CC discontinuity */
		else if( iCC != iExpected )
		{
			/* Both resync and sync at the same time */
			if( bStart )
			{
				SirannonWarning( this,  "PID(%d), CC discontinuity: %d != %d + 1 mod 16, resyncing succeeded", iPID, iCC, pPAT->mCC[iPID] );
				pPAT->mCC[iPID] = iCC;
				bSkip = true;
			}
			/* Lost sync */
			else
			{
				SirannonWarning( this,  "PID(%d), CC discontinuity: %d != %d + 1 mode 16, ingnoring packets until next sync", iPID, iCC, pPAT->mCC[iPID] );
				pPAT->mCC[iPID] = LOST_SYNC;
				return -1;
			}
		}
		/* Handle the normal case */
		else
		{
			/* Register CC */
			pPAT->mCC[iPID] = iCC;
		}
	}
	/* Adapation field */
	int iAdapation = 0;
	if( iField & 0x02 )
	{
		/* Length of the adapation */
		iAdapation = oStream.read( 8 ) + 1;

		/* Process adaptation field */
		if( iAdapation > 1 )
		{
			int iDiscontinuity = oStream.read( 1 );
			int iRandom = oStream.read( 1 );
			int iPriority = oStream.read( 1 );
			int iFlags = oStream.read( 5 );

			/* Register PCR */
			if( iFlags & 0x10 )
			{
				oStream.read( 1 );
				oStream.read( 32 );
				oStream.read( 15 );
			}
		}
	}
	/* Calculate payload */
	uint8_t* pPayload = pData + 4 + iAdapation;
	int iPayload = 188 - 4 - iAdapation;

	/* Parse if it is special */
	debug( 2, "parsed packet PID(%d) CC(%d) adapation(%d) payload(%d)",
			iPID, iCC, iAdapation, iPayload );
	if( iPID == 0 )
	{
		parsePAT( pPayload, iPayload );
	}
	else if( pPAT->mChannels.count( iPID ) )
	{
		parsePMT( pPayload, iPayload, iPID );
	}
	else if( pPAT->mStreams.count( iPID ) )
	{
		parsePKT( pPayload, iPayload, iPID, bStart, bSkip );
	}
	else
		RuntimeError( this, "PID(%d), unknown packet should have been ignored", iPID );

	/* Done */
	return 0;
}

int TSDemultiplexer::parsePAT( uint8_t* pData, int iSize )
{
	/* Start parsing the PAT */
	IBits oStream( pData, iSize );

	/* Verify pointer */
	int iPointer = oStream.read( 8 );
	if( iPointer != 0 )
		return SirannonWarning( this,  "PAT, pointer: 0 != %d", iPointer );

	/* Verify table ID */
	int iTableID = oStream.read( 8 );
	if( iTableID != 0 )
		return SirannonWarning( this,  "PAT, table_id: 0 != %d", iTableID );

	/* Verify syntax */
	int iSyntax = oStream.read( 4 );
	if( iSyntax != 0xB )
		return SirannonWarning( this,  "PAT, wrong syntax: 0xB != 0x%X", iSyntax );

	/* Verify lenght */
	int iLength = oStream.read( 12 );
	if( iLength < 5 )
		return SirannonWarning( this,  "PAT, payload too small" );

	/* Verify network ID */
	int iID = oStream.read( 16 );
	if( pPAT->iID < 0 )
		pPAT->iID = iID;
	else if( pPAT->iID != iID )
		return SirannonWarning( this,  "PAT, new ID(%d) does not current ID(%d)", iID, oPAT.iID );

	/* Verify syntax */
	iSyntax = oStream.read( 2 );
	if( iSyntax != 0x03 )
		return SirannonWarning( this,  "PAT, wrong syntax: 0x03 != 0x%X", iSyntax );

	/* Verify version */
	int iVersion = oStream.read( 5 );
	if( pPAT->iVersion < 0 )
		pPAT->iVersion = iVersion;
	else if( iVersion != pPAT->iVersion )
	{
		pPAT->iVersion = iVersion;
		debug( 1, "PAT(0), changing version" );
	}
	/* Ignore section info */
	int iNext = oStream.read( 1 );
	int iSection = oStream.read( 8 );
	int iLast = oStream.read( 8 );

	/* Remaining length */
	iLength -= 5;

	/* Register each channel */
	while( iLength > 4 )
	{
		/* Register channel */
		int iChannel = oStream.read( 16 );

		/* Verify syntax */
		iSyntax = oStream.read( 3 );
		if( iSyntax != 0x07 )
			return SirannonWarning( this,  "PAT, wrong syntax: 0x07 != 0x%02X", iSyntax );

		/* Register PID */
		int iPID = oStream.read( 13 );

		/* Remaining length */
		iLength -= 4;

		/* Add the channel if this program is selected */
		if( mInt["channel"] < 0 or mInt["channel"] == iChannel )
		{
			/* Add the channel if it does not exist yet */
			if( not pPAT->mChannels.count( iPID ) )
			{
				/* PAT channel map */
				pPAT->mChannels[iPID].iChannel = iChannel;
				pPAT->mChannels[iPID].iVersion = -1;
				pPAT->mCC[iPID] = FIRST_SYNC;

				debug( 1, "PAT, adding channel(%d) with PMT(%d)", iChannel, iPID );
			}
		}
	}
	/* Ignore CRC */
	oStream.read( 32 );

	/* Verify length */
	if( iLength < 4 )
		return SirannonWarning( this,  "PAT, payload too small" );
	else if( iLength > 4 )
		return SirannonWarning( this,  "PAT, payload too large" );

	/* Done */
	return 0;
}

int TSDemultiplexer::parsePMT( uint8_t* pData, int iSize, int iPMT )
{
	/* Our PMT */
	PMT_t* pPMT = &(pPAT->mChannels[iPMT]);

	/* Start parsing the PAT */
	IBits oStream( pData, iSize );

	/* Verify pointer */
	int iPointer = oStream.read( 8 );
	if( iPointer != 0 )
		return SirannonWarning( this,  "PMT(%d), pointer: 0 != %d", iPMT, iPointer );

	/* Verify table ID */
	int iTableID = oStream.read( 8 );
	if( iTableID != 0x02 )
		return SirannonWarning( this,  "PMT(%d), table_id: 2 != %d", iPMT, iTableID );

	/* Verify syntax */
	int iSyntax = oStream.read( 4 );
	if( iSyntax != 0x0B )
		return SirannonWarning( this,  "PMT(%d), wrong syntax: 0xB != 0x%X", iPMT, iSyntax );

	/* Verify lenght */
	int iLength = oStream.read( 12 );
	if( iLength < 5 )
		return SirannonWarning( this,  "PMT(%d), payload too small", iPMT );

	/* Verify network ID */
	int iChannel = oStream.read( 16 );
	if( pPMT->iChannel != iChannel )
		return SirannonWarning( this,  "PMT(%d), channel(%d) differs from PAT channel(%d)", iPMT, iChannel, pPMT->iChannel );

	/* Verify syntax */
	iSyntax = oStream.read( 2 );
	if( iSyntax != 0x3 )
		return SirannonWarning( this,  "PMT(%d), wrong syntax: 0x3 != 0x%X", iPMT, iSyntax );

	/* Verify version */
	int iVersion = oStream.read( 5 );
	if( pPMT->iVersion < 0 )
		pPMT->iVersion = iVersion;
	else if( iVersion != pPMT->iVersion )
	{
		pPMT->iVersion = iVersion;
		debug( 1, "PMT(%d), changing version", iPMT );
	}
	/* Ignore section info */
	int iNext = oStream.read( 1 );
	int iSection = oStream.read( 8 );
	int iLast = oStream.read( 8 );

	/* Register PCR-PID */
	iSyntax =  oStream.read( 3 );
	pPMT->iPCR = oStream.read( 13 );
	iSyntax =  oStream.read( 6 );
	if( iSyntax != 0x3C )
		return SirannonWarning( this,  "PMT(%d), wrong syntax: 0x3C != 0x%02X", iPMT, iSyntax );

	/* Skip extra info */
	int iInfo = oStream.read( 10 );
	oStream.seek( iInfo );

	/* Remaining length */
	iLength -= 9 + iInfo;
	if( iLength < 0 )
		return SirannonWarning( this,  "PMT(%d), payload too small", iPMT );

	/* Register each stream */
	while( iLength > 4 )
	{
		/* Register stream type */
		int iStream = oStream.read( 8 );
		iSyntax = oStream.read( 3 );

		/* Verify syntax */
		if( iSyntax != 0x07 )
			return SirannonWarning( this,  "PMT(%d), wrong syntax 2: 0x7 != 0x%X", iPMT, iSyntax );

		/* Register PID */
		int iPID = oStream.read( 13 );

		/* Verify syntax */
		iSyntax = oStream.read( 6 );
		if( iSyntax != 0x3C )
			return SirannonWarning( this,  "PMT(%d), wrong syntax 2: 0x3C != 0x%02X", iPMT, iSyntax );

		/* Ignore info */
		int iInfo = oStream.read( 10 );
		oStream.seek( iInfo );

		/* Remaining length */
		iLength -= 5 + iInfo;

		/* Stream */
		codec_t::type iCodec = GetPmtCodec(iStream);

		/* Ignore the stream if the codec is unkown */
		if( iCodec == codec_t::NO )
		{
			SirannonWarning( this,  "channel(%d) PID(%d), ignorning unknown stream type (0x%02X)",
						iChannel, iPID, iStream );
			continue;
		}
		/* Add the stream if needed */
		if( not pPMT->mStreams.count( iPID ) )
		{
			/* Channel stream map */
			pPMT->mStreams[iPID] = iStream;

			/* Create a buffer for this stream if needed */
			if( not pPAT->mStreams.count( iPID ) )
			{
				/* Register the stream */
				STREAM_t* pStream = new STREAM_t ();
				pStream->iUnit = 0;
				pStream->iPos = 0;
				pStream->iStream = iStream;
				pStream->iCodec = iCodec;
				pStream->iContent = CodecToContent(pStream->iCodec);
				pStream->iRoute = pStream->iContent == content_t::audio ? iAudioRoute++ : iVideoRoute++;
				pStream->iXstream = nextStreamID();
				pPAT->mStreams[iPID] = pStream;
				pPAT->mCC[iPID] = FIRST_SYNC;

				debug( 1, "channel(%d) PID(%d), adding stream type (0x%02X/%s)",
						pPMT->iChannel, iPID, iStream, CodecToString(pStream->iCodec) );
			}
		}
	}

	/* Ignore CRC */
	oStream.read( 32 );

	/* Verify length */
	if( iLength < 4 )
		return SirannonWarning( this,  "PMT(%d), payload too small", iPMT );
	else if( iLength > 4 )
		return SirannonWarning( this,  "PMT(%d), payload too large", iPMT );

	/* Done */
	return 0;
}

int TSDemultiplexer::parsePKT( uint8_t* pData, int iSize, int iPID, bool bNew, bool bSkip )
{
	/* Stream container */
	STREAM_t* pStream = pPAT->mStreams[iPID];

	/* If a new PES starts, release the current */
	if( bNew and pStream->iPos )
	{
		/* Produce a warning for a truncated PES */
		if( bSkip )
			SirannonWarning( this,  "PID(%d), packet loss, truncating PES to %d B", iPID, pStream->iPos );

		/* Produce the previous PES packet */
		MediaPacketPtr pckt ( new MediaPacket( packet_t::media, content_t::mixed, pStream->iPos ) );

		/* Add data */
		pckt->push_back( pStream->pData, pStream->iPos );

		/* Strip the PES-header */
		IBits oHeader( pckt->data(), pckt->size() );
		oHeader.seek( 4 + 2 + 2 );
		int iExtra = oHeader.read( 8 );
		oHeader.seek( iExtra );
		pckt->pop_front( oHeader.size() );

		/* Full set the packet info */
		pckt->dts = pckt->pts = pckt->inc = 0;
		pckt->unitnumber = pStream->iUnit++;
		pckt->xstream = pStream->iXstream;
		pckt->xroute = pStream->iRoute;
		pckt->codec = pStream->iCodec;
		pckt->content = pStream->iContent;
		pckt->error = bSkip;
		pckt->desc = &pStream->desc;

		/* Send */
		debug( 1, "PID(%d), unmuxed %s", iPID, pckt->c_str() );
		route( pckt );

		/* Reset */
		pStream->iPos = 0;
	}

	/* Add the new data if present */
	if( pData )
	{
		/* Check if no memory overflow */
		if( pStream->iPos + iSize > iMaxFrame )
		{
			SirannonWarning( this,  "PID(%d), frame size exceeds 1MB, truncating PES to %d B", iPID, pStream->iPos );
		}
		else
		{
			/* Add to buffer */
			memcpy( pStream->pData + pStream->iPos, pData, iSize );
			pStream->iPos += iSize;
		}
	}
	return 0;
}
