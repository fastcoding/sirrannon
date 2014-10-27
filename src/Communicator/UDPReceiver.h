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
#ifndef UDP_RECEIVER_H_
#define UDP_RECEIVER_H_
#include "Communicator.h"
#include "SirannonSocket.h"

class UDPReceiver : public MediaReceiver
{
public:
	UDPReceiver( const string& sName, ProcessorManager* pScope );
	~UDPReceiver();
	void init( void );

protected:
	void close( void );
	void handleReceive( void );

	static const int buffer_size = 1500;
	uint8_t pBuffer [1500]; /* pBuffer to receive data */
	UDPSocket*  pSocket;
	timestamp_t iLastDts;
};

#endif /*UDP_RECEIVER_H_*/
