#ifndef RTSPSESSION_H_
#define RTSPSESSION_H_
#include "sirannon.h"
#include "Communicator/MediaSession.h"
#include "Interfaces.h"
#include "RTSPServer.h"
#include "ConnectionInterface.h"
#include "Communicator/HTTP/HTTPConnection.h"
#include "HighLevel/ProxyStreamer.h"
#include "sdp.h"

class RTSPSession : public MediaSession
{
public:
	RTSPSession( const string& sName, ProcessorManager* pScope );

	void init( void );

protected:
	~RTSPSession();
	int newStreamer( void );
	int analyzeUrl( void );
	int analyzeUrl1( void );
	void requestHandler( void );
	int handleOPTIONS( void );
	int handleDESCRIBE( void );
	int handleSETUP( void );
	int handlePLAY( void );
	int handlePAUSE( void );
	int handleTEARDOWN( void );
	int handleUNSUPPORTED( void );

	HTTPConnection* pHTTP;
	ProxyStreamer* pStreamer;
	uint32_t iToken;
	HTTPConnection::fields_t mFields;
	string sApp, sMedia;
	ContainerDescriptor oSDP;
	bool bVLC;
};

#endif /* RTSPSESSION_H_ */
