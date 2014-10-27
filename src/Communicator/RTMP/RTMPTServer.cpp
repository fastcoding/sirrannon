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
#include "RTMPTServer.h"
#include "RandomGenerator.h"
#include "Bits.h"
#define SIRANNON_USE_BOOST_REGEX
#include "Boost.h"

/**
 * RTMPT SERVER
 * @component RTMPT-server
 * @type media-server
 * @param port, int, 80, listen to this port for incoming connections
 * @info Listens for incoming RTMPT connections. This is the tunneled version of RTMP.
 **/

REGISTER_CLASS( RTMPTServer, "RTMPT-server" );

RTMPTServer::RTMPTServer( const char* sName, ProcessorManager* pScope )
	: RTMPServer(sName, pScope)
{
	mInt["port"] = 80;
	mFields["Cache-Control"] = "no-cache";
	mFields["Connection"] = "Keep-Alive";
	mFields["Server"] = "Sirannon";
	mFields["Content-Type"] = "application/x-fcs";
}

RTMPTServer::~RTMPTServer()
{
	/* Delete rogue connections that were closed by the session but never were terminated by "close" from the client */
	int iCount = 0;
	for( map<string,RTMPTConnection*>::iterator i = mUrl2Connection.begin(); i != mUrl2Connection.end(); i++ )
	{
		if( i->second->terminateInternal )
		{
			delete i->second;
			iCount++;
		}
	}
	if( iCount )
		debug( 1, "deleting zombie connections(%d)", iCount );
}

void RTMPTServer::init( void )
{
	RTMPServer::init();
}

void RTMPTServer::process()
{
	/* Accept new connections */
	TCPSocket* pSocket;
	while( (pSocket = pListen->accept() ) )
	{
		debug( 1, "new connection" );
		pSocket->setBlocking();
		pSocket->setNonThrowing();
		createThread( bind( &RTMPTServer::handleNewSocket, this, pSocket ) );
	}
	/* Schedule the processors */
	oProcessorManager.scheduleProcessors();
}

int RTMPTServer::handleNewSocket( TCPSocket* pRawSocket )
{
	/* Protected the dynamic memory */
	auto_ptr<TCPSocket> pSocket( pRawSocket );
	auto_ptr<HTTPConnection> pHTTP( new HTTPConnection( pSocket.get(), "HTTP/1.1", bDebug ) );

	/* Parse request */
	while( parseHTTPRequest( pHTTP.get() ) >= 0 )
	{ }
	debug( 1, "Socket closed by client" );

	/* Unregister thread */
	return 0;
}

int RTMPTServer::endSession( MediaSession* pSession, ConnectionInterface* pVirtualConnection )
{
	/* Cast to RTMPT-connection */
	RTMPTConnection* pConnection = dynamic_cast<RTMPTConnection*>( pVirtualConnection );
	if( not pConnection )
		RuntimeError( this, "connection not of type RTMPT-connection" );

	/* Externaly or internal terminate? */
	if( pConnection->terminateExternal or is_stopped() )
	{
		/* We cannot unlock pConnection->oLocalMutex, however, delete destroyed the oMutex */
		debug( 1, "terminating connection (external trigger)" );
		delete pConnection;
	}
	else
	{
		/* Defer the deletion of the connection, until a new connection arrives */
		debug( 1, "deferring termination of connection (internal trigger)" );
		pConnection->terminateInternal = true; /* What if no more message comes? Time-out required! */
	}
	return 0;
}

int RTMPTServer::parseHTTPRequest( HTTPConnection* pHTTP )
{
	/* Receive an HTTP message together with the content */
	if( pHTTP->receiveMessage( true ) <= 0 )
		return -1;
	int iContent = pHTTP->getContentSize();

	/* Analyze the URL */
	RTMPTConnection* pConnection = NULL;
	int iMode = RTMPTConnection::INVALID;
	if( analyzeUrl( pHTTP->getUrl(), iMode, pConnection ) < 0 )
		return -1;

	/* Handle the different modes */
	if( iMode == RTMPTConnection::HAIL )
	{
		debug( 1, "RTMPT: mode(hail) content(%d)", iContent );
		strcpy( sResponse, "127.0.0.1" );

		if( pHTTP->sendReply( 200, "OK", (uint8_t*)sResponse, strlen(sResponse), mFields ) < 0 )
			return -1;
	}
	else if( iMode == RTMPTConnection::OPEN )
	{
		debug( 1, "RTMPT: mode(open) content(%d)", iContent );
		strcpy( sResponse, pConnection->hash.c_str() );
		strcat( sResponse, "\n" );

		if( pHTTP->sendReply( 200, "OK", (uint8_t*)sResponse, strlen(sResponse), mFields ) < 0 )
			return -1;
	}
	else if( iMode == RTMPTConnection::SEND or iMode == RTMPTConnection::IDLE )
	{
		Lock_t oLock( pConnection->oMutex );
		if( iMode == RTMPTConnection::SEND )
		{
			debug( 1, "RTMPT: mode(send) content(%d)", iContent );

			/* Shift buffer back to start */
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
			/* Enough room in buffer? */
			if( iContent > pConnection->buffer_in_end - pConnection->offset_in )
				return SirannonWarning( this,  "input buffer overflow: session is not retrieving data fast enough");

			/* Copy HTTP content data */
			memcpy( pConnection->offset_in, pHTTP->getContent(), pHTTP->getContentSize() );
			pConnection->offset_in += iContent;

			/* Signal that data is available */
			//debug( 3, "received data(%d)", iContent );
			pConnection->oCondition.notify_all();
		}
		else
		{
			debug( 1, "RTMPT: mode(idle) content(%d)", iContent );
		}

		/* Prepare the HTTP reply header */
		int iReplyContent = pConnection->offset_out - pConnection->buffer_out;
		pConnection->buffer_out[-1] = 0x01; // Always prepend 0x01

		/* Send OK */
		if( pHTTP->sendReply( 200, "OK", pConnection->buffer_out - 1, iReplyContent + 1, mFields ) < 0 )
			return -1;

		/* Buffer is processed */
		pConnection->offset_out = pConnection->buffer_out;
	}
	else if( iMode == RTMPTConnection::CLOSE )
	{
		/* Prepare the HTTP reply header */
		debug( 1, "RTMPT: mode(close) content(%d)", iContent );
		sResponse[0] = 0x01;

		/* Send OK */
		if( pHTTP->sendReply( 200, "OK", (uint8_t*)sResponse, 1, mFields ) < 0 )
			return -1;

		/* Set the terminate flag */
		Lock_t oLock( pConnection->oMutex );
		debug( 1, "deferring termination of connection (external trigger)" );
		pConnection->terminateExternal = true;
		pConnection->oCondition.notify_all();
	}
	/* Release lock and maybe kill the connection */
	if( pConnection )
	{
		if( pConnection->terminateInternal and pConnection->terminateExternal )
		{
			Lock_t oLock( oLocalMutex );
			debug( 1, "terminating connection (internal+external trigger)" );
			mUrl2Connection.erase( pConnection->hash );
			delete pConnection;
		}
	}
	return 0;
}

static const regex oUrlExpression( "/(\\w+)/(\\w+)(?:/(\\d+))?" );
static const regex oHeaderExpression( "([\\w\\-]+): *(\\S+)" );

int RTMPTServer::analyzeUrl( const string& sUrl, int& iMode, RTMPTConnection*& pConnection )
{
	/* Parse the POST line */
	cmatch oParse;
	if( not regex_search( sUrl.c_str(), oParse, oUrlExpression ) )
		return SirannonWarning( this,  "SyntaxError: malformatted line(%s)", sUrl.c_str() );

	/* Matches */
	string sMode( oParse[1].first, oParse[1].second );
	string sHash( oParse[2].first, oParse[2].second );
	int iVersion = oParse[3].matched ? atoi( oParse[3].first ) : 0;

	if( sMode == "idle" or sMode == "send" )
	{
		/* Retrieve connection */
		Lock_t oLock( oLocalMutex );
		pConnection = mUrl2Connection[sHash];
		if( not pConnection )
		{
			mUrl2Connection.erase( sHash );
			return SirannonWarning( this,  "ValueError: session(%s) does not exist, command(%s)", sHash.c_str(), sUrl.c_str() );
		}
		/* Mode */
		if( sMode == "idle" )
			iMode = RTMPTConnection::IDLE;
		else if( sMode == "send" )
			iMode = RTMPTConnection::SEND;
		else
			return SirannonWarning( this,  "ValueError: mode(%s) not supported, command(%s)", sMode.c_str(), sUrl.c_str() );
		debug( 2, "HTTP POST: mode(%s) hash(%s) version(%d)", sMode.c_str(), sHash.c_str(), iVersion );
	}
	else if( sMode == "open" )
	{
		/* Create a new connection & session  */
		pConnection = new RTMPTConnection();

		/* Create a new hash of 16 bytes */
		OBits oBase( 17 );
		for( int i = 0; i < 16; i++ )
			oBase.write( 8, oRandom.crand() );
		oBase.write( 8, 0 );
		sHash.assign( (char*) oBase.data() );

		/* Sanity */
		Lock_t oLock( oLocalMutex );
		if( mUrl2Connection[sHash] )
			return SirannonWarning( this,  "ValueError: hash(%s) already exists, command(%s)", sHash.c_str(), sUrl.c_str() );

		/* Save mappings */
		iMode = RTMPTConnection::OPEN;
		pConnection->hash = sHash;
		mUrl2Connection[sHash] = pConnection;

		/* Safely create the session */
		MediaSession* pSession = newSession( pConnection );
		debug( 2, "HTTP POST: mode(open) hash(%s) version(1)", sHash.c_str() );
	}
	else if( sMode == "fcs" )
	{
		/* Mapping */
		iMode = RTMPTConnection::HAIL;
		debug( 2, "HTTP POST: mode(hail)" );
	}
	else if( sMode == "close" )
	{
		/* Retrieve connection */
		Lock_t oLock( oLocalMutex );
		pConnection = mUrl2Connection[sHash];
		if( not pConnection )
		{
			return SirannonWarning( this,  "ValueError: session(%s) does not exist, command(%s)", sHash.c_str(), sUrl.c_str() );
		}
		/* Scrap the mapping */
		mUrl2Connection.erase( sHash );

		iMode = RTMPTConnection::CLOSE;
		debug( 2, "HTTP POST: mode(close) hash(%s) version(%d)", sHash.c_str(), iVersion );
	}
	else
		return SirannonWarning( this,  "SyntaxError: malformatted mode(%s) hash(%s) version(%d) line(%s)", sMode.c_str(), sHash.c_str(), iVersion, sUrl.c_str() );
	return 0;
}
