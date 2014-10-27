#ifndef HTTPSESSION_H_
#define HTTPSESSION_H_
#include "Communicator/MediaSession.h"
#include "HTTPConnection.h"
#include "HighLevel/ProxyStreamer.h"

class HTTPSession : public MediaSession
{
public:
	HTTPSession( const char* sName, ProcessorManager* pScope );
	virtual ~HTTPSession();

	virtual void init( void );

protected:
	void requestHandler( void );
	void createStreamer( void );
	void analyzeUrl( void );
	void receive( MediaPacketPtr& pPckt );
	void receive_end( MediaPacketPtr& pPckt );
	void receive_reset( MediaPacketPtr& pPckt );

	HTTPConnection* pHTTP;
	HTTPConnection::fields_t mFields;
	ProxyStreamer* pStreamer;
	string sMedia, sApp;
	bool bClose;
};

#endif /* HTTPSESSION_H_ */
