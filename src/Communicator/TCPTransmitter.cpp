/*****************************************************************************
 * (c) 2006-2010 Sirannon
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
#include "TCPTransmitter.h"
#include "Utils.h"

/**
 * TCP TRANSMITTER
 * @component TCP-transmitter
 * @properties threaded
 * @type transmitter
 * @param connect, bool, true, if true, connect to the server, if false, listen for an incoming connection
 * @info Provides a non-blocking TCP socket to the network.
 **/

REGISTER_CLASS( TCPTransmitter, "TCP-transmitter" );

TCPTransmitter::TCPTransmitter( const string& sName, ProcessorManager* pScope )
	: MediaTransmitter(sName, pScope), pListen(NULL), pSocket(NULL), bConnect(false),
	  bExtension(true)
{
	mInt["port"] = 5000;
	mBool["connect"] = true;
	mString["destination"] = "127.0.0.1";
	mBool["extension"] = true;
}

TCPTransmitter::~TCPTransmitter()
{
	delete pListen;
	delete pSocket;
}

void TCPTransmitter::init( void )
{
	/* Base class */
	MediaTransmitter::init();

	/* Active or passive? */
	bExtension = mBool["extension"];
	bConnect = mBool["connect"];
	if( not bConnect )
	{
		pListen = new TCPServerSocket( mInt["port"] );
		pListen->setBlocking();
	}
	/* Create a thread */
	forcePriviligedThread( -1 );
	bReceive = true; //  Kludge, will be flipped to false, the actual desired setting
	bSchedule = true;
}

void TCPTransmitter::process( )
{
	/* Create the socket */
	if( not pSocket )
	{
		if( bConnect )
		{
			IPAddress oAddress( mString["destination"] );
			pSocket = new TCPSocket( oAddress.getIPStr(), oAddress.getPort() );
			debug( 1, "Connected(%s)", mString["destination"].c_str() );
		}
		else
		{
			pSocket = pListen->accept();
			if( not pSocket )
				IOError( this, "Could not accept socket on port %d",  mInt["port"] );
			debug( 1, "Accepted connection" );
		}
		bReceive = true;
		bSchedule = false;
	}
}

void TCPTransmitter::receive( MediaPacketPtr& pPckt )
{
	/* Additional information about the stream */
	debug( 1, "Sending(%s)", pPckt->c_str() );
	if( bExtension )
	{
		/* Create the Sirannon header */
		encode_header( pPckt );
		encode_length( pPckt );
	}
	else
	{
		/* Ignore empty packets */
		if( pPckt->size() == 0 )
		{
			route( pPckt );
			return;
		}
	}

	/* Transmit the data */
	if( pSocket->sendSmart( pPckt->data(), pPckt->size() ) < 0 )
		RuntimeError( this, "Send failed: %s", pPckt->c_str() );

	/* Route */
	route( pPckt );
}
