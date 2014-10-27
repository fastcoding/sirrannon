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
#ifndef TCP_RECEIVER_H_
#define TCP_RECEIVER_H_
#include "Communicator.h"
#include "SirannonSocket.h"
#include "Interfaces.h"

class TCPReceiver : public MediaReceiver
{
public:
	TCPReceiver( const string& sName, ProcessorManager* pScope );
	~TCPReceiver();

protected:
	void init( void );
	void mainThreadA( void );

	TCPServerSocket* pListen;
	TCPSocket* pSocket;
	bool bConnect;
	uint8_t pBuffer [64*KIBI];
};

#endif /*TCP_RECEIVER_H_*/
