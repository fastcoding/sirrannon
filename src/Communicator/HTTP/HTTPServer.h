#ifndef HTTPSERVER_H_
#define HTTPSERVER_H_
#include "Communicator/MediaServer.h"

class HTTPServer : public MediaServer
{
public:
	HTTPServer( const char* sName, ProcessorManager* pScope );
	virtual ~HTTPServer();
	virtual void init( void );
	virtual const vector<string>& getBitrates( void ) const;

protected:
	vector<string> vBitrates;
};

#endif /* HTTPSERVER_H_ */
