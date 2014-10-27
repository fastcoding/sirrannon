#ifndef RTSPSERVER_H_
#define RTSPSERVER_H_
#include "Communicator/MediaServer.h"

class RTSPServer : public MediaServer
{
public:
	RTSPServer( const string& sName, ProcessorManager* pScope );
};

#endif /* RTSPSERVER_H_ */
