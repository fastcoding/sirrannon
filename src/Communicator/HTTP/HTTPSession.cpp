#include "HTTPSession.h"
#include "Url.h"
#include "Frame.h"

REGISTER_CLASS( HTTPSession, "HTTP-session" );

HTTPSession::HTTPSession( const char* sName, ProcessorManager* pScope )
	: MediaSession(sName, pScope), pHTTP(NULL), pStreamer(NULL), bClose(false)
{ }

HTTPSession::~HTTPSession()
{
	delete pHTTP;
}

void HTTPSession::init( void )
{
	MediaSession::init();

	pHTTP = new HTTPConnection( pConnection, "HTTP/1.1", true );

	createThread( bind( &HTTPSession::requestHandler, this ) );
}

void HTTPSession::requestHandler( void )
{
	while( true )
	{
		/* Receive the request */
		pHTTP->receiveMessage( true );

		/* Parse it */
		analyzeUrl();

		/* Mime type for response */
		mFields.clear();
		const char* sMime = pHTTP->generateMimeType( oFormat );
		if( not strlen( sMime ) )
			RuntimeError( this, "Could not determine MIME type for (%s)", pHTTP->getUrl().c_str() );
		mFields["Content-Type"] = sMime;

		if( sApp == "M3U-LIST" or sApp == "M3U-FILE" )
		{
			/* Open the file */
			string sFile = pServer->getPath( sMedia );
			FILE* oFile = fopen( sFile.c_str(), "r" );
			if( not oFile )
				IOError( "Could not open file(%s)", sMedia.c_str() );

			/* Read the data */
			uint8_t oBuffer [4096];
			size_t iRead = fread( oBuffer, 1, sizeof(oBuffer), oFile );
			if( ferror( oFile ) )
				IOError( "Could not read from file(%s)", sMedia.c_str() );
			if( not feof( oFile ) )
				RuntimeError( "M3U file too large (>%d)", sizeof(oBuffer) );
			fclose( oFile );

			/* Directly push on the HTTP connection */
			mFields["Connection"] = "keep-alive";
			pHTTP->sendReply( 200, "OK", oBuffer, iRead, mFields );
		}
		else if( sApp == "M3U" or sApp == "APPLE" )
		{
			/* We do not have any physical M3U files on the server, we generate the M3U message on
			 * the fly.
			 * Sample URL: http://localhost/M3U/localhost/FILE@TS/flash/da.flv/DUMMY/sirannon.m3u
			 */
			static const char sM3UTemplate [] =
					"#EXTM3U\r\n"
					"#EXT-X-MEDIA-SEQUENCE:0\r\n"
					"#EXT-X-TARGETDURATION:%d\r\n"
					"#EXTINF:%d,\r\n"
					"%s\r\n"
					"#EXT-X-ENDLIST\r\n";

			/* Generate the URL on-the-fly */
			char sUrl[sMedia.length() + 10]; // http:// (7) /FILE@TS/ (9) EOS (1)
			if( sApp == "M3U" )
				snprintf( sUrl, sizeof(sUrl), "http://%s", sMedia.c_str() );
			else
				snprintf( sUrl, sizeof(sUrl), "/FILE@TS/%s", sMedia.c_str() );

			/* Fill in the data */
			char sM3U[ sizeof(sM3UTemplate) + sizeof(sUrl) + 21 ]; // 10 for each int
			int iDuration = 30;
			snprintf( sM3U, sizeof(sM3U), sM3UTemplate, iDuration, iDuration, sUrl );
			debug( 1, "generated M3U:\n%s", sM3U );

			/* Succes */
			mFields["Connection"] = "keep-alive";
			pHTTP->sendReply( 200, "OK", (uint8_t*)sM3U, strlen(sM3U), mFields );
		}
		else
		{
			/* Create the streamer */
			createStreamer();

			if( sApp == "HTTP" )
			{
				/* We can fill out the size of the file and keep the connection alive */
				char sTemp [21];
				snprintf( sTemp, sizeof(sTemp), "%d", pStreamer->getDescriptor()->bytesize );
				mFields["Connection"] = "keep-alive";
				mFields["Content-Length"] = sTemp;
				pHTTP->sendReply( 200, "OK", NULL, 0, mFields );
			}
			else
			{
				mFields["Connection"] = "close";
				pHTTP->sendReply( 200, "OK", NULL, 0, mFields );
				bClose = true;
			}
			/* Instant play */
			double fSpeed = 1.1;
			if( oFormat.first != mux_t::TS )
				fSpeed = 2.0;
			pStreamer->play( fSpeed );
			bSchedule = true;
		}
	}
}

void HTTPSession::analyzeUrl( void )
{
	/* Analyze the url to detect the application and file */
	oFormat.first = mux_t::NO;
	iTarget = target_t::NO;
	URL_parse( pHTTP->getUrl(), sApp, oFormat, iTarget, sMedia );

	/* Not defined in the url? Then make an educated guess */
	if( oFormat.first == mux_t::NO )
	{
		if( strncmp( sApp.c_str(), "M3U", 3 ) == 0 )
			oFormat.first = mux_t::M3U;
		else if( sApp == "APPLE" )
			oFormat.first = mux_t::M3U;
		else if( sApp == "HTTP" )
			oFormat = ExtensionToContainer( sMedia );

		if( oFormat.first == mux_t::NO )
			RuntimeError( "Could not guess container type for URL(%s)", pHTTP->getUrl().c_str() );
	}
	debug( 1, "Client requested url(%s) app(%s) format(%s+%s+%s) media(%s)", pHTTP->getUrl().c_str(), sApp.c_str(),
			MuxToString(oFormat.first), CodecToString(oFormat.second), TargetToString(iTarget), sMedia.c_str() );
}

void HTTPSession::createStreamer( void )
{
	map<string,bool> mBool;
	map<string,string> mString;
	map<string,int> mInt;
	map<string,double> mDouble;
	map<string,void*> mPrivate;
	vector<pair<MediaProcessor*,int> > oOut;

	/* Name */
	string sFile = pServer->getPath( sMedia );

	/* Streamer options */
	oOut.push_back( pair<MediaProcessor*,int>( this, 0 ) );
	mString["filename"] = sFile;
	mString["url"] = sMedia;
	mInt["loop"] = 1;
	mString["mode"] = "default";
	mBool["aggregate"] = false;
	mBool["ts-mode"] = false;
	mString["scheduler"] = "basic";
	mBool["pause"] = true;
	mInt["scheduler-buffer"] = 1000;
	mInt["multiplexer-delay"] = 0;
	mInt["scheduler-delay"] = 0;
	mBool["debug"] = this->mBool["debug"];
	mPrivate["session"] = this;
	mBool["video-mode"] = true;
	mBool["audio-mode"] = true;
	mInt["mode"] = MediaServer::HTTP;
	mInt["format"] = oFormat.first;
	mInt["codec"] = oFormat.second;
	mInt["target"] = iTarget;

	/* Create the streamer */
	MediaProcessor* pProcessor = oProcessorManager.createProcessorDynamic(
			sApp, "streamer",
			NULL, &oOut,
			&mInt, &mDouble, &mString, &mBool, &mPrivate );
	pStreamer = dynamic_cast<ProxyStreamer*>( pProcessor  );
	if( not pStreamer )
		TypeError( this, "Application(%s) is not of type ProxyStreamer", sApp.c_str() );

	/* Activate the player inside the streamer */
	pStreamer->createSource();
	pStreamer->createBuffer();
}

void HTTPSession::receive( MediaPacketPtr& pPckt )
{
	debug( 1, "sending: %s", pPckt->c_str_long() );
	pConnection->sendSmart( pPckt->data(), pPckt->size() );
	route( pPckt );
}

void HTTPSession::receive_reset( MediaPacketPtr& pPckt )
{
	debug( 1, "Ignorning %s", pPckt->c_str_short() );
	route( pPckt );
}

void HTTPSession::receive_end( MediaPacketPtr& pPckt )
{
	debug( 1, "Handling %s", pPckt->c_str_short() );
	route( pPckt );

	/* Terminate the streamer */
	pStreamer->end();
	pStreamer = NULL;

	/* If the connection was set to close we must shutdown */
	if( bClose )
		this->end();
}
