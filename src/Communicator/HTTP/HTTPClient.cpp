#include "HTTPClient.h"
#include "Url.h"
#include "Frame.h"
#include "OSSupport.h"

/**
 * HTTP CAPTURE
 * @component HTTP-capture
 * @type media-client
 * @param chunk-size, int, 65536, in bytes, the size of the chunks from the container to generate
 * @info Requests and receives a stream via HTTP. Keeps the container and generates chunks from it.
 **/

/**
 * HTTP CLIENT
 * @component HTTP-client
 * @type media-client
 * @param format, string, , the format of the container, if not defined, guess the format based on the extension in the URL
 * @param M3U, bool, false, if true, the URL links to an extended M3U file used by Apple Live HTTP Streaming. The URLs contained in the M3U response will be contacted in turn.
 * @info Requests and receives a stream via HTTP. Demultiplexes the container and generates frames.
 **/

REGISTER_CLASS( HTTPCapture, "HTTP-capture" );
REGISTER_CLASS( HTTPClient, "HTTP-client" );

HTTPCapture::HTTPCapture( const string& sName, ProcessorManager* pScope )
	: MediaProcessor(sName, pScope), iMaxChunk(KIBI*64), pHTTP(NULL), pConnection(NULL), iUnit(0),
	  bM3U(false)
{
	mInt["chunk-size"] = 64 * KIBI;
	mString["url"] = "flash/da.flv";
	mBool["M3U"] = false;
}

HTTPCapture::~HTTPCapture()
{
	delete pHTTP;
	delete pConnection;
}

void HTTPCapture::init( void )
{
	MediaProcessor::init();

	iMaxChunk = mInt["chunk-size"];
	sUrl = mString["url"];
	bM3U = mBool["M3U"];

	pDesc = addMedia();

	if( bAutoPlay )
		play();

	/* Create thread */
	createThread( bind( &HTTPCapture::handleCommunication, this ) );
}

void HTTPCapture::handleCommunication( void )
{
	/* Parse the URL */
	string sProtocol, sServer, sMedia;
	int iPort;
	URL_canonize( sUrl, "http" );
	debug( 1, "%s", sUrl.c_str() );
	URL_parse( sUrl, sProtocol, sServer, iPort, sMedia );
	debug( 1, "connecting server(%s) port(%d) url(%s)", sServer.c_str(), iPort, sUrl.c_str() );

	/* Parse the address */
	IPAddress oAddr ( sServer, iPort );
	if( not oAddr.valid() )
		RuntimeError( "Invalid server address(%s)", oAddr.getAddressStr().c_str() );

	/* Create the TCP connection */
	pConnection = new TCPSocket( oAddr.getIPStr(), oAddr.getPort() );
	debug( 1, "connection succesful" );

	/* Create the HTTP /communicator with this */
	pHTTP = new HTTPConnection( pConnection, "HTTP/1.1", true );

	/* Send message */
	mFields["Connection"] = "keep-alive";
	mFields["Host"] = oAddr.getAddressStr();
	mFields["User-Agent"] = string("Sirannon/") + VERSION;
	pHTTP->sendMessage( "GET", sUrl.c_str(), NULL, 0, mFields );

	/* If it only M2U we need to connect a second time */
	if( bM3U )
	{
		/* Receive the reply in M3U format */
		pHTTP->receiveReply( true );
		if( pHTTP->getStatusNumber() != HTTPConnection::OK )
			RuntimeError( this, "Server responded %d: %s", pHTTP->getStatusNumber(), pHTTP->getStatus().c_str() );
		pConnection->setNonThrowing();
		debug( 1, "Reply OK" );

		/* Verify content type */
		if( pHTTP->mFields["Content-Type"] != "application/x-mpegURL" and
			pHTTP->mFields["Content-Type"] != "application/vnd.apple.mpegurl" )
			ValueError( this, "Unexpected Content-Type(%s)", pHTTP->mFields["Content-Type"].c_str() );

		/* Extract the url from the message */
		string sM3U( (char*)pHTTP->getContent(), pHTTP->getContentSize() );
		debug( 2, "received M3U:\n%s", sM3U.c_str() );
		int iPos1 = sM3U.find( "http://" );
		int iPos2 = sM3U.find( "\r\n", iPos1 );
		if( iPos1 == string::npos or iPos2 == string::npos or iPos2 < iPos1 + 7 )
			ValueError( this, "No HTTP-address found in M3U response:\n%s", sM3U.c_str() );
		string sNewUrl( sM3U.c_str() + iPos1, iPos2 - iPos1 );
		debug( 2, "requesting URL(%s)", sNewUrl.c_str() );

		/* Request this URL at the server */
		pHTTP->sendMessage( "GET", sNewUrl.c_str(), NULL, 0, mFields );
	}
	/* Reply */
	pHTTP->receiveReply( false );
	if( pHTTP->getStatusNumber() != HTTPConnection::OK )
		RuntimeError( this, "Server responded %d: %s", pHTTP->getStatusNumber(), pHTTP->getStatus().c_str() );
	pConnection->setNonThrowing();
	debug( 1, "Reply OK" );

	/* Wait until play */
	while( not bPlay )
		oQuantum.sleep();

	/* First send the remainder of the HTTP buffer */
	int iResidual = pHTTP->getResidualSize( true );
	if( iResidual )
	{
		MediaPacketPtr pPckt( new MediaPacket( packet_t::media, content_t::mixed, iResidual ) );
		pPckt->push_back( pHTTP->getContent(), iResidual );
		handlePacket( pPckt );
	}
	/* Receive the rest of the chunks */
	while( true )
	{
		/* Create a media packet */
		MediaPacketPtr pPckt( new MediaPacket( packet_t::media, content_t::mixed, iMaxChunk ) );

		/* Receive the HTTP continuation chunks */
		int iBytes = pConnection->receiveRaw( pPckt->data(), iMaxChunk );
		if( iBytes == 0 )
		{
			debug( 1, "Connection closed by server" );
			break;
		}
		else if( iBytes < 0 )
			IOError( this, "Socket error (receive): %s", strError() );
		else
			pPckt->push_back( iBytes );

		handlePacket( pPckt );

		/* Wait until play */
		while( not bPlay )
			oQuantum.sleep();
	}
	/* Generate end packet */
	MediaPacketPtr pEnd( new MediaPacket( packet_t::end, content_t::mixed, 0 ) );
	handlePacket( pEnd );
}

inline void HTTPCapture::handlePacket( MediaPacketPtr& pPckt )
{
	pPckt->unitnumber = iUnit++;
	pPckt->desc = pDesc;
	debug( 1, "received %s", pPckt->c_str_full(10) );
	route( pPckt );
}

int HTTPCapture::seek( uint32_t )
{
	return -1;
}

int HTTPCapture::play( double fSPeed )
{
	debug( 1, "play" );
	bPlay = true;
	return 0;
}

int HTTPCapture::pause( void )
{
	bPlay = false;
	return 0;
}

int HTTPCapture::flush( void )
{
	return -1;
}

bool HTTPCapture::ready( void ) const
{
	return true;
}

HTTPClient::HTTPClient( const string& sName, ProcessorManager* pScope )
	: Block(sName, pScope)
{
	mInt["auto-play"] = true;
	mString["url"] = "";
	mString["format"] = "";
}

HTTPClient::~HTTPClient()
{ }

void HTTPClient::init( void )
{
	MediaProcessor::init();
	bDebug = true;
	bAutoPlay = mBool["auto-play"];

	/* Determine the format */
	container_t oFormat( mux_t::NO, codec_t::NO );
	if( mString["format"].length() )
		oFormat.first = StringToMux( mString["format"].c_str() );

	/* Format not pregiven or invalid  */
	if( oFormat.first == mux_t::NO )
	{
		/* We must parse the URL to find out */
		target_t::type iTarget( target_t::NO );
		string sApp, sMedia;
		URL_parse( mString["url"], sApp, oFormat, iTarget, sMedia );

		if( oFormat.first == mux_t::NO )
		{
			if( sApp == "HTTP" )
				oFormat = ExtensionToContainer( sMedia );
			else if( sApp == "M3U" )
				oFormat.first = mux_t::M3U;
			else if( sApp == "APPLE" )
				oFormat.first = mux_t::TS;
			else if( getMediaProcessorGenerator().find( sApp ) == getMediaProcessorGenerator().end() )
				oFormat = ExtensionToContainer( sMedia );

			if( oFormat.first == mux_t::NO )
				RuntimeError( "Could not guess container type for URL(%s)", mString["url"].c_str() );
		}
	}
	debug( 1, "connecting(%s) format(%s/%s)", mString["url"].c_str(), MuxToString(oFormat.first),
			CodecToString(oFormat.second) );

	/* Create capture, generates a file stream */
	MediaProcessor* pProcessor = oProcessorManager.createProcessor( "HTTP-capture", "HTTP-capture" );
	pCapture = dynamic_cast<ClientInterface*>( pProcessor );
	pProcessor->setBool( "auto-play", true );
	pProcessor->setBool( "debug", true );
	pProcessor->setString( "url", mString["url"] );
	if( oFormat.first == mux_t::M3U )
	{
		pProcessor->setBool( "M3U", true );
		pProcessor->setInt( "chunk-size", 188*25 );
		oFormat.first = mux_t::TS;
	}
	/* Create ffmpeg-demux which generates unmuxed frames */
	pProcessor = oProcessorManager.createProcessor( "FFMPEG-demultiplexer", "FFMPEG-demultiplexer" );
	pSource = dynamic_cast<SourceInterface*>( pProcessor );
	pProcessor->setParams( this );
	pProcessor->setInt( "format", oFormat.first );
	pProcessor->setInt( "codec", oFormat.second );
	pProcessor->setInt( "chunk-size", KIBI );
	pProcessor->setBool( "debug", true );
	pProcessor->setBool( "mov-frame", mBool["mov-frame"] );
	pProcessor->setBool( "skip-AUD", true );

	/* Create scheduler to prevent premature release of packets */
	pProcessor = oProcessorManager.createProcessor( "basic-scheduler", "scheduler" );
	pBuffer = dynamic_cast<BufferInterface*>( pProcessor );
	pProcessor->setBool( "pause", true );
	pProcessor->setBool( "debug", false );
	pProcessor->setBool( "stress", true );
	pProcessor->setInt( "delay", 0 );
	pProcessor->setInt( "buffer", 10000 );

	/* Output */
	out = oProcessorManager.createProcessor( "out", "out" );
	initOut();

	/* Connect and init */
	oProcessorManager.setRoute( "HTTP-capture", "FFMPEG-demultiplexer", 0 );
	oProcessorManager.setRoute( "FFMPEG-demultiplexer", "scheduler", 0 );
	oProcessorManager.setRoute( "scheduler", "out", 0 );
	oProcessorManager.initProcessors();

	/* Autoplay */
	if( bAutoPlay )
		play();
}

int HTTPClient::seek( uint32_t )
{
	return -1;
}

int HTTPClient::play( double fSpeed )
{
	debug( 1, "play" );
	bSchedule = true;
	return pCapture->play( fSpeed ) or pBuffer->play( fSpeed );
}

int HTTPClient::pause( void )
{
	debug( 1, "pause" );
	return pCapture->pause() or pBuffer->pause();
}

int HTTPClient::flush( void )
{
	return -1;
}

bool HTTPClient::ready( void ) const
{
	return pSource->ready();
}

const ContainerDescriptor* HTTPClient::getDescriptor( void ) const
{
	return pSource->getDescriptor();
}
