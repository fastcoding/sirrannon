#ifndef DYNAMICSTREAMER_H_
#define DYNAMICSTREAMER_H_
#include "FileStreamer.h"
#include "Communicator/MediaServer.h"

class ProxyStreamer : public FileStreamer
{
public:
	ProxyStreamer( const string& sName, ProcessorManager* pScope );

	virtual const ContainerDescriptor* getDescriptor( void ) const;
	virtual void createSource( void );
	virtual void createBuffer( void );
	virtual void createTransmitter( MediaDescriptor* pDesc = NULL );
	virtual int play( double fSpeed );

protected:
	virtual ~ProxyStreamer();

	virtual void init( void );
	virtual bool ready( void ) const;
	void connectPacketizers( const string& sDst );

	bool bReady, bRawFile, bTranscoding;
	MediaServer* pSession;
	MediaServer::MediaServer_t iMediaServer;
	container_t oFormat;
	target_t::type iTarget;
	MediaProcessor *pLink, *pScheduler;
	ContainerDescriptor oTranscodedContainer;
	bool bMovFrame;
};

class RTMPProxy : public ProxyStreamer
{
public:
	RTMPProxy( const string& sName, ProcessorManager* pScope );
	virtual ~RTMPProxy() { };
};

class RTMPTProxy : public ProxyStreamer
{
public:
	RTMPTProxy( const string& sName, ProcessorManager* pScope );
	virtual ~RTMPTProxy() { };
};

class RTSPProxy : public ProxyStreamer
{
public:
	RTSPProxy( const string& sName, ProcessorManager* pScope );
	virtual ~RTSPProxy() { };
};

class HTTPProxy : public ProxyStreamer
{
public:
	HTTPProxy( const string& sName, ProcessorManager* pScope );
	virtual ~HTTPProxy() { };
};

class FileProxy : public ProxyStreamer
{
public:
	FileProxy( const string& sName, ProcessorManager* pScope );
	virtual ~FileProxy() { };
};

class FileHTTPProxy : public ProxyStreamer
{
public:
	FileHTTPProxy( const string& sName, ProcessorManager* pScope );
	virtual ~FileHTTPProxy() { };
};

class LiveProxy : public ProxyStreamer
{
public:
	LiveProxy( const string& sName, ProcessorManager* pScope );
	virtual ~LiveProxy() { };
};

class H264Proxy : public ProxyStreamer
{
public:
	H264Proxy( const string& sName, ProcessorManager* pScope );
	virtual ~H264Proxy() { };
};


#endif /* DYNAMICSTREAMER_H_ */
