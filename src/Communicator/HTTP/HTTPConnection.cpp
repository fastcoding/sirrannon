#include "HTTPConnection.h"
#define SIRANNON_USE_BOOST_REGEX
#include "Boost.h"
#include "OSSupport.h"
#include "SirannonSocket.h"

HTTPConnection::HTTPConnection( ConnectionInterface* pConnection, const string& sApplication, bool bDebug )
	: pConnection(pConnection), sApplication(sApplication),
	  iMessage(0), iOffset(0), pLastMarker(pRcvBuffer-2), iHeader(0), iContent(0), bDebug(bDebug)
{ }

HTTPConnection::~HTTPConnection()
{ }

int HTTPConnection::receiveReply( bool bContent )
{
	return receive( &HTTPConnection::parseReplyHeader, bContent );
}

int HTTPConnection::receiveMessage( bool bContent )
{
	return receive( &HTTPConnection::parseRequestHeader, bContent );
}

int HTTPConnection::receive( function1<int, HTTPConnection*> oParseFunction, bool bContent )
{
	/* Shift was what we received too much back to start */
	if( iOffset > iMessage )
	{
		memmove( pRcvBuffer, pRcvBuffer + iMessage, iOffset - iMessage );
		iOffset -= iMessage;
	}
	else
	{
		iOffset = 0;
	}
	pLastMarker = pRcvBuffer - 2;

	/* Find the end of the message */
	bool bMessage = false;
	while( not bMessage )
	{
		/* Search for marker from the last known position */
		uint8_t* pCheck = pLastMarker + 2;
		while( not bMessage and pCheck + 1 < pRcvBuffer + iOffset )
		{
			if( pCheck[0] == '\r' and pCheck[1] == '\n' )
			{
				if( pCheck == pLastMarker + 2 )
					bMessage = true;
				pLastMarker = pCheck;
			}
			pCheck++;
		}
		/* Receive more data */
		if( not bMessage )
		{
			int iBytes = pConnection->receiveRaw( pRcvBuffer + iOffset, sizeof(pRcvBuffer) - iOffset - 1 );
			if( iBytes <= 0 )
				return -1;
			iOffset += iBytes;
		}
	}
	/* Found a message */
	if( bDebug )
		fprintf( stderr, "HTTP: RECEIVED [%"LL"u]\n%s",
				SirannonTime::getUpTime().convertMsecs(), (char*)pRcvBuffer );
	iHeader = pLastMarker - pRcvBuffer + 2;

	/* Parse the message */
	if( oParseFunction( this ) < 0 )
		SyntaxError( "Malformatted %s header", sApplication.c_str() );

	/* Content */
	iContent = atoi( mFields["Content-Length"].c_str() );
	iMessage = iHeader + iContent;

	/* Receive the rest */
	if( bContent )
	{
		/* Verify buffer size */
		if( iMessage > sizeof(pRcvBuffer) )
			IOError( "HTTP receive buffer too small: %d < %d", sizeof(pRcvBuffer), iMessage );

		if( iOffset < iMessage )
		{
			if( pConnection->receiveSmart( pRcvBuffer + iOffset, iMessage - iOffset ) <= 0 )
				return -1;
			iOffset = iMessage;
		}
	}
	/* Found a message */
	return iMessage;
}

uint32_t HTTPConnection::getResidualSize( bool bDynamic ) const
{
	if( bDynamic and iHeader == iMessage )
		return iOffset - iHeader;
	return MIN( iOffset - iHeader, iMessage );
}

int HTTPConnection::parseRequestHeader( void )
{
	char* sMessage = (char*) pRcvBuffer;
	cmatch oParse;
	mFields.clear();

	/* Head of the header */
	*pLastMarker = 0;
	static const regex oHeadExpression( "(\\w+) (\\S+) (\\S+)\\r\\n" );
	if( not regex_search( sMessage, oParse, oHeadExpression ) )
		return -1;
	sCommand.assign( oParse[1].first, oParse[1].second );
	sUrl.assign( oParse[2].first, oParse[2].second );
	sProtocol.assign( oParse[3].first, oParse[3].second );
	sMessage += ( oParse.suffix().first - sMessage );

	/* Key and value pairs of the header */
	static const regex oBodyExpression( "([\\w\\-]+): (.*?)\\r\\n" );
	while( sMessage < (char*)pLastMarker )
	{
		if( not regex_search( sMessage, oParse, oBodyExpression ) )
			return -1;
		string sKey( oParse[1].first, oParse[1].second );
		mFields[sKey].assign( oParse[2].first, oParse[2].second );
		sMessage += ( oParse.suffix().first - sMessage );
	}
	return iHeader;
}

int HTTPConnection::parseReplyHeader( void )
{
	char* sMessage = (char*) pRcvBuffer;
	cmatch oParse;
	mFields.clear();

	/* Head of the header */
	*pLastMarker = 0;
	static const regex oHeadExpression( "(\\S+) (\\d+) (\\S+)\\r\\n" );
	if( not regex_search( sMessage, oParse, oHeadExpression ) )
		return -1;
	sProtocol.assign( oParse[1].first, oParse[1].second );
	iStatus = atoi( oParse[2].first );
	sStatus.assign( oParse[3].first, oParse[3].second );
	sMessage += ( oParse.suffix().first - sMessage );

	/* Key and value pairs of the header */
	static const regex oBodyExpression( "([\\w\\-]+): (.*?)\\r\\n" );
	int iStep = 0;
	while( sMessage < (char*)pLastMarker )
	{
		if( not regex_search( sMessage, oParse, oBodyExpression ) )
			return -1;
		iStep++;
		string sKey( oParse[1].first, oParse[1].second );
		mFields[sKey].assign( oParse[2].first, oParse[2].second );
		sMessage += ( oParse.suffix().first - sMessage );
	}
	return iHeader;
}

int HTTPConnection::sendFile( const string& sFilename, const string& sRange, bool bClose, SirannonTime oTimeOut )
{
	/* Range header */
	fields_t mFields;
	int64_t iMin = 0, iMax = 0;
	if( sRange.length() )
	{
		const static regex oRangeExpr( "bytes=(\\d+)-(\\d*)" );
		cmatch oParse;
		if( regex_search( sRange.c_str(), oParse, oRangeExpr ) )
		{
			iMin = atoi( oParse[1].first );
			iMax = atoi( oParse[2].first );
		}
	}
	/* Open the file */
	FILE* pFile = fopen( sFilename.c_str(), "r" );
	if( not pFile )
		IOError( "HTTP: Could not open(%s): %s", sFilename.c_str(), strError() );
	int iFile = fileno( pFile );
	if( iFile < 0 )
		IOError( "HTTP: Could not process(%s): %s", sFilename.c_str(), strError() );

	/* File size */
	fseek( pFile, 0L, SEEK_END );
	int64_t iBytes = ftell( pFile );
	fseek( pFile, iMin, SEEK_SET );
	if( iMax == 0 )
		iMax = iBytes - 1;

	/* Sanity */
	if( iMax > iBytes - 1 )
		ValueError( "HTTP: Invalid range: %s > actual size(%"LL"d)", sRange.c_str(), iBytes );
	int iRange = iMax - iMin + 1;
	if( iRange <= 0 )
		ValueError( "HTTP: Invalid range: %s", sRange.c_str() );

	/* We can fill out the size of the file and keep the connection alive */
	stringstream sBytes;
	sBytes << iRange;
	if( bClose )
		mFields["Connection"] = "close";
	mFields["Content-Length"] = sBytes.str();
	if( sRange.length() )
		mFields["Range"] = sRange;

	/* Send reply and data */
	sendReply( 200, "OK", NULL, 0, mFields );
	pConnection->sendfileSmart( iFile, iRange, oTimeOut );

	/* Done */
	fclose( pFile );
	return 0;
}

int HTTPConnection::sendReply( int iStatus, const char* sStatus, const uint8_t* pData, uint32_t iSize, map<string,string>& mFields )
{
	/* Standard OK header */
	snprintf( sSndBuffer, sizeof(sSndBuffer), "%s %d %s\r\n", sProtocol.c_str(), iStatus, sStatus );

	return send( pData, iSize, mFields );
}

int HTTPConnection::sendMessage( const char* sCommand, const char* sUrl, const uint8_t* pData, uint32_t iSize, map<string,string>& mFields )
{
	/* Standard Request header */
	snprintf( sSndBuffer, sizeof(sSndBuffer), "%s %s %s\r\n", sCommand, sUrl, sApplication.c_str() );

	return send( pData, iSize, mFields );
}

int HTTPConnection::send( const uint8_t* pData, uint32_t iSize, map<string,string>& mFields )
{
	/* Add content field */
	if( iSize )
	{
		snprintf( sTemp, sizeof(sTemp), "%u", iSize );
		mFields["Content-Length"] = sTemp;
	}
	/* Fields */
	for( map<string,string>::const_iterator i = mFields.begin(); i != mFields.end(); ++i )
	{
		snprintf( sTemp, sizeof(sTemp), "%s: %s\r\n", i->first.c_str(), i->second.c_str() );
		strcat( sSndBuffer, sTemp );
	}
	/* End marker */
	strcat( sSndBuffer, "\r\n" );

	/* Send over the Socket */
	if( bDebug )
		fprintf( stderr, "HTTP: SEND [%"LL"u]\n%s",
				SirannonTime::getUpTime().convertMsecs(), sSndBuffer );
	pConnection->sendSmart( (uint8_t*)sSndBuffer, strlen(sSndBuffer) );
	if( iSize )
		pConnection->sendSmart( pData, iSize );

	return iSize + strlen(sSndBuffer);
}

const char* HTTPConnection::generateMimeType( container_t oFormat )
{
	using namespace mux_t;
	using namespace codec_t;
	switch( oFormat.first )
	{
	case TS:
		return "video/MP2T";

	case M3U:
		return "application/vnd.apple.mpegurl";

	case MOV:
		return "video/mp4";

	case mux_t::WEBM:
		return "video/webm";

	case FLV:
		return "video/x-flv";

	case PS:
		return "video/x-mpeg";

	case ES:
		switch( oFormat.second )
		{
		case mp1v:
		case mp2v:
			return "video/x-mpeg";

		case mp4v:
			return "video/mpeg4-generic";

		case avc:
			return "video/H264";

		case ac3:
			return "audio/ac3";

		case vorbis:
			return "audio/vorbis";

		case mp4a:
			return "audio/mpeg4-generic";

		case mp2a:
			return "audio/x-mpeg";

		case mp1a:
			return "audio/mpeg3";

		default:
			return "";
		}
	default:
		return "";
	}
}

int send_HTTP_message( const string& sAddress, const string& sCommand, const string& sMessage )
{
	int iVal;
	TCPSocket oSocket( sAddress, 80 );
	HTTPConnection oHTTP( &oSocket, "HTTP/1.1", false );
	HTTPConnection::fields_t mFields;
	if( ( iVal = oHTTP.sendMessage( "POST", sCommand.c_str(), (uint8_t*)sMessage.c_str(), sMessage.length(), mFields ) ) < 0 )
		return iVal;
	if( ( iVal = oHTTP.receiveReply( false ) ) < 0 )
		return iVal;
	return oHTTP.getStatusNumber() != 200;
}
