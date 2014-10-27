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
#include "FileStreamer.h"
#include "sdp.h"
#include "Frame.h"

/** HIGH LEVEL
 * @component high-level
 * @properties abstract
 * @type core
 **/

/** FILE STREAMER
 * @component streamer
 * @type high-level
 * @param filename, string, , the path of the container to stream
 * @param mode, string, default, what content to read from the container (video, audio, default)
 * @param loop, int, 1,  the number of times to play the stream, -1 being infinite, 0 interpreted as 1
 * @param seek, int, 0, the time index to seek to into the container
 * @param aggregate, bool, false, option of some packetizers
 * @param multiplexer-delay, int, 0, delay of the multiplexer, used for transport streams or RTMP
 * @param scheduler, string, basic, which sort of scheduler to use (basic, frame, gop, window)
 * @param scheduler-buffer, int, 1000, in ms, the buffer size in time of the scheduler
 * @param scheduler-delay, int, 0, in ms, the delay of the scheduler
 * @param loss, double, 0.0, if larger than 0, randomly lose packets with the give chance
 * @param transmitter, string, rtp, type of the transmitter(rtp, udp, tcp)
 * @param port, int, 4000, source port of the transmitter
 * @param destination, string, 127.0.0.1:5000, destination of the transmitter
 * @param ts-mode, bool, false, if true, multiplex the streams into a transport stream
 * @param RTMP-mode, bool, false, if true, muliplex the streams into RTMP chunk streams as output from this component
 * @param RTMP-streamID, int, 1, streamID used for each RTMP chunk stream
 * @param RTMP-video-chunkID, int, 7, RTMP chunk stream ID for video
 * @param RTMP-audio-chunkID, int, 6, RTMP chunk stream ID for audio
 * @param RTMP-chunk-size, int, 4096, chunk size for the RTMP chunk streams
 * @info This component provides a streaming solution without having to construct
 * a scheme of components. Its many parameters reflect the options of the hidden
 * underlying components (delay, port, destination\textellipsis).
 **/
REGISTER_CLASSES( FileStreamer, "streamer", 1 );


FileStreamer::FileStreamer( const string& sName, ProcessorManager* pScope )
	: Block(sName, pScope), bVideo(true), bAudio(true)
{
	/* source */
	mString["filename"] = "";
	mString["mode"] = "default";
	mInt["loop"] = 1;
	mInt["seek"] = 0;

	/* packetizer */
	mBool["aggregate"] = false;

	/* multiplexer */
	mBool["ts-mode"] = false;
	mBool["RTMP-mode"] = false;
	mBool["RTMP-streamID"] = 1;
	mInt["RTMP-video-chunkID"] = 7;
	mInt["RTMP-audio-chunkID"] = 6;
	mInt["RTMP-chunk-size"] = 4096;
	mInt["multiplexer-delay"] = 0;

	/* scheduler */
	mString["scheduler"] = "window";
	mBool["pause"] = false; /* not in gui */
	mInt["scheduler-buffer"] = 1000;
	mInt["scheduler-delay"] = 0;

	/* Dropper */
	mDouble["loss"] = 0.0;

	/* Transmitter */
	mString["transmitter"] = "rtp";
	mInt["port"] = 4000;
	mString["destination"] = "127.0.0.1:5000";
	mInt["payload"] = -1; /* not in gui */
	mBool["pts"] = false; /* not in gui */
	mBool["extension"] = false; /* not in gui */
}

void FileStreamer::init( void )
{
	/* Base class, skip Block base class */
	MediaProcessor::init();

	/* If there is no server pointer init ourselves */
	if( mPrivate.count( "session" ) == 0 )
		createComponents();
}

void FileStreamer::createSource( void )
{
	/* Reader or player? */
	codec_t::type iVideo = codec_t::NO, iAudio = codec_t::NO;

	/* Analyse the file */
	char sBuffer [1024];
	ContainerDescriptor oContainer;
	mux_t::type oMux;
	if( SDPConstruct( mString["filename"], 0, false, &oContainer, oMux ) < 0 )
		RuntimeError( this, "could not read from file '%s'", mString["filename"].c_str() );

	/* Translate mode */
	bVideo = bAudio = false;
	if( mString["mode"] == "video" )
	{
		bVideo = true;
	}
	else if( mString["mode"] == "audio" )
	{
		bAudio = true;
	}
	else if( mString["mode"] == "default" or mString["mode"] == "" )
	{
		bVideo = bAudio = true;
	}
	else
		RuntimeError( this, "invalid mode type '%s', options: 'video', 'audio' or 'default'", mString["mode"].c_str() );

	const MediaDescriptor* pDesc;
	pDesc = oContainer.getVideoDescriptor();
	if( pDesc and bVideo )
		iVideo = pDesc->codec;
	else
		bVideo = false;

	pDesc = oContainer.getAudioDescriptor();
	if( pDesc and bAudio )
		iAudio = pDesc->codec;
	else
		bAudio = false;

	/* Some checks */
	if( bVideo and bAudio and ( not mBool["ts-mode"] and not mBool["RTMP-mode"] ) )
		RuntimeError( this, "cannot both stream video and audio without transport streams" );

	if( not bVideo and not bAudio )
		RuntimeError( this, "no requested %s-content present in '%s'", mString["mode"].c_str(), mString["filename"].c_str() );

	if( oContainer.empty() )
		RuntimeError( this, "no supported content in '%s'", mString["filename"].c_str() );

	/* Create the source */
	//if( oMux == mux_t::ES and ( oContainer[0].codec == codec_t::avc or oContainer[0].codec == codec_t::svc ) )
	//	pSource = dynamic_cast<SourceInterface*>( oProcessorManager.createProcessor( "avc-reader", "source" ) );
	//else
		pSource = dynamic_cast<SourceInterface*>( oProcessorManager.createProcessor( "ffmpeg-reader", "source" ) );
	if( not pSource )
		RuntimeError( this, "Reader not of type(SourceInterface)" );

	oProcessorManager.setString( "source", "filename", mString["filename"] );
	oProcessorManager.setBool( "source", "video-mode", bVideo );
	oProcessorManager.setBool( "source", "audio-mode", bAudio );
	oProcessorManager.setInt( "source", "seek", mInt["seek"] );
	oProcessorManager.setBool( "source", "sdp", false );
	oProcessorManager.setInt( "source", "loop", mInt["loop"] );
	oProcessorManager.setBool( "source", "repeat-parameter-sets", true );
	oProcessorManager.setBool( "source", "skip-SEI", true );
	oProcessorManager.setBool( "source", "debug", false );
	oProcessorManager.setBool( "source", "mov-frame", mBool["RTMP-mode"] );
	oProcessorManager.setInt(  "source", "dts-start", mBool["RTMP-mode"] ? 0 : 1000 );

	/* Create packetizer */
	if( mBool["ts-mode"] )
	{
		oProcessorManager.createProcessor( "pes-packetizer", "video-packetizer" );
		oProcessorManager.createProcessor( "pes-packetizer", "audio-packetizer" );
	}
	else if( mBool["RTMP-mode"] )
	{
		oProcessorManager.createProcessor( "RTMP-packetizer", "video-packetizer" );
		oProcessorManager.createProcessor( "RTMP-packetizer", "audio-packetizer" );
		oProcessorManager.setBool( "video-packetizer", "debug", false );
		oProcessorManager.setInt( "video-packetizer", "stream-ID", mInt["RTMP-streamID"] );
		oProcessorManager.setInt( "video-packetizer", "chunk-ID", mInt["RTMP-video-chunkID"] );
		oProcessorManager.setInt( "video-packetizer", "chunk-size", mInt["RTMP-chunk-size"] );
		oProcessorManager.setInt( "audio-packetizer", "stream-ID", mInt["RTMP-streamID"] );
		oProcessorManager.setInt( "audio-packetizer", "chunk-ID", mInt["RTMP-audio-chunkID"] );
		oProcessorManager.setInt( "audio-packetizer", "chunk-size", mInt["RTMP-chunk-size"] );
	}
	else
	{
		if( bVideo )
		{
			switch( iVideo )
			{
			case codec_t::mp4v:
				oProcessorManager.createProcessor( "mp4-packetizer", "video-packetizer" );
				break;

			case codec_t::mp1v:
			case codec_t::mp2v:
				oProcessorManager.createProcessor( "mp2-packetizer", "video-packetizer" );
				break;

			case codec_t::avc:
			case codec_t::svc:
			case codec_t::mvc:
				oProcessorManager.createProcessor( "avc-packetizer", "video-packetizer" );
				break;

			default:
				RuntimeError( this, "no packetizer for codec '%s'", CodecToString(iVideo) );
			}
		}
		else
			oProcessorManager.createProcessor( "out", "video-packetizer" );

		if( bAudio )
		{
			switch( iAudio )
			{
			case codec_t::mp4a:
				oProcessorManager.createProcessor( "mp4-packetizer", "audio-packetizer" );
				break;

			case codec_t::mp1a:
			case codec_t::mp2a:
				oProcessorManager.createProcessor( "mp2-packetizer", "audio-packetizer" );
				break;

			case codec_t::anb:
			case codec_t::awb:
				oProcessorManager.createProcessor( "AMR-packetizer", "audio-packetizer" );
				break;

			case codec_t::ac3:
				oProcessorManager.createProcessor( "AC3-packetizer", "audio-packetizer" );
				break;

			default:
				RuntimeError( this, "no packetizer for codec '%s'", CodecToString(iAudio) );
			}
		}
		else
			oProcessorManager.createProcessor( "out", "audio-packetizer" );
	}
}

void FileStreamer::createBuffer( void )
{
	/* Create the multiplexer */
	if( mBool["ts-mode"] )
		oProcessorManager.createProcessor( "ts-multiplexer", "multiplexer" );
	else if( mBool["RTMP-mode"] )
		oProcessorManager.createProcessor( "in", "multiplexer" );
	else
		oProcessorManager.createProcessor( "in", "multiplexer" );
	oProcessorManager.setBool( "multiplexer", "debug", false );
	oProcessorManager.setBool( "multiplexer", "delay", mInt["multiplexer-delay"] );

	/* Create the scheduler */
	char sBuffer [64];
	snprintf( sBuffer, sizeof(sBuffer), "%s-scheduler", mString["scheduler"].c_str() );
	pBuffer = dynamic_cast<BufferInterface*>( oProcessorManager.createProcessor( sBuffer, "scheduler" ) );
	if( not pBuffer )
		RuntimeError( this, "Scheduler not of type(BufferInterface)" );
	oProcessorManager.setInt( "scheduler", "delay", mInt["scheduler-delay"] );
	oProcessorManager.setInt( "scheduler", "buffer", mInt["scheduler-buffer"] );
	oProcessorManager.setBool( "scheduler", "pause", mBool["pause"] );
	oProcessorManager.setBool( "scheduler", "debug", false );
}

void FileStreamer::createTransmitter( MediaDescriptor* pDesc )
{
	/* Create the transmitter */
	char sBuffer [64];
	snprintf( sBuffer, sizeof(sBuffer), "%s-transmitter", mString["transmitter"].c_str() );
	if( not mBool["RTMP-mode"] and mString["transmitter"] != "none" )
	{
		MediaProcessor* pProcessor = oProcessorManager.createProcessor( sBuffer, "transmitter" );
		if( not pProcessor )
			RuntimeError( this, "invalid transmitter type: %s", sBuffer );
		pVideoTransmitter = dynamic_cast<TransmitterInterface*>( pProcessor );
		oProcessorManager.setInt( "transmitter", "port", mInt["port"] );
		oProcessorManager.setString( "transmitter", "destination", mString["destination"] );
		oProcessorManager.setInt( "transmitter", "payload", mInt["payload"] );
		oProcessorManager.setBool( "transmitter", "pts", mBool["pts"] );
		oProcessorManager.setBool( "transmitter", "extension", mBool["extension"] );
		oProcessorManager.setBool( "transmitter", "debug", mBool["debug"] );
	}
	else
	{
		oProcessorManager.createProcessor( "in", "transmitter" );
	}
	/* Create the frame dropper */
	if( mDouble["loss"] > 0.0 )
	{
		oProcessorManager.createProcessor( "random-classifier", "classifier" );
		oProcessorManager.setDouble( "classifier", "P(loss)", mDouble["loss"] );
		oProcessorManager.setBool( "classifier", "discard", true );
	}
	else
	{
		oProcessorManager.createProcessor( "in", "classifier" );
	}

	/* Create the sink */
	oProcessorManager.createProcessor( "sink", "sink" );
	oProcessorManager.setBool( "sink", "audio-mode", bAudio );
	oProcessorManager.setBool( "sink", "video-mode", bVideo );
	oProcessorManager.setBool( "sink", "debug", true );

	/* Output */
	out = oProcessorManager.createProcessor( "out", "out" );

	/* Connect the components */
	oProcessorManager.setRoute( "source", "video-packetizer", 100 );
	oProcessorManager.setRoute( "source", "audio-packetizer", 200 );
	oProcessorManager.setRoute( "video-packetizer", "multiplexer", 0 );
	oProcessorManager.setRoute( "audio-packetizer", "multiplexer", 0 );
	oProcessorManager.setRoute( "multiplexer", "scheduler", 0 );
	oProcessorManager.setRoute( "scheduler", "classifier", 0 );
	oProcessorManager.setRoute( "classifier", "transmitter", 0 );
	oProcessorManager.setRoute( "transmitter", "sink", 0 );
	oProcessorManager.setRoute( "sink", "out", 0 );

	/* Init */
	oProcessorManager.initProcessors();
	initOut();

	/* We always schedule */
	bSchedule = true;
}

const ContainerDescriptor* FileStreamer::getDescriptor( void ) const
{
	return pSource->getDescriptor();
}

bool FileStreamer::ready( void ) const
{
	return pSource->ready();
}

void FileStreamer::process( void )
{
	/* Schedule the processors */
	if( not oProcessorManager.scheduleProcessors() )
	{
		/* Terminate */
		end();
	}
}

int FileStreamer::play( double fSpeed ) synchronized
{
	/* SOurce */
	BufferInterface* pProcessor;
	pProcessor = dynamic_cast<BufferInterface*>( pSource );
	if( pProcessor )
	{
		if( pProcessor->play( fSpeed ) < 0 )
			RuntimeError( this, "Play failed" );
	}
	/* Scheduler */
	if( pBuffer )
	{
		if( pBuffer->play( fSpeed ) < 0 )
			RuntimeError( this, "Play failed" );
	}
	return 0;
} end_synchronized

int FileStreamer::pause( void ) synchronized
{
	BufferInterface* pProcessor;

	debug( 1, "pause" );
	pProcessor = dynamic_cast<BufferInterface*>( pSource );
	if( pProcessor )
		if( pProcessor->pause() < 0 )
			RuntimeError( this, "Pause failed" );

	pProcessor = dynamic_cast<BufferInterface*>( pBuffer );
	if( pProcessor )
		if( pProcessor->pause() < 0 )
			RuntimeError( this, "Pause failed" );
	return 0;
} end_synchronized

int FileStreamer::seek( uint32_t iTimeIndex ) synchronized
{
	SourceInterface* pProcessor;

	pProcessor = dynamic_cast<SourceInterface*>( pSource );
	if( pProcessor )
		if( pProcessor->seek( iTimeIndex ) < 0 )
			RuntimeError( this, "Seek failed" );

	pProcessor = dynamic_cast<SourceInterface*>( pBuffer );
	if( pProcessor )
		if( pProcessor->seek( iTimeIndex ) < 0 )
			RuntimeError( this, "Seek failed" );
	return 0;
} end_synchronized
