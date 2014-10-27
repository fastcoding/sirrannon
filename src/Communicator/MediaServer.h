#ifndef MEDIASERVER_H_
#define MEDIASERVER_H_
#include "sirannon.h"
#include "MediaSession.h"
#include "Interfaces.h"
#include "SirannonSocket.h"
#define SIRANNON_USE_BOOST_FILESYSTEM
#include "Boost.h"

class MediaSession;
class MediaServer: public MediaProcessor, public NestedInterface
{
public:
	typedef enum{ RTMP, HTTP, RTSP } MediaServer_t;
	virtual ~MediaServer();
	virtual int endSession( MediaSession* pSession, ConnectionInterface* pConnection );
	virtual string getPath( const string& sUrl ) const;
	string convertStem( const string& sUrl ) const;
	void getMetadata( const string& sUrl, MediaDescriptor* pDesc ) const;

protected:
	MediaServer( const string& sName, ProcessorManager* pScope );

	virtual void handleError( SirannonException* pException, ProcessorManager* pManager, MediaProcessor* pProcessor );
	virtual void init( void );
	virtual void process( void );
	virtual MediaSession* newSession( ConnectionInterface* pConnection );

	string sRoot;
	TCPServerSocket* pListen;	/* Our accept Socket */
	int iSession;
	const char* sSession;
};

#endif /* MEDIASERVER_H_ */
