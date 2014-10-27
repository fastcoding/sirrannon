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
#include "TranscoderSession.h"
#include "Scheduler/Scheduler.h"
#include "Reader/Reader.h"

REGISTER_CLASS( TranscoderSession, "transcoder-session" );

TranscoderSession::TranscoderSession( const string& sName, ProcessorManager* pScope )
	: FileStreamer(sName, pScope)
{
	/* Reader */
	mString["filename"] = "";
}

static SirannonTime oStart(0);
void TranscoderSession::init( void )
{
	/* Base class, skip block base class */
	MediaProcessor::init();

	/* Current hack */
	string sFileName = mString["filename"];
	string sIP;
	int iSrc=0, iCtrl=0, iProxy=0;
	bool bAudio = false;
	if( oStart.zero() or (SirannonTime::getUpTime() + 2000 - oStart).convertMsecs() > 5000 )
		oStart = SirannonTime::getUpTime() + 2000;
	SirannonTime oStart2 = oStart;
	if( sFileName == "da-r1.flv" ) //  tbnode-n3.ifip-32-1.ibcn-video.wall.test // 10.0.173.6
	{
		sIP = "10.1.1.4";
		iSrc = 4000;
		iProxy = 5000;
		iCtrl = 6000;
		bAudio = true;
	}
	else if( sFileName == "da-r2.flv" ) //  tbnode-n11:0
	{
		sIP = "10.1.1.5";
		iSrc = 4008;
		iProxy = 5008;
		iCtrl = 6001;
		oStart2 += 4000;
	}
	else if( sFileName == "da-r3.flv" ) //  tbnode-n19:0
	{
		sIP = "10.1.1.6";
		iSrc = 4016;
		iProxy = 5016;
		iCtrl = 6002;
		oStart2 += 4000;
	}
	else if( sFileName == "da-r4.flv" ) //  tbnode-n27:0
	{
		sIP = "10.1.1.7";
		iSrc = 4024;
		iProxy = 5024;
		iCtrl = 6003;
		oStart2 += 4000;
	}
	else
		RuntimeError( this, "should not happen: '%s'", sFileName.c_str() );

	/* Create the Reader */
	oProcessorManager.createProcessor( "ffmpeg-Reader", "Reader" );
	oProcessorManager.setString( "Reader", "filename", mString["filename"] );
	oProcessorManager.setBool( "Reader", "video-mode", true );
	oProcessorManager.setBool( "Reader", "audio-mode", bAudio );
	oProcessorManager.setBool( "Reader", "sdp", false );
	oProcessorManager.setInt( "Reader", "loop", 1 );
	oProcessorManager.setBool( "Reader", "repeat-parameter-sets", true );
	oProcessorManager.setBool( "Reader", "skip-SEI", true );
	oProcessorManager.setBool( "Reader", "mov-frame", false );
	oProcessorManager.setInt(  "Reader", "dts-start", 0 );
	oProcessorManager.setBool( "Reader", "debug", false );

	/* Create Sirannon-Packetizer */
	oProcessorManager.createProcessor( "xstream-Packetizer", "Packetizer" );

	/* Create Scheduler */
	oProcessorManager.createProcessor( "basic-Scheduler", "video-Scheduler" );
	oProcessorManager.setInt( "video-Scheduler", "delay", 0 );
	oProcessorManager.setInt( "video-Scheduler", "buffer", 1000 );
	oProcessorManager.setDouble( "video-Scheduler", "speed", 20. );
	oProcessorManager.setBool( "video-Schedulerr", "pause", false );
	oProcessorManager.setBool( "video-Scheduler", "stress", true );
	oProcessorManager.setBool( "video-Scheduler", "debug", false );

	/* Create the differiator */
	oProcessorManager.createProcessor( "differentiator", "differentiator" );
	oProcessorManager.setInt( "differentiator", "nodes", 1 );
	oProcessorManager.setInt( "differentiator", "processors", 4 );
	oProcessorManager.setInt( "differentiator", "base-src-port", iSrc );
	oProcessorManager.setInt( "differentiator", "base-dst-port", 5000 );
	oProcessorManager.setInt( "differentiator", "nodes-per-lan", 1000000 );
	oProcessorManager.setBool( "differentiator", "debug", false );
	oProcessorManager.setString( "differentiator", "base-dst-ip", sIP );

	/* Create the receiver */
	oProcessorManager.createProcessor( "Proxy", "Proxy" );
	oProcessorManager.setInt( "Proxy", "streams", 4 );
	oProcessorManager.setInt( "Proxy", "base-port", iProxy );
	oProcessorManager.setInt( "Proxy", "delay", 0 );
	oProcessorManager.setBool( "Proxy", "debug", false );

	/* Create Packetizer */
	oProcessorManager.createProcessor( "RTMP-Packetizer", "video-Packetizer" );
	oProcessorManager.setBool( "video-Packetizer", "debug", false );
	oProcessorManager.setInt( "video-Packetizer", "streamID", mInt["RTMP-streamID"] );
	oProcessorManager.setInt( "video-Packetizer", "chunkID", mInt["RTMP-video-chunkID"] );
	oProcessorManager.setInt( "video-Packetizer", "chunkSize", mInt["RTMP-chunk-size"] );
	oProcessorManager.createProcessor( "RTMP-Packetizer", "audio-Packetizer" );
	oProcessorManager.setBool( "audio-Packetizer", "debug", false );
	oProcessorManager.setInt( "audio-Packetizer", "streamID", mInt["RTMP-streamID"] );
	oProcessorManager.setInt( "audio-Packetizer", "chunkID", mInt["RTMP-audio-chunkID"] );
	oProcessorManager.setInt( "audio-Packetizer", "chunkSize", mInt["RTMP-chunk-size"] );

	/* Create Multiplexer */
	if( bAudio )
	{
		oProcessorManager.createProcessor( "std-Multiplexer", "Multiplexer" );
		oProcessorManager.setInt( "Multiplexer", "delay", 0 );
		oProcessorManager.setInt( "Multiplexer", "streams", 2 );
	}
	else
		oProcessorManager.createProcessor( "in", "Multiplexer" );

	/* Create Scheduler */
	oProcessorManager.createProcessor( "basic-Scheduler", "Proxy-Scheduler" );
	oProcessorManager.setInt( "Proxy-Scheduler", "delay", 0 );
	oProcessorManager.setInt( "Proxy-Scheduler", "buffer", 10000 );
	//oProcessorManager.setInt( "Proxy-Scheduler", "absolute-delay", oStart2.convertMsecs() );
	oProcessorManager.setBool( "Proxy-Scheduler", "pause", false );
	oProcessorManager.setBool( "Proxy-Scheduler", "stress", true );
	oProcessorManager.setBool( "Proxy-Scheduler", "debug", true );

	/* Create a rate control */
	oProcessorManager.createProcessor( "rate-control", "rate-control" );
	oProcessorManager.setInt( "rate-control", "feedback-port", iCtrl );
	oProcessorManager.setInt( "rate-control", "number-of-streams", 4 );

	/* Output */
	out = oProcessorManager.createProcessor( "out", "out" );

	/* Connect the components */
	oProcessorManager.setRoute( "Reader", "Packetizer", 100 );
	oProcessorManager.setRoute( "Packetizer", "video-Scheduler", 0 );
	oProcessorManager.setRoute( "video-Scheduler", "differentiator", 0 );
	oProcessorManager.setRoute( "Proxy", "video-Packetizer", 0 );
	oProcessorManager.setRoute( "Reader", "audio-Packetizer", 200 );
	oProcessorManager.setRoute( "video-Packetizer", "Multiplexer", 0 );
	oProcessorManager.setRoute( "audio-Packetizer", "Multiplexer", 0 );
	oProcessorManager.setRoute( "Multiplexer", "Proxy-Scheduler", 0 );
	oProcessorManager.setRoute( "Proxy-Scheduler", "out", 0 );

	/* Init */
	oProcessorManager.initProcessors();
	initOut();

	/* We always schedule */
	bSchedule = true;
}

const ContainerDescriptor* TranscoderSession::getDescriptor( void ) const
{
	Reader* pReader = dynamic_cast<Reader*>( oProcessorManager.getProcessor( "Reader" ) );
	if( pReader )
		return pReader->getDescriptor();
	return NULL;
}

int TranscoderSession::play( double fSpeed )
{
	Scheduler* pScheduler = dynamic_cast<Scheduler*>( oProcessorManager.getProcessor( "Proxy-Scheduler" ) );
	if( pScheduler )
		return pScheduler->play( 20 );
	else
		return -1;
}

int TranscoderSession::pause( void )
{
	Scheduler* pScheduler = dynamic_cast<Scheduler*>( oProcessorManager.getProcessor( "Proxy-Scheduler" ) );
	if( pScheduler )
		return pScheduler->pause();
	else
		return -1;
}

int TranscoderSession::seek( uint32_t iTimeIndex )
{
	return -1;
}
