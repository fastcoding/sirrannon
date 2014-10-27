#ifndef MEDIASESSION_H_
#define MEDIASESSION_H_
#include "sirannon.h"
#include "MediaServer.h"
#include "Interfaces.h"
#include "ConnectionInterface.h"

class MediaServer;
class MediaSession : public MediaProcessor, public NestedInterface
{
public:
	MediaSession( const string& sName, ProcessorManager* pScope );
	virtual ~MediaSession();

	virtual void init( void );

protected:
	virtual void process( void );
	virtual void handleError( SirannonException* pException, ProcessorManager* pManager, MediaProcessor* pProcessor );

	MediaServer* pServer;
	ConnectionInterface* pConnection;
	target_t::type iTarget;
	container_t oFormat;
};

#endif /* MEDIASESSION_H_ */
