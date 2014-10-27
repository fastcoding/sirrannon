#ifndef RTMPTPROXY_H_
#define RTMPTPROXY_H_
#include "RTMPClient.h"
#include "RTMPTConnection.h"
#include "SirannonSocket.h"
#include "Communicator/HTTP/HTTPConnection.h"

class RTMPTClient : public RTMPClient
{
public:
	RTMPTClient( const char* sName, ProcessorManager* pProc );
	virtual ~RTMPTClient();
	virtual void init( void );

protected:
	virtual void mainThreadA( void );
	virtual void mainThreadB( void );
	virtual int parseHTTPResponse( const char* sMode );
	virtual int handleEnd( void );

private:
	TCPSocket* pSocket;
	string sHash;
	int iCounter;
	SirannonTime oSleep;
	RTMPTConnection* pConnection;
	HTTPConnection* pHTTP;
	char sHTTP [512];
	thread* pTunneledThread;
	uint8_t cZero;
	char sCommand [64];
	HTTPConnection::fields_t mFields;
};

#endif /* RTMPTPROXY_H_ */
