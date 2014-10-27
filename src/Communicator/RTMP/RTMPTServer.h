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
#ifndef RTMPT_SERVER_H_
#define RTMPT_SERVER_H_
#include "RTMPServer.h"
#include "RTMPTConnection.h"
#include "Communicator/HTTP/HTTPConnection.h"


class RTMPTServer : public RTMPServer
{
public:
	RTMPTServer( const char* sName, ProcessorManager* pScope );
	virtual ~RTMPTServer();

	void init( void );
	void process( void );
	virtual int endSession( MediaSession* pSession, ConnectionInterface* pConnection );
	int handleNewSocket( TCPSocket* pRawSocket );

protected:
	int parseHTTPRequest( HTTPConnection* pHTTP );
	int analyzeUrl( const string& sUrl, int& iMode, RTMPTConnection*& pConnection );

	map<string,RTMPTConnection*> mUrl2Connection;
	HTTPConnection::fields_t mFields;
	char sResponse [KIBI];

private:
	mutex oLocalMutex;
};

#endif /* RTMPT_SERVER_H_ */
