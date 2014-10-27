#include "Url.h"
#include "SirannonException.h"
#include "Frame.h"
#define SIRANNON_USE_BOOST_REGEX
#include "Boost.h"
#include "sirannon.h"

static void URL_hack( string& sMedia )
{
	/* Kludge:
	 * 	1. FireFox requires at this time that the extension of the request file MUST be .webm
	 * 	2. When requesting a stream with a different extension like (.flv) that is
	 *     dynamically transcoded to webM, he refuses to play because he thinks he cannot
	 *     play an .flv file!
	 *  3. Add /DUMMY/dummy.webm at the end of the URL to hack this situation
	 */
	int iHack = sMedia.find( "/DUMMY" );
	if( iHack != string::npos )
		sMedia.erase( iHack, sMedia.length() - iHack );
}

static int URL_reduce( const string& sUrl )
{
	if( sUrl[0] == '/' )
		return 0;
	int iPos1 = sUrl.find( "://" ) ;
	if( iPos1 != string::npos )
	{
		int iPos2 = sUrl.find( "/", iPos1+3 );
		if( iPos2 != string::npos )
			return iPos2;
	}
	else
	{
		int iPos2 = sUrl.find( '/' );
		if( iPos2 != string::npos )
			return iPos2;
	}
	return 0;
}

static bool URL_exists( const string& sUrl, const char* sProtocol )
{
	char sTmp [16];
	if( strlen(sProtocol) != 4 )
		return false;
	strcpy( sTmp, sProtocol );
	strcat( sTmp, "://" );
	return strncmp( sUrl.c_str(), sTmp, 7 ) == 0;
}

static void URL_option( const char* sOpt, int iOpt, container_t& oFormat, target_t::type& iTarget )
{
	iTarget = target_t::NO;

	if( strncasecmp( sOpt, "TS", iOpt ) == 0 )
	{
		oFormat.first = mux_t::TS;
	}
	else if( strncasecmp( sOpt, "M3U", iOpt ) == 0 )
	{
		oFormat.first = mux_t::M3U;
	}
	else if( strncasecmp( sOpt, "FLV", iOpt ) == 0 )
	{
		oFormat.first = mux_t::FLV;
	}
	else if( strncasecmp( sOpt, "WEBM", iOpt ) == 0 )
	{
		oFormat.first = mux_t::WEBM;
	}
	else if( strncasecmp( sOpt, "IPAD", iOpt ) == 0 )
	{
		oFormat.first = mux_t::TS;
		iTarget = target_t::IPAD1;
	}
	else if( strncasecmp( sOpt, "IPHONE", iOpt ) == 0 )
	{
		oFormat.first = mux_t::TS;
		iTarget = target_t::IPHONE4;
	}
	else if( strncasecmp( sOpt, "YOUTUBE", iOpt ) == 0 )
	{
		oFormat.first = mux_t::WEBM;
		iTarget = target_t::YOUTUBE;
	}
	else
		ValueError( "Unsupported option(%s)", sOpt );
}

void URL_canonize( string& sUrl, const char* sProtocol )
{
	if( not URL_exists( sUrl, sProtocol ) )
	{
		sUrl.insert( 0, "://" );
		sUrl.insert( 0, sProtocol );
	}
}

void URL_parse( const string& sUrl,
		string& sProtocol, string& sServer, int& iPort, string& sMedia )
{
	const static regex oUrlExpression( "(\\w+)://(\\S+?)(:\\d+)?/(\\S+)" );
	cmatch oParse;
	if( not regex_match( sUrl.c_str(), oParse, oUrlExpression ) )
		SyntaxError( "Invalid URL: %s", sUrl.c_str() );

	sProtocol.assign( oParse[1].first, oParse[1].second );
	sServer.assign( oParse[2].first, oParse[2].second );
	if( oParse[3].matched )
		iPort = atoi( oParse[3].first+1 );
	else
	{
		/* Put the default port */
		if( sProtocol == "rtmp" )
			iPort = 1935;
		else if( sProtocol == "rtsp" )
			iPort = 554;
		else
			iPort = 80;
	}
	sMedia.assign( oParse[4].first, oParse[4].second );
	URL_hack( sMedia );
}

void URL_parse( const string& sUrl,
		string& sProtocol, string& sServer, int& iPort, string& sApp, string& sMedia )
{
	const static regex oUrlExpression( "(\\w+)://(\\S+?)(:\\d+)?/(\\S+?)/(\\S+)" );
	cmatch oParse;
	if( not regex_match( sUrl.c_str(), oParse, oUrlExpression ) )
		SyntaxError( "Invalid URL: %s", sUrl.c_str() );

	sProtocol.assign( oParse[1].first, oParse[1].second );
	sServer.assign( oParse[2].first, oParse[2].second );
	if( oParse[3].matched )
		iPort = atoi( oParse[3].first+1 );
	else
	{
		/* Put the default port */
		if( sProtocol == "rtmp" )
			iPort = 1935;
		else if( sProtocol == "rtsp" )
			iPort = 554;
		else
			iPort = 80;
	}
	sApp.assign( oParse[4].first, oParse[4].second );
	sMedia.assign( oParse[5].first, oParse[5].second );
	URL_hack( sMedia );
}

void URL_parse( const string& sUrl,
		string& sProtocol, string& sServer, int& iPort, string& sApp, container_t& oFormat, target_t::type& iTarget, string& sMedia )
{
	const static regex oUrlExpression( "(\\w+)://(\\S+?)(:\\d+)?/(\\S+?)(?:@(\\S+?))?/(\\S+)" );
	cmatch oParse;
	if( not regex_match( sUrl.c_str(), oParse, oUrlExpression ) )
		SyntaxError( "Invalid URL: %s", sUrl.c_str() );

	sProtocol.assign( oParse[1].first, oParse[1].second );
	sServer.assign( oParse[2].first, oParse[2].second );
	if( oParse[3].matched )
		iPort = atoi( oParse[3].first+1 );
	else
	{
		/* Put the default port */
		if( sProtocol == "rtmp" )
			iPort = 1935;
		else if( sProtocol == "rtsp" )
			iPort = 554;
		else
			iPort = 80;
	}
	sApp.assign( oParse[4].first, oParse[4].second );
	sMedia.assign( oParse[6].first, oParse[6].second );
	URL_hack( sMedia );

	/* Determine the format */
	if( oParse[5].matched )
		URL_option( oParse[5].first, oParse[5].second - oParse[5].first, oFormat, iTarget );
}

void URL_parse( const string& sUrl, string& sApp, container_t& oFormat, target_t::type& iTarget, string& sMedia )
{
	int iOffset = URL_reduce( sUrl );

	const static regex oUrlExpression( "/(\\S+?)(?:@(\\S+?))?/(\\S+)" );
	cmatch oParse;
	if( not regex_match( sUrl.c_str() + iOffset, oParse, oUrlExpression ) )
		SyntaxError( "Invalid URL: %s", sUrl.c_str() );

	sApp.assign( oParse[1].first, oParse[1].second );
	sMedia.assign( oParse[3].first, oParse[3].second );
	URL_hack( sMedia );

	/* Determine format */
	if( oParse[2].matched )
		URL_option( oParse[2].first, oParse[2].second - oParse[2].first, oFormat, iTarget );
}

void URL_construct( string& sUrl,
		const char* sProtocol, const char* sServer, int iPort, const char* sApp, const char* sMedia )
{
	char sUrlTmp [1024];
	snprintf( sUrlTmp, sizeof(sUrlTmp), "http://%s:%d/%s/%s", sServer, iPort, sApp, sMedia );
	sUrl = sUrlTmp;
}

void URL_construct( string& sUrl,
		const char* sProtocol, const char* sServer, int iPort, const char* sMedia )
{
	char sUrlTmp [1024];
	snprintf( sUrlTmp, sizeof(sUrlTmp), "http://%s:%d/%s", sServer, iPort, sMedia );
	sUrl = sUrlTmp;
}

/* Walk over a URL to determine the container type */
void URL_walk( const string& sUrl, container_t& oFormat, target_t::type& iTarget )
{
	/* Analyse the URL */
	string sApp, sMedia;
	URL_parse( sUrl, sApp, oFormat, iTarget, sMedia );

	/* Simple if specified with @format */
	if( oFormat.first != mux_t::NO )
		return;

	/* Otherwise recognize whether the application is of sirannon type or not */
	if( sApp == "M3U" )
	{
		oFormat.first = mux_t::M3U;
	}
	else if( sApp == "FILE" or sApp == "HTTP" or sApp == "streamer" or not getMediaProcessorGenerator().count( sApp ) )
	{
		int iPos = sMedia.rfind( '.' );
		if( iPos != string::npos )
			oFormat = StringToContainer( sMedia.c_str() + iPos + 1 );
	}
	else
	{
		URL_walk( sMedia, oFormat, iTarget );
	}
}

void URL_options( const string& sUrl, string& sMedia, map<string,string>& mOptions )
{
	size_t iPos = sUrl.find('?');
	if( iPos != string::npos )
	{
		vector<string> vSplit;
		split( sUrl.substr( iPos + 1, sUrl.length() - iPos - 1 ), vSplit, "&" );
		for( int i = 0; i < vSplit.size(); ++i )
		{
			vector<string> vSplit2;
			split( vSplit[i], vSplit2, "=" );
			if( vSplit2.size() == 2 )
				mOptions[vSplit2[0]] = vSplit2[1];
		}
		sMedia = sUrl.substr( 0, iPos );
	}
	else
		sMedia = sUrl;
}
