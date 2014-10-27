#ifndef RTSPCLIENT_H_
#define RTSPCLIENT_H_
#include "Block.h"
#include "Communicator/HTTP/HTTPConnection.h"
#include "SirannonSocket.h"
#include "Interfaces.h"

class RTSPClient : public Block, public ClientInterface
{
public:
	RTSPClient( const string& sName, ProcessorManager* pScope );

	bool ready( void ) const;
	void init( void );
	void handleError( SirannonException* pException, ProcessorManager* pManager, MediaProcessor* pProcessor );

protected:
	~RTSPClient();

	void prepareMessage( void );
	int seek( uint32_t iSeek );
	int play( double fSpeed = -1. );
	int pause( void );

	void handleCommunication( void );
	int sendDESCRIBE( void );
	int sendSETUP( void );
	int sendPLAY( void );
	int sendTEARDOWN( void );

	bool bMetaData;
	int iSeq;
	HTTPConnection* pHTTP;
	TCPSocket* pConnection;
	string sUrl, sSession;
	HTTPConnection::fields_t mFields;
	char sRequest [KIBI];
};

#endif /* RTSPCLIENT_H_ */
