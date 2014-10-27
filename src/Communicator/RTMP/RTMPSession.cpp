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
#include "RTMPSession.h"
#include "Scheduler/Scheduler.h"
#include "RandomGenerator.h"
#include "Flash.h"
#include "Frame.h"
#include "Url.h"
#include "OSSupport.h"

REGISTER_CLASS( RTMPSession, "RTMP-session" );

RTMPSession::RTMPSession( const char* sName, ProcessorManager* pScope )
	: MediaSession(sName, pScope),
	  iServerState(UINITIALIZED),
	  bFirstCommandChunk(true), iTransID(0), iStart(-1), iDuration(-1),
	  pStreamer(NULL), iCapabilities(31), iAMF(AMF::AMF0), iClientID(oRandom.next()), iMessages(0),
	  iAudioID(6), iVideoID(5), bVideo(false), bAudio(false), bPauseAfterSeek(false),
	  oServerEpoch(0), bCrypto(false), iSeek(-1), iMainStream(content_t::video),
	  bPause(false), iSeekTarget(0), iFinalTimestamp(0), iStreamer(0), bUnpause(false),
	  sApp("streamer"), iCurrent(0), fQuickBuffer(-1.)
{
	mBool["thread"] = true;
	mString["streamer"] = "streamer";
}

RTMPSession::~RTMPSession()
{ }

void RTMPSession::init( void )
{
	/* Base class */
	MediaSession::init();

	/* Always schedule */
	bSchedule = true;

	/* Parse pointer data */
	sApp = mString["streamer"];
	RTMPChunkStream::init( MediaSession::pConnection );

	/* Create another trhead for the control connection */
	createThread( bind( &RTMPSession::mainThreadA, this ) );

	debug( 1, "setup session with streamID(%d)", iStreamID );
}

void RTMPSession::process( void )
{
	MediaSession::process();

	/* Ping */
	if( bPing )
	{
		if( (SirannonTime::getCurrentTime() - oLastPing).convertMsecs() > 2000 )
		{
			 if( sendUserControl( PING_REQUEST, SirannonTime::getCurrentTime().convertMsecs() ) )
			 {
				SirannonWarning( this,  "ping, could not send ping" );
				end();
				return;
			 }
			 oLastPing = SirannonTime::getCurrentTime();
		}
	}
	/* Seek */
	if( iSeek >= 0 )
		handleSeek( );
}

void RTMPSession::receive_end( MediaPacketPtr& pPckt )
{
	debug( 1, "Stream END, %s", pPckt->c_str() );

	iFinalTimestamp = pPckt->dts / 90;

	if( handleStreamEnd( ) )
	{
		SirannonWarning( this,  "Socket error, failed to send" );
		route( pPckt );
		end();
		return;
	}
}

void RTMPSession::receive_reset( MediaPacketPtr& pPckt )
{
	/* Ignore reset */
	route( pPckt );
}

void RTMPSession::receive( MediaPacketPtr& pPckt )
{
	/* Log the time */
	iCurrent = pPckt->dts / 90;

	/* Stop fast seeking? */
	if( iSeekTarget )
	{
		if( pPckt->dts / 90 >= iSeekTarget )
		{
			debug( 1, "fast seek buffering completed: %d -> %d", iSeekTarget - iOtherBuffer, iSeekTarget );
			iSeekTarget = 0;
			streamPlay();
		}
	}
	/* While seeking during pause only send one frame after the pause */
	if( bPauseAfterSeek )
		return;
	else if( bPause )
	{
		if( pPckt->content == iMainStream and pPckt->frameend )
		{
			bPauseAfterSeek = true;
			streamPause();
		}
	}
	/* Debug */
	//debug( 2, "Sending %s", pPckt->c_str() );

	/* Send the packet */
	int iStatus;
	if( (iStatus = sendToSocket( pPckt->data(), pPckt->size() ) ) <= 0 )
	{
		SirannonWarning( this,  "Socket status(%d) errno(%d)", iStatus, errno );
		route( pPckt );
		end();
		return;
	}
	/* Done */
	route( pPckt );
}

void RTMPSession::mainThreadA( void )
{
	/* Setup up the connections (chunkID 2 & 3)*/
	if( handleHandshake() )
		return end();

	if( handleApplicationConnect() )
		return end();

	/* Receive messages */
	while( true )
	{
		if( not receiveMessage() )
		{
			SirannonWarning( this,  "handle streaming, could not process new message" );
			return end();
		}
	}
}

void RTMPSession::handleError( SirannonException* pException, ProcessorManager* pManager, MediaProcessor* pProcessor )
{
	/* The streamer produced an Error( this, "RuntimeError", e.g. file does not exist, improper codec, etc.) */
	/* Send error to client */
	SirannonWarning( this,  "Handling %s", pException->what() );
	sendCommand( "NetStream.Failed", "RuntimeError" );

	/* We cannot continue now */
	end();
}

int RTMPSession::initStream( const string& sRequestedStream )
{
	sMedia = sRequestedStream;
	string sUrl;
	sUrl.append( "/" );
	sUrl.append( sApp );
	sUrl.append( "/" );
	sUrl.append( sMedia );
	URL_parse( sUrl, sApp, oFormat, iTarget, sMedia );
	debug( 1, "Client requested url(%s) app(%s) media(%s)", sUrl.c_str(), sApp.c_str(), sMedia.c_str() );

	/* Find the file */
	sFileName = pServer->getPath( sMedia );
	return 0;
}

int RTMPSession::createStream( void )
{
	/* Parameters for creating the block */
	vector<pair<MediaProcessor*,int> > oOut;
	map<string,bool> mBool;
	map<string,string> mString;
	map<string,int> mInt;
	map<string,double> mDouble;
	map<string,void*> mPrivate;

	/* Streamer options */
	oOut.push_back( pair<MediaProcessor*,int>( this, 0 ) ); // Connect the streamer with ourselves
	mString["filename"] = sFileName;
	mString["url"] = sMedia;
	mInt["loop"] = 1;
	mString["mode"] = "default";
	mBool["aggregate"] = false;
	mBool["rtmp-mode"] = true;
	mBool["ts-mode"] = false;
	mString["scheduler"] = "frame";
	mBool["pause"] = true;
	mInt["scheduler-buffer"] = MAX(iOtherBuffer,1000);
	mInt["multiplexer-delay"] = 0;
	mInt["scheduler-delay"] = (sApp == "LIVE") ? 1000 : 0;
	mInt["RTMP-streamID"] = 1;
	mInt["RTMP-audio-chunkID"] = 6; //iAudioID;
	mInt["RTMP-video-chunkID"] = 5; // iVideoID;
	mInt["RTMP-chunk-size"] = 1024; // iSelfChunkSize;
	mBool["RTMP-mode"] = true;
	mBool["debug"] = bDebug;
	mPrivate["session"] = this;
	mInt["mode"] = MediaServer::RTMP;
	mInt["format"] = mux_t::RTMP;
	mInt["target"] = iTarget;

	/* Kill the current streamer */
	if( pStreamer )
		oProcessorManager.stopProcessor( pStreamer->str() );

	/* Name */
	char sStreamerName [32];
	sprintf( sStreamerName, "streamer-%d", iStreamer++ );

	/* Create the block */
	pStreamer = oProcessorManager.createProcessorDynamic(
			sApp, sStreamerName,
			NULL, &oOut,
			&mInt, &mDouble, &mString, &mBool, &mPrivate );
	pStreamerInterface = dynamic_cast<StreamerInterface*>( pStreamer );
	if( not pStreamer or not pStreamerInterface )
		return SirannonWarning( this,  "Could not create source" );

	/* Activate */
	pStreamerInterface->createComponents();
	return 0;
}

int RTMPSession::streamPlay( double fSpeed )
{
	debug( 1, "playing" );
	if( pStreamer )
		return pStreamerInterface->play( fSpeed );
	return 0;
}

int RTMPSession::streamPause( void )
{
	debug( 1, "pausing" );
	if( pStreamer )
		return pStreamerInterface->pause();
	return 0;
}

int RTMPSession::streamSeek( uint32_t iSeek )
{
	/* Firstly flush the current streamer and pause it so no messages arrive before
	 * the RTMP response is sent */
	/* This only works because:
	 * 		1: pStreamer is scheduled by us
	 * 		2: pStreamer contains no threads
	 * 		3: pStreamer is not active atm
	 */
	SirannonTime oCurrent = SirannonTime::getCurrentTime();
	if( not pStreamer )
		return 0;

	if( pStreamer->flush() )
		return -1;
	if( flush() )
		return -1;
	if( pStreamerInterface->pause() )
		return -1;
	if( pStreamerInterface->seek( iSeek ) )
		return -1;

	/* Check how long the seek took */
	int iTime = (SirannonTime::getCurrentTime() - oCurrent).convertMsecs();
	debug( 1, "seeking complete time(%d ms)", iTime );
	return 0;
}

int RTMPSession::handleHandshake( void )
{
	/* Buffers */
	Lock_t oLock( oSendMutex );
	oSendMessage.clear();

	/* Receive the C0 */
	IBits oReceiveMessage( 4096 );
	if( receiveFromSocket( oReceiveMessage.data(), 1 ) <= 0 )
		return SirannonWarning( this,  "handshake, failed to receive chunk C0" );
	debug( 1, "received C0" );

	/* Parse C0 */
	oReceiveMessage.clear();
	uint8_t iVersion = oReceiveMessage.read( 8  );

	/* Check version */
	if( iVersion < 3 )
		return SirannonWarning( this,  "handshake, chunk C0, old version (%hhu) not supported, version (3) required", iVersion );
	else if( iVersion == 3 )
		debug( 1, "client RTMP version 3" );
	else if( iVersion < 32 )
		SirannonWarning( this,  "handshake, chunk C0, version (%hhu) not supported, degrading to version (3)");
	else
		return SirannonWarning( this,  "handshake, chunk C0, illegal version (%hhX)", iVersion );

	/* Receive the C1 */
	if( receiveFromSocket( oReceiveMessage.data(), oHandshake.HANDSHAKE_SIZE ) <= 0 )
		return SirannonWarning( this,  "handshake, failed to recive chunk C1" );
	//SirannonTime oC1Time = SirannonTime::getCurrentTime();
	debug( 1, "received C1" );

	/* Send S0 */
	oSendMessage.clear();
	oSendMessage.write( 8, iRTMPVersion );
	if( sendToSocket( oSendMessage.data(), oSendMessage.size() ) <= 0 )
		return SirannonWarning( this,  "handshake, failed to send chunk S0" );
	debug( 1, "sent S0" );

	/* Send S1 */
	const uint8_t* S1 = oHandshake.generateS1( oReceiveMessage.data(), oHandshake.HANDSHAKE_SIZE );
	if( sendToSocket( S1, oHandshake.HANDSHAKE_SIZE ) <= 0 )
		return SirannonWarning( this,  "handshake, failed to send chunk S1" );
	if( oHandshake.isClientCrypto() )
		debug( 1, "Detected cryptographical handshake" );
	debug( 1, "sent S1" );

	/* Send S2 */
	if( sendToSocket( oHandshake.generateS2(), oHandshake.HANDSHAKE_SIZE ) <= 0 )
		return SirannonWarning( this,  "handshake, failed to send chunk S2" );
	debug( 1, "sent S2" );

	/* Receive C2 */
	if( receiveFromSocket( oReceiveMessage.data(), oHandshake.HANDSHAKE_SIZE ) <= 0 )
		return SirannonWarning( this,  "handshake, failed to receive chunk C2" );
	debug( 1, "received C2" );

	/* Parse C2 */
	if( not oHandshake.validateC2( oReceiveMessage.data(), oHandshake.HANDSHAKE_SIZE ) )
		debug( 1, "Invalid %sC2, ignoring violation", oHandshake.isServerCrypto() ? "crypto-":"" );
	else
		debug( 1, "Valid %sC2", oHandshake.isServerCrypto() ? "crypto-":"" );
	return 0;
}

int RTMPSession::handleApplicationConnect( void )
{
	/* Wait for the "connect" command */
	if( receiveMessages( COMMAND_AMF0, "connect" ) )
		return SirannonWarning( this,  "failed to process command 'connect'" );

	/* Send */
	if( sendWindowACKSize() )
		return SirannonWarning( this,  "failed to send Window ACK Size");

	if( sendSetPeerBandwidth() )
		return SirannonWarning( this,  "failed to send Set Peer Bandwidth");

	/* Send User control message */
	if( sendUserControl( STREAM_BEGIN, 0 ) )
		return SirannonWarning( this,  "failed to send control message 'User Control' of type STREAM_BEGIN" );

	/* Respond with succes */
	if( sendCommandConnectResponse( false ) )
		return SirannonWarning( this,  "failed to send command message response 'connect'" );

	/* And send an ACK */
	if( sendACK() )
		return SirannonWarning( this,  "failed to send control response 'ACK'" );

	/* Receive the createStream command */
	if( receiveMessages( iAMF == AMF::AMF0 ? COMMAND_AMF0 : COMMAND_AMF3, "createStream" ) )
		return SirannonWarning( this,  "failed to process (NetStream.CreateStream)" );

	/* Send response */
	if( sendCommandCreateStreamResponse( false ) )
		return SirannonWarning( this,  "failed to send (NetStream.CreateStream.Response)'" );
	return 0;
}

int RTMPSession::handlePlay( const string& sRequestedStream )
{
	/* Create the streamer block */
	if( initStream( sRequestedStream ) or createStream() )
	{
		/* Respond with failure */
		if( sendCommand( "NetStream.Play.StreamNotFound" ) )
			return SirannonWarning( this,  "play, failed to send message 'NetStream.Play.NotFound'" );
		return -1;
	}
	/* This function blocks untill the source is ready */
	while( not pStreamerInterface->ready() )
		oQuantum.sleep();

	/* Reset all pause and seeks */
	iSeek = -1;
	bPause = bPauseAfterSeek = false;
	iCurrent = 0;

	/* Set chunk size */
	if( sendSetChunkSize( 1024 ) )
		return SirannonWarning( this,  "play, failed to send control message 'setChunkSize'" );

	/* Two user controls */
	if( sendUserControl( STREAM_IS_RECORDED, 1 ) )
		return SirannonWarning( this,  "play, failed to send message 'STREAM_IS_RECORDED'" );

	if( sendUserControl( STREAM_BEGIN, 1 ) )
		return SirannonWarning( this,  "play, failed to send message 'STREAM_BEGIN'" );

	/* Respond with succes */
	string sDesc1 = string("Playing and resetting ") + sFileName + ".";
	string sDesc2 = string("Started playing ") + sFileName + ".";

	if( sendCommand( "NetStream.Play.Reset", sDesc1.c_str() ) )
		return SirannonWarning( this,  "play, failed to send message 'NetStream.Play.Reset'" );

	if( sendCommand( "NetStream.Play.Start", sDesc2.c_str()  ) )
		return SirannonWarning( this,  "play, failed to send command 'NetStream.Play.Start'" );

	/* Meta data */
	if( sendMetaData() )
		return SirannonWarning( this,  "play, failed to send meta-data message 'onMetaData'" );

	/* Effective play */
	streamPlay( fQuickBuffer );
	//iSeekTarget = iOtherBuffer;

	return 0;
}

int RTMPSession::handleSeek( void )
{
	/* Effective */
	int iLocalSeek = iSeek;
	iSeek = -1;

	/* Do the effective streamer seek */
	if( streamSeek( iLocalSeek ) )
	{
		/* Notify */
		if( sendCommand( "NetStream.Seek.Failed" ) )
			return SirannonWarning( this,  "command seek, could not send (NetStream.Seek.Failed)" );
		SirannonWarning( this,  "command seek, could not seek" );
		return 0;
	}
	/* Three user controls */
	if( sendUserControl( STREAM_EOF, 1 ) )
		return SirannonWarning( this,  "seek/unpause, failed to send (UserControl.Stream.EOF)" );

	if( sendUserControl( STREAM_IS_RECORDED, 1 ) )
		return SirannonWarning( this,  "seek/unpause, failed to send (UserControl.Stream.IsRecorded)" );

	if( sendUserControl( STREAM_BEGIN, 1 ) )
		return SirannonWarning( this,  "seek/unpause, failed to send message (UserControl.Stream.Begin)" );

	char sDesc [2048];
	iCurrent = iLocalSeek;
	if( bUnpause )
	{
		/* Notify */
		snprintf( sDesc, sizeof(sDesc), "Seeking to %d (stream ID: %d).", iLocalSeek, iStreamID );
		if( sendCommand( "NetStream.Unpause.Notify", sDesc ) )
			return SirannonWarning( this,  "seek/unpause, failed to send (NetStream.Unpause.Notify)" );
	}
	else
	{
		/* Notify */
		snprintf( sDesc, sizeof(sDesc), "Unpausing %s.", sMedia.c_str() );
		if( sendCommand( "NetStream.Seek.Notify", sDesc ) )
			return SirannonWarning( this,  "seek/unpause, failed to send (NetStream.Seek.Notify)" );
	}
	/* Play */
	snprintf( sDesc, sizeof(sDesc), "Playing %s.", sMedia.c_str() );
	if( sendCommand( "NetStream.Play.Start", sDesc ) )
		return SirannonWarning( this,  "seek/unpause, failed to send command 'NetStream.Play.Start'" );

	/* Meta data */
	if( sendMetaData() )
		return SirannonWarning( this,  "seek/unpause, failed to send meta-data message 'onMetaData'" );

	/* Play if not paused */
	//iSeekTarget = iLocalSeek + iOtherBuffer;
	if( streamPlay( fQuickBuffer ) < 0 )
			return -1;
	bUnpause = bPauseAfterSeek = false;

	return 0;
}

int RTMPSession::handleStreamEnd( void )
{
	/* Send notification */
	{
		Lock_t oLock( oSendMutex );
		oSendMessage.clear();
		oSendMessage.writeAMF_String( AMF::AMF0, "onPlayStatus" );
		oSendMessage.writeAMF_ObjectStart( AMF::AMF0 );
		oSendMessage.writeAMF_ObjectNumber( AMF::AMF0, "duration", 0 ); // 1
		oSendMessage.writeAMF_ObjectNumber( AMF::AMF0, "bytes", 0 ); // iSendBytes
		oSendMessage.writeAMF_ObjectString( AMF::AMF0, "level", "status" );
		oSendMessage.writeAMF_ObjectString( AMF::AMF0, "code", "NetStream.Play.Complete" );
		oSendMessage.writeAMF_ObjectEnd( AMF::AMF0 );
		debug( 1, "SENDING, NetStream.Play.Complete" );
		if( sendMessage( oSendMessage, METADATA_AMF0, 4, iStreamID, iFinalTimestamp ) )
			return SirannonWarning( this,  "stream end, could not send notification NetStream.Play.Complete" );
	}
	/* Send USER control */
	if( sendUserControl( STREAM_EOF, iStreamID ) )
		return SirannonWarning( this,  "stream end, could not send STREAM_EOF" );

	/* Send notification 2 */
	string sDesc = string("Stopped playing ") + sFileName + ".";
	if( sendCommand( "NetStream.Play.Stop", sDesc.c_str() ) )
		return SirannonWarning( this,  "stream end, could not sent notification NetStream.Play.Stop" );

	/* End of stream */
	return 0;
}

int RTMPSession::handleCommandAMF0( message_t* pMessage )
{
	AMFIBits& oReceiveMessage = pMessage->oBuffer;

	/* Save command name */
	pMessage->command = oReceiveMessage.readAMF_String( AMF::AMF0 );

	if( pMessage->command == "connect" )
		return handleCommandConnect( AMF::AMF0, oReceiveMessage );
	else if( pMessage->command == "createStream")
		return handleCommandCreateStream( AMF::AMF0, oReceiveMessage );
	else if( pMessage->command == "play" )
		return handleCommandPlay( AMF::AMF0, oReceiveMessage, pMessage->message_length );
	else if( pMessage->command == "pause" )
		return handleCommandPause( iAMF, oReceiveMessage );
	else if( pMessage->command == "pauseRaw" )
		return handleCommandPauseRaw( iAMF, oReceiveMessage );
	else if( pMessage->command == "getStreamLength" )
		return handleCommandStreamLength( AMF::AMF0, oReceiveMessage );
	else if( pMessage->command == "seek" )
		return handleCommandSeek( AMF::AMF0, oReceiveMessage );
	else if( pMessage->command == "deleteStream" )
		return handleCommandDeleteStream( AMF::AMF0, oReceiveMessage );
	else
		SirannonWarning( this,  "command message AMF0, ignoring command %s\n%s",
				pMessage->command.c_str(), oReceiveMessage.str(pMessage->message_length).c_str() );
	return 0;
}

int RTMPSession::handleCommandAMF3( message_t* pMessage )
{
	AMFIBits& oReceiveMessage = pMessage->oBuffer;
	/* Don't ask me why but skip the first byte */
	AMF::version iAMF = AMF::AMF0;
	if( oReceiveMessage.peek( 8 ) )
		iAMF = AMF::AMF3;
	else
		oReceiveMessage.seek( 1 );

	/* Save command name */
	pMessage->command = oReceiveMessage.readAMF_String( iAMF );

	if( pMessage->command == "connect" )
		return handleCommandConnect( iAMF, oReceiveMessage );
	else if( pMessage->command == "createStream")
		return handleCommandCreateStream( iAMF, oReceiveMessage );
	else if( pMessage->command == "play" )
		return handleCommandPlay( iAMF, oReceiveMessage, pMessage->message_length );
	else if( pMessage->command == "pause" )
		return handleCommandPause( iAMF, oReceiveMessage );
	else if( pMessage->command == "pauseRaw" )
		return handleCommandPauseRaw( iAMF, oReceiveMessage );
	else if( pMessage->command == "seek" )
		return handleCommandSeek( iAMF, oReceiveMessage );
	else if( pMessage->command == "getStreamLength" )
		return handleCommandStreamLength( iAMF, oReceiveMessage );
	else if( pMessage->command == "deleteStream" )
		return handleCommandDeleteStream( iAMF, oReceiveMessage );
	else
		SirannonWarning( this,  "command message AMF0, ignoring command %s\n%s",
						pMessage->command.c_str(), oReceiveMessage.str(pMessage->message_length).c_str() );
	return 0;
}

int RTMPSession::handleCommandConnect( AMF::version iAMF, AMFIBits& oReceiveMessage )
{
	/* Transcaction ID */
	uint64_t iTransID = oReceiveMessage.readAMF_Number( iAMF );
	if( iTransID != 1 )
		return SirannonWarning( this,  "command message 'connect', expected transcation ID 1, received %d", iTransID );

	/* Request */
	string sFlashVer, sSwfUrl, sTcUrl, sPageUrl, sObjectEncoding;
	uint32_t fAudioCodecs=0, fVideoCodecs=0, fObjectEncoding=0;

	/* Parse command object */
	oReceiveMessage.readAMF_ObjectStart( iAMF );
	do
	{
		string sKey = oReceiveMessage.readAMF_Key( iAMF );
		if( sKey == "app" )
			sApp = oReceiveMessage.readAMF_String( iAMF );
		else if( sKey == "flashVer" )
			sFlashVer = oReceiveMessage.readAMF_String( iAMF );
		else if( sKey == "swfUrl" )
			sSwfUrl = oReceiveMessage.readAMF_String( iAMF );
		else if( sKey == "tcUrl" )
			sUrl = oReceiveMessage.readAMF_String( iAMF );
		else if( sKey == "fpad" )
			oReceiveMessage.readAMF_Boolean( iAMF );
		else if( sKey == "audioCodecs" )
			fAudioCodecs = oReceiveMessage.readAMF_Number( iAMF );
		else if( sKey == "videoCodecs" )
			fVideoCodecs = oReceiveMessage.readAMF_Number( iAMF );
		else if( sKey == "pageUrl" )
			sPageUrl = oReceiveMessage.readAMF_String( iAMF );
		else if( sKey == "capabilities" )
			oReceiveMessage.readAMF_Number( iAMF );
		else if( sKey == "videoFunction" )
			oReceiveMessage.readAMF_Number( iAMF );
		else if( sKey == "objectEncoding" )
		{
			this->iAMF = (AMF::version) oReceiveMessage.readAMF_Number( iAMF );
			if( this->iAMF == AMF::AMF3 )
				debug( 1, "Switching to AMF 3" );
		}
		else
		{
			if( oReceiveMessage.readAMF_Skip( iAMF ) )
				return SirannonWarning( this,  "command connect, failed at unkown field(%s)", sKey.c_str() );
			else
				SirannonWarning( this,  "command connect, skipping unkown field(%s)", sKey.c_str() );
		}
	} while( not oReceiveMessage.readAMF_ObjectEnd( iAMF ) );

	debug( 1, "RECEIVED: command connect, id(%"LL"d) app(%s) flashVer(%s) AMF(%d) video(%X) audio(%X)",
			iTransID, sApp.c_str(), sFlashVer.c_str(), (int)this->iAMF, fVideoCodecs, fAudioCodecs );
	return 0;
}

int RTMPSession::handleCommandCreateStream( AMF::version iAMF, AMFIBits& oReceiveMessage )
{
	/* Verify transaction ID */
	iTransID = oReceiveMessage.readAMF_Number( iAMF );

	/* Ignore Command Object */
	debug( 1, "RECEIVED: command createStream, id(%d)", iTransID );
	return 0;
}

int RTMPSession::handleCommandPlay( AMF::version iAMF, AMFIBits& oReceiveMessage, uint32_t iMessage )
{
	/* Verify transaction ID */
	iTransID = oReceiveMessage.readAMF_Number( iAMF );
	if( iTransID != 0 )
		return SirannonWarning( this,  "command play, expected transaction id(0), received (%d)", iTransID );

	/* Command object */
	oReceiveMessage.readAMF_Null( iAMF );

	/* Stream name */
	string sRequestedStream = oReceiveMessage.readAMF_String( iAMF );

	/* Other options */
	bool bReset = true;
	if( oReceiveMessage.size() < iMessage )
		iStart = oReceiveMessage.readAMF_Number( iAMF );
	if( oReceiveMessage.size() < iMessage )
		iDuration = oReceiveMessage.readAMF_Number( iAMF );
	if( oReceiveMessage.size() < iMessage )
		bReset = oReceiveMessage.readAMF_Boolean( iAMF ); // FIXME

	/* Try to play this stream */
	debug( 1, "RECEIVED: (NetStream.Play), stream(%s) start(%d) duration(%d) reset(%d)",
			sRequestedStream.c_str(), iStart, iDuration, bReset );
	return handlePlay( sRequestedStream );
}

int RTMPSession::handleCommandPauseRaw( AMF::version iAMF, AMFIBits& oReceiveMessage )
{
	/* Parse */
	int iTransID = oReceiveMessage.readAMF_Number( iAMF );
	oReceiveMessage.readAMF_Null( iAMF );
	bool bPause = oReceiveMessage.readAMF_Boolean( iAMF );
	int iTime = oReceiveMessage.readAMF_Number( iAMF );

	/* ASSUME: pauseRaw has no side effect (replies, etc) besides simply pausing the stream */
	if( bPause )
	{
		debug( 1, "RECEIVED: command(pauseRaw) timestamp(%d)", iTime );
		return streamPause();
	}
	else
	{
		debug( 1, "RECEIVED: command(unpauseRaw) timestamp(%d)", iTime );
		return streamPlay();
	}
}

int RTMPSession::handleCommandPause( AMF::version iAMF, AMFIBits& oReceiveMessage )
{
	/* Parse */
	int iTransID = oReceiveMessage.readAMF_Number( iAMF );
	oReceiveMessage.readAMF_Null( iAMF );
	bool bPause = oReceiveMessage.readAMF_Boolean( iAMF );
	int iTime = oReceiveMessage.readAMF_Number( iAMF );

	/* Effective pause */
	if( bPause )
	{
		/* Only effectively pause while streaming */
		debug( 1, "RECEIVED: command(NetStream.Pause) timestamp(%d)", iTime );
		this->bPause = true;

		/* Notification */
		if( sendCommand( "NetStream.Pause.Notify" ) )
			return SirannonWarning( this,  "command(NetStream.Pause.Notify), could not send notification" );
	}
	else
	{
		/* Start playing again */
		debug( 1, "RECEIVED: command(NetStream.Unpause) timestamp(%d)", iTime );
		this->bPause = false;

		/* Unpause has the same effect as seeking */
		iSeek = iTime;
		bUnpause = true;
	}
	return 0;
}

int RTMPSession::handleCommandSeek( AMF::version iAMF, AMFIBits& oReceiveMessage )
{
	/* Parsed */
	int iTransID = oReceiveMessage.readAMF_Number( iAMF );
	oReceiveMessage.readAMF_Null( iAMF );
	iSeek = oReceiveMessage.readAMF_Number( iAMF );
	debug( 1, "RECEIVED: command seek (%d)", iSeek );

	return 0;
}

int RTMPSession::handleCommandStreamLength( AMF::version iAMF, AMFIBits& oReceiveMessage )
{
	/* Ignore the content */
	debug( 1, "RECEVIED: command getStreamLength file(%s)", sMedia.c_str() );

	/* Send response */
	if( sendCommandStreamLength( ) )
		return SirannonWarning( this,  "command getStreamLength, failed to send response" );
	return 0;
}

int RTMPSession::handleCommandDeleteStream( AMF::version iAMF, AMFIBits& oReceiveMessage )
{
	/* Parsed */
	int iTransID = oReceiveMessage.readAMF_Number( iAMF );
	oReceiveMessage.readAMF_Null( iAMF );
	oReceiveMessage.readAMF_Number( iAMF );

	debug( 1, "RECEIVED: command deleteStream" );

	end();
	return 0;
}

int RTMPSession::sendCommandConnectResponse( bool bFailure )
{
	Lock_t oLock( oSendMutex );
	oSendMessage.clear();
	if( bFailure)
		oSendMessage.writeAMF_String( AMF::AMF0, "_error" );
	else
		oSendMessage.writeAMF_String( AMF::AMF0, "_result" );
	oSendMessage.writeAMF_Number( AMF::AMF0, 1 );

	/* Properties object */
	oSendMessage.writeAMF_Null( AMF::AMF0 );

	/* Information object */
	oSendMessage.writeAMF_ObjectStart( AMF::AMF0 );

	oSendMessage.writeAMF_ObjectNumber( AMF::AMF0, "objectEncoding", iAMF );
	oSendMessage.writeAMF_ObjectNull( AMF::AMF0, "application" );
	oSendMessage.writeAMF_ObjectString( AMF::AMF0, "level", "status" );
	if( bFailure )
	{
		oSendMessage.writeAMF_ObjectString( AMF::AMF0, "description", "Connection failed." );
		oSendMessage.writeAMF_ObjectString( AMF::AMF0, "code", "NetConnection.Connect.InvalidApp" );
	}
	else
	{
		oSendMessage.writeAMF_ObjectString( AMF::AMF0, "description", "Connection succeeded." );
		oSendMessage.writeAMF_ObjectString( AMF::AMF0, "code", "NetConnection.Connect.Success" );

	}
	oSendMessage.writeAMF_ObjectString( AMF::AMF0, "fmsVer", "RED5/0,9,0,0" );
	oSendMessage.writeAMF_ObjectEnd( AMF::AMF0 );

	debug( 1, "SENDING, command connectResponse" );
	return sendMessage( oSendMessage, COMMAND_AMF0, 3, 0, iCurrent );
}

int RTMPSession::sendCommandCreateStreamResponse( bool bFailure )
{
	Lock_t oLock( oSendMutex );
	oSendMessage.clear();
	if( bFailure )
		oSendMessage.writeAMF_String( AMF::AMF0, "_error" );
	else
		oSendMessage.writeAMF_String( AMF::AMF0, "_result" );
	oSendMessage.writeAMF_Number( AMF::AMF0, iTransID );
	oSendMessage.writeAMF_Null( AMF::AMF0 );
	if( iAMF == AMF::AMF3 ) oSendMessage.writeAMF_SwitchToAMF3( AMF::AMF0 );
	oSendMessage.writeAMF_Number( iAMF, iStreamID );
	debug( 1, "SENDING, command createStreamResponse" );

	/* Send */
	return sendMessage( oSendMessage, COMMAND_AMF0, 3, 0, iCurrent );
}

int RTMPSession::sendCommandStreamLength( void )
{
	/* Response */
	Lock_t oLock( oSendMutex );
	oSendMessage.clear();
	oSendMessage.writeAMF_String( AMF::AMF0, "_result" );
	oSendMessage.writeAMF_Number( AMF::AMF0, 2 );
	oSendMessage.writeAMF_Null( AMF::AMF0 );
	oSendMessage.writeAMF_Number( AMF::AMF0, 1 );

	/* Send */
	debug( 1, "SENDING: command getStreamLength response" );
	return sendMessage( oSendMessage, COMMAND_AMF0, 3, 0, iCurrent );
}

int RTMPSession::sendMetaData( void )
{
	/* Obtain the meta data from the source */
	const ContainerDescriptor* pDesc = pStreamerInterface->getDescriptor();

	/* Check audio */
	const MediaDescriptor* pAudioDesc = pDesc->getAudioDescriptor();
	if( pAudioDesc )
	{
		bAudio = SirannonToFlash( pAudioDesc->codec );
		if( not bAudio )
			SirannonWarning( this, "Flash does not support(%s)", CodecToString(pAudioDesc->codec) );
	}
	/* Check video */
	const MediaDescriptor* pVideoDesc = pDesc->getVideoDescriptor();
	if( pVideoDesc )
	{
		bVideo = SirannonToFlash( pVideoDesc->codec );
		if( not bVideo )
			SirannonWarning( this, "Flash does not support(%s)", CodecToString(pVideoDesc->codec) );
	}
	/* We must at least have one media type */
	if( not bVideo and not bAudio )
		RuntimeError( this, "Source contains no supported video or audio" );

	/* Get some info about the file */
	if( bAudio )
		iMainStream = content_t::audio;
	if( bVideo )
		iMainStream = content_t::video;
	int iStreams = 0;
	int iChunkID = 5;
	int iAudioBitrate=0, iVideoBitrate=0;
	if( bVideo )
	{
		iVideoID = iChunkID++;
		iStreams++;
		if( pVideoDesc->bitrate )
			iVideoBitrate = pVideoDesc->bitrate;
		else
		{
			debug( 1, "send MetaData, video bitrate is 0 in media descriptor" );
			iVideoBitrate = 1000000;
		}
	}
	if( bAudio )
	{
		iAudioID = iChunkID++;
		iStreams++;
		if( pAudioDesc->bitrate >= 0 )
			iAudioBitrate = pAudioDesc->bitrate;
		else
		{
			debug( 1, "send MetaData, audio bitrate is 0 in media descriptor" );
			iAudioBitrate = 128000;
		}
	}
	if( not iStreams )
		return SirannonWarning( this,  "send MetaData, video nor audio in container" );
	double fDuration = pDesc->duration / 1000000.;

	if( bVideo and not pVideoDesc->inc )
		return SirannonWarning( this,  "Field 'inc' of video descriptor is 0" );

	Lock_t oLock( oSendMutex );
	oSendMessage.clear();
	oSendMessage.writeAMF_String( AMF::AMF0, "onMetaData" );
	oSendMessage.writeAMF_ObjectStart( AMF::AMF0 );
	oSendMessage.writeAMF_ObjectDouble( AMF::AMF0,"duration", fDuration );
	oSendMessage.writeAMF_ObjectDouble( AMF::AMF0, "starttime", 0 );
	oSendMessage.writeAMF_ObjectDouble( AMF::AMF0, "totalduration", fDuration );
	if( bAudio ) oSendMessage.writeAMF_ObjectNumber( AMF::AMF0, "audiocodecid", SirannonToFlash(pAudioDesc->codec) );
	if( bVideo ) oSendMessage.writeAMF_ObjectNumber( AMF::AMF0, "videocodecid", SirannonToFlash(pVideoDesc->codec) );
	if( bAudio ) oSendMessage.writeAMF_ObjectNumber( AMF::AMF0, "audiochannels", pAudioDesc->channels );
	if( bAudio ) oSendMessage.writeAMF_ObjectNumber( AMF::AMF0, "audiosamplerate", pAudioDesc->samplerate );
	if( bVideo ) oSendMessage.writeAMF_ObjectNumber( AMF::AMF0, "videoframerate", 90000 / pVideoDesc->inc ); // eg 24 fps
	if( bAudio ) oSendMessage.writeAMF_ObjectNumber( AMF::AMF0, "aacaot", 2 ); // 2
	if( bVideo ) oSendMessage.writeAMF_ObjectNumber( AMF::AMF0, "width", pVideoDesc->width );
	if( bVideo ) oSendMessage.writeAMF_ObjectNumber( AMF::AMF0, "height", pVideoDesc->height );
	if( bVideo ) oSendMessage.writeAMF_ObjectDouble( AMF::AMF0, "videodatarate", iVideoBitrate / 1000. );
	if( bAudio ) oSendMessage.writeAMF_ObjectDouble( AMF::AMF0, "audiodatarate", iAudioBitrate / 1000. );
	oSendMessage.writeAMF_ObjectDouble( AMF::AMF0, "totaldatarate", (iVideoBitrate+iAudioBitrate) / 1000. );
	oSendMessage.writeAMF_ObjectNumber( AMF::AMF0, "bytelength", pDesc->bytesize );
	oSendMessage.writeAMF_ObjectBoolean( AMF::AMF0, "canseekontime", true );
	oSendMessage.writeAMF_ObjectEnd( AMF::AMF0 );

	/* Send */
	if( pAudioDesc )
		debug( 1, "audio metadata: id(%d) channels(%d) samplerate(%d) rate(%f)",
				SirannonToFlash(pAudioDesc->codec), pAudioDesc->channels, pAudioDesc->samplerate, iAudioBitrate / 1000. );
	debug( 1, "SENDING: (MetaData.onMetaData) video(%d) audio(%d) duration(%f) size(%d) totaldatarate(%f)",
		(int)bVideo, (int)bAudio, fDuration, pDesc->bytesize, (iVideoBitrate+iAudioBitrate) / 1000. );
	return sendMessage( oSendMessage, METADATA_AMF0, 4, iStreamID, iCurrent );
}

int RTMPSession::sendCommand( const char* sCode, const char* sDesc, const char* sCmd )
{
	Lock_t oLock( oSendMutex );
	oSendMessage.clear();
	oSendMessage.writeAMF_String( AMF::AMF0, sCmd );
	oSendMessage.writeAMF_Number( AMF::AMF0, 1 );
	oSendMessage.writeAMF_Null( AMF::AMF0 );
	if( iAMF == AMF::AMF3 ) oSendMessage.writeAMF_SwitchToAMF3( AMF::AMF0 );
	oSendMessage.writeAMF_ObjectStart( iAMF );
	oSendMessage.writeAMF_ObjectNumber( iAMF, "clientid", 1 );
	oSendMessage.writeAMF_ObjectString( iAMF, "level", "status" );
	oSendMessage.writeAMF_ObjectString( iAMF, "details", sFileName.c_str() );
	oSendMessage.writeAMF_ObjectString( iAMF, "description", sDesc );
	oSendMessage.writeAMF_ObjectString( iAMF, "code", sCode );
	oSendMessage.writeAMF_ObjectEnd( iAMF );

	debug( 1, "SENDING: command %s", sCode );
	return sendMessage( oSendMessage, COMMAND_AMF0, 4, iStreamID, iCurrent );
}

int RTMPSession::printMessage( message_t* pMessage )
{
	if( not pScope->getVerbose() )
		return 0;

	fprintf( stderr, "length(%d) stream_id(%d) type(%d) command(%s) timestamp(%d) chunk_id(%d)\n",
						pMessage->message_length, pMessage->message_stream_id, pMessage->message_type_id,
						pMessage->command.c_str(), pMessage->timestamp, pMessage->cs_id );

	return 0;
}
