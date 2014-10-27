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
#ifndef UDP_TRANSMITTER_H_
#define UDP_TRANSMITTER_H_
#include "Communicator.h"
#include "SirannonSocket.h"

class UDPTransmitter : public MediaTransmitter
{
public:
	UDPTransmitter( const string& sName, ProcessorManager* pScope );
	~UDPTransmitter( );

protected:
	/* Initialize and create the Socket */
	void init( void );

	/* Transmit a packet over the network */
	void receive( MediaPacketPtr& pckt );
	void receive_end( MediaPacketPtr& pckt )   { receive( pckt ); }
	void receive_reset( MediaPacketPtr& pckt ) { receive( pckt ); }

	/* Our Socket */
	UDPSocket* pSocket;
	uint8_t header [128];
	sockaddr_in oSockAddr;
	bool bExtension;
};

#endif /*UDP_TRANSMITTER_H_*/
