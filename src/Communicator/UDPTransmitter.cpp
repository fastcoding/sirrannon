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
#include "UDPTransmitter.h"
#include "Utils.h"

/**
 * UDP TRANSMITTER
 * @component UDP-transmitter
 * @type transmitter
 * @info Provides a non-blocking UDP socket to the network.
 **/

REGISTER_CLASS( UDPTransmitter, "UDP-transmitter" );

UDPTransmitter::UDPTransmitter( const string& sName, ProcessorManager* pScope )
	: MediaTransmitter(sName, pScope), pSocket(NULL), bExtension(false)
{
	/* Parameter default */
	mBool["extension"]	= false;
	mInt["port"]		        = 4000;
	mString["destination"]      = "127.0.0.1:5000";
}

UDPTransmitter::~UDPTransmitter()
{
	delete pSocket;
}

void UDPTransmitter::init( void )
{
	/* Base class */
	MediaTransmitter::init();
	bExtension = mBool["extension"];

	/* Address string */
	IPAddress oIPAddr ( mString["destination"] );
	if( not oIPAddr.valid() )
		ValueError( this, "Translated ip(%s) from address(%s) invalid",
				oIPAddr.getAddressStr().c_str(), mString["destination"].c_str() );
	oSockAddr = oIPAddr.getSockAddr();

	/* Create the socket */
	pSocket = new UDPSocket( mInt["port"], oSockAddr );

	/* Multicast */
	if( mInt["multicast-TTL"] >= 0 )
		pSocket->setMulticastTTL( mInt["multicast-TTL"] );
}

void UDPTransmitter::receive( MediaPacketPtr& pPckt )
{
	/* Additional information about the stream */
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
	debug( 1, "sending: %s", pPckt->c_str() );
	int iRet = pSocket->sendSmart( pPckt->data(), pPckt->size() );
	if( iRet < 0 )
		IOError( this, "Send failed: %s", strError() );
	else if( iRet == 0 )
		IOError( this, "Send failed: 0" );

	/* Send it away */
	route( pPckt );
}
