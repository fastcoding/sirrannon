#include "sirannon.h"
#include "HTTPConnection.h"
#include "SirannonSocket.h"
#include "Block.h"

class HTTPCapture : public MediaProcessor, public ClientInterface
{
public:
	HTTPCapture( const string& sName, ProcessorManager* pScope );
	void init( void );

	/* Commands */
	virtual int seek( uint32_t );
	virtual int play( double f = 0.0 );
	virtual int pause( void );
	virtual int flush( void );
	virtual bool ready( void ) const;

protected:
	~HTTPCapture();

	void handlePacket( MediaPacketPtr& pPckt );
	void handleCommunication( void );

	string sUrl;
	HTTPConnection* pHTTP;
	TCPSocket* pConnection;
	HTTPConnection::fields_t mFields;
	int iMaxChunk, iUnit;
	MediaDescriptor* pDesc;
	bool bPlay, bM3U;
};

class HTTPClient : public Block, public ClientInterface
{
public:
	HTTPClient( const string& sName, ProcessorManager* pScope );
	void init( void );
	virtual const ContainerDescriptor* getDescriptor( void ) const;

	/* Commands */
	virtual int seek( uint32_t );
	virtual int play( double f = 0.0 );
	virtual int pause( void );
	virtual int flush( void );
	virtual bool ready( void ) const;

protected:
	~HTTPClient();

	SourceInterface* pSource;
	ClientInterface* pCapture;
	BufferInterface* pBuffer;
};
