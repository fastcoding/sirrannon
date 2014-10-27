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
#include "RTPReceiver.h"
#include "Frame.h"
#include "SirannonTime.h"
#include "IPAddress.h"
#include "ffmpeg.h"
#define SIRANNON_USE_BOOST_FILESYSTEM
#include "Boost.h"

/**
 * RTP RECEIVER
 * @component RTP-receiver
 * @type receiver
 * @param tracefile, string, , if defined, the path of the trace where to log information about the received packets
 * @param buffer, int, 8388608, in bytes, the size of the underlyinhg UDP buffer, increase this value when receiving high bitrate streams, make sure yours OS accepts such large values (see \url{http://www.29west.com/docs/THPM/udp-buffer-sizing.html})
 * @param hash-file, string, , if defined, the path of a file in which the content of a header extension with ID(EXT_HASH: 0xB) is written
 * @info Receives RTP streams using the open source library jrtplib.
 * RTCP packets are automatically generated. The additional header is parsed from the
 * RTP header extension if present.
 **/
REGISTER_CLASSES( RTPReceiver, "RTP-receiver", 1 );

RTPReceiver::RTPReceiver( const string& sName, ProcessorManager* pScope )
	: MediaReceiver(sName, pScope), iLastDts(-1), iFrame(-1), iSubFrame(0),
	iFirstUnit(0), iFirstDts(0), bFirst(true), iBaseDts(0)
{
	mInt["port"] = 5000;
	mBool["multicast"] = false;
	mString["multicast-server"] = "238.0.0.1";
	mInt["route"] = 0;
}

RTPReceiver::~RTPReceiver()
{
	session.LeaveAllMulticastGroups();
}

void RTPReceiver::init( void )
{
	/* Base class */
	MediaReceiver::init();
	pDesc->inc = 0;

	/* Set values */
	RTPSessionParams sessionparams;
	RTPUDPv4TransmissionParams transparams;
	sessionparams.SetOwnTimestampUnit( 1.0 / 90000.0);
	sessionparams.SetMaximumPacketSize( 1500 );
	sessionparams.SetSessionBandwidth( 4000000 );
	sessionparams.SetUsePollThread( false ); /* We control the polling ourselves */
	transparams.SetPortbase( mInt["port"] ); /* our port */

	/* Buffer */
	if( mInt["buffer"] > 0 )
		transparams.SetRTPReceiveBuffer( mInt["buffer"] );
	else
		transparams.SetRTPReceiveBuffer( 8*MEBI );

	/* Create the session */
	int iStatus = session.Create( sessionparams, &transparams );
	if( iStatus < 0 )
		RuntimeError( this, RTPGetErrorString(iStatus).c_str() );
	debug( 1, "created rtp session" );

	/* Set some other values */
	session.SetDefaultPayloadType( 96 );
	session.SetDefaultMark( false );
	session.SetNameInterval( 1 );
	session.SetLocalName( "Sirannon", 8 );

	/* Join a multicast group */
	if( mBool["multicast"] )
	{
		uint8_t oDefaultAddr [] = {238,0,0,1};
		RTPIPv4Address rtp_addr = RTPIPv4Address( oDefaultAddr, 5000 );
		IPAddress 	     x_addr = IPAddress( mString["multicast-server"] );
		rtp_addr.SetIP( x_addr.getIPArray() );
		rtp_addr.SetPort( x_addr.getPort() );
		session.JoinMulticastGroup( rtp_addr );
	}
	/* Open the tracefile if specified */
	if( mString["tracefile"] != "" )
	{
		oTrace.open( mString["tracefile"].c_str(), "w" );
		if( not oTrace.active() )
			RuntimeError( this, "while trying to open %s", mString["tracefile"].c_str() );
	}
	debug( 1, "RTP listening on port %d", mInt["port"] );

	/* Activate already */
	bSchedule = true;
}

void RTPReceiver::process( void )
{
	/* Process incoming udp-packets */
	session.Poll();

	/* Process incoming rtp-packets */
	session.BeginDataAccess();
	if( session.GotoFirstSource() )
	{
		do
		{
			RTPPacket* pRTP;
			while( ( pRTP = session.GetNextPacket() ) )
			{
				/* Create new packet */
				MediaPacketPtr pckt ( new MediaPacket( packet_t::media, content_t::mixed, pRTP->GetPayloadLength() ) );

				/* Relative start points */
				if( bFirst )
				{
					iFirstDts = pRTP->GetTimestamp();
					iFirstUnit = pRTP->GetExtendedSequenceNumber();
					bFirst = false;
				}
				/* Is there extension data present to give detailed information? */
				if(	pRTP->HasExtension() )
				{
					switch( pRTP->GetExtensionID() )
					{
					case EXT_META:
						/* Parse the header with timing info */
						decode_header( pckt, pRTP->GetExtensionData() );
						break;

					case EXT_FILLER:
						delete pRTP;
						continue;
					}
					/* Additonal meta-data */
					decode_additional( pckt );
				}
				else
				{
					/* Deduce info ourselves */
					pckt->unitnumber = pRTP->GetExtendedSequenceNumber() - iFirstUnit;
					pckt->dts = pRTP->GetTimestamp() - iFirstDts;
					pckt->subframenumber = 0;

					/* Additonal meta-data */
					decode_additional( pckt );

					/* Specific descriptor data */
					decodeDescriptor( pckt.get() );

					/* Translate timestamp for audio */
					if(	pckt->content == content_t::audio and pckt->mux != mux_t::TS )
					{
						/* Verify sample rate is defined */
						if( not pckt->desc->samplerate )
							ValueError( this, "Samplerate is zero" );
						pckt->dts = pckt->pts = av_rescale_q( pckt->dts, toAVRational(1,pckt->desc->samplerate), toAVRational(pckt->desc->timebase) );
					}
					/* Relative time */
					pckt->dts = pckt->pts = pckt->dts + iBaseDts;

					/* Detect a difference in timestamp as a new frame */
					if( pckt->dts != iLastDts )
					{
						if( iLastDts >= 0 and pDesc->inc == 0 )
							pDesc->inc = pckt->dts - iLastDts;
						iFrame++;
						iLastDts = pckt->dts;
						pckt->framestart = true;
					}
					else
						pckt->framestart = false;
					pckt->framenumber = iFrame;
					pckt->frameend = pRTP->HasMarker();
					pckt->inc = pDesc->inc;
				}
				/* Add data */
				pckt->push_back( pRTP->GetPayloadData(), pRTP->GetPayloadLength() );

				/* Debug */
				if( pckt->framestart )
					debug( 1, "received %s", pckt->c_str_long() );
				else
					debug( 2, "received %s", pckt->c_str() );
				if( pckt->type == packet_t::end )
					debug( 1, "end %s", pckt->c_str() ) ;

				/* Tracefile */
				if( oTrace.active() )
					oTrace.write( "%f\tid %d\tudp %d\n",
									SirannonTime::getCurrentTime().convertDouble(),
									pRTP->GetSequenceNumber(),
									pRTP->GetPayloadLength() );

				/* Send our pckt away */
				route( pckt );

				/* Processed the rtp-packet */
				delete pRTP;
			}
		} while( session.GotoNextSource() );
	}
	session.EndDataAccess();
}
