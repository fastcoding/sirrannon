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
#include "UDPReceiver.h"
#include "Utils.h"

/**
 * UDP RECEIVER
 * @component UDP-receiver
 * @type receiver
 * @info Provides a non-blocking UDP socket from the network.
 **/

REGISTER_CLASS( UDPReceiver, "UDP-receiver" );

UDPReceiver::UDPReceiver( const string& sName, ProcessorManager* pScope )
	: MediaReceiver(sName, pScope), iLastDts(-1), pSocket(NULL)
{
	mBool["extension"] = false;
	mBool["multicast"] = false;
	mInt["port"] = 5000;
}

UDPReceiver::~UDPReceiver()
{
	if( pSocket )
		close();
	delete pSocket;
}

void UDPReceiver::init( void )
{
	/* Base class */
	MediaReceiver::init();

	/* Set local port */
	pSocket = new UDPSocket( mInt["port"] );
	pSocket->setNonThrowing();
	pSocket->setBlocking();

	/* Start already */
	bSchedule = true;

	/* Set buffersize */
	pSocket->setBufferSize( 8388608 ); /* 8 MB receive pBuffer */

	/* Multicast */
	if( mBool["multicast"] )
	{
		IPAddress addr = IPAddress( mString["server"] );
		pSocket->joinGroup( addr.getIPStr() );
	}
	createThread( bind( &UDPReceiver::handleReceive, this ) );
}

void UDPReceiver::close( void)
{
	/* Multicast */
	if( mBool["multicast"] )
	{
		IPAddress addr = IPAddress( mString["server"] );
		pSocket->leaveGroup( addr.getIPStr() );
	}
}

void UDPReceiver::handleReceive( void )
{
	while( true )
	{
		/* Try to receive something on our Socket */
		int iBuffer = pSocket->receiveRaw( pBuffer, buffer_size );

		/* Status */
		if( iBuffer < 0 )
		{
			RuntimeError( this, "Couldn't receive data from host" );
		}
		else if( iBuffer == 0 )
		{
			/* We reached the end, close connection */
			close();

			/* Make our own control packet */
			if( not mBool["extension"] )
			{
				MediaPacketPtr pPckt ( new MediaPacket( packet_t::media, content_t::mixed, 0 ) );
				debug( 1, "created - %s", pPckt->c_str() );
				route( pPckt );
			}
			return;
		}
		else
		{
			int iOffset = 0;
			while( iOffset < iBuffer )
			{
				/* Test */
				if( iOffset )
					SirannonWarning( this, "More than one UDP packet obtained by call to recv()" );

				/* Create new packet */
				MediaPacketPtr pPckt ( new MediaPacket( packet_t::media, content_t::mixed, iBuffer-iOffset ) );

				/* Is there extension data present to give detailed information? */
				if( mBool["extension"] )
				{
					/* Decode length */
					int iMessage = decode_length( pBuffer + iOffset );
					if( iMessage > iBuffer - 2 )
						ValueError( this, "Illegal encoded length(%d) buffer(%d: %s)", iMessage, iBuffer, strArray(pBuffer,iBuffer).c_str() );

					/* Acquire information extra information */
					int iHeader =  decode_header( pPckt, pBuffer + iOffset + 2 );

					/* Additional decoding */
					decode_additional( pPckt );

					/* Inc */
					if( iLastDts == -1 )
						iLastDts = pPckt->dts;
					if( pPckt->dts != iLastDts )
					{
						pDesc->inc = pPckt->dts - iLastDts;
						iLastDts = pPckt->dts;
					}
					/* Add rest */
					pPckt->push_back( pBuffer + iOffset + 2 + iHeader, iMessage - iHeader );
					iOffset += 2 + iMessage;
				}
				else
				{
					/* Add data */
					pPckt->push_back( pBuffer, iBuffer );
					iOffset = iBuffer;
				}
				/* Send */
				debug( 1, "received - %s", pPckt->c_str() );
				route( pPckt );
			}
		}
	}
}
