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
#include "RTMPClient.h"
#include "SirannonSocket.h"
#include "Frame.h"
#include "Flash.h"
#include "Url.h"
#include "OSSupport.h"

/**
 * RTMP CLIENT
 * @component RTMP-client
 * @type media-client
 * @param mov-frame, bool, false, if true, keep the frames in MOV/MP4 structure for AVC and AAC
 * @param chunk-size, int, 65536, in bytes, the size of the chunks from the container to generate
 * @info Requests and receives a stream via RTMP. Generates audio and video frames.
 **/

REGISTER_CLASS( RTMPClient, "RTMP-client" );

/* Test servers:
 * vod01.netdna.com play vod/demo.flowplayer/Extremists.flv
 * cyzy7r959.rtmphost.com flowplayer skyandice-40
 * sxxzuyhny94n5.cloudfront.net cfx/st cfx/st examples/flowplayer-700
 *
 *
 * Streamer=rtmpt://212.187.212.162/flash&file=bt-c746692596bdcd1cf428876c3cdf3d41c9d24c61&type=rtmp&height=288&width=352&displayheight=288&autostart=true&repeat=false
 * config={"log":{"level":"debug","filter":"org.flowplayer.slowmotion.*"},"plugins":{"slowmotion":{"url":"flowplayer.slowmotion-3.1.5-dev.swf"},"rtmp":{"url":"flowplayer.rtmp-3.1.4-dev.swf","netConnectionUrl":"rtmp://vod01.netdna.com/play"}},"clip":{"provider":"rtmp","scaling":"orig","url":"vod/demo.flowplayer/Extremists.flv"},"playerId":"trickplay","playlist":[{"provider":"rtmp","scaling":"orig","url":"vod/demo.flowplayer/Extremists.flv"}]}
 * config={"clip":{"url":"vod/demo.flowplayer/metacafe.flv","provider":"maxcdn"},"plugins":{"maxcdn":{"url":"flowplayer.rtmp-3.1.3.swf","netConnectionUrl":"rtmp://vod01.netdna.com/play"}},"playerId":"fms","playlist":[{"url":"vod/demo.flowplayer/metacafe.flv","provider":"maxcdn"}]}
 *
 * <embed width="630" height="354" flashvars="publisherGuard=&amp;hide_chat=true&amp;searchquery=&amp;backgroundImageUrl=http://static-cdn.justin.tv/jtv_user_pictures/wildearthtv-320x240-1.jpg&amp;channel=wildearthtv&amp;hostname=www.justin.tv&amp;auto_play=true&amp;servers=josh&amp;publisherTimezoneOffset=-120" wmode="transparent" allowscriptaccess="always" allowfullscreen="true" allownetworking="all" quality="high" bgcolor="#000000" name="live_site_player_flash" id="live_site_player_flash" src="http://www-cdn.justin.tv/widgets/live_site_player.r84bea4101afacefe6693808b7f47bf52f32ec4f5.swf" type="application/x-shockwave-flash">
 *
 *
 * Verified test servers:
 * 		* http://www.longtailvideo.com/support/tutorials/Streaming-Video-with-the-JW-Player
 * 		  edge01.fms.dutchview.nl botr bunny.flv
 * 		* vod01.netdna.com play vod/demo.flowplayer/metacafe.flv
 * 		* 157.157.65.94 eyjafjallajokull fimmvorduhals
 */

RTMPClient::RTMPClient( const char* sName, ProcessorManager* pProc )
	: MediaProcessor(sName, pProc), iAMF(AMF::AMF0), iAudioRoute(200), iVideoRoute(100),
	  iAudioStream(nextStreamID()), iVideoStream(nextStreamID()),
	  iVideoUnit(0), iAudioUnit(0), bVideo(true), bAudio(true), bFlush(false), bPause(false),
	  bAudioMetaData(false), bVideoMetaData(false), bWait(true), bAnnexB(true),
	  pVideoDesc(NULL), pAudioDesc(NULL), bVideoMov(false), bAudioMov(false),
	  iLastAudioDts(0), iLastVideoDts(0)
{
	mInt["video-route"] = 100;
	mInt["audio-route"] = 200;
	mString["url"] = "rtmp://localhost/RTMP/flash/da.flv";
	mBool["mov-frame"] = false;
	mBool["auto-play"] = true;
}

RTMPClient::~RTMPClient()
{
	delete pConnection;
	while( not mBuffer.empty() )
	{
		delete mBuffer.front();
		mBuffer.pop();
	}
}

void RTMPClient::init( void )
{
	/* Base class */
	MediaProcessor::init();

	/* Params */
	iVideoRoute = mInt["video-route"];
	iAudioRoute = mInt["audio-route"];
	bAnnexB = not mBool["mov-frame"];
	bAutoPlay = mBool["auto-play"];
	oTimeOut = 25000;

	/* Verify */
	sUrl = mString["url"];
	if( not sUrl.length() )
		ValueError( this, "URL is an empty string" );
	URL_canonize( sUrl, "rtmp" );
	URL_parse( sUrl, sProtocol, sServer, iPort, sApp, sMedia );

	/* Blocking thread */
	createThread( bind( &RTMPClient::mainThreadA, this ) );
}

bool RTMPClient::ready( void ) const
{
	return (not bVideo or bVideoMetaData) and (not bAudio or bAudioMetaData);
}

void RTMPClient::process( void )
{
	/* Send all buffered packets */
	if( not bWait )
	{
		while( not mBuffer.empty() )
		{
			MediaPacketPtr pPckt( mBuffer.front() );
			debug( 1, "parsed %s", pPckt->c_str_long() );
			mBuffer.pop();
			route( pPckt );
		}
	}
}

void RTMPClient::mainThreadA( void )
{
	/* Parse the address */
	IPAddress oAddr ( sServer, iPort );
	if( not oAddr.valid() )
		RuntimeError( this, "invalid server address(%s)", oAddr.getAddressStr().c_str() );
	debug( 1, "connecting(%s)", sUrl.c_str() );

	/* Create the TCP connection */
	pConnection = new TCPSocket( oAddr.getIPStr(), oAddr.getPort() );

	/* Main activity */
	handleCommunication();
}

int RTMPClient::handleCommunication( void )
{
	/* Start communicating */
	debug( 1, "connected" );
	do
	{
		/* Do the handshake */
		if( handleHandshake() < 0 )
			RuntimeError( this, "Handshake failed" );
		debug( 1, "Handshake succes" );

		/* Connect */
		if( handleApplicationConnect() < 0 )
			RuntimeError( this, "Application Connect failed" );
		debug( 1, "Connect success" );

		/* Play */
		if( handlePlay() < 0 )
			RuntimeError( this, "Play failed" );
		debug( 1, "Play success" );

		/* Streaming until stop */
		while( true )
		{
			/* Receive messages */
			if( receiveResponse( "NetStream.Play.Stop", COMMAND_AMF0 ) < 0 )
				break;

			/* Wait for this code */
			if( sCode == "NetStream.Play.Stop" )
			{
				handleEnd();
				return 0;
			}
		}
	} while( 0 );
	debug( 1, "handleCommunication terminated" );
	return 0;
}

int RTMPClient::handleHandshake( void )
{
	/* Buffers */
	OBits oSendMessage( 4096 );
	IBits oReceiveMessage( 4096 );

	/* Send C0 */
	oSendMessage.clear();
	oSendMessage.write( 8, RTMP_VERSION );
	if( sendToSocket( oSendMessage.data(), oSendMessage.size() ) <= 0 )
		return SirannonWarning( this,  "handshake, failed to send chunk C0" );
	debug( 1, "sent C0" );

	/* Send C1 */
	if( sendToSocket( oHandshake.generateC1( true ), oHandshake.HANDSHAKE_SIZE ) <= 0 )
		return SirannonWarning( this,  "handshake, failed to send chunk S1" );
	debug( 1, "sent C1" );

	/* Receive S0 */
	oReceiveMessage.clear();
	if( receiveFromSocket( oReceiveMessage.data(), 1 ) <= 0 )
		return SirannonWarning( this,  "handshake, failed to recive chunk S0" );
	int iVersion = oReceiveMessage.read( 8 );

	/* Check version */
	if( iVersion < 3 )
		return SirannonWarning( this,  "handshake, old server version (%d), version (3) required", iVersion );
	else if( iVersion == 3 )
		debug( 1, "handshake, server RTMP version 3" );
	else if( iVersion < 32 )
		return SirannonWarning( this,  "handshake, server version (%d) failed to degrade, to version (3)", iVersion );
	else
		return SirannonWarning( this,  "handshake, illegal server version (%d)", iVersion );

	/* Receive S1 */
	oReceiveMessage.clear();
	if( receiveFromSocket( oReceiveMessage.data(), oHandshake.HANDSHAKE_SIZE ) <= 0 )
		return SirannonWarning( this,  "handshake, failed to receive chunk S1" );
	debug( 1, "received S1" );
	// We don't require anything from S1

	/* Send C2 */
	const uint8_t* C2 = oHandshake.generateC2( oReceiveMessage.data(), oHandshake.HANDSHAKE_SIZE );
	if( sendToSocket( C2, oHandshake.HANDSHAKE_SIZE ) <= 0 )
		return SirannonWarning( this,  "handshake, failed to send chunk C2" );
	debug( 1, "sent C2" );

	/* Receive S2 */
	oReceiveMessage.clear();
	if( receiveFromSocket( oReceiveMessage.data(), oHandshake.HANDSHAKE_SIZE ) <= 0 )
		return SirannonWarning( this,  "handshake, failed to receive chunk S2" );
	debug( 1, "received S2" );

	/* Verify S2 */
	if( not oHandshake.validateS2( oReceiveMessage.data(), oHandshake.HANDSHAKE_SIZE ) )
		debug( 1, "Invalid %sS2, ignoring violation", oHandshake.isClientCrypto() ? "crypto-":"" );
	else
		debug( 1, "Valid %sS2", oHandshake.isClientCrypto() ? "crypto-":"" );


	/* Handshake succesfull */
	return 0;
}

int RTMPClient::handleApplicationConnect( void )
{
	/* Send connect */
	if( sendCommandConnect() < 0 )
		return SirannonWarning( this,  "could not send (NetConnection.Connect)" );

	/* Wait for command response */
	if( receiveResponse( "NetConnection.Connect", COMMAND_AMF0 ) < 0 )
		return -1;

	return 0;
}

int RTMPClient::handlePlay( void )
{
	/* Create stream */
	if( sendCommandCreateStream() < 0 )
		return SirannonWarning( this,  "could not send (NetConnection.CreateStream)" );

	/* Response */
	if( receiveResponse( "NetConnection.CreateStream", COMMAND_AMF0, "_result" ) < 0 )
		return -1;

	/* Send buffer length */
	if( sendUserControl( SET_BUFFER_LENGTH, 6000 ) < 0 )
		return SirannonWarning( this,  "could not send (UserControl.Buffer.SetLength)" );

	/* Succes or failure? */
	iStreamID = mResponseInt["value"];
	if( sStatus != "_result" )
		return SirannonWarning( this,  "malformatted (NetConnection.CreateStream) response" );

	/* Play */
	if( sendCommandPlay() < 0 )
		return SirannonWarning( this,  "could not send (NetStream.Play)" );

	/* Send buffer length */
	//if( sendUserControl( SET_BUFFER_LENGTH, 3000 ) < 0 )
	//	return SirannonWarning( this,  "could not send (UserControl.Buffer.SetLength)" );

	/* Response 1 */
	if( receiveResponse( "NetStream.Play.Reset", COMMAND_AMF0 ) < 0 )
		return -1;

	/* Succes or failure? */
	if( sCode == "NetStream.Play.StreamNotFound" )
		return -1;
	else if( sCode == "NetStream.Play.Reset" )
		;
	else
		return SirannonWarning( this,  "malformatted (NetStream.Play.Reset) response: %s, %s", sCode.c_str(), sDesc.c_str() );

	/* Response 1 */
	if( receiveResponse( "NetStream.Play.Start", COMMAND_AMF0 ) < 0 )
		return -1;

	 /* Auto-play */
	 if( mBool["auto-play"] )
		 play( 1.0 );

	/* Succes or failure? */
	 if( sCode != "NetStream.Play.Start" )
		 return SirannonWarning( this,  "malformatted (NetStream.Play.Reset) response: %s: %s", sCode.c_str(), sDesc.c_str() );

	/* Meta data */
	 if( receiveResponse( "MetaData.OnMetaData", METADATA_AMF0, "onMetaData" ) < 0 )
	 	return -1;

	/* Some codec info */
	return 0;
}

int RTMPClient::receiveResponse( const string& sCommand, int iMessageType, const char* sMode  )
{
	/* Receieve */
	if( receiveMessages( iMessageType, sMode ) < 0 )
		return SirannonWarning( this,  "failed to receive (%s)", sCommand.c_str() );

	/* Succes or failure? */
	sCode = mResponseString["code"];
	sDesc = mResponseString["description"];

	/* Failure */
	if( sStatus == "_error" )
		return SirannonWarning( this,  "RECEIVED: (%s) status(%s) code(%s) message(%s)", sCommand.c_str(), sStatus.c_str(), sCode.c_str(), sDesc.c_str() );
	else if( sCode == "NetConnection.Connect.Success" )
		debug( 1, "RECEIVED: (%s) status(%s) code(%s) message(%s) server(%s)", sCommand.c_str(), sStatus.c_str(), sCode.c_str(), sDesc.c_str(), mResponseString["fmsVer"].c_str() );
	else
		debug( 1, "RECEIVED: (%s) status(%s) code(%s) message(%s)", sCommand.c_str(), sStatus.c_str(), sCode.c_str(), sDesc.c_str() );
	return 0;
}

int RTMPClient::sendCommandConnect( void )
{
	Lock_t oLock( oSendMutex );
	oSendMessage.clear();
	int iAMF = AMF::AMF0;

	/* Command structure */
	oSendMessage.writeAMF_String( iAMF, "connect" );
	oSendMessage.writeAMF_Number( iAMF, 1 ); // transaction ID 1
	oSendMessage.writeAMF_ObjectStart( iAMF );
	oSendMessage.writeAMF_ObjectString( iAMF, "app", sApp.c_str() );
	oSendMessage.writeAMF_ObjectString( iAMF, "flashVer", "LNX 10,0,45,2" );
	oSendMessage.writeAMF_ObjectString( iAMF, "tcUrl", sUrl.c_str() );
	oSendMessage.writeAMF_ObjectNumber( iAMF, "audioCodecs", SUPPORT_SND_AAC ); // 0x0FFF ); //
	oSendMessage.writeAMF_ObjectNumber( iAMF, "videoCodecs", SUPPORT_VID_H264 ); // 0x00FF ); //
	oSendMessage.writeAMF_ObjectNumber( iAMF, "capabilities", 15 );
	oSendMessage.writeAMF_ObjectNumber( iAMF, "videoFunction", 1 );
	oSendMessage.writeAMF_ObjectNumber( iAMF, "objectEncoding", iAMF );
	oSendMessage.writeAMF_ObjectBoolean( iAMF, "fpad", false );
	oSendMessage.writeAMF_ObjectEnd( iAMF );

	/* Send */
	debug( 1, "SENDING: (NetConnection.Connect)" );
	return sendMessage( oSendMessage, COMMAND_AMF0, 3, 0, 0 );
}

int RTMPClient::sendCommandCreateStream( void )
{
	Lock_t oLock( oSendMutex );
	oSendMessage.clear();
	int iAMF = AMF::AMF0;

	/* Command structure */
	oSendMessage.writeAMF_String( iAMF, "createStream" );
	oSendMessage.writeAMF_Number( iAMF, 2 ); // transaction ID 2
	oSendMessage.writeAMF_Null( iAMF );

	/* Send */
	debug( 1, "SENDING: (NetConnection.CreateStream)" );
	return sendMessage( oSendMessage, COMMAND_AMF0, 3, 0, 0 );
}

int RTMPClient::sendCommandPlay( void )
{
	Lock_t oLock( oSendMutex );
	oSendMessage.clear();
	int iAMF = AMF::AMF0;

	/* Command structure */
	oSendMessage.writeAMF_String( iAMF, "play" );
	oSendMessage.writeAMF_Number( iAMF, 0 ); // transaction ID 0
	oSendMessage.writeAMF_Null( iAMF );
	oSendMessage.writeAMF_String( iAMF, sMedia.c_str() );
	//oSendMessage.writeAMF_Number( iAMF, -2 );
	//oSendMessage.writeAMF_Number( iAMF, -1 );
	//oSendMessage.writeAMF_Boolean( iAMF, true );

	/* Send */
	debug( 1, "SENDING: (NetStream.Play) file(%s)", sMedia.c_str() );
	return sendMessage( oSendMessage, COMMAND_AMF0, 4, iStreamID, 0 );
}

int RTMPClient::sendCommandPauseRaw( bool bPause, uint32_t iPause )
{
	Lock_t oLock( oSendMutex );
	oSendMessage.clear();
	int iAMF = AMF::AMF0;

	/* Command structure */
	oSendMessage.writeAMF_String( iAMF, "pauseRaw" );
	oSendMessage.writeAMF_Number( iAMF, 0 ); // Transaction ID
	oSendMessage.writeAMF_Null( iAMF );
	oSendMessage.writeAMF_Boolean( iAMF, bPause );
	oSendMessage.writeAMF_Number( iAMF, iPause );

	debug( 1, "SENDING: (NetStream.pauseRaw) time-index(%d ms)", iPause );
	return sendMessage( oSendMessage, COMMAND_AMF0, 4, iStreamID, 0 );
}

int RTMPClient::sendCommandSeek( uint32_t iSeek )
{
	Lock_t oLock( oSendMutex );
	oSendMessage.clear();
	int iAMF = AMF::AMF0;

	/* Command structure */
	oSendMessage.writeAMF_String( iAMF, "seek" );
	oSendMessage.writeAMF_Number( iAMF, 0 ); // transaction ID 0
	oSendMessage.writeAMF_Null( iAMF );
	oSendMessage.writeAMF_Number( iAMF, iSeek );

	/* Send */
	debug( 1, "SENDING: (NetStream.Seek) index(%d ms)", iSeek );
	return sendMessage( oSendMessage, COMMAND_AMF0, 4, iStreamID, 0 );
}

int RTMPClient::handleCommandAMF0( message_t* pMessage )
{
	AMFIBits& oReceiveMessage = pMessage->oBuffer;
	pMessage->command = sStatus = oReceiveMessage.readAMF_String( iAMF );
	int iTransID = oReceiveMessage.readAMF_Number( iAMF );
	iTimestamp = pMessage->timestamp;

	mResponseBool.clear();
	mResponseInt.clear();
	mResponseString.clear();

	/* Receive COMMAND object */
	debug( 1, "RECEIVED: (NetStream/NetConnection) status(%s)", sStatus.c_str() );
	if( oReceiveMessage.readAMF( iAMF, mResponseInt, mResponseString, mResponseBool, "value" ) < 0 )
	{
		SirannonWarning( this,  "Ignoring malformatted command response message\n%s", oReceiveMessage.str( 100 ).c_str() );
		return 0;
	}
	/* Receive RESPONSE object */
	if( oReceiveMessage.readAMF( iAMF, mResponseInt, mResponseString, mResponseBool, "value" ) < 0 )
	{
		SirannonWarning( this,  "Ignoring malformatted command response message\n%s", oReceiveMessage.str( 100 ).c_str() );
		return 0;
	}
	return 0;
}

int RTMPClient::handleMetaDataAMF0( message_t* pMessage )
{
	AMFIBits& oReceiveMessage = pMessage->oBuffer;
	pMessage->command = sStatus = oReceiveMessage.readAMF_String( iAMF );

	/* Receive Meta data object */
	mResponseBool.clear();
	mResponseInt.clear();
	mResponseString.clear();
	if( oReceiveMessage.readAMF( iAMF, mResponseInt, mResponseString, mResponseBool, "value" ) < 0 )
	{
		SirannonWarning( this,  "malformatted metadata message\n%s", oReceiveMessage.str( 100 ).c_str() );
		return -1;
	}

	debug( 1, "RECEIVED: (MetaData) status(%s)", sStatus.c_str() );
	if( sStatus == "onMetaData" )
	{
		/* Video present? */
		codec_t::type iCodec = codec_t::NO;
		if( mResponseInt.count( "videocodecid" ) )
			iCodec = FlashVideoToSirannon( mResponseInt["videocodecid"]  );
		else if( mResponseString.count( "videocodecid" ) )
			iCodec = FlashToSirannon( mResponseString["videocodecid"].c_str()  );
		else
			iCodec = codec_t::NO;

		/* Parse video info */
		if( iCodec != codec_t::NO )
		{
			bVideo = true;
			pVideoDesc = addMedia();
			pVideoDesc->content = content_t::video;
			pVideoDesc->codec = iCodec;
			pVideoDesc->bitrate = mResponseInt["videodatarate"] * 1000;
			pVideoDesc->framerate = mResponseInt["videoframerate"];
			pVideoDesc->height = mResponseInt["height"];
			pVideoDesc->width = mResponseInt["width"];
			pVideoDesc->profile = mResponseInt["avcprofile"];
			pVideoDesc->level = mResponseInt["level"];
			pVideoDesc->route =  iVideoRoute ? iVideoRoute : oContainer.size() * 100;
			if( pVideoDesc->framerate > 0 )
				pVideoDesc->inc = 90000 / pVideoDesc->framerate;

			bVideoMov = pVideoDesc->codec & codec_t::H264;
			if( not bVideoMov )
				bVideoMetaData = true;
		}
		else
			bVideo = false;

		/* Find audio info */
		iCodec = codec_t::NO;
		if( mResponseInt.count( "audiocodecid" ) )
			iCodec = FlashAudioToSirannon( mResponseInt["audiocodecid"]  );
		else if( mResponseString.count( "audiocodecid" ) )
			iCodec = FlashToSirannon( mResponseString["audiocodecid"].c_str()  );
		else
			iCodec = codec_t::NO;

		/* Parse audio info */
		if( iCodec != codec_t::NO )
		{
			bAudio = true;
			pAudioDesc = addMedia();
			pAudioDesc->content = content_t::audio;
			pAudioDesc->codec = iCodec;
			pAudioDesc->channels = mResponseInt["audiochannels"];
			pAudioDesc->samplerate = mResponseInt["audiosamplerate"];
			pAudioDesc->bitrate =  mResponseInt["audiodatarate"] * 1000;
			pAudioDesc->route = iAudioRoute ? iAudioRoute : oContainer.size() * 100;
			bAudioMov = pAudioDesc->codec == codec_t::mp4a;
			if( not bAudioMov )
				bAudioMetaData = true;

			/* Fix sample rates */
			if( pAudioDesc->samplerate == 44000 )
				pAudioDesc->samplerate = 44100;
			else if( pAudioDesc->samplerate == 22000 )
				pAudioDesc->samplerate = 22050;
			else if( pAudioDesc->samplerate == 11000 )
				pAudioDesc->samplerate = 11025;
		}
		else
			bAudio = false;

		/* Parse global info */
		printState();
		oContainer.bytesize = mResponseInt["bytelength"];
		oContainer.duration = mResponseInt["duration"] * 1000000;  // Internally in us, communicated in seconds

		/* Reload */
		pVideoDesc = oContainer.getVideoDescriptor();
		pAudioDesc = oContainer.getAudioDescriptor();

		/* Info */
		debug( 1, "metadata: duration(%"LL"d) video(%s) w(%"LL"d) h(%"LL"d) audio(%s)",
					mResponseInt["duration"],
					CodecToString( pVideoDesc ? pVideoDesc->codec : codec_t::NO ),
					mResponseInt["width"], mResponseInt["height"],
					CodecToString( pAudioDesc ? pAudioDesc->codec : codec_t::NO ) );
	}
	else if( sStatus == "onPlayStatus" )
	{
	}
	else if( sStatus == "|RtmpSampleAccess" )
	{
	}
	else
	{
		printState();
		SirannonWarning( this,  "unknown metadata command %s", sStatus.c_str() );
	}
	return 0;
}

int RTMPClient::handleVideo( message_t* pMessage )
{
	/* Extract the FLV flag and determine codec */
	AMFIBits& oReceiveMessage = pMessage->oBuffer;
	int iKey = oReceiveMessage.read( 4 );
	int iCodecID = oReceiveMessage.read( 4 );
	int iSuffix = -1;

	codec_t::type iCodec;
	switch( iCodecID )
	{
	case FLASH_VIDEO_H263:
	case FLASH_VIDEO_SCREEN:
	case FLASH_VIDEO_SCREEN_2:
		iCodec = codec_t::other;
		break;

	case FLASH_VIDEO_VP6:
		iCodec = codec_t::vp6;
		iSuffix = oReceiveMessage.read( 8 );
		break;

	case FLASH_VIDEO_VP6_ALPHA:
		iCodec = codec_t::vp6f;
		iSuffix = oReceiveMessage.read( 8 );
		break;

	case FLASH_VIDEO_H264:
		iCodec = codec_t::avc;
		iSuffix = oReceiveMessage.read( 32 );
		break;

	default:
		return SirannonWarning( this,  "Unknown Flash CodecID(%d)", iCodecID );
	}
	debug( 1, "hex %s", oReceiveMessage.str( 20 ).c_str() );

	/* Sanity */
	if( pMessage->message_length < oReceiveMessage.size() + 2 )
	{
		SirannonWarning( this, "Message too short: %d < %d + 2", pMessage->message_length, oReceiveMessage.size() );
		return 0;
	}
	debug( 1, "new video payload(%d) suffix(%d)", pMessage->message_length, iSuffix );

	/* Check the special case for AVC parameter sets */
	if( iCodec == codec_t::avc and iSuffix == 0 )
	{
		/* Fill out extra data once */
		bVideoMetaData = true;
		if( not pVideoDesc->getExtraSize() )
		{
			pVideoDesc->setExtraData( oReceiveMessage.cur(), pMessage->message_length - oReceiveMessage.size() );
			oAnnexB.AVCC2META( pVideoDesc );
			debug( 1, "Parsed video extra data: size(%d)", pVideoDesc->getExtraSize() );
		}
		else
			debug( 1, "Parsed video extra data (repeat)" );
		return 0;
	}
	else
	{
		if( pVideoDesc->getExtraSize() == 0 )
		{
			if( iCodec == codec_t::vp6 or iCodec == codec_t::vp6f )
			{
				debug( 1, "Setting vp6 extra data" );
				pVideoDesc->setExtraData( oReceiveMessage.cur(), 1 );
			}
		}
		/* Regular video packet */
		MediaPacketPtr pPckt ( new MediaPacket( packet_t::media, content_t::video, pMessage->message_length ) );

		/* Payload */
		pPckt->push_back( oReceiveMessage.cur(), pMessage->message_length - oReceiveMessage.size() );

		/* Deduce inc, only possible on the second packet or later */
		if( iLastVideoDts > 0 )
			pVideoDesc->inc = pMessage->timestamp * 90 - iLastVideoDts;

		/* Meta data */
		pPckt->unitnumber = iVideoUnit++;
		pPckt->framenumber = pPckt->unitnumber;
		if( pMessage->timestamp * 90 == iLastVideoDts ) // HACK
			pPckt->dts = iLastVideoDts = pMessage->timestamp * 90 + 90;
		else
			pPckt->dts = iLastVideoDts = pMessage->timestamp * 90;
		pPckt->inc = pVideoDesc->inc;
		pPckt->codec = pVideoDesc->codec;
		pPckt->mux = bVideoMov ? mux_t::MOV : mux_t::ES;
		pPckt->framestart = pPckt->frameend = true;
		pPckt->key = ( iKey == FLASH_VIDEO_KEY_FRAME );
		pPckt->desc = pVideoDesc;
		pPckt->xroute = pPckt->desc->route;
		pPckt->xstream = iVideoStream;
		if( pPckt->codec == codec_t::avc )
			pPckt->pts = pPckt->dts + (iSuffix & 0xFFFFFF) * 90;
		else
			pPckt->pts = pPckt->dts;

		/* Convert if needed */
		FlowLock_t oLock( oFlowMutex );
		if( bAnnexB and bVideoMov )
			oAnnexB.convertES( pPckt, mBuffer, pPckt->key );
		else
			mBuffer.push( pPckt.release() );
		debug( 1, "Parsed %s", mBuffer.back()->c_str_long() );
	}
	return 0;
}

int RTMPClient::handleAudio( message_t* pMessage )
{
	/* Ignore while flushing */
	//if( bFlush )
	//	return 0;

	/* Ignore an empty message */
	if( pMessage->message_length == 0 )
	{
		debug( 1, "parsed empty audio packet" );
		return 0;
	}
	/* Extract the FLV flag and determine codec */
	AMFIBits& oReceiveMessage = pMessage->oBuffer;
	int iCodecID = oReceiveMessage.read( 4 );
	int iFlags = oReceiveMessage.read( 4 );
	int iSuffix = 0;

	codec_t::type iCodec;
	switch( iCodecID )
	{

	case FLASH_AUDIO_AAC:
		iSuffix = oReceiveMessage.read( 8 );
		iCodec = codec_t::mp4a;
		break;

	case FLASH_AUDIO_MP3:
	case FLASH_AUDIO_MP3_8:
		iCodec = codec_t::mp1a;
		break;

	case FLASH_AUDIO_ADPCM:
	case FLASH_AUDIO_PCM:
	case FLASH_AUDIO_NELLY_16_MONO:
	case FLASH_AUDIO_NELLY_8_MONO:
	case FLASH_AUDIO_NELLY:
	case FLASH_AUDIO_G_711_A:
	case FLASH_AUDIO_G_771_M:
	case FLASH_AUDIO_AUDIO_RESERVED:
	case FLASH_AUDIO_SPEEX:
	case FLASH_AUDIO_DEVICE_SPECIFIC:
		iCodec = codec_t::other;
		break;

	default:
		return SirannonWarning( this,  "Unknown Flash AudioCodecID(%d)", iCodecID );
	}
	/* Catch the extra data for AAC */
	if( iCodec == codec_t::mp4a and iSuffix == 0 )
	{
		/* Fill out extra data once */
		bAudioMetaData = true;
		if( not pAudioDesc->getExtraSize() )
		{
			pAudioDesc->setExtraData( oReceiveMessage.cur(), pMessage->message_length - oReceiveMessage.size() );
			debug( 1, "Received audio extra data: size(%d) (%02hhX%02hhX)",
					pAudioDesc->getExtraSize(), pAudioDesc->getExtraData()[0], pAudioDesc->getExtraData()[1] );

			/* Some configs give too few information in metadata object. Need extract the parameters
			 * from the extra_data */
			debug( 1, "%s", pAudioDesc->str().c_str() );
			MP4MediaConverter::ESDS2META( pAudioDesc );
			debug( 1, "%s", pAudioDesc->str().c_str() );
		}
		else
			debug( 1, "Parsed audio extra data (repeat)" );
		return 0;
	}
	else
	{
		/* Regular video packet */
		MediaPacketPtr pPckt ( new MediaPacket( packet_t::media, content_t::audio, pMessage->message_length ) );

		/* Payload */
		pPckt->push_back( oReceiveMessage.cur(), pMessage->message_length - oReceiveMessage.size() );

		/* Deduce inc, only possible on the second packet or later */
		if( iLastAudioDts > 0 )
			pAudioDesc->inc = pMessage->timestamp * 90 - iLastAudioDts;

		/* Deduce sample size, only possible when inc is know */
		if( pAudioDesc->framesize == 0 and pAudioDesc->inc > 0 )
			pAudioDesc->framesize = pAudioDesc->inc * pAudioDesc->samplerate / 90000;
		//debug( 1, "inc %d %d", pAudioDesc->inc, pAudioDesc->framesize );

		/* Meta data */
		pPckt->unitnumber = iAudioUnit++;
		pPckt->framenumber = pPckt->unitnumber;
		pPckt->dts = iLastAudioDts = pMessage->timestamp * 90;
		pPckt->pts = pPckt->dts;
		pPckt->inc = pAudioDesc->inc;
		pPckt->mux = bAudioMov ? mux_t::MOV : mux_t::ES;
		pPckt->framestart = pPckt->frameend = true;
		pPckt->key = true;
		pPckt->desc = pAudioDesc;
		pPckt->xroute = pPckt->desc->route;
		pPckt->xstream = iAudioStream;
		pPckt->codec = pAudioDesc->codec;

		/* Convert if needed */
		FlowLock_t oLock( oFlowMutex );
		if( bAnnexB and bAudioMov )
			oAnnexB.convertES( pPckt, mBuffer, false );
		else
			mBuffer.push( pPckt.release() );
		debug( 1, "Parsed %s", mBuffer.back()->c_str_long() );
	}
	return 0;
}

int RTMPClient::handleEnd( void )
{
	if( pVideoDesc )
	{
		/* Generate a video end packet */
		MediaPacketPtr pVideo ( new MediaPacket( packet_t::end, content_t::video, 0 ) );

		pVideo->unitnumber = iVideoUnit++;
		pVideo->framenumber = pVideo->unitnumber;
		pVideo->dts = iTimestamp * 90;
		pVideo->pts = pVideo->dts;
		pVideo->inc = pVideoDesc->inc;
		pVideo->mux = mux_t::MOV;
		pVideo->framestart = pVideo->frameend = true;
		pVideo->key = false;
		pVideo->desc = pVideoDesc;
		pVideo->xroute = pVideoDesc->route;
		pVideo->xstream = iVideoStream;
		pVideo->codec = pVideoDesc->codec;
		debug( 1, "Parsed %s", pVideo->c_str() );
		{
			FlowLock_t oLock( oFlowMutex );
			mBuffer.push( pVideo.release() );
		}
	}
	/* Generate an audio end packet */
	if( pAudioDesc )
	{
		MediaPacketPtr pAudio ( new MediaPacket( packet_t::end, content_t::audio, 0 ) );

		pAudio->unitnumber = iAudioUnit++;
		pAudio->framenumber = pAudio->unitnumber;
		pAudio->dts = iTimestamp * 90;
		pAudio->pts = pAudio->dts;
		pAudio->inc = pAudioDesc->inc;
		pAudio->mux = mux_t::MOV;
		pAudio->framestart = pAudio->frameend = true;
		pAudio->key = false;
		pAudio->desc = pAudioDesc;
		pAudio->xroute = pAudioDesc->route;
		pAudio->xstream = iAudioStream;
		pAudio->codec = pAudioDesc->codec;
		debug( 1, "Parsed %s", pAudio->c_str() );
		{
			FlowLock_t oLock( oFlowMutex );
			mBuffer.push( pAudio.release() );
		}
	}
	return 0;
}

void RTMPClient::printState( void )
{
	fprintf( stderr, "IntProperty={\n" );
	for( map<string,uint64_t>::iterator i = mResponseInt.begin(); i != mResponseInt.end(); i++ )
		fprintf( stderr, "\t%s = %"LL"d\n", i->first.c_str(), i->second );
	fprintf( stderr, "}\n" );

	fprintf( stderr, "StringProperty={\n" );
	for( map<string,string>::iterator i = mResponseString.begin(); i != mResponseString.end(); i++ )
		fprintf( stderr, "\t%s = '%s'\n", i->first.c_str(), i->second.c_str() );
	fprintf( stderr, "}\n" );

	fprintf( stderr, "BooleanProperty={\n" );
	for( map<string,bool>::iterator i = mResponseBool.begin(); i != mResponseBool.end(); i++ )
		fprintf( stderr, "\t%s = %d\n", i->first.c_str(), i->second );
	fprintf( stderr, "}\n" );
}

int RTMPClient::flush( void ) synchronized
{
	bFlush = true;
	return 0;
} end_synchronized

int RTMPClient::seek( uint32_t iSeek ) synchronized
{
	sendCommandSeek( iSeek );
	return 0;
} end_synchronized

int RTMPClient::play( double fSpeed ) synchronized
{
	bWait = false;
	bSchedule = true;
	debug( 1, "play" );
	if( bPause )
	{
		bPause = false;
		return sendCommandPauseRaw( false, 0 );
	}
	return 0;
} end_synchronized

int RTMPClient::pause( void ) synchronized
{
	debug( 1, "pause" );
	if( not bPause )
	{
		bPause = true;
		return sendCommandPauseRaw( true, 0 );
	}
	return 0;
} end_synchronized
