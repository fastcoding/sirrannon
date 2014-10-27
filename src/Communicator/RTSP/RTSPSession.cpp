#include "RTSPSession.h"
#include "sdp.h"
#include "Frame.h"
#include "RandomGenerator.h"
#include "Url.h"
#define SIRANNON_USE_BOOST_REGEX
#include "Boost.h"
using namespace content_t;

REGISTER_CLASS( RTSPSession, "RTSP-session" );

RTSPSession::RTSPSession( const string& sName, ProcessorManager* pScope )
	: MediaSession(sName, pScope), pHTTP(NULL), pStreamer(NULL),
	  sApp("RTSP"), iToken(oRandom.next()), bVLC(false)
{ }

RTSPSession::~RTSPSession()
{
	delete pHTTP;
}

void RTSPSession::init( void )
{
	MediaSession::init();
	pHTTP = new HTTPConnection( pConnection, "application/sdp", bDebug );

	createThread( bind( &RTSPSession::requestHandler, this) );
}

int RTSPSession::newStreamer( void )
{
	map<string,bool> mBool;
	map<string,string> mString;
	map<string,int> mInt;
	map<string,double> mDouble;
	map<string,void*> mPrivate;

	/* Streamer options */
	mString["filename"] = pServer->getPath( sMedia );
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
	mString["destination"] = ((TCPSocket*)pConnection)->getForeignAddress();
	mPrivate["session"] = this;
	mBool["video-mode"] = true;
	mBool["audio-mode"] = true;
	mBool["ts-mode"] = (oFormat.first == mux_t::TS);
	mInt["mode"] = MediaServer::RTSP;
	mInt["format"] = (oFormat.first == mux_t::TS) ? mux_t::TS : mux_t::RTP;
	mInt["target"] = iTarget;
	mString["interface"] = ((TCPSocket*)pConnection)->getLocalAddress();

	/* Create the streamer */
	MediaProcessor* pProcessor = oProcessorManager.createProcessorDynamic(
			sApp, "streamer",
			NULL, NULL,
			&mInt, &mDouble, &mString, &mBool, &mPrivate );
	pStreamer = dynamic_cast<ProxyStreamer*>( pProcessor  );
	if( not pStreamer )
		TypeError( this, "Application(%s) is not of type ProxyStreamer", sApp.c_str() );

	/* Activate the player inside the streamer */
	pStreamer->createSource();
	pStreamer->createBuffer();
	return 0;
}

void RTSPSession::requestHandler( void )
{
	while( true )
	{
		/* New message */
		if( pHTTP->receiveMessage( true ) < 0 )
			break;

		/* Debug */
		{
			debug( 1, "command(%s) url(%s) protocol(%s)",
			pHTTP->getCommand().c_str(), pHTTP->getUrl().c_str(), pHTTP->getProtocol().c_str() );
			const HTTPConnection::fields_t& mFields = pHTTP->getFields();
			for( HTTPConnection::fields_t::const_iterator i = mFields.begin(); i != mFields.end(); ++i )
			{
				debug( 1, "(%s): (%s)", i->first.c_str(), i->second.c_str() );
			}
		}
		/* Detect if the client is VLC */
		if( pHTTP->mFields["User-Agent"].find( "VLC" ) != string::npos )
			bVLC = true;

		/* Obtain URL */
		if( not sMedia.length() )
			if( analyzeUrl() < 0 )
				break;

		/* Create streamer */
		if( not pStreamer )
			if( newStreamer() < 0 )
				break;

		/* Catch bad requests */
		if( not pHTTP->mFields.count( "CSeq" ) )
		{
			mFields.clear();
			pHTTP->sendReply( 400, "Bad Request", NULL, 0, mFields );
			continue;
		}
		/* RTSP commands */
		int iStatus = 0;
		if( pHTTP->getCommand() == "OPTIONS" )
		{
			iStatus = handleOPTIONS();
		}
		else if( pHTTP->getCommand() == "DESCRIBE" )
		{
			iStatus = handleDESCRIBE();
		}
		else if( pHTTP->getCommand() == "SETUP" )
		{
			iStatus = handleSETUP();
		}
		else if( pHTTP->getCommand() == "PLAY" )
		{
			iStatus = handlePLAY();
		}
		else if( pHTTP->getCommand() == "PAUSE" )
		{
			iStatus = handlePAUSE();
		}
		else if( pHTTP->getCommand() == "TEARDOWN" )
		{
			iStatus = handleTEARDOWN();
		}
		else
			iStatus = handleUNSUPPORTED();

		if( iStatus < 0 )
			break;

	}
	end();
}

int RTSPSession::analyzeUrl( void )
{
	/* Analyze the url to detect the application and file */
	string sUrl = pHTTP->getUrl();
	URL_parse( sUrl, sApp, oFormat, iTarget, sMedia );
	debug( 1, "Client requested url(%s) app(%s) media(%s)", pHTTP->getUrl().c_str(), sApp.c_str(), sMedia.c_str() );
	return 0;
}

int RTSPSession::analyzeUrl1( void )
{
	/* Analyze the url */
	string sTrack;
	URL_parse( pHTTP->getUrl(), sApp, oFormat, iTarget, sTrack );

	/* Analyze the media part for the track */
	static const regex oUrlExpression( "(\\S+?)(?:/track(\\d+))?" );
	cmatch oParse;
	if( not regex_match( sTrack.c_str(), oParse, oUrlExpression ) )
		ValueError( this, "Invalid URL: %s", pHTTP->getUrl().c_str() );

	/* Track ID */
	if( oParse[2].matched )
		return atoi( oParse[2].first );
	else
		return 0;
}

int RTSPSession::handleOPTIONS( void )
{
	/* Construct response */
	mFields.clear();
	mFields["CSeq"] = pHTTP->mFields["CSeq"];
	mFields["Public"] = "DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE";

	/* Send */
	return pHTTP->sendReply( 200, "OK", NULL, 0, mFields );
}

int RTSPSession::handleDESCRIBE( void )
{
	/* Construct response */
	mFields.clear();
	mFields["CSeq"] = pHTTP->mFields["CSeq"];

	/* Accept field */
	if( pHTTP->mFields["Accept"].find( "application/sdp" ) == string::npos )
		return pHTTP->sendReply( 456, "Header Field Not Valid", NULL, 0, mFields );

	/* Construct the sdp */
	SDPConstruct( pStreamer->getDescriptor(), 0, mBool["transport-streams"] or oFormat.first == mux_t::TS, &oSDP );

	/* Patch all the SDP pieces together */
	string sSDP;
	SDPString( &oSDP, sSDP );
	debug( 1, "SDP:\n%s", sSDP.c_str() );

	/* Extra fields */
	mFields["Content-Type"] = "application/sdp";

	/* Send response */
	return pHTTP->sendReply( 200, "OK", (uint8_t*)sSDP.c_str(), sSDP.length(), mFields );
}

int RTSPSession::handleSETUP( void )
{
	/* Construct response */
	mFields.clear();
	mFields["CSeq"] = pHTTP->mFields["CSeq"];

	/* Analyze a field */
	if( pHTTP->mFields.find( "Transport" ) == mFields.end() )
		return pHTTP->sendReply( 456, "Header Field Not Valid", NULL, 0, mFields );

	/* Find the client port field */
	const static regex oExpression( "RTP/AVP;unicast;client_port=(\\d+)\\-(\\d+)" );
	cmatch oParse;
	if( not regex_match( pHTTP->mFields["Transport"].c_str(), oParse, oExpression ) )
		return pHTTP->sendReply( 461, "Unsupported Transport", NULL, 0, mFields );

	/* Map the track to an SDP */
	int iTrack = analyzeUrl1();
	if( iTrack < 0 )
		return pHTTP->sendReply( 404, "Not Found", NULL, 0, mFields );
	else if( iTrack == 0 )
		return pHTTP->sendReply( 459, "Aggregate Operation Not Allowed", NULL, 0, mFields );
	else if( iTrack-1 >= oSDP.size() )
		return pHTTP->sendReply( 404, "Not Found", NULL, 0, mFields );
	MediaDescriptor* pDesc = &oSDP[iTrack-1];
	pDesc->clientPort = atoi( oParse[1].first );

	/* Create the transmitters now */
	pStreamer->createTransmitter( pDesc );

	/* Transport response */
	char sResponse [1024];
	snprintf( sResponse, sizeof(sResponse), "RTP/AVP;unicast;client_port=%u-%u;server_port=%u-%u",
			pDesc->clientPort, pDesc->clientPort+1, pDesc->serverPort, pDesc->serverPort+1 );

	/* Session ID */
	char sSession[128];
	snprintf( sSession, sizeof(sSession), "%u", iToken );
	mFields["Session"] = sSession;
	mFields["Transport"] = sResponse;

	/* Send response */
	return pHTTP->sendReply( 200, "OK", NULL, 0, mFields );
}

int RTSPSession::handlePLAY( void )
{
	/* Construct response */
	mFields.clear();
	mFields["CSeq"] = pHTTP->mFields["CSeq"];

	/* Map the track to an SDP */
	int iTrack = analyzeUrl1();
	if( iTrack < 0 )
		return pHTTP->sendReply( 404, "Not Found", NULL, 0, mFields );
	else if( iTrack > 0 )
		return pHTTP->sendReply( 460, "Only Aggregate Operation Allowed", NULL, 0, mFields );

	/* Range field is copy */
	if( mFields.count( "Range") )
		mFields["Range"] = pHTTP->mFields["Range"];

	/* Execute command */
	pStreamer->play( -1.0 );
	bSchedule = true;

	/* Send response */
	return pHTTP->sendReply( 200, "OK", NULL, 0, mFields );
}

int RTSPSession::handlePAUSE( void )
{
	/* Construct response */
	mFields.clear();
	mFields["CSeq"] = pHTTP->mFields["CSeq"];

	/* Execute command */
	pStreamer->pause();

	/* Send response */
	return pHTTP->sendReply( 200, "OK", NULL, 0, mFields );
}

int RTSPSession::handleTEARDOWN( void )
{
	/* Construct response */
	mFields.clear();
	mFields["CSeq"] = pHTTP->mFields["CSeq"];

	/* Send response */
	pHTTP->sendReply( 200, "OK", NULL, 0, mFields );

	/* We are done */
	end();
	return 0;
}

int RTSPSession::handleUNSUPPORTED( void )
{
	/* Construct response */
	mFields.clear();
	mFields["CSeq"] = pHTTP->mFields["CSeq"];

	/* Send response */
	return pHTTP->sendReply( 501, "Not implemented", NULL, 0, mFields );
}
