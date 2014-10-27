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
#include "TCPReceiver.h"
#include "Utils.h"

/**
 * TCP RECEIVER
 * @component TCP-receiver
 * @type receiver
 * @param connect, bool, false, if true, connect to the server, if false, listen for an incoming connection
 * @info Provides a non-blocking TCP socket from the network.
 **/

REGISTER_CLASS( TCPReceiver, "TCP-receiver" );

TCPReceiver::TCPReceiver( const string& sName, ProcessorManager* pScope )
	: MediaReceiver(sName, pScope), pSocket(NULL), pListen(NULL), bConnect(true)
{
	mInt["port"] = 5000;
	mBool["connect"] = false;
	mString["server"] = "127.0.0.1:5000";
}

TCPReceiver::~TCPReceiver( )
{
	delete pListen;
	delete pSocket;
}

void TCPReceiver::init( void )
{
	/* Base class */
	MediaReceiver::init();

	/* Active or passive? */
	bConnect = mBool["connect"];
	if( not bConnect )
	{
		debug( 1, "connecting to..." );
		pListen = new TCPServerSocket( mInt["port"] );
		pListen->setBlocking();
	}
	/* Create a thread */
	createThread( bind( &TCPReceiver::mainThreadA, this ) );
}

void TCPReceiver::mainThreadA( void )
{
	/* Obtain a connection */
	if( not pSocket )
	{
		if( bConnect )
		{
			IPAddress oAddress( mString["server"] );
			if( not oAddress.valid() )
				ValueError( this, "Invalid server url(%s)", mString["server"].c_str() );
			pSocket = new TCPSocket( oAddress.getIPStr(), oAddress.getPort() );
			debug( 1, "Connected(%s)", mString["server"].c_str() );
		}
		else
		{
			pSocket = pListen->accept();
			debug( 1, "Accepted connection" );
		}
	}

	/* Keep receiveing packets */
	while( true )
	{
		/* Obtain the size of the message */
		pSocket->receiveSmart( pBuffer, 2 );
		uint32_t iSize = decode_length( pBuffer );
		if( iSize > sizeof(pBuffer) )
			ValueError( this, "Message size(%d) larger than buffer size(%d)", iSize, sizeof(pBuffer) );

		/* Obtain the rest of the message */
		pSocket->receiveSmart( pBuffer, iSize );

		/* Create a packet */
		MediaPacketPtr pPckt( new MediaPacket( packet_t::media, content_t::video, iSize ) );

		/* Reconstruct packet meta data */
		uint32_t iHeader = decode_header( pPckt, pBuffer );
		decode_additional( pPckt );

		/* Reconstruct data */
		pPckt->push_back( pBuffer + iHeader, iSize - iHeader );

		/* Done */
		debug( 1, "received(%s)", pPckt->c_str() );
		FlowLock_t oLock( oFlowMutex );
		route( pPckt );
	}
}
