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
#include "RTPTransmitter.h"
#include "Frame.h"
#include "Reader/FFmpegReader.h"
#include "IPAddress.h"
#include "PortManager.h"
#include "RandomGenerator.h"
#include "str.h"

/**
 * RTP TRANSMITTER
 * @component RTP-transmitter
 * @type transmitter
 * @param pts, bool, false, if true, the streamer uses the PTS of a packet instead of the DTS as time stamp in the RTP header. This can solve the problem where VLC ometimes interprets the RTP timestamp as a PTS instead of a DTS
 * @param, interface, string, , if defined, bind the created UDP sockets to this interface, enter the interface in the form of an IP address
 * @param payload, int, -1, payload type (PT) of the RTP packets, -1 means leaving the decision to the component
 * @param tracefile, string, , if defined, the path of the trace where to log information about the sent packets
 * @param mtu, int, 1500, in bytes, the maximum packet size accepted by the RTP session
 * @param force-sequence-number, bool, false, if true, force the RTP sequence number to follow the unitnumber, hence if you remove packets beforehand, the sequence number will also have gaps, if false, use the default RTP implementation
 * @info Provides the RTP/UDP protocol using the open source library jrtplib
 * RTCP packets are automatically generated. The extra information of the sirannon
 * (sirannon-extension) is added as a header extension in RTP packets and this keeps
 * it compatible with a standard player.
 **/

REGISTER_CLASS( SirannonRTPTransmitter, "RTP-transmitter" );

/* Constructor */
SirannonRTPTransmitter::SirannonRTPTransmitter( const string& sName, ProcessorManager* pScope )
	: MediaTransmitter(sName, pScope), iPrevTiming(-1), bExtension(true), bPts(false),
	iPayload(-1), bForce(false), iForce(oRandom.next())
{
	mBool["extension"] = false;
	mInt["port"] = 5000;
	mBool["pts"] = false;
	mInt["mtu"] = 1500;
	mString["tracefile"] = "";
	mString["destination"] = "";
	mInt["payload"] = -1;
	mInt["buffer"] = 32 * 1024 * 1024;
	mBool["hack"] = false;
	mString["hash-file"] = "";
	mBool["force-sequence-number"] = false;
	mString["interface"] = "";
}

void SirannonRTPTransmitter::receive_end( MediaPacketPtr& pPckt )
{
	/* Transmit the end packet over the network, in xtension mode */
	if( bExtension )
		receive( pPckt );
	else
		route( pPckt );
}

void SirannonRTPTransmitter::receive_reset( MediaPacketPtr& pPckt )
{
	receive_end( pPckt );
}

void SirannonRTPTransmitter::process( void )
{
	/* Generate RTCP packets */
	pSession.Poll();
}

void SirannonRTPTransmitter::init( void )
{
	/* Base class */
	MediaTransmitter::init();

	/* For compatibility, allow both names */
	if( not mString["destination"].size() )
		mString["destination"] = mString["client"];

	/* Set params */
	RTPSessionParams sessionparams;
	RTPUDPv4TransmissionParams transparams;
	sessionparams.SetOwnTimestampUnit( 1 / 90000. );
	sessionparams.SetMaximumPacketSize( mInt["mtu"] );
	sessionparams.SetSessionBandwidth( 4000000.0 );
	int iPort = mInt["port"];
	transparams.SetPortbase( iPort > 0 ? iPort : oPortManager.next() );

	/* Bind to the correct interface if provided */
	if( mString["interface"].length() )
	{
		IPAddress oAddr( mString["interface"] );
		transparams.SetBindIP( oAddr.getIP() );
	}
	/* Buffer */
	if( mInt["buffer"] > 0 )
		transparams.SetRTPSendBuffer( mInt["buffer"] );
	else
		transparams.SetRTPSendBuffer( 8 * 1024 * 1024 );

	/* Multicast */
	if( mInt["multicast-TTL"] >= 0 )
		transparams.SetMulticastTTL( mInt["multicast-TTL"] );

	/* Create the session */
	int iStatus = pSession.Create( sessionparams, &transparams );
	if( iStatus < 0)
		RuntimeError( this, RTPGetErrorString(iStatus).c_str() );

	/* Set some other values */
	iPayload = mInt["payload"];
	pSession.SetDefaultPayloadType( 96 );
	pSession.SetDefaultMark( false );

	/* Store the address */
	IPAddress oIP ( mString["destination"] );
	if( not oIP.valid() )
		RuntimeError( this, "Translated address '%s' from '%s' invalid", oIP.getAddressStr().c_str(), mString["destination"].c_str() );
	RTPIPv4Address oRTP = RTPIPv4Address( oIP.getIPArray(), oIP.getPort() );

	/* Add the destination */
	pSession.AddDestination( oRTP );

	/* Open the tracefile if specified */
	if( mString["tracefile"].size() )
	{
		oTrace.open( mString["tracefile"].c_str(), "w" );
		if( not oTrace.active() )
			RuntimeError( this, "while trying to open %s", mString["tracefile"].c_str() );
	}
	/* Cached values */
	bPts = mBool["pts"];
	bExtension = mBool["extension"];
	bForce = mBool["force-sequence-number"];

	/* Start polling */
	bSchedule = true;
	debug( 1, "rtp server(%d) client(%s)", mInt["port"], oIP.getAddressStr().c_str() );

	/* Send filler packets */
	if( bExtension and mBool["hack"] )
	{
		uint8_t pNULL [1500];
		memset( pNULL, 0, 1500 );
		if( int iStatus = pSession.SendPacketEx( pNULL, 1400, 96, true, 0, EXT_FILLER, pHeader, 1 ) )
			RuntimeError( this, RTPGetErrorString(iStatus).c_str() );
	}
}

void SirannonRTPTransmitter::receive( MediaPacketPtr& pPckt )
{
	/* Convert timestamps for audio (not for transport streams) */
	int32_t iDts = pPckt->dts, iPts = pPckt->pts;
	if( pPckt->content == content_t::audio and pPckt->mux != mux_t::TS and pPckt->desc )
	{
		iDts = av_rescale_q( pPckt->dts, toAVRational(pPckt->desc->timebase), toAVRational(1,pPckt->desc->samplerate) );
		iPts = av_rescale_q( pPckt->pts, toAVRational(pPckt->desc->timebase), toAVRational(1,pPckt->desc->samplerate) );
	}
	/* Either use DTS (RTP standard) or PTS (RTP dialect) as timing */
	int iTiming = bPts ? iPts : iDts;
	if( iPrevTiming >= 0 and iTiming != iPrevTiming )
		pSession.IncrementTimestamp( iTiming - iPrevTiming );
	iPrevTiming = iTiming;

	/* Force sequencenumber */
	if( bForce )
		pSession.packetbuilder.seqnr = pPckt->unitnumber + iForce;

	/* Send the packet */
	int iStatus = 0;
	debug( 2, "transmitting %s (dts: %d)", pPckt->c_str(), iDts );
	if( bExtension )
	{
		/* Encode Sirannon timing & put it in the header extension */
		encode_header( pPckt, pHeader );

		iStatus = pSession.SendPacketEx( pPckt->data(), pPckt->size(), calculate_payload(pPckt), pPckt->frameend, 0,
			EXT_META, pHeader, 6 );
	}
	else
	{
		iStatus = pSession.SendPacket( pPckt->data(), pPckt->size(), calculate_payload(pPckt), pPckt->frameend, 0 );
	}
	/* Check if succesful */
	if( iStatus < 0 )
		RuntimeError( this, RTPGetErrorString(iStatus).c_str() );

	/* Tracefile */
	if( oTrace.active() )
	{
		oTrace.write( "%f\tid %d\tudp %d\n",
						SirannonTime::getCurrentTime().convertDouble(),
						pSession.packetbuilder.GetSequenceNumber()-1,
						pPckt->size() );
	}
	/* Send away */
	pPckt->dts += 3600;
	route( pPckt );
}

uint32_t SirannonRTPTransmitter::getSequenceNumber( void ) const
{
	return pSession.packetbuilder.GetSequenceNumber();
}

uint32_t SirannonRTPTransmitter::getTimestamp( void ) const
{
	return pSession.packetbuilder.GetTimestamp();
}

uint32_t SirannonRTPTransmitter::getPort( void ) const
{
	return mInt.find( "port" )->second;
}

int SirannonRTPTransmitter::calculate_payload( MediaPacketPtr& pPckt )
{
	if( iPayload < 0 )
	{
		if( pPckt->mux == mux_t::TS )
			return 33;
		else if( pPckt->codec == codec_t::mp1a or pPckt->codec == codec_t::mp2a )
			return 14;
		else if( pPckt->codec == codec_t::mp1v or pPckt->codec == codec_t::mp2v )
			return 32;
		else if( pPckt->content == content_t::video )
			return 96;
		else if( pPckt->content == content_t::audio )
			return 97;
		else
			return 96;
	}
	else
		return iPayload;
}
