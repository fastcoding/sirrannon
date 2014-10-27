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
#include "AVCReader.h"
#include "Frame.h"
#include "sdp.h"

/**
 * AVC READER
 * @component avc-reader
 * @type reader
 * @param skip-SEI, bool, false, if true, ignore the SEI NAL-units
 * @param skip-AUD, bool, false, if true, ignore the AUD NAL-units
 * @param mov-frame, bool, false, if true, convert NAL-units into MP4/MOV/F4V frames
 * @info Reads in a raw H264 video file. Each generated \textit{MediaPacket} contains
 * one H.264 NAL-unit, possibly generating multiple \textit{MediaPackets} per frame.
 **/
REGISTER_CLASS( AVCReader, "avc-reader" );

AVCReader::AVCReader( const string& sName, ProcessorManager* pScope )
	: Reader(sName, pScope), iSubFrame(0), bKey(false), bSkipSEI(false), bSkipAUD(false),
	  iMaxBuffer(128*KIBI), bMOV(false)
{
	/* Default values */
	mBool["skip-SEI"] = false;
	mBool["skip-AUD"] = false;
	mInt["video-route"] = 100;
	mInt["dts-start"] = 1000;
	mInt["loop"] = 1;
	mBool["sdp"] = false;
	mBool["mov-frame"] = false;

	/* Reset the status */
	iUnit = 0, iFrame = 0, iBound = -1, iIndex = 0, iEndIndex = 0, iPOC = 0;
	dts = 100000, iRefDts = -1, iBaseDts = 0xffffffff, iCurDts = -1, iLastDts = -1;
	old_type = frame_t::no_frame;
	oH264Decode = NAL_t();
	oInc.num = 1;
	oInc.den = 25;
	memset( &oH264Decode, 0, sizeof(oH264Decode) );
	streamID = nextStreamID();
	iRoute = 100;
	pBuffer = (uint8_t*) realloc( NULL, iMaxBuffer );
}

AVCReader::~AVCReader( void )
{
	for( uint32_t i = 0; i < vStack.size(); i++ )
		delete vStack[i];
	free( pBuffer );
}

void AVCReader::init( void )
{
	/* Cached */
	bSkipSEI = mBool["skip-SEI"];
	bSkipAUD = mBool["skip-AUD"];
	iRoute = mInt["video-route"];
	bMOV = mBool["mov-frame"];
	if( iRoute <= 0 )
		iRoute = 100;

	/* Base class */
	Reader::init();

	/* Prepare */
	prepareBuffer();

	/* Desc */
	pVideoDesc = addMedia();
	pVideoDesc->codec = codec_t::avc;

	/* Fill out the extra data */
	oConvertor.buildVideoExtraData( pBuffer, iMaxBuffer, pVideoDesc );
	debug( 1, "extra data(%d: %s)", pVideoDesc->getExtraSize(),
	  		strArray(pVideoDesc->getExtraData(),pVideoDesc->getExtraSize()).c_str() );
}

void AVCReader::prepareBuffer( void )
{
	/* Initial data */
	iIndex = 0, iEndIndex = 0, iBound = -1;

	/* Reset the status	*/
	memset( &oH264Decode, 0, sizeof(oH264Decode) );

	/* Fill our pBuffer */
	iEndIndex = fread( pBuffer, sizeof(uint8_t), iMaxBuffer, oFile );

	/* Verify if the buffer is filled correctly */
	if( feof( oFile ) and iEndIndex == 0 )
		RuntimeError( this, "Empty file(%s)", mString["filename"].c_str() );
	if( ferror( oFile ) )
		RuntimeError( this, "Could not read from file(%s)", mString["filename"].c_str() );
}

bool AVCReader::doStep( void )
{
	/* Assert that our current iIndex points to a NAL */
	MediaPacketPtr pPckt;
	if( not H264_is_start_code( pBuffer + iIndex ) )
		RuntimeError( this, "Could not find a start code:", strArray( pBuffer+iIndex, min(iMaxBuffer-iIndex,25U) ).c_str() );

	/* Find the iOffset to the next NAL */
	int iOffset;
	bool bLast = false;
	while( true )
	{
		iOffset = H264_find_NAL( pBuffer + iIndex, iEndIndex - iIndex );

		/* Found a NAL */
		if( iOffset < iEndIndex - iIndex )
			break;

		/* End of stream */
		if( feof( oFile ) )
		{
			iOffset = iEndIndex - iIndex;
			bLast = true;
			break;
		}

		/* If the index of the current NAL is larger than zero, shift the buffer */
		if( iIndex > 0 )
		{
			memmove( pBuffer, pBuffer + iIndex, iEndIndex - iIndex );
			iEndIndex -= iIndex;
			iIndex = 0;
		}
		/* NAL unit too large for buffer, enlarge it */
		else
		{
			iMaxBuffer *= 2;
			pBuffer = (uint8_t*) realloc( pBuffer, iMaxBuffer );
			SirannonWarning( this, "Buffer too small for NAL, increasing buffer to (%d kB)", iMaxBuffer / 1024 );
		}
		/* Refill */
		iEndIndex += fread( pBuffer + iEndIndex, sizeof(uint8_t), iMaxBuffer - iEndIndex, oFile );

		/* Check for error while reading the stream */
		if( ferror( oFile ) )
			RuntimeError( this, "Could not read from file (%s)", mString["filename"].c_str() );
	}
	/* Filter packets */
	uint8_t iNAL = H264_NAL_type( pBuffer + iIndex );
	if( ( bSkipSEI and iNAL == NAL_UNIT_TYPE_SEI ) or
		( bSkipAUD and iNAL == NAL_UNIT_TYPE_ACCESS_UNIT_DELIM ) )
	{
		iIndex += iOffset;
		return true;
	}
	/* Standard path: New packet */
	pPckt = MediaPacketPtr( new MediaPacket( packet_t::media, content_t::video, iOffset ) );

	/* Parameters independent of any type of packet */
	pPckt->codec = codec_t::avc;
	pPckt->xroute = iRoute;
	pPckt->xstream = streamID;
	pPckt->desc = pVideoDesc;

	/* NAL sits between pBuffer[iIndex] and pBuffer[iIndex+iOffset], let's copy it to data and analyse */
	pPckt->push_back( pBuffer + iIndex, iOffset );

	/* Increase iIndex */
	iIndex += iOffset;

	/* Classify the packet and we are done */
	bool ret = classify( pPckt );

	/* End of stream? */
	if( bLast )
	{
		if( mInt["loop"] < 0 or iLoop++ < mInt["loop"] )
		{
			/* Send a reset packet */
			pPckt = MediaPacketPtr( new MediaPacket( packet_t::reset, content_t::video, 0 ) );
			pPckt->xroute = iRoute;
			classify_ctrl( pPckt );

			/* Seek to start */
			fseek( oFile, 0, SEEK_SET );

			/* Reload */
			prepareBuffer();
		}
		else
		{
			/* Close file */
			closeBuffer();

			/* Send an end packet */
			pPckt = MediaPacketPtr( new MediaPacket( packet_t::end, content_t::video, 0 ) );
			pPckt->xroute = iRoute;
			classify_ctrl( pPckt );

			/* Stop the infinite loop */
			bSchedule = false;
		}
		return false;
	}
	return true;
}

bool AVCReader::classify( MediaPacketPtr& pPckt )
{
	/* Read the next nal unit into our decode and determine if it was a boundary */
	int iJump = H264_parse_NAL( pPckt->data(), pPckt->size(), &oH264Decode );
	if( iJump < 0 )
		RuntimeError( this, "failed to parse" );
	if( iBound < 0 )
		iJump = 0; /* supress iBound for the first NAL */
	iBound = iJump;

	/* Send the away the currently stored frame */
	if( iBound > 0 )
	{
		send_frame();
		iFrame++;
	}
	/* Dts iOffset */
	if( iBaseDts == -1 )
		iBaseDts = mInt["dts-start"] * 90;

	/* Determine the frametype */
	pPckt->frame = H264FrameToXFrame( oH264Decode.nal_unit_type, oH264Decode.slice_type );
	if( pPckt->frame == frame_t::no_frame )
		SirannonWarning( this,  "unknown NAL type (%d)", oH264Decode.nal_unit_type );
	else if( pPckt->frame == frame_t::other )
		SirannonWarning( this,  "unsupported NAL type (%d)", oH264Decode.nal_unit_type );
	debug( 3, "stack %s %d", pPckt->c_str(), oH264Decode.pic_order_cnt );
	if( IsIFrame( pPckt.get() ) )
		bKey = true;

	/* Acquire the freq from the SPS unit */
	if( pPckt->frame == frame_t::SPS )
	{
		if( oH264Decode.time_scale == 0 )
		{
			debug( 1, "no framerate defined in SPS-packet, assuming 25 fps" );
			oInc.num = 1;
			oInc.den = 50;
		}
		else
		{
			oInc.num = oH264Decode.num_units_in_tick;
			oInc.den = oH264Decode.time_scale;
		}
		pVideoDesc->inc = (int64_t) oInc.num * 90000 * 2 / oInc.den;
		oInc.num *= 90000 * 2;
		pVideoDesc->height = (oH264Decode.pic_height_in_map_units_minus1+1) * 16;
		pVideoDesc->width = (oH264Decode.pic_width_in_mbs_minus1+1) * 16;
		debug( 1, "video timebase(%d/%d) inc(%d) width(%d) height(%d) ref_frames(%d, %d) POC(%d)",
				oInc.num, oInc.den, pVideoDesc->inc, pVideoDesc->width, pVideoDesc->height,
				oH264Decode.num_ref_frames_in_pic_order_cnt_cycle, oH264Decode.num_ref_frames, oH264Decode.pic_order_cnt_type );
	}
	/* SVC Meta-data */
	if( oH264Decode.svc )
	{
		pPckt->codec = codec_t::svc;
		pPckt->T = oH264Decode.temporal_id;
		pPckt->Q = oH264Decode.quality_id;
		pPckt->D = oH264Decode.dependency_id;
	}
	/* Store iPOC */
	if( oH264Decode.pic_order_cnt_valid )
		iPOC = oH264Decode.pic_order_cnt;

	/* Ensure POC reset */
	if( pPckt->frame == frame_t::IDR )
		iPOC = 0;

	/* Save current packet */
	vStack.push_back( pPckt.release() );

	/* Did we read a full frame ? */
	return not iBound;
}

void AVCReader::send_frame( void )
{
	/* Check */
	if( vStack.empty() )
		RuntimeError( this, "buffer underflow" );

	if( iRefDts < 0 or iPOC == 0 )
		iRefDts = iBaseDts + (int64_t) iFrame * oInc.num / oInc.den;

	/* iLoop over the NAL of the frame and assign data */
	int iSubFrame = 0;
	for( uint32_t i = 0; i < vStack.size(); i++ )
	{
		MediaPacket* pPckt = vStack[i];

		/* Number info */
		pPckt->unitnumber = iUnit++;
		pPckt->framenumber = iFrame;
		pPckt->subframenumber = iSubFrame++;

		/* Timing info */
		pPckt->dts = iBaseDts + (int64_t) pPckt->framenumber * oInc.num / oInc.den;

		/* POC */
		pPckt->pts = iRefDts + (int64_t) ( oH264Decode.num_ref_frames + iPOC / 2 ) * oInc.num / oInc.den;
		pPckt->inc = pVideoDesc->inc;

		/* Flags */
		pPckt->key = bKey;
		pPckt->framestart = false;
		pPckt->frameend   = false;
	}

	/* Assign frame start & frame end */
	vStack.front()->framestart = true;
	vStack.back()->frameend = true;

	/* Remember */
	if( iLastDts < 0 )
		iLastDts = vStack.front()->dts;
	iCurDts = vStack.front()->dts;

	/* Send the vStack away */
	for( uint32_t i = 0; i < vStack.size(); i++ )
	{
		MediaPacketPtr pPckt( vStack[i] );
		vStack[i] = NULL;

		/* Convert if needed */
		if( bMOV )
			pPckt = oConvertor.convertMP4( pPckt );

		/* Send away */
		if( pPckt.get() )
		{
			printFrame( pPckt );
			route( pPckt );
		}
	}
	vStack.clear();
	bKey = false;
}

void AVCReader::classify_ctrl( MediaPacketPtr& pPckt )
{
	/* Send the current frame away */
	send_frame();

	/* Set timing info */
	iFrame++;
	pPckt->framenumber = iFrame;
	pPckt->unitnumber  = iUnit++;
	pPckt->subframenumber = 0;
	pPckt->framestart = true;
	pPckt->frameend   = true;
	pPckt->inc = pVideoDesc->inc;
	pPckt->dts = iBaseDts + (int64_t) pPckt->framenumber * oInc.num / oInc.den;
	pPckt->pts = pPckt->dts;
	pPckt->xstream = streamID;
	pPckt->desc = pVideoDesc;
	pPckt->mux = mux_t::ES;
	pPckt->codec = codec_t::avc;
	printFrame( pPckt );

	/* Send away */
	route( pPckt );
}

void AVCReader::printFrame( MediaPacketPtr& pPckt )
{
	debug( 1, "parsed %s POC(%u)", pPckt->c_str(), iPOC );
}
