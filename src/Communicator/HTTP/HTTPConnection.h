#ifndef HTTPCOMMUNICATOR_H_
#define HTTPCOMMUNICATOR_H_
#include "sirannon.h"
#include "ConnectionInterface.h"
#include "Utils.h"

class HTTPConnection
{
public:
	typedef map<string,string> fields_t;

	/* Status codes */
	const static uint32_t OK = 200;

	HTTPConnection( ConnectionInterface* pSocket, const string& sApplication, bool bDebug=false );
	virtual ~HTTPConnection();

	const string& getCommand( void ) const;
	const string& getUrl( void ) const;
	const string& getProtocol( void ) const;
	const uint8_t* getHeader( void ) const;
	const fields_t& getFields( void ) const;
	uint32_t getHeaderSize( void ) const;
	const uint8_t* getContent( void ) const;
	uint32_t getContentSize( void ) const;
	const string& getStatus( void ) const;
	uint32_t getStatusNumber( void ) const;
	uint32_t getResidualSize( bool bDynamic=false ) const;
	static const char* generateMimeType( container_t oFormat );

	int sendFile( const string& sFilename, const string& sRange, bool bClose=false, SirannonTime oTimeOut=5000 );
	int sendReply( int iStatus, const char* sStatus, const uint8_t* pData, uint32_t iSize, map<string,string>& mFields );
	int sendMessage( const char* sCommand, const char* sUrl, const uint8_t* pData, uint32_t iSize, map<string,string>& mFields );

	int receiveReply( bool bContent );
	int receiveMessage( bool bContent );
	int receive( function1<int, HTTPConnection*> oParseFunction, bool bContent );

	int parseRequestHeader( void );
	int parseReplyHeader( void );

private:
	int send( const uint8_t* pData, uint32_t iSize, map<string,string>& mFields );

	uint8_t pRcvBuffer [64000];
	char sSndBuffer [64000];
	char sTemp [KIBI];
	ConnectionInterface* pConnection;
	uint8_t* pLastMarker;
	uint32_t iOffset, iMessage, iHeader, iContent, iStatus;
	bool bDebug;

public:
	string sUrl, sCommand, sProtocol, sApplication, sStatus;
	fields_t mFields;
};

/* Convenience wrapper */
int send_HTTP_message( const string& sAddress, const string& sCommand, const string& sMessage );

inline const uint8_t* HTTPConnection::getHeader( void ) const
{
	return pRcvBuffer;
}

inline uint32_t HTTPConnection::getHeaderSize( void ) const
{
	return iHeader;
}

inline const uint8_t* HTTPConnection::getContent( void ) const
{
	return pRcvBuffer + iHeader;
}

inline uint32_t HTTPConnection::getContentSize( void ) const
{
	return iContent;
}

inline const string& HTTPConnection::getCommand( void ) const
{
	return sCommand;
}

inline const string& HTTPConnection::getUrl( void ) const
{
	return sUrl;
}

inline const string& HTTPConnection::getProtocol( void ) const
{
	return sProtocol;
}

inline const HTTPConnection::fields_t& HTTPConnection::getFields( void ) const
{
	return mFields;
}

inline const string& HTTPConnection::getStatus( void ) const
{
	return sStatus;
}

inline uint32_t HTTPConnection::getStatusNumber( void ) const
{
	return iStatus;
}

#endif /* HTTPCOMMUNICATOR_H_ */
