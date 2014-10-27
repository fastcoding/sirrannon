#include "RTMPTClient.h"
#include "Url.h"

/**
 * RTMPT CLIENT
 * @component RTMPT-client
 * @type media-client
 * @param polling, int, 1000, in ms, the time interval between two successive HTTP POSTs during play
 * @info Requests and receives a stream via RTMPT. Generates audio and video frames.
 **/
REGISTER_CLASS( RTMPTClient, "RTMPT-client" );

RTMPTClient::RTMPTClient( const char* sName, ProcessorManager* pProc )
	: RTMPClient(sName, pProc), iCounter(0), pSocket(NULL), oSleep(1000), pTunneledThread(NULL),
	  cZero(0), pHTTP(NULL)
{
	mInt["polling"] = 1000;
	mString["server"] = "localhost";
	mInt["port"] = 1937;
	mString["application"] = "streamer";
	mFields["Accept"] = "*/*";
	mFields["Cache-Control"] = "no-cache";
	mFields["Connection"] = "Keep-Alive";
	mFields["User-Agent"] = "Sirannon";
	mFields["Content-Type"] = "application/x-fcs";
}

RTMPTClient::~RTMPTClient()
{
	delete pSocket;
	delete pHTTP;
}

void RTMPTClient::init( void )
{
	RTMPClient::init();

	oSleep = SirannonTime( mInt["polling"] );
}

void RTMPTClient::mainThreadA( void )
{
	/* Verify */
	sUrl = mString["url"];
	if( not sUrl.length() )
		ValueError( this, "URL is an empty string" );
	URL_canonize( sUrl, "rtmpt" );
	URL_parse( sUrl, sProtocol, sServer, iPort, sApp, sMedia );

	/* Parse the address */
	IPAddress oAddr ( sServer, iPort );
	if( not oAddr.valid() )
		RuntimeError( this, "invalid server address(%s)", oAddr.getAddressStr().c_str() );
	sUrl = string("rtmpt://") + oAddr.getAddressStr() + "/" + sApp;
	debug( 1, "connecting(%s)", sUrl.c_str() );

	/* Create the TCP connection */
	pSocket = new TCPSocket( oAddr.getIPStr(), oAddr.getPort() );
	pHTTP = new HTTPConnection( pSocket, "HTTP/1.1", bDebug );
	RTMPClient::pConnection = new RTMPTConnection();
	RTMPTClient::pConnection = dynamic_cast<RTMPTConnection*>( RTMPClient::pConnection );

	/* Create the tunneling thread */
	pTunneledThread = createThread( bind( &RTMPTClient::mainThreadB, this ) );

	/* Main activity */
	handleCommunication();
}

int RTMPTClient::handleEnd( void )
{
	/* Base class */
	RTMPClient::handleEnd();

	/* End the tunnel thread */
	cancelThread( pTunneledThread->get_id() );

	/* Send the close message */
	snprintf( sCommand, sizeof(sCommand), "/close/%s", sHash.c_str() );
	pHTTP->sendMessage( "POST", sCommand, &cZero, 1, mFields );
	return 0;
}

void RTMPTClient::mainThreadB( void )
{
	/* Open */
	pHTTP->sendMessage( "POST", "/open/1", &cZero, 1, mFields );
	parseHTTPResponse( "open" );

	/* Start polling the server */
	uint32_t iContent = 0;
	while( true )
	{
		/* How much content to send? */
		{
			Lock_t oLock( pConnection->oMutex );
			iContent = pConnection->offset_out - pConnection->buffer_out;
			if( iContent == 0 )
			{
				snprintf( sCommand, sizeof(sCommand), "/idle/%s/%d", sHash.c_str(), iCounter++  );
				pHTTP->sendMessage( "POST", sCommand, &cZero, 1, mFields );
			}
			else
			{
				snprintf( sCommand, sizeof(sCommand)-1, "/send/%s/%d", sHash.c_str(), iCounter++  );
				pHTTP->sendMessage( "POST", sCommand, pConnection->buffer_out, iContent, mFields );
				pConnection->offset_out = pConnection->buffer_out;
			}
		}
		/* Receive reply */
		parseHTTPResponse( "send" );

		/* Sleep in between */
		if( ready() and (not bAudio or iAudioUnit > 250) and (not bVideo or iVideoUnit > 250) )
			oSleep.sleep();
		//else
		//	oQuantum.sleep();
	}
}


int RTMPTClient::parseHTTPResponse( const char* sMode )
{
	/* Receive the reply */
	debug( 1, "parsing HTTP response" );
	pHTTP->receiveReply( false );

	/* Prepare buffer */
	int iRemaining = pHTTP->getContentSize();
	if( iRemaining > 1 )
	{
		/* Shift buffer back to start */
		Lock_t oLock( pConnection->oMutex );
		int iCurrent = pConnection->offset_in - pConnection->current_in;
		if( iCurrent >= 0 )
		{
			if( iCurrent > 0 )
			{
				SirannonWarning( this,  "unprocessed data(%d bytes) remaining in buffer", iCurrent );
				memmove( pConnection->buffer_in, pConnection->current_in, iCurrent );
			}
			pConnection->offset_in -=  pConnection->current_in - pConnection->buffer_in;
			pConnection->current_in = pConnection->buffer_in;
		}
	}
	/* Receive the content */
	if( pHTTP->getResidualSize() > 0 )
	{
		int iReceive = MIN( pHTTP->getResidualSize(), iRemaining );
		if( not strcmp( sMode, "open" ) )
		{
			Lock_t oLock( pConnection->oMutex );
			memcpy( pConnection->offset_in, pHTTP->getContent(), iReceive );
			pConnection->offset_in += iReceive;
			debug( 1, "RECEIVE OPEN: %d", iReceive );
		}
		else if( iReceive > 1 )
		{
			Lock_t oLock( pConnection->oMutex );
			memcpy( pConnection->offset_in, pHTTP->getContent() + 1, iReceive - 1 );
			pConnection->offset_in += iReceive - 1;
			debug( 1, "RECEIVE (1+DATA): %d", iReceive - 1 );
		}
		else
		{
			debug( 1, "RECEIVE (IDLE): %d", iReceive - 1 );
		}
		iRemaining -= iReceive;
	}
	else
	{
		/* Special case */
		uint8_t sTmp;
		if( strcmp( sMode, "open" ) )
		{
			debug( 1, "RECEIVE (1): 1" );
			pSocket->receiveSmart( &sTmp, 1 );
			iRemaining--;
		}
	}
	/* Receive the rest */
	if( iRemaining > 0 )
	{
		debug( 1, "RECEIVE (REM): %d", iRemaining );
		Lock_t oLock( pConnection->oMutex );
		pSocket->receiveSmart( pConnection->offset_in, iRemaining );
		pConnection->offset_in += iRemaining;
	}
	/* Special cases */
	if( not strcmp( sMode, "open" ) )
	{
		Lock_t oLock( pConnection->oMutex );
		sHash.assign( (char*)pConnection->offset_in - pHTTP->getContentSize(), pHTTP->getContentSize() - 1 );
		debug( 1, "Obtained hash(%s)", sHash.c_str() );
		pConnection->hash = sHash;
		pConnection->offset_in -= pHTTP->getContentSize();
	}
	return 0;
}
