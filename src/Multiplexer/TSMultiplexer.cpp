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
#include "TSMultiplexer.h"
#include "crc.h"

/**
 * TS MULTIPLEXER
 * @component TS-multiplexer
 * @type multiplexer
 * @param mtu, int, 1500, in bytes, the maximum size of an aggregated packet, typically for an iMTU of 1500 it means 7 Transport Stream packets (1370 bytes)
 * @param aggregate, bool, true, if true, several transport stream packets will be aggregated until their size would exceed the iMTU
 * @param shape, int, 200, in ms, the amount of media to multiplex each pass, this value overwrites the value of delay
 * @param pcr-delay, int, 400, in ms, how much the PCR is in advance of the DTS, the pcr-delay must be lower than the initial DTS of the reader component
 * @param mux-rate, int, -1, in kbps, if -1, no fixed multiplexing rate, if 0, the component will guess the mux rate based on the bitrate of the input, if > 0, use this value as mux-rate
 * @param continuous-timestamps, bool, true, if true, use the PCR clock as timestamp for the transport stream, if false, use the timestamps of the PCR stream as timestamps for the transport stream
 * @param interleave, bool, true, if false, all transport stream packets belonging to one frame from a PID are consecutive, if true, they are interleaved with transport stream packets from other PIDs (e.g. audio)
 * @param psi-on-key, bool, false, if true, generate an extra PSI triplet (SDT, PAT and PMT) before the start of every key frame of the PCR stream
 * @info Multiplexes audio and video into an MPEG Transport Stream (TS). CAVEAT: If the video or audio come from different readers, you must set the parameter streams or initial-delay, or the
 * component will throw an error.
 **/

/* MediaProcessorFactory */
REGISTER_CLASSES( TSMultiplexer, "ts-multiplexer", 1 );

const char sProvider [] = "Sirannon";
const char sService [] = "Service01";

TSMultiplexer::TSMultiplexer( const string& sName, ProcessorManager* pScope )
	: Multiplexer(sName, pScope), oHeader(188),
	  iMTU(1500), iRefStream(-1), audioStream(-1), videoStream(-1), bAggr(true),
	  iShape(18000), iMuxRate(0), iPCRDelay(36000), iPCR(-1), bCont(true), iSources(-1),
	  iLocalRefStream(-1), bInterleave(true), bPSIonKey(false)
{
	mInt["mtu"] = iMTU;
	mBool["aggregate"] = true;
	mInt["shape"] = 200;
	mInt[ "pcr-delay" ] = 400; /* delay of 400 ms */
	mInt[ "mux-rate" ] = -1;
	mBool["continuous-timestamps"] = true;
	mBool["interleave"] = true;
	mBool["psi-on-key"] = false;

	memset( SI, 0, sizeof(SI) );
	SI[SDT].PID = 0x11;
	SI[SDT].interval = 500;
	SI[PAT].PID = 0x00;
	SI[PAT].interval = 100;
	SI[PMT].PID = 0xFFF;
	SI[PMT].interval = 100;
	SI[PCR].interval = 20;
	SI[FILL].PID = 0x1FFF;
};

TSMultiplexer::~TSMultiplexer( )
{
	while( vOut.size() )
	{
		delete vOut.front();
		vOut.pop_front();
	}
}

void TSMultiplexer::init( void )
{
	/* Base class */
	Multiplexer::init();

	/* Force delay to at least the shape */
	iShape = mInt["shape"] * 90;
	iDelay = max( iDelay, iShape + 3600 );
	iPCRDelay = mInt["pcr-delay"] * 90;
	bCont = mBool["continuous-timestamps"];
	bAggr = mBool["aggregate"];
	bInterleave = mBool["interleave"];
	bPSIonKey = mBool["psi-on-key"];
	iMTU = mInt["mtu"];
}

void TSMultiplexer::findRefStream( void )
{
	/* Find the iRefStream */
	int32_t iGuessedMuxRate = 0;
	for( map<int,muxx_t>::iterator q = mMux.begin(); q != mMux.end(); q++ )
	{
		if( not q->second.vQueue.empty() )
		{
			/* Video ? */
			MediaPacket* pPckt = q->second.vQueue.front();
			if( pPckt->content == content_t::video )
			{
				iRefStream = q->first;
				videoStream = q->first;
				q->second.iPID = 0x100;
				iPCR = pPckt->dts;
			}
			else if( pPckt->content == content_t::audio )
			{
				if( iRefStream < 0 )
				{
					iRefStream = q->first;
					iPCR = pPckt->dts;
				}
				audioStream = q->first;
				q->second.iPID = 0x101;
			}
			iGuessedMuxRate += pPckt->desc->bitrate;
		}
	}
	/* Found anything? */
	if( iRefStream < 0 )
		RuntimeError( this, "Video nor audio content in buffers" );

	/* Selected mux rate */
	iMuxRate = mInt["mux-rate"];
	if( iMuxRate < 0 )
	{
		iMuxRate = 1;
	}
	else if( iMuxRate == 0 )
	{
		iMuxRate = iGuessedMuxRate;
		if( iMuxRate == 0 )
		{
			SirannonWarning( this, "Could not guess mux rate" );
			iMuxRate = 1;
		}
	}
	/* Repition rates */
	if( iMuxRate == 1 )
	{
		SI[SDT].cycle = 500;
		SI[PAT].cycle = 100;
		SI[PCR].cycle = 3;
	}
	else
	{
		SI[PCR].cycle = iMuxRate * SI[PCR].interval / ( 8 * 188 * 1000 );
		SI[SDT].cycle = iMuxRate * SI[SDT].interval / ( 8 * 188 * 1000 );
		SI[PAT].cycle = iMuxRate * SI[PAT].interval / ( 8 * 188 * 1000 );
	}
	/* PID of the PCR stream */
	SI[PCR].PID = mMux[iRefStream].iPID;
	SI[PAT].size = 17;
	SI[PMT].size = sizePMT();
	SI[SDT].size = sizeSDT();
	SI[FILL].size = 184;
	SI[PCR].size = 0;
	SI[PCR].count = SI[PCR].cycle;
	iSources = mMux.size();
	debug( 1, "SI info: rate(%d kbps) SDT(%d) PAT/PMT(%d) PCR(%d)", iMuxRate, SI[SDT].cycle, SI[PAT].cycle, SI[PCR].cycle );
}

void TSMultiplexer::process( void )
{
	/* Ensure enough data */
	if( not checkMux() )
		return;

	/* Sanity */
	if( iSources > 0 and mMux.size() != iSources )
		ValueError( this, "Multiple sources detected, please set the parameter 'streams' to an appropriate value." );

	/* Find the reference stream */
	if( iRefStream < 0 )
		findRefStream();

	/* Select packet with lowest dts */
	iLocalRefStream = selectMux();
	int iStream = iLocalRefStream;

	/* Limit */
	timestamp_t iStartDts = 0, iDuration = iShape;
	if( iLocalRefStream >= 0 )
		iStartDts = mMux[iLocalRefStream].vQueue.front()->dts;

	/* Cycle untill we process iShape amont of time */
	while( true )
	{
		if( iStream < 0 )
		{
			/* End packet */
			MediaPacketPtr pInPckt( resetMux() );
			iDuration = pInPckt->dts - iStartDts;
			vOut.push_back( pInPckt.release() );

			/* Emit SDT, PMT after this */
			for( int i = SDT; i <= PMT; ++i )
				SI[i].count = 0;
			break;
		}
		else
		{
			/* Normal packet */
			MediaPacket* pPckt = mMux[iStream].vQueue.front();

			/* Stop when we have processed iShape amount of time */
			if( ( ( iStream == iLocalRefStream ) and ( pPckt->dts >= iStartDts + iShape ) ) or
				  ( mMux[iLocalRefStream].vQueue.front()->type != packet_t::media ) )
			{
				iDuration = pPckt->dts - iStartDts;
				break;
			}
			/* Split into TS packets */
			while( step( iStream ) and not bInterleave ) { };

			/* Select packet with lowest dts for the next step */
			iStream = selectMux();
		}
	}
	/* Analyze the output buffer */
	int iPacket = 0, iTotal = vOut.size();
	timestamp_t iRefDts = 0;

	/* Release output buffer */
	while( not vOut.empty() )
	{
		/* Obtain the packet */
		MediaPacketPtr pPckt( vOut.front() );
		vOut.pop_front();

		/* Control packet? */
		if( pPckt->type != packet_t::media )
		{
			if( pTSPckt.get() )
				mux( pTSPckt );
			mux( pPckt );
		}
		else
		{
			/* Fill out the timing info */
			if( bCont )
				pPckt->dts = iStartDts + iPacket++ * iDuration / iTotal;
			else if( pPckt->xstream == iLocalRefStream )
				iRefDts = pPckt->dts;
			else
				pPckt->dts = iRefDts;

			/* Aggregate or simple? */
			if( bAggr )
			{
				/* Make a new packet for the aggregate */
				if( not pTSPckt.get() )
				{
					pTSPckt.reset( new MediaPacket( pPckt->type, pPckt->content, iMTU ) );
					pTSPckt->set_metadata( pPckt.get() );
				}
				/* Copy data */
				pTSPckt->push_back( pPckt->data(), pPckt->size() );

				/* Send the packet */
				if( pTSPckt->size() + 188 > (uint32_t)iMTU  )
					mux( pTSPckt );
			}
			else
			{
				mux( pPckt );
			}
		}
	}
}

bool TSMultiplexer::step( int iStream )
{
	/* Packet */
	muxx_t* pMux = &mMux[iStream];
	MediaPacket* pPckt = pMux->vQueue.front();

	/* Fresh packet ? */
	bool bStart = false;
	if( pMux->iOrigSize < 0 )
	{
		pMux->iOrigSize = pPckt->size();
		pMux->iOrigDTS = pPckt->dts;
		bStart = true;
	}
	/* Special effect for key frames */
	if( bPSIonKey and bStart and iStream == iRefStream and pPckt->key )
	{
		/* Force SDT, PAT & PMT to be generated */
		for( int i = SDT; i <= PMT; ++i )
			SI[i].count = 0;
	}
	/* Tick */
	bool bPCR = false;
	for( int i = SDT; i <= PMT; ++i )
	{
		SI[i].count++;
		if( SI[i].count == SI[i].cycle )
			SI[i].count = 0;
	}
	if( iStream == iLocalRefStream )
	{
		if( iMuxRate > 1 or bStart )
			SI[PCR].count++;
		if( SI[PCR].count >= SI[PCR].cycle )
		{
			SI[PCR].count = 0;
			bPCR = true;
		}
	}
	/* Generate SI packets at regular intervals */
	if( SI[SDT].count == 1 )
	{
		createTSPacket( pMux, SDT, true, false );
	}
	if( SI[PAT].count == 1 )
	{
		createTSPacket( pMux, PAT, true, false );
		createTSPacket( pMux, PMT, true, false );
	}
	/* Transport streams with fixed bitrate */
	if( iMuxRate > 1 and ( pPckt->dts - iPCR ) > iPCRDelay )
	{
		/* Generate an empty TS packet to fix the bitrate */
		if( bPCR )
			createTSPacket( pMux, PCR, false, true );
		else
			createTSPacket( pMux, FILL, false, false );
	}
	else
	{
		/* Create a TS packet with real data */
		int iData = createTSPacket( pMux, PES, bStart, bPCR );

		/* Increase the DTS */
		pPckt->dts += iData * pPckt->inc / pMux->iOrigSize; /* Above 135 Mbit this will fail */
	}
	/* Packet depleted? */
	if( not pPckt->size() )
	{
		delete pPckt;
		pMux->vQueue.pop_front();
		pMux->iOrigSize = -1;
		return false;
	}
	return true;
}

int TSMultiplexer::createTSPacket( muxx_t* pMux, packet_t iType, bool bStart, bool bPCR )
{
	MediaPacket* pPckt = pMux->vQueue.front();
	bool bRandom = (iType == PES) and bStart and pPckt->key and pPckt->content == content_t::video;

	/* Create a new TS packet */
	MediaPacket* pTSPckt = new MediaPacket( pPckt->type, pPckt->content, 188 );

	/* In which queue to place the packet ? */
	vOut.push_back( pTSPckt );

	/* Meta data */
	pTSPckt->set_metadata( pPckt );
	pTSPckt->dts = pMux->iOrigDTS;
	pTSPckt->framestart = true;
	pTSPckt->frameend = true;
	pTSPckt->mux = mux_t::TS;

	/* Payload size */
	uint8_t iSize;
	if( iType != PES )
		iSize = SI[iType].size;
	else if( bPCR )
		iSize = MIN( pPckt->size(), 184 - 8 );
	else if( bRandom )
		iSize = MIN( pPckt->size(), 184 - 2 );
	else
		iSize = MIN( pPckt->size(), 184 );

	/* What PID and CC? */
	int16_t iPID, iCC;
	if( iType == PCR )
	{
		iPID = pMux->iPID;
		iCC = pMux->iCC;
	}
	else if( iType == FILL )
	{
		iPID = SI[FILL].PID;
		iCC = 0;
	}
	else if( iType != PES )
	{
		iPID = SI[iType].PID;
		iCC = (SI[iType].cc++) % 16;
	}
	else
	{
		iPID = pMux->iPID;
		iCC = (pMux->iCC++) % 16;
	}
	debug( 3, "created: type(%s) PID(%hd) cc(%hd) PAYLOAD(%d) DTS(%d) PTS(%d) PCR(%d/%d) START(%d) SYNC(%d)", strType(iType), iPID, iCC, iSize, pTSPckt->dts, pTSPckt->pts, (int)bPCR, iPCR, (int) bStart, (int)bRandom );

	/* Fill out the header fields */
	fillTSHeader( pTSPckt, iSize, iPID, iCC, bStart, bPCR, bRandom );

	/* Body: Push in payload */
	if( iType == PES )
	{
		/* Add the data */
		pTSPckt->push_back( pPckt->data(), iSize );
		if( pTSPckt->size() != 188 )
			RuntimeError( this, "transport stream packet size(%d) != 188", pTSPckt->size() );
		pPckt->pop_front( iSize );
	}
	else if( iType == FILL )
	{
		pushFILL( pTSPckt );
	}
	else if( iType == PAT )
	{
		pushPAT( pTSPckt );
	}
	else if( iType == PMT )
	{
		pushPMT( pTSPckt );
	}
	else if( iType == SDT )
	{
		pushSDT( pTSPckt );
	}
	/* Increase timer */
	iPCR += 188 * 8 * 90000 / iMuxRate;
	return iSize;
}

void TSMultiplexer::fillTSHeader( MediaPacket* pTSPckt,
	int iSize, int iPID, int iCC, bool bStart, bool bPCR, bool bDoRandom )
{
	/* Write the transport stream header */
	oHeader.clear();
	oHeader.write( 8, 0x47 );
	oHeader.write( 1, 0 ); /* transport error */
	oHeader.write( 1, bStart );
	oHeader.write( 1, 0 ); /* priority */
	oHeader.write( 13, iPID ); /* PID */
	oHeader.write( 2, 0 ); /* scrambling */
	if( iSize > 0 )
		oHeader.write( 2, iSize < 184 ? 0x3 : 0x1 ); /* adapatation field */
	else
		oHeader.write( 2, 0x2 );
	oHeader.write( 4, iCC ); /* continuity counter */

	/* Adapation field */
	if( iSize < 184  )
	{
		/* Number of stuffed bytes */
		int iExtend = 184 - 1 - iSize; /* 1 for the length field itself */
		oHeader.write( 8, iExtend );
		if( iExtend > 0 )
		{
			/* Flags */
			oHeader.write( 1, 0 );
			oHeader.write( 1, bDoRandom );
			oHeader.write( 1, bDoRandom );
			oHeader.write( 5, bPCR ? 0x10 : 0x00 );
			if( bPCR )
			{
				/* Actual PCR */
				timestamp_t iCurPCR = 0;
				if( iMuxRate > 1 )
					iCurPCR = iPCR + (4+7)*8*90000 / iMuxRate;
				else
					iCurPCR = pTSPckt->dts - iPCRDelay;

				/* Write */
				oHeader.write( 1, 0 );
				oHeader.write( 32, iCurPCR );
				oHeader.write( 6, 0xFF );
				oHeader.write( 9, 0 );
				iExtend -= 6;
			}
			/* Stuff em in */
			int iStuff = iExtend - 1;
			for( int i = 0; i < iStuff; i++ )
				oHeader.write( 8, 0xFF );
		}
	}
	/* Write away */
	pTSPckt->push_back( oHeader.data(), oHeader.size() );
}

void TSMultiplexer::pushPAT( MediaPacket* pTSPckt )
{
	/* Write the bits */
	oHeader.clear();
	oHeader.write( 16, 0x0000 ); /* Push in 0 for pointer & table-id */
	oHeader.write( 8, 0xB0 ); /* Push in 0xB0 for syntax compliance */
	oHeader.write( 8, 0x0D ); /* Size */
	oHeader.write( 8, 0x00 );
	oHeader.write( 8, 0x01 );
	oHeader.write( 8, 0xC0 | (SI[PAT].version)%32 << 1 | 0x01 );
	oHeader.write( 24, 0x000000 );
	oHeader.write( 8, 0x01 ); /* Push in 1 as channel */
	oHeader.write( 3, 0xFF );
	oHeader.write( 13, SI[PMT].PID ); /* Push in 42 as PID */



	/* Compute the CRC */
	uint8_t* pData = oHeader.data();
	uint32_t iCrc = 0xFFFFFFFF;
	for( int i = 1; i < 13; i++ )
		iCrc = mpeg_crc32( pData[i], iCrc );
	oHeader.write( 32, iCrc );

	/* Write em away */
	pTSPckt->push_back( oHeader.data(), oHeader.size() );
}

void TSMultiplexer::pushPMT( MediaPacket* pTSPckt )
{
	/* Write the PMT header */
	oHeader.clear();
	oHeader.write( 8, 0x00 ); /* Push in 0 for pointer */
	oHeader.write( 8, 0x02 ); /* Push in 2 for table_id */
	oHeader.write( 8, 0xB0 ); /* Push in 0xB0 for syntax compliance */
	oHeader.write( 8, 0x00 ); /* Size unknown for the moment */
	oHeader.write( 8, 0x00 );
	oHeader.write( 8, 0x01 ); /* Push in 1 as channel */
	oHeader.write( 8, 0xC0 | (SI[PMT].version)%32 << 1 | 0x01 ); /* Push in version and bit for complete table */
	oHeader.write( 8, 0x00 ); /* Push in 0 for section_id */
	oHeader.write( 8, 0x00 ); /* Push in 0 for last_section_id */
	oHeader.write( 3, 0xFF );
	oHeader.write( 13, SI[PCR].PID ); /* Push in 45 as PID who contains the PCR */
	oHeader.write( 8, 0xF0 );
	oHeader.write( 8, 0x00 ); /* Push in the number of bytes for the blank descriptors */

	for( map<int,muxx_t>::iterator i = mMux.begin(); i != mMux.end(); i++ )
	{
		/* Sanity */
		muxx_t* pMux = &i->second;
		if( pMux->vQueue.empty() )
			RuntimeError( this, "empty queue while constructing PMT" );

		/* Find the codec type and the corresponding PMT ID */
		codec_t::type iCodec = pMux->vQueue.front()->codec;
		int iID = GetPmtID( iCodec );
		if( iID <= 0 )
			RuntimeError( this, "Unsupported codec (%s) in (%s)",  CodecToString(iCodec), pMux->vQueue.front()->c_str_long() );

		/* Special case for MPEG 1/2 audio because FFmpeg does not always produce the correct codec_id
		 * e.g. declaring MPEG 1 audio as MP2A
		 * For all newer formats such as FLV this is not a problem (MPEG1/2 share the same ID), however for
		 * transport streams MPEG 1 and 2 have a seperate ID */
		if( iCodec == codec_t::mp1a or iCodec == codec_t::mp2a )
		{
			if( pMux->codec == codec_t::NO )
			{
				/* Check the audio packet for a startcode */
				MediaPacket* pPckt = pMux->vQueue.front();
				if( pPckt->size() > 14 )
				{
					IBits oHeader( pPckt->data(), pPckt->size() );

					/* Skip PES */
					oHeader.seek( 14 );

					/* Find sync byte */
					oHeader.find( 0xFFF, 12 );
					if( oHeader.rem() > 13 )
					{
						oHeader.read( 12 );
						if( oHeader.read( 1 ) )
							pMux->codec = codec_t::mp1a;
						else
							pMux->codec = codec_t::mp2a;
					}
				}
			}
			iID = GetPmtID( pMux->codec );
		}
		/* Write the enttry */
		oHeader.write( 8, iID );
		oHeader.write( 3, 0xFF );
		oHeader.write( 13, pMux->iPID ); /* Push in ES_ID */
		oHeader.write( 8, 0xF0 );

		/* Special extension for wmv */
		if( iCodec == codec_t::wmv1 or
			iCodec == codec_t::wmv2 or
			iCodec == codec_t::wmv3 or
			iCodec == codec_t::wmvc     )
		{
			// 10 A0 0E 57 4D 56 33 01 40 00 F0 00 04 4E 39 1A 01
			oHeader.write( 8, 0x10 ); /* ES_info_length */
			oHeader.write( 8, 0xA0 ); /* discriptor_tag */
			oHeader.write( 8, 0x0E ); /* discriptor_length */

			/* Fill out the FourCC of the codec */
			const char* s = CodecToString( iCodec );
			oHeader.write( 8, s[0] );
			oHeader.write( 8, s[1] );
			oHeader.write( 8, s[2] );
			oHeader.write( 8, s[3] );

			/* Information about the video present? */
			if( not( pTSPckt->desc and pTSPckt->desc->width > 0 and pTSPckt->desc->height > 0 ) )
				RuntimeError( this, "width or height not defined" );
			int width = pTSPckt->desc->width, height = pTSPckt->desc->height, extra = 4;

			oHeader.write( 16, width );
			oHeader.write( 16, height );
			oHeader.write( 16, extra );
			oHeader.write( 8, 0x4E );
			oHeader.write( 8, 0x39 );
			oHeader.write( 8, 0x1A );
			oHeader.write( 8, 0x01 );
		}
		else
		{
			oHeader.write( 8, 0x00 ); /* ES_info_length */
		}
	}
	/* The size is now known */
	uint8_t* pData = oHeader.data();
	pData[3] = oHeader.size();

	/* Compute & write the CRC */
	uint32_t iCrc = 0xFFFFFFFF;
	for( int j = 1; j < oHeader.size(); j++ )
		iCrc = mpeg_crc32( pData[j], iCrc );
	oHeader.write( 32, iCrc );

	/* Write em away */
	pTSPckt->push_back( oHeader.data(), oHeader.size() );
}

void TSMultiplexer::pushSDT( MediaPacket* pTSPckt )
{
	/* Write the PMT header */
	oHeader.clear();
	oHeader.write( 8, 0x00 ); /* Push in 0 for pointer */
	oHeader.write( 8, 0x42 ); /* Push in 2 for table_id */
	oHeader.write( 8, 0xB0 ); /* Push in 0xB0 for syntax compliance */
	oHeader.write( 8, 0x00 ); /* Size unknown for the moment */
	oHeader.write( 8, 0x00 );
	oHeader.write( 8, 0x01 ); /* Push in 1 as channel */
	oHeader.write( 8, 0xC0 | (SI[SDT].version)%32 << 1 | 0x01 ); /* Push in version and bit for complete table */
	oHeader.write( 8, 0x00 ); /* Push in 0 for section_id */
	oHeader.write( 8, 0x00 ); /* Push in 0 for last_section_id */
	oHeader.write( 16, 0x01 ); /* ID */
	oHeader.write( 8, 0xFF );
	oHeader.write( 16, 0x01 ); /* ID */
	oHeader.write( 8, 0xFC );
	oHeader.write( 16, 0x80 | ( strlen(sProvider) + strlen(sService) + 2 + 3 ) ); /* ??? */
	oHeader.write( 8, 0x48 );
	oHeader.write( 8, strlen(sProvider) + strlen(sService) + 2 + 1 ); /* ??? */
	oHeader.write( 8, 0x01 );
	oHeader.write( 8, strlen(sProvider) );
	oHeader.write_buffer( (uint8_t*)sProvider, strlen(sProvider) );
	oHeader.write( 8, strlen(sService) );
	oHeader.write_buffer( (uint8_t*)sService, strlen(sService) );

	/* The size is now known */
	uint8_t* pData = oHeader.data();
	pData[3] = oHeader.size();

	/* Compute & write the CRC */
	uint32_t iCrc = 0xFFFFFFFF;
	for( int j = 1; j < oHeader.size(); j++ )
		iCrc = mpeg_crc32( pData[j], iCrc );
	oHeader.write( 32, iCrc );

	/* Write em away */
	pTSPckt->push_back( oHeader.data(), oHeader.size() );
}

void TSMultiplexer::pushFILL( MediaPacket* pTSPckt )
{
	uint8_t* pData = pTSPckt->data() + pTSPckt->size();
	pTSPckt->push_back( SI[FILL].size );
	memset( pData, 0, SI[FILL].size );
}

uint32_t TSMultiplexer::sizePMT( void )
{
	 /* Basic size */
	 int iSize = 17;

	/* bytes for stream _id declarations */
	iSize += 5 * mMux.size();

	/* Any special codecs */
	for( map<int,muxx_t>::iterator i = mMux.begin(); i != mMux.end(); i++ )
	{
		codec_t::type iCodec = i->second.vQueue.front()->codec;
		switch( iCodec )
		{
		case codec_t::wmv1:
		case codec_t::wmv2:
		case codec_t::wmv3:
		case codec_t::wmvc:
			iSize += 10;
		default:
			break;
		}
	}
	return iSize;
}

uint32_t TSMultiplexer::sizeSDT( void )
{
	return 26 + strlen(sProvider) + strlen(sService);
}

const char* TSMultiplexer::strType( packet_t iType )
{
	switch( iType )
	{
	case PAT:
		return "PAT";
	case PMT:
		return "PMT";
	case SDT:
		return "SDT";
	case FILL:
		return "NULL";
	case PCR:
		return "PCR";
	case PES:
		return "PES";
	default:
		return "???";
	}
}

void TSMultiplexer::mux( MediaPacketPtr& pckt )
{
	/* Force to a common unit stream */
	pckt->unitnumber = iUnit++;
	pckt->xstream = streamID;
	debug( 2, "muxed(%s)", pckt->c_str() );

	/* Done */
	route( pckt );
}
