#include "MediaServer.h"
#include "Frame.h"

/**
 * MEDIA SERVER
 * @component media-server
 * @properties abstract, threaded, scheduled
 * @type core
 * @param interface, string, , if defined, listen to incoming connections on this specific interface
 * @param media, string, dat/media, path of the folder in which to search for requested files
 * @info Media Servers are complex components, listening to a specific TCP port for
 * incoming connections and dynamically creating a session as nested component for each
 * new connection. URLs in the request should be of the form:
 * \texttt{<protocol>://<server-address>/<application>/<media>}.
 * For example \texttt{rtmpt://myserver.com/FILE/flash/example.flv} with
 * server(\texttt{myserver.com}), application(\texttt{FILE}), media(\texttt{flash/example.flv}).
 * Each session creates a nested component of the type \textit{application} given in the
 * URL. Typical applications are FILE, HTTP, RTMP-proxy, RTMPT-proxy, RTSP-proxy,
 * HTTP-proxy.
 **/

/**
 * MEDIA CLIENT
 * @component media-client
 * @properties abstract, threaded, scheduled, buffered
 * @type core
 * @param url, string, , the url to retrieve
 * @param auto-play, bool, true, if true, instantly play the stream
 * @info Media Clients connect to servers and retrieve a stream using a
 * particular protocol. The component outputs frames just as would a a Reader
 * component. In this way streams can be requested and captured (using for example
 * \textit{FFMPEG-writer} or \textit{writer}) or resent using a different protocol
 * (for example converting an RTMP stream to a HTTP stream). Proxy applications for
 * Media Servers (RTMP-proxy, RTMPT-proxy, RTSP-proxy, HTTP-proxy) use this feature.
 **/

MediaServer::MediaServer( const string& sName, ProcessorManager* pScope )
	: MediaProcessor(sName, pScope), NestedInterface(sName, pScope),
	  pListen(NULL), iSession(0), sSession(NULL)
{
	mInt["port"] = 1935;
	mString["interface"] = "";
	mString["media"] = "dat/media/flash";
}

MediaServer::~MediaServer()
{
	delete pListen;
}

string MediaServer::convertStem( const string& sUrl ) const
{
	const filesystem::path sUrlPath( sUrl );
	const filesystem::path sStem( sUrlPath.branch_path() / sUrlPath.stem() );
	return sStem.native_file_string();
}

string MediaServer::getPath( const string& sUrl ) const
{
	filesystem::path oFull( sRoot );
	oFull = oFull / sUrl;
	return oFull.native_file_string();
}

void MediaServer::getMetadata( const string& sUrl, MediaDescriptor* pDesc ) const
{
	string sStem = convertStem( sUrl );
	sStem.append( ".mp4" );
	string sPath = getPath( sStem );
	if( not boost::filesystem::exists( sPath ) )
		RuntimeError( "Container (%s) does not exist", sPath.c_str() );
	GetMetadata( sPath.c_str(), NULL, pDesc );
}

void MediaServer::init( void )
{
	MediaProcessor::init();

	/* Check folder */
	sRoot = mString["media"];
	if( not filesystem::exists( sRoot ) )
		IOError( this, "Directory(%s) unavailable", sRoot.c_str() );

	/* Create the server Socket that listen for incoming sessions */
	if( not mString["interface"].length() )
		pListen = new TCPServerSocket( mInt["port"] );
	else
		pListen = new TCPServerSocket( mString["interface"], mInt["port"] );
	if( not pListen )
		RuntimeError( this, "could not bind server TCP Socket on port %d", mInt["port"] );
	pListen->setNonBlocking();
	debug( 1, "listening for connections on port(%d)", mInt["port"] );

	/* Loop */
	bSchedule = true;
}

void MediaServer::process( void )
{
	/* Accept new connections */
	TCPSocket* pSocket;
	while( (pSocket = pListen->accept()) )
	{
		debug( 1, "Accepted new client" );

		/* Create the session */
		MediaSession* pSession = newSession( pSocket );
	}
	/* Handle the processors */
	oProcessorManager.scheduleProcessors();
}

void MediaServer::handleError( SirannonException* pException, ProcessorManager* pManager, MediaProcessor* pProcessor )
{
	/* A session made an error, terminate it */
	SirannonWarning( this,  "Handling %s (%s)", pException->what(), pProcessor->c_str() );
	pProcessor->end();
}

MediaSession* MediaServer::newSession( ConnectionInterface* pConnection )
{
	/* Create a session to handle this Socket */
	char sSessionName [256];
	sprintf( sSessionName, "session-%d", iSession );

	/* Create a dynamic block */
	map<string,void*> mPrivate;
	mPrivate["server"] = this;
	mPrivate["connection"] = pConnection;
	MediaSession* pSession = dynamic_cast<MediaSession*>( oProcessorManager.createProcessorDynamic( sSession, sSessionName, NULL, NULL,
			&mInt, &mDouble, &mString, &mBool, &mPrivate ) );
	if( not pSession )
		RuntimeError( this, "Could not create (%s)", sSession );
	iSession++;
	return pSession;
}

int MediaServer::endSession( MediaSession* pSession, ConnectionInterface* pConnection )
{
	delete pConnection;
	return 0;
}
