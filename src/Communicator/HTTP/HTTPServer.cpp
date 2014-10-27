#include "HTTPServer.h"

/**
 * HTTP SERVER
 * @component HTTP-server
 * @type media-server
 * @param port, int, 80, listen to this port for incoming connections
 * @param cache, bool, false, if true, cache generated HLS segments
 * @param cached, bool, false, if true, used cached segments and playlists when available
 * @param segment, int, 10, in seconds, the duration of one HLS segment
 * @param high, int, 1000, in kbit/s, the bitrate of the high quality HLS encoding
 * @param medium, int, 500, in kbit/s, the bit rate of the medium quality HLS encoding
 * @param low, int, 200, in kbits/s, the bit rate of low quality HLS encoding
 * @info Listens for incoming HTTP connections. Does not support pausing or seeking.
 * In the URL use application HTTP for file based transfer of requested file. Use the application FILE@FORMAT (eg. FILE@FLV, FILE@WEBM) to change the container type.
 * Example URLs: \texttt{http://myserver.com/HTTP/demo.mov}, \texttt{http://192.168.1.3/FILE@FLV/demo.mov}
 **/

REGISTER_CLASS( HTTPServer, "HTTP-server" );

HTTPServer::HTTPServer( const char* sName, ProcessorManager* pScope )
	: MediaServer(sName, pScope)
{
	sSession = "HTTP-session";
	mBool["cache"] = false;
	mBool["cached"] = false;
	mInt["segment"] = 10;
	vBitrates.push_back( "high" );
	//vBitrates.push_back( "medium" );
	//vBitrates.push_back( "low" );
}

HTTPServer::~HTTPServer()
{ }

void HTTPServer::init( void )
{
	MediaServer::init();
	mInt["low"] *= 1000;
	mInt["medium"] *= 1000;
	mInt["high"] *= 1000;
}

const vector<string>& HTTPServer::getBitrates( void ) const
{
	return vBitrates;
}
