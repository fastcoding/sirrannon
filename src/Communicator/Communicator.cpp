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
#include "Communicator.h"
#include "Endianness.h"

/**
 * TRANSMITTER
 * @component transmitter
 * @properties abstract
 * @type core
 * @param port, int, 4000, source port
 * @param destination, string, 127.0.0.1:5000, the IP address of the receiver
 * @param buffer, int, 0, if >0, the size of the protocol buffer, your OS must still accept this value, check "UDP Buffer Sizing" in Google for more information
 * @param extension, bool, false, if true, add an additional header with sirannon frame numbers to the packet, although making it incompatible with a standard player (except for RTP)
 * @param multicast-TTL, int, -1, the TTL when sending to a multicast destination (not for TCP), -1 disables this
 * @info Transmitters provide the interface to the network for the following protocols:
 * UDP, TCP and RTP/UDP. These components send the packets without any delay or
 * buffering.
 **/

/**
 * RECEIVER
 * @component receiver
 * @properties abstract, scheduled
 * @type core
 * @param port, int, 5000, reception port
 * @param video-route, int, 100, the xroute that will be assigned to packets containing video
 * @param audio-route, int, 200, the xroute that will be assigned to packets containing audio
 * @param buffer, int, 0, if >0, the size of the protocol buffer, your OS must still accept this value, check "UDP Buffer Sizing" in Google for more information
 * @param extension, bool, false, if true, the additional sirannon header is parsed from the packet. Caveat, if the header was not present the stream will be corrupted except for RTP. Conversely, if the header was present and this value is false, the stream will be corrupted except for RTP
 * @param multicast, bool, false, if true, join a multicast address (not for TCP)
 * @param multicast-server, string, , multicast address (not for TCP)
 * @info Receivers provide the interface from the network for the following protocols:
 * UDP, TCP and RTP/UDP.
 **/

MediaTransmitter::MediaTransmitter( const string& sName, ProcessorManager* pScope )
	: MediaProcessor(sName, pScope), oHeader(128)
{
	mInt["buffer"] = 8*MEBI;
	mInt["multicast-TTL"] = -1;
}

MediaReceiver::MediaReceiver( const string& sName, ProcessorManager* pScope )
	: MediaProcessor(sName, pScope), iStream(nextStreamID()), oHeader(128),
	  iVideo(100), iAudio(200)
{
	mInt["video-route"]	= 100;
	mInt["audio-route"]	= 200;
	mInt["buffer"] = 8*MEBI;
}

void MediaTransmitter::init( void )
{
	MediaProcessor::init();
}

void MediaReceiver::init( void )
{
	MediaProcessor::init();

	iVideo = mInt["video-route"];
	iAudio = mInt["audio-route"];

	pDesc = addMedia();
	if( mPrivate["descriptor"] )
		*pDesc = *(MediaDescriptor*) mPrivate["descriptor"];
}

int MediaReceiver::decode_length( uint8_t* header )
{
	IBits oHeader( header, 2 );
	return oHeader.read( 16 );
}

int MediaTransmitter::encode_length( uint8_t* header, uint16_t size )
{
	OBits oHeader( header, 2 );
	oHeader.write( 16, size );
	return 2;
}

int MediaTransmitter::encode_length( MediaPacketPtr& pPckt )
{
	oHeader.clear();
	oHeader.write( 16, pPckt->size() );
	pPckt->push_front( oHeader.data(), oHeader.size() );
	return 2;
}

int MediaTransmitter::encode_header( const MediaPacketPtr& pckt, uint8_t* pHeader )
{
	oHeader.clear();
	oHeader.write( 32, pckt->dts );
	oHeader.write( 32, pckt->inc );
	oHeader.write( 32, pckt->unitnumber );
	oHeader.write( 32, pckt->framenumber );
	oHeader.write( 8, pckt->subframenumber );
	oHeader.write( 16, pckt->codec );
	oHeader.write( 2, pckt->type );
	oHeader.write( 2, pckt->content );
	oHeader.write( 4, pckt->mux );
	oHeader.write( 5, pckt->frame );
	oHeader.write( 1, pckt->framestart );
	oHeader.write( 1, pckt->frameend );
	oHeader.write( 1, pckt->key );
	oHeader.write( 24, 0xFFFFFF );
	memcpy( pHeader, oHeader.data(), oHeader.size() );
	return 24;
}

void MediaTransmitter::encode_header( MediaPacketPtr& pckt )
{
	oHeader.clear();
	oHeader.write( 32, pckt->dts );
	oHeader.write( 32, pckt->inc );
	oHeader.write( 32, pckt->unitnumber );
	oHeader.write( 32, pckt->framenumber );
	oHeader.write( 8, pckt->subframenumber );
	oHeader.write( 16, pckt->codec );
	oHeader.write( 2, pckt->type );
	oHeader.write( 2, pckt->content );
	oHeader.write( 4, pckt->mux );
	oHeader.write( 5, pckt->frame );
	oHeader.write( 1, pckt->framestart );
	oHeader.write( 1, pckt->frameend );
	oHeader.write( 1, pckt->key );
	pckt->push_front( oHeader.data(), oHeader.size() );
}

int MediaReceiver::decode_header( MediaPacketPtr& pckt, const uint8_t* pHeader )
{
	IBits oHeader ( pHeader, 21 );
	pckt->dts = oHeader.read( 32 );
	pckt->inc = oHeader.read( 32 );
	pckt->unitnumber = oHeader.read( 32 );
	pckt->framenumber = oHeader.read( 32 );
	pckt->subframenumber = oHeader.read( 8 );
	pckt->codec = (codec_t::type) oHeader.read( 16 );
	pckt->type = (packet_t::type) oHeader.read( 2 );
	pckt->content = (content_t::type)oHeader.read( 2 );
	pckt->mux = (mux_t::type) oHeader.read( 4 );
	pckt->frame = (frame_t::type) oHeader.read( 5 );
	pckt->framestart = (bool) oHeader.read( 1 );
	pckt->frameend = (bool) oHeader.read( 1 );
	pckt->key = (bool) oHeader.read( 1 );
	pDesc->inc = pckt->inc;
	pDesc->codec = pckt->codec;
	return 21;
}

void MediaReceiver::decodeDescriptor( MediaPacket* pPckt )
{
	pPckt->type = packet_t::media;
	pPckt->content = pDesc->content;
	pPckt->mux = mux_t::RTP;
	pPckt->codec = pDesc->codec;
	pPckt->xroute = pDesc->route;
}

void MediaReceiver::decode_additional( MediaPacketPtr& pckt )
{
	/* Fill in timing info */
	pckt->desc = pDesc;
	pckt->pts = pckt->dts;

	/* Fill in frequency & route */
	pckt->xstream = iStream;
	if( pckt->content == content_t::video )
		pckt->xroute = iVideo;
	else if( pckt->content == content_t::audio )
		pckt->xroute = iAudio;
	else
		pckt->xroute = iVideo;
}
