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
#ifndef RTMP_SESSION_H_
#define RTMP_SESSION_H_
#include "RTMPChunkStream.h"
#include "Communicator/MediaSession.h"
#include "RTMPHandshake.h"

class RTMPSession : public MediaSession, public RTMPChunkStream
{
public:
	RTMPSession( const char* sName, ProcessorManager* pProc );
	~RTMPSession();

protected:
	void init( void );
	void process( void );
	virtual void handleError( SirannonException* pException, ProcessorManager* pManager, MediaProcessor* pProcessor );

	/* Server thread */
	virtual void mainThreadA( void );

	/* Streaming part */
	void receive( MediaPacketPtr& pPckt );
	void receive_end( MediaPacketPtr& pPckt );
	void receive_reset( MediaPacketPtr& pPckt );
	int initStream( const string& sRequestedStream );
	int createStream( void );
	int streamPlay( double fSpeed=-1. );
	int streamPause( void );
	int streamSeek( uint32_t iSeek );

	/* Server state handlers */
	int handleHandshake( void );
	int handleApplicationConnect( void );
	int handlePlay( const string& sRequestedStream );
	int handleSeek( void );
	int handleStreaming( void );
	int handleStreamEnd( void );
	int handleRestart( void );

	/* Client request handlers */
	int handleCommandAMF0( message_t* pMessage );
	int handleCommandAMF3( message_t* pMessage );
	int handleCommandConnect( AMF::version iAMF, AMFIBits& oReceiveMessage );
	int handleCommandPlay( AMF::version iAMF, AMFIBits& oReceiveMessage, uint32_t iMessage );
	int handleCommandCreateStream( AMF::version iAMF, AMFIBits& oReceiveMessage );
	int handleCommandPause( AMF::version iAMF, AMFIBits& oReceiveMessage );
	int handleCommandPauseRaw( AMF::version iAMF, AMFIBits& oReceiveMessage );
	int handleCommandSeek( AMF::version iAMF, AMFIBits& oReceiveMessage );
	int handleCommandStreamLength( AMF::version iAMF, AMFIBits& oReceiveMessage );
	int handleCommandDeleteStream( AMF::version iAMF, AMFIBits& oReceiveMessage );

	/* Server responses and commands */
	int sendCommandConnectResponse( bool bFailure );
	int sendCommandCreateStreamResponse( bool bFailure );
	int sendCommandStreamLength( void );
	int sendMetaData( void );
	int sendCommand( const char* sCode, const char* sDesc="", const char* sCmd="onStatus" );

	/* Tools */
	int printMessage( message_t* pMessage );

	const static int iRTMPVersion = 3;
	AMF::version iAMF;
	int iTransID, iStart, iDuration, iAMFError, iCapabilities, iClientID,
	iMessages, iAudioID, iVideoID, iSeek, iMainStream,
	iSeekTarget, iStreamer;
	timestamp_t iFinalTimestamp, iCurrent;
	double fQuickBuffer;
	serverState_t iServerState;
	bool bFirstCommandChunk, bCrypto, bPause, bUnpause, bVideo, bAudio, bPauseAfterSeek;
	string sUrl, sMedia, sFileName, sTag, sApp;
	MediaProcessor* pStreamer;
	StreamerInterface* pStreamerInterface;
	SirannonTime oServerEpoch;
	RTMPHandshake oHandshake;
};

#endif /* RTMP_SESSION_H_ */
