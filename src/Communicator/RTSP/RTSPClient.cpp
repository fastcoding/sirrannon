#include "RTSPClient.h"
#include "PortManager.h"
#include "Frame.h"
#include "Url.h"
#include "sdp.h"
#define SIRANNON_USE_BOOST_REGEX
#include "Boost.h"

/**
 * RTSP CLIENT
 * @component RTSP-client
 * @type media-client
 * @info Requests and receives a stream via RTSP. Generates audio and video frames.
 **/
REGISTER_CLASS( RTSPClient, "RTSP-client" );

RTSPClient::RTSPClient( const string& sName, ProcessorManager* pScope )
	: Block(sName, pScope),
	  pHTTP(NULL), pConnection(NULL),
	  bMetaData(false),
	  iSeq(0)
{
	mString["url"] = "rtsp://localhost:5554/RTSP/flash/da.flv";
	mBool["auto-play"] = true;
}

RTSPClient::~RTSPClient()
{
	delete pHTTP;
	delete pConnection;
}

void RTSPClient::handleError( SirannonException* pException, ProcessorManager* pManager, MediaProcessor* pProcessor )
{
	pScope->handleError( pException, pManager, this );
}

void RTSPClient::init( void )
{
	MediaProcessor::init();

	bAutoPlay = mBool["auto-play"];

	out = oProcessorManager.createProcessor( "out", "out" );

	createThread( bind( &RTSPClient::handleCommunication, this ) );
}

void RTSPClient::handleCommunication( void )
{
	/* Parse the URL */
	string sProtocol, sServer, sMedia;
	int iPort;
	sUrl = mString["url"];
	URL_canonize( sUrl, "rtsp" );
	URL_parse( sUrl, sProtocol, sServer, iPort, sMedia );

	/* Parse the address */
	IPAddress oAddr ( sServer, iPort );
	if( not oAddr.valid() )
		RuntimeError( this, "Invalid server address(%s)", oAddr.getAddressStr().c_str() );
	debug( 1, "connecting(%s)", sUrl.c_str() );

	/* Create the TCP connection */
	pConnection = new TCPSocket( oAddr.getIPStr(), oAddr.getPort() );

	/* Create the HTTP /communicator with this */
	pHTTP = new HTTPConnection( pConnection, "RTSP/1.0" );

	/* Send DESCRIBE & SETUP */
	sendDESCRIBE();
	sendSETUP();

	/* Auto-play? */
	if( bAutoPlay )
		play();
}

int RTSPClient::sendDESCRIBE( void )
{
	/* DESCRIBE send */
	prepareMessage();
	mFields["Accept"] = "application/sdp";
	pHTTP->sendMessage( "DESCRIBE", sUrl.c_str(), NULL, 0, mFields );

	/* DESCRIBE reply */
	pHTTP->receiveReply( true );
	if( pHTTP->getStatusNumber() != HTTPConnection::OK )
		RuntimeError( this, "Server replied failure: %d %s", pHTTP->getStatusNumber(), pHTTP->getStatus().c_str() );

	/* Parse SDP */
	if( pHTTP->getContentSize() == 0 )
		RuntimeError( this, "No content in DESCRIBE reply" );
	mux_t::type iMux;
	if( SDPParse( (char*)pHTTP->getContent(), iMux, &oContainer ) < 0 )
		RuntimeError( this, "Failed to parse SDP response:\n%s", (char*)pHTTP->getContent() );

	/* We can only guess the duration.. */
	//oContainer.duration = 1000000000;
	debug( 1, "DESCRIBE succesful:\n%s", (char*)pHTTP->getContent() );
	return 0;
}

int RTSPClient::sendSETUP( void )
{
	/* Send a SETUP for each stream */
	int iMedia = oContainer.size();
	int i = 0;
	for( 	ContainerDescriptor::iterator pDesc = oContainer.begin();
			pDesc != oContainer.end(); ++i )
	{
		/* Do we support this codec? */
		using namespace codec_t;
		const char* sType = NULL;
		switch( pDesc->codec )
		{
		case avc:
			sType = "avc-unpacketizer";
			break;

		case mp4a:
		case mp4v:
			sType = "MP4-unpacketizer";
			break;

		case mp1v:
		case mp2v:
		case mp1a:
		case mp2a:
			sType = "MP2-unpacketizer";
			break;

		case anb:
		case awb:
			sType = "AMR-unpacketizer";
			break;

		default:
			RuntimeError( this, "No unpacketizer for codec(%s)", CodecToString(pDesc->codec) );
			pDesc = oContainer.erase( pDesc );
			continue;
		}
		/* Determine the port */
		int iPort = pDesc->clientPort;
		if( iPort <= 0 )
			iPort = oPortManager.next();

		/* Send the request */
		string sFinalUrl = sUrl + "/" + pDesc->track;
		prepareMessage();
		snprintf( sRequest, sizeof(sRequest), "RTP/AVP;unicast;client_port=%d-%d", iPort, iPort+1 );
		mFields["Transport"] = sRequest;
		pHTTP->sendMessage( "SETUP", sFinalUrl.c_str(), NULL, 0, mFields );

		/* Handle the response */
		pHTTP->receiveReply( true );
		if( pHTTP->getStatusNumber() != HTTPConnection::OK )
			RuntimeError( this, "SETUP: Server responds failure( %d %s)",
					pHTTP->getStatusNumber(), pHTTP->getStatus().c_str() );

		/* Required fields Transport and Session */
		if( not pHTTP->mFields.count( "Session" ) or not pHTTP->mFields.count( "Transport") )
			SyntaxError( this, "SETUP: Missing fields(Session/Transport):\n%s", (char*)pHTTP->getContent() );
		sSession = pHTTP->mFields["Session"];

		/* Parse Transport field */
		const static regex oExpression( "RTP/AVP;unicast;client_port=(\\d+)\\-(\\d+);server_port=(\\d+)\\-(\\d+)" );
		cmatch oParse;
		if( not regex_match( pHTTP->mFields["Transport"].c_str(), oParse, oExpression ) )
			SyntaxError( this, "SETUP: Malformatted field(Transport)" );
		pDesc->clientPort = atoi( oParse[1].first );
		pDesc->serverPort = atoi( oParse[3].first );

		/* Create the receiver */
		snprintf( sRequest, sizeof(sRequest), "receiver-%d", i );
		MediaProcessor* pReceiver = oProcessorManager.createProcessor( "RTP-receiver", sRequest );
		pReceiver->setInt( "port", pDesc->clientPort );
		pReceiver->setInt( "audio-route", pDesc->route );
		pReceiver->setInt( "video-route", pDesc->route );
		pReceiver->setBool( "debug", bDebug );
		pReceiver->setPrivate( "descriptor", &(*pDesc) );

		/* Create the unpacketizer */
		snprintf( sRequest, sizeof(sRequest), "unpacketizer-%d", i );
		MediaProcessor* pUnpacketizer = oProcessorManager.createProcessor( sType, sRequest );
		pUnpacketizer->setBool( "debug", false );
		pReceiver->setRoute( 0, pUnpacketizer );

		/* Add a transformer when dealing with MP4A & AVC */
		if( pDesc->codec == mp4a )
		{
			/* Create a convertor */
			snprintf( sRequest, sizeof(sRequest), "transformer-%d", i );
			MediaProcessor* pConvertor = oProcessorManager.createProcessor( "frame-transformer", sRequest );
			pConvertor->setBool( "ES", not mBool["mov-frame"] );
			pUnpacketizer->setRoute( 0, pConvertor );
			pConvertor->setRoute( 0, out );
		}
		else if( pDesc->codec == svc or pDesc->codec == avc )
		{
			/* Create a restamper */
			snprintf( sRequest, sizeof(sRequest), "restamp-%d", i );
			MediaProcessor* pRestamp = oProcessorManager.createProcessor( "restamp", sRequest );

			/* Create a convertor */
			snprintf( sRequest, sizeof(sRequest), "transformer-%d", i );
			MediaProcessor* pConvertor = oProcessorManager.createProcessor( "frame-transformer", sRequest );
			pConvertor->setBool( "ES", not mBool["mov-frame"] );

			pUnpacketizer->setRoute( 0, pRestamp );
			pRestamp->setRoute( 0, pConvertor );
			pConvertor->setRoute( 0, out );
		}
		else
		{
			pUnpacketizer->setRoute( 0, out );
		}
		debug( 1, "Created receivers for track(%d) codec(%s) content(%s)",
				i, CodecToString(pDesc->codec), ContentToString(pDesc->content) );
		++pDesc;
	}
	debug( 1, "Filtered available streams from %d to %d", iMedia, oContainer.size() );
	bMetaData = true;
	return 0;
}

int RTSPClient::sendPLAY( void )
{
	/* Send PLAY */
	prepareMessage();
	mFields["Session"] = sSession;
	mFields["Range"] = "npt=0-";
	pHTTP->sendMessage( "PLAY", sUrl.c_str(), NULL, 0, mFields );

	/* Handle the response */
	pHTTP->receiveReply( true );
	if( pHTTP->getStatusNumber() != HTTPConnection::OK )
		RuntimeError( this, "PLAY, Server responds failure(%d %s)", pHTTP->getStatusNumber(), pHTTP->getStatus().c_str() );
	debug( 1, "PLAY sucessful" );
	return 0;
}

int RTSPClient::sendTEARDOWN( void )
{
	prepareMessage();
	return pHTTP->sendMessage( "TEARDOWN", sUrl.c_str(), NULL, 0, mFields );
}

void RTSPClient::prepareMessage( void )
{
	mFields.clear();
	snprintf( sRequest, sizeof(sRequest), "%d", iSeq++ );
	mFields["CSeq"] = sRequest;
}

int RTSPClient::play( double fSpeed )
{
	/* Activate this component */
	debug( 1, "play" );
	if( not bSchedule )
	{
		initOut();
		oProcessorManager.initProcessors();
		bSchedule = true;
	}
	/* Send the aggregate play message */
	if( sendPLAY() < 0 )
		return -1;
	return 0;
}

int RTSPClient::seek( uint32_t iSeek )
{
	return -1;
}

int RTSPClient::pause( void )
{
	return -1;
}

bool RTSPClient::ready( void ) const
{
	return bMetaData;
}
