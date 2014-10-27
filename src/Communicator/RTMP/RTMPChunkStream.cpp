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
#include "RTMPChunkStream.h"
#include <signal.h>

RTMPChunkStream::RTMPChunkStream( void )
 : 	pConnection(NULL),
	oReceiveChunk(64), oSendChunk(MAX_MESSAGE_SIZE),
	iReceivedBytes(0), iDeltaReceivedBytes(0), iSentBytes(0),
	iSelfACKWindow(64000), iOtherChunkSize(128), iSelfChunkSize(128), iOtherACKWindow(64000),
	iOtherBuffer(0), iSetBufferCounter(0), oLastPing(0), bPing(false), iStreamID(1),
	oTimeOut(5000), oSendMessage(MAX_CHUNK_SIZE)
{
	mHandler[SET_CHUNK_SIZE] = &RTMPChunkStream::handleSetChunkSize;
	mHandler[ABORT_MESSAGE] = &RTMPChunkStream::handleAbort;
	mHandler[ACK] = &RTMPChunkStream::handleACK;
	mHandler[USER_CONTROL] = &RTMPChunkStream::handleUserCtrl;
	mHandler[WINDOW_ACK_SIZE] = &RTMPChunkStream::handleWindowACKSize;
	mHandler[SET_PEER_BANDWIDTH] = &RTMPChunkStream::handleSetPeerBandwidth;
	mHandler[AUDIO] = &RTMPChunkStream::handleAudio;
	mHandler[VIDEO] = &RTMPChunkStream::handleVideo;
	mHandler[COMMAND_AMF3] = &RTMPChunkStream::handleCommandAMF3;
	mHandler[COMMAND_AMF0] = &RTMPChunkStream::handleCommandAMF0;
	mHandler[METADATA_AMF3] = &RTMPChunkStream::handleMetaDataAMF3;
	mHandler[METADATA_AMF0] = &RTMPChunkStream::handleMetaDataAMF0;
	mHandler[SHARED_AMF3] = &RTMPChunkStream::handleSharedObjectAMF3;
	mHandler[SHARED_AMF0] = &RTMPChunkStream::handleSharedObjectAMF0;
	mHandler[AGGREGATE] = &RTMPChunkStream::handleAggregate;
}

RTMPChunkStream::~RTMPChunkStream()
{
	for( map<int,message_t*>::iterator i = mReceiveChunk.begin(); i != mReceiveChunk.end(); i++ )
		delete i->second;
	for( map<int,message_t*>::iterator i = mSendChunk.begin(); i != mSendChunk.end(); i++ )
		delete i->second;
}

void RTMPChunkStream::init( ConnectionInterface* pConnection )
{
	/* Identifier */
	this->pConnection = pConnection;
}

int RTMPChunkStream::receiveMessages( uint32_t iMessageType, const char* sCommand )
{
	/* Keep receiving messages untill we see the desired one */
	message_t* pMessage = NULL;
	do
	{
		pMessage = receiveMessage();
		if( not pMessage )
			return -1;
	}
	while( pMessage->message_type_id != iMessageType or
		   ( sCommand and pMessage->command != sCommand ) );
	return 0;
}

message_t* RTMPChunkStream::receiveMessage( void )
{
	/* Keep receiving chunks until we get a complete message
	 * NOTE: we can receive parts of different messages interleaved
	 */
	message_t* pMessage = NULL;
	do
	{
		pMessage = receiveChunk();
		if( not pMessage )
			return NULL;
	}
	while( pMessage->current_length > 0 );

	/* Handle the message according the message type ID */
	//debug( 4, "received\n%s", pMessage->oBuffer.str(pMessage->message_length).c_str() );
	handlerMethod_t pFunc = mHandler[pMessage->message_type_id];
	if( not pFunc )
	{
		/* Failure */
		SirannonWarning( "Message type(%d) ignored", pMessage->message_type_id );
		return pMessage;
	}
	else
	{
		/* Execute the handler */
		int iRet = (this->*(pFunc))( pMessage );
		if( iRet < 0 )
		{
			if( iRet == -2 )
				RuntimeError( "Message type(%d) not supported", pMessage->message_type_id );
			return NULL;
		}
		else
			return pMessage;
	}
	return NULL;
}

message_t* RTMPChunkStream::receiveChunk( void )
{
	/* Prepare */
	oReceiveChunk.clear();

	/* Receive first byte */
	if( receiveFromSocket( oReceiveChunk.data(), 1 ) <= 0 )
		return NULL;
	uint8_t iFmt = oReceiveChunk.read( 2 );
	uint8_t iCsID = oReceiveChunk.read( 6 );

	/* Calculate header */
	int iSmartRcv = 0;
	if( iCsID == 0 )
		iSmartRcv += 1;
	else if( iCsID == 1 )
		iSmartRcv += 2;

	if( iFmt == 0 )
		iSmartRcv += 11;
	else if( iFmt == 1 )
		iSmartRcv += 7;
	else if( iFmt == 2 )
		iSmartRcv += 3;

	/* Receive the rest of the header */
	if( iSmartRcv > 0 )
	{
		if( receiveFromSocket( oReceiveChunk.data() + 1, iSmartRcv ) <= 0 )
			return NULL;
	}
	/* Calculate the chunkID */
	int iChunkID;
	if( iCsID == 0 )
		iChunkID = 64 + oReceiveChunk.read( 8 );
	else if( iCsID == 1 )
		iChunkID = 64 + oReceiveChunk.read( 8 ) + oReceiveChunk.read( 8 ) * 256;
	else
		iChunkID = iCsID;

	/* Finally we can map it to the correct message */
	bool bNew = false;
	message_t* pMessage = mReceiveChunk[iChunkID];
	if( not pMessage )
	{
		mReceiveChunk[iChunkID] = pMessage = new message_t();
		bNew = true;
	}
	pMessage->cs_id = iChunkID;

	/* When a message is fragmented only fmt 3 is allowed for the non-first fragments */
	if( iFmt < 3 and pMessage->current_length )
	{
		RuntimeError( "Chunk stream (%d) corrupted, new message while previous message not completed", iChunkID );
		return NULL;
	}
	/* Very first chunk must have format 0 */
	if( bNew and iFmt > 0 )
	{
		RuntimeError( "Chunk stream (%d) corrupted, first message of chunk stream must have format 0", iChunkID );
		return NULL;
	}
	/* Parse chunk message header */
	if( iFmt == 0 )
	{
		pMessage->timestamp = oReceiveChunk.read( 24 );
		pMessage->timestamp_delta = 0;
		pMessage->message_length = oReceiveChunk.read( 24 );
		pMessage->message_type_id = oReceiveChunk.read( 8 );
		pMessage->message_stream_id = oReceiveChunk.read( 32 );
	}
	else if( iFmt == 1 )
	{
		pMessage->timestamp_delta = oReceiveChunk.read( 24 );
		pMessage->message_length = oReceiveChunk.read( 24 );
		pMessage->message_type_id = oReceiveChunk.read( 8 );
	}
	else if( iFmt == 2 )
	{
		pMessage->timestamp_delta = oReceiveChunk.read( 24 );
	}
	/* Extended timestamp */
	if( pMessage->timestamp >= 0x00FFFFFF )
	{
		if( receiveFromSocket( oReceiveChunk.data() + oReceiveChunk.size(), 4 ) <= 0 )
			return NULL;
		pMessage->timestamp = oReceiveChunk.read( 32 );
	}
	/* Security */
	if( pMessage->message_length > MAX_MESSAGE_SIZE )
		OutOfBoundsError( "RTMP message too large: %d > %d", pMessage->message_length, MAX_MESSAGE_SIZE );

	/* Determine payload size */
	//debug( 4, "received chunk\n%s", oReceiveChunk.str( oReceiveChunk.size() ).c_str() );
	int	iPayloadSize = MIN( pMessage->message_length - pMessage->current_length, (uint32_t)iOtherChunkSize );

	/* Add to message */
	if( iPayloadSize > 0 and receiveFromSocket( pMessage->oBuffer.data() + pMessage->current_length, iPayloadSize ) <= 0 )
		return NULL;
	pMessage->current_length += iPayloadSize;
//	fprintf( stderr, "received chunk(%d) message(%d) payload(%d) total(%d) current(%d)\n",
//			iChunkID, pMessage->message_type_id, iPayloadSize, pMessage->message_length, pMessage->current_length );

	/* End of message? */
	if( pMessage->current_length == pMessage->message_length )
	{
		pMessage->current_length = 0;
		pMessage->timestamp += pMessage->timestamp_delta;
		pMessage->oBuffer.clear();
	}
	/* Check for ACK */
	if( iSelfACKWindow > 0 and iDeltaReceivedBytes >= iSelfACKWindow )
	{
		if( sendACK() )
		{
			RuntimeError ( "Receive chunk, failed to send ACK" );
			return NULL;
		}
	}
	return pMessage;
}

int RTMPChunkStream::receiveFromSocket( uint8_t* pData, int iSize )
{
	/* Receive */
	int iStatus;
	Lock_t oLock( oReceiveMutex );
	iStatus = pConnection->receiveSmart( pData, iSize, oTimeOut );
	if( iStatus > 0 )
	{
		iReceivedBytes += iSize;
		iDeltaReceivedBytes += iSize;
	}
	/* Succes? */
	return iStatus;
}

int RTMPChunkStream::sendToSocket( const uint8_t* pData, int iSize )
{
	/* Send */
	int iStatus;
	iStatus = pConnection->sendSmart( pData, iSize );
	if( iStatus > 0 )
		iSentBytes += iSize;

	/* Succes? */
	return iStatus;
}

int RTMPChunkStream::sendMessage( AMFOBits& oSendMessage, int iMessageType, int iChunkID, int iStreamID, uint32_t iTimeStamp )
{
	uint8_t* pData = oSendMessage.data();
	int iMessage = oSendMessage.size();
	int iSendMessage = 0;
	int iChunks = 0;

	/* Determine which format to use */
	int iStatus;
	message_t* pMessage = mSendChunk[iChunkID];
	if( not pMessage )
		mSendChunk[iChunkID] = pMessage = new message_t();

	if( pMessage->message_length == 0 or iTimeStamp > 0 )
		pMessage->fmt = 0;
	else if( pMessage->message_type_id == (uint32_t) iMessageType and
			 pMessage->message_length == (uint32_t) iMessage	)
		pMessage->fmt = 3;
	else
		pMessage->fmt = 1;
	/* Do like WoWZa, always full header */
	//pMessage->fmt = 0;

	/* Remember */
	pMessage->cs_id = iChunkID;
	pMessage->message_type_id = iMessageType;
	pMessage->message_length = iMessage;

	/* Write first chunk */
	oSendChunk.clear();
	oSendChunk.write( 2, pMessage->fmt ); // fmt
	oSendChunk.write( 6, iChunkID );
	if( pMessage->fmt < 3 )
	{
		oSendChunk.write( 24, iTimeStamp ); // timestamp
		if( pMessage->fmt < 2 )
		{
			oSendChunk.write( 24, iMessage );
			oSendChunk.write( 8, iMessageType );
			if( pMessage->fmt < 1 )
			{
				oSendChunk.write( 8, iStreamID );
				oSendChunk.write( 24, 0 );
			}
		}
	}
	/* Write the payloads */
	while( iSendMessage < iMessage )
	{
		int iPayload = MIN( iMessage - iSendMessage, iSelfChunkSize );
		if( iSendMessage > 0 )
		{
			oSendChunk.write( 2, 3 );
			oSendChunk.write( 6, iChunkID );
		}
		oSendChunk.write_buffer( pData + iSendMessage, iPayload );
		iSendMessage += iPayload;
		iChunks++;
	}
	/* Debug */
//	if( bDebug and pScope->getVerbose() >= 4 )
//	{
//		char* s = strArray( oSendChunk.data(), oSendChunk.size() );
//		fprintf( stderr, "sending length(%d) stream_id(%d) type(%02X) chunk_id(%d) chunks(%d)\n%s\n\n",
//			iMessage, 0, iMessageType, iChunkID, iChunks, s );
//		delete [] s;
//	}
	/* Send */
	if( sendToSocket( oSendChunk.data(), oSendChunk.size() ) <= 0 )
		return -1;
	else
		return 0;
}

int RTMPChunkStream::syncMessages( int iMsgType, const char* sCommand, const int* pCounter, int iTarget )
{
	while( *pCounter < iTarget )
	{
		if( receiveMessages( iMsgType, sCommand ) )
			return -1;
	}
	return 0;
}

int RTMPChunkStream::sendACK( void )
{
	Lock_t oLock( oSendMutex );
	oSendMessage.clear();
	oSendMessage.write( 32, iReceivedBytes );
	//debug( 3, "SENDING: (Control.ACK) delta(%d) total(%d)", iDeltaReceivedBytes, iReceivedBytes );
	iDeltaReceivedBytes = 0; // FIXME SendBytes upated in other thread by receive
	return sendMessage( oSendMessage, ACK, 2, 0, 0 );
}

int RTMPChunkStream::sendSetChunkSize( uint32_t iNewChunkSize )
{
	iSelfChunkSize = iNewChunkSize;
	Lock_t oLock( oSendMutex );
	oSendMessage.clear();
	oSendMessage.write( 32, iSelfChunkSize );
	//debug( 1, "SENDING: (Control.SetChunkSize) size(%d)", iSelfChunkSize );
	return sendMessage( oSendMessage, SET_CHUNK_SIZE, 2, 0, 0 );
}

int RTMPChunkStream::sendWindowACKSize( void )
{
	/* Server imposes this Windows ACK Size
	 * when ACKing messages from the server */
	Lock_t oLock( oSendMutex );
	oSendMessage.clear();
	oSendMessage.write( 32, iOtherACKWindow );
	//debug( 1, "SENDING: (Control.WindowACKSize) other-window(%d)", iOtherACKWindow );
	return sendMessage( oSendMessage, WINDOW_ACK_SIZE, 2, 0, 0 );
}

int RTMPChunkStream::sendSetPeerBandwidth( void )
{
	Lock_t oLock( oSendMutex );
	oSendMessage.clear();
	oSendMessage.write( 32, iSelfACKWindow );
	oSendMessage.write( 8, 0 );
	//debug( 1, "SENDING: (Control.SetPeerBandwidth) self-window(%d)", iSelfACKWindow );
	return sendMessage( oSendMessage, SET_PEER_BANDWIDTH, 2, 0, 0 );
}

int RTMPChunkStream::sendUserControl( userControl_t iType, int iVal )
{
	Lock_t oLock( oSendMutex );
	oSendMessage.clear();
	oSendMessage.write( 16, iType );
	//int iDebug = 1;
	//const char* sType = NULL;
	switch( iType )
	{
	case STREAM_BEGIN:
		//sType = "UserControl.Stream.Begin";
		oSendMessage.write( 32, iVal );
		break;

	case STREAM_IS_RECORDED:
		//sType = "UserControl.Stream.IsRecorded";
		oSendMessage.write( 32, iVal );
		break;

	case PING_REQUEST:
		//sType = "UserControl.Ping.Request";
		oSendMessage.write( 32, iVal );
		//iDebug = 3;
		break;

	case STREAM_EOF:
		//sType = "UserControl.Stream.EOF";
		oSendMessage.write( 32, iVal );
		break;

	case PING_RESPONSE:
		//sType = "UserControl.Ping.Response";
		oSendMessage.write( 32, iVal );
		//iDebug = 3;
		break;

	case SET_BUFFER_LENGTH:
		//sType = "UserControl.Buffer.SetLength";
		oSendMessage.write( 32, iStreamID );
		oSendMessage.write( 32, iVal );
		break;

	default:
		return -1;
	}
	//debug( 4, "SENDING: (%s) value(%u)", sType, iVal );
	return sendMessage( oSendMessage, USER_CONTROL, 2, 0, 0 );
}

int RTMPChunkStream::handleSetChunkSize( message_t* pMessage )
{
	AMFIBits& oReceiveMessage = pMessage->oBuffer;
	int iRqClientChunkSize = oReceiveMessage.read( 32 );
	if( iOtherChunkSize >= 128 and iOtherChunkSize <= 65536 )
	{
		iOtherChunkSize = iRqClientChunkSize;
		//debug( 1, "RECEIVED: (Control.SetChunkSize) value(%d)", iRqClientChunkSize );
		return 0;
	}
	else
	{
		SirannonWarning( "Requested chunk size out of bounds: %d not in [128,65536], resolving to 1024", iRqClientChunkSize );
		return 1024;
	}
}

int RTMPChunkStream::handleAbort( message_t* pMessage )
{
	AMFIBits& oReceiveMessage = pMessage->oBuffer;
	int iStreamID = oReceiveMessage.read( 32 );
	//debug( 1, "RECEIVED: (Control.Abort)" );
	return 0;
}

int RTMPChunkStream::handleACK( message_t* pMessage )
{
	AMFIBits& oReceiveMessage = pMessage->oBuffer;
	int iClientReportedBytes = oReceiveMessage.read( 32 );
	//debug( 3, "RECEIVED: (Control.ACK) received(%d)", iClientReportedBytes );
	return 0;
}

int RTMPChunkStream::handleUserCtrl( message_t* pMessage )
{
	AMFIBits& oReceiveMessage = pMessage->oBuffer;
	int iType = oReceiveMessage.read( 16 ), iVal = 0;
	//const char* sType = "";
	int iRet = 0;
	//int iDebug = 1;

	switch( iType )
	{
	case STREAM_BEGIN:
		//sType = "UserControl.Stream.Begin";
		break;

	case STREAM_DRY:
		//sType = "UserControl.Stream.Dry";
		break;

	case STREAM_EOF:
		//sType = "UserControl.Stream.EOF";
		break;

	case STREAM_IS_RECORDED:
		//sType = "UserControl.Stream.IsRecorded";
		break;

	case SET_BUFFER_LENGTH:
		oReceiveMessage.seek( 4 );
		iOtherBuffer = iVal = oReceiveMessage.read( 32 );
		iSetBufferCounter++;
		//sType = "UserControl.Buffer.SetLength";
		break;

	case PING_REQUEST:
		//sType = "UserControl.Ping.Request";
		iVal = oReceiveMessage.read( 32 );
		iRet = sendUserControl( PING_RESPONSE, iVal );
		//iDebug = 3;
		break;

	case PING_RESPONSE:
		iVal = oReceiveMessage.read( 32 );
		//sType = "UserControl.Ping.Response";
		//iDebug = 3;
		break;
	}
	//debug( iDebug, "RECEIVED: (%s) value(%u)", sType, iVal );
	return iRet;
}

int RTMPChunkStream::handleWindowACKSize( message_t* pMessage )
{
	/* Client imposes this Window ACK Size
	 * when ACKing control messages from the client */
	AMFIBits& oReceiveMessage = pMessage->oBuffer;
	iSelfACKWindow = oReceiveMessage.read( 32 ); // Recveived bytes by the client
	oLastPing = SirannonTime::getCurrentTime();
	bPing = true;
	//debug( 1, "RECEIVED: (Control.WindowsACKSize) forced-window(%d)", iSelfACKWindow );
	return 0;
}

int RTMPChunkStream::handleSetPeerBandwidth( message_t* pMessage )
{
	AMFIBits& oReceiveMessage = pMessage->oBuffer;
	int iRequestedOtherACKWindow = oReceiveMessage.read( 32 );
	int iImportance = oReceiveMessage.read( 8 );
	//debug( 1, "RECEIVED: (Control.SetPeerbandwidth) new-other-window(%d) old-other-window(%d)", iRequestedOtherACKWindow, iOtherACKWindow );
	iOtherACKWindow = iRequestedOtherACKWindow;
	return sendWindowACKSize();
}
