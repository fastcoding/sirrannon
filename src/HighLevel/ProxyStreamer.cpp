#include "ProxyStreamer.h"
#include "Frame.h"
#include "PortManager.h"
#include "Reader/Reader.h"
#include "Url.h"
#include "str.h"
REGISTER_CLASS( RTMPProxy, "RTMP-proxy" );
REGISTER_CLASS( RTMPTProxy, "RTMPT-proxy" );
REGISTER_CLASS( RTSPProxy, "RTSP-proxy" );
REGISTER_CLASS( HTTPProxy, "HTTP-proxy" );
REGISTER_CLASSES( FileProxy, "FILE", 1 );
REGISTER_CLASSES( FileProxy, "HLS", 2 );
REGISTER_CLASSES( FileHTTPProxy, "HTTP", 1 );
REGISTER_CLASS( LiveProxy, "LIVE" );
REGISTER_CLASS( H264Proxy, "H264" );

ProxyStreamer::ProxyStreamer( const string& sName, ProcessorManager* pScope )
	: FileStreamer(sName, pScope), bReady(false), bRawFile(true), oFormat(mux_t::NO, codec_t::NO),
	  iMediaServer(MediaServer::RTSP), bTranscoding(false), iTarget(target_t::NO), bMovFrame(false)
{
	mString["client"] = "RTMP-client";
	mBool["video-mode"] = true;
	mBool["audio-mode"] = true;
	mInt["format"] = mux_t::NO;
	mInt["codec"] = codec_t::NO;
	mInt["target"] = target_t::NO;
	mInt["mode"] = MediaServer::RTSP;
	mBool["apple"] = false;
	mString["cache"] = "";
	mInt["min-ts"] = -1;
	mInt["max-ts"] = -1;
	mInt["length"] = -1;
	mString["scheduler"] = "basic-scheduler";
}

ProxyStreamer::~ProxyStreamer()
{ }

bool ProxyStreamer::ready( void ) const
{
	return bReady;
}

void ProxyStreamer::init( void )
{
	MediaProcessor::init();

	/* Check parent */
	bVideo = mBool["video-mode"];
	bAudio = mBool["audio-mode"];
	oFormat.first = (mux_t::type) mInt["format"];
	oFormat.second = (codec_t::type) mInt["codec"];
	iTarget = (target_t::type) mInt["target"];
	iMediaServer = (MediaServer::MediaServer_t) mInt["mode"];

	/* Deduce */
	bMovFrame = (iMediaServer == MediaServer::HTTP and oFormat.first != mux_t::TS and oFormat.first != mux_t::ES ) or (iMediaServer == MediaServer::RTMP);
}

void ProxyStreamer::createSource( void )
{
	/* Create reader */
	bRawFile = (mString["client"] == "basic-reader");
	MediaProcessor* pClient = pLink = oProcessorManager.createProcessor( mString["client"], "client" );
	pSource = dynamic_cast<SourceInterface*>( pClient );
	if( not pSource )
		ValueError( this, "Player of invalid type(%s)", mString["client"].c_str() );
	pClient->setString( "filename", mString["filename"] );
	pClient->setString( "url", mString["url"] );
	pClient->setBool( "debug", bDebug and mString["client"] != "ffmpeg-reader" and mString["client"] != "basic-reader" );
	pClient->setBool( "video-mode", bVideo );
	pClient->setBool( "audio-mode", bAudio );
	pClient->setInt( "loop", mInt["loop"] );
	pClient->setBool( "repeat-parameter-sets", true );
	pClient->setBool( "skip-AUD", oFormat.first == mux_t::TS );
	pClient->setBool( "mov-frame", bMovFrame );
	pClient->setInt( "dts-start", 0 );
	pClient->setBool( "auto-play", false );
	pClient->setInt( "audio-route", 0 );
	pClient->setInt( "video-route", 0 );
	pClient->setInt( "min-ts", mInt["min-ts"] );
	pClient->setInt( "max-ts", mInt["max-ts"] );
	pClient->setInt( "length", mInt["length"] );
	pClient->setBool( "fix-PTS", false );
	pClient->initProcessor();

	/* Sleep until the reader is ready */
	while( not pSource->ready() )
		oQuantum.sleep();

	/* Convert the codecs if necessary */
	const ContainerDescriptor* pContainer = pSource->getDescriptor();
	if( oFormat.first != mux_t::NO and oFormat.first != mux_t::ES and not bRawFile )
	{
		/* Create a hub to join all transcoded and untouched streams */
		pLink = oProcessorManager.createProcessor( "dummy", "transcoder-join" );
		pLink->initProcessor();

		/* Create the transcoders */
		for( int i = 0; i < pContainer->size(); ++i )
		{
			char sTranscoder [32];
			const MediaDescriptor* pDesc = &(*pContainer)[i];
			snprintf( sTranscoder, sizeof(sTranscoder), "transcoder-%d", i );

			/* Determine the target codec */
			codec_t::type iCodec = VerifyCodecForContainer( oFormat, pDesc->codec );
			if( iCodec == codec_t::NO )
				RuntimeError( this, "No suitable codec found to replace codec(%s) for use in container(%s)",
						CodecToString(pDesc->codec), MuxToString(oFormat.first) );

			if( pDesc->codec != iCodec or iTarget != target_t::NO )
			{
				debug( 1, "adding transcoder (%s->%s)", CodecToString(pDesc->codec), CodecToString(iCodec) );
				MediaProcessor* pTranscoder = NULL;

				if( pDesc->content == content_t::video )
				{
					/* Create a transcoder to the new video codec */
					pTranscoder = oProcessorManager.createProcessor( "transcoder-video", sTranscoder );
					pTranscoder->setInt( "output-codec-id", iCodec );
					pTranscoder->setInt( "route", pDesc->route );
					pTranscoder->setBool( "mov-frame", bMovFrame );
					pTranscoder->setString( "target", TargetToString(iTarget) );
					pTranscoder->setBool( "debug", true );
					pTranscoder->setPrivate( "desc", (void*)pDesc );
					pTranscoder->setInt( "min-ts", mInt["min-ts"] );
					pTranscoder->setInt( "max-ts", mInt["max-ts"] );
					pTranscoder->setInt( "width", mInt["width"] );
					pTranscoder->setInt( "height", mInt["height"] );
					pTranscoder->setInt( "bitrate", mInt["bitrate"] );
				}
				/* Transode the audio */
				else if( pDesc->content == content_t::audio )
				{
					/* Create a transcoder to the new audio */
					pTranscoder = oProcessorManager.createProcessor( "transcoder-audio", sTranscoder );
					pTranscoder->setInt( "output-codec-id", iCodec );
					pTranscoder->setInt( "route", pDesc->route );
					pTranscoder->setBool( "debug", true );
					pTranscoder->setString( "target", TargetToString(iTarget) );
					pTranscoder->setPrivate( "desc", (void*)pDesc );
					pTranscoder->setBool( "mov-frame", bMovFrame );
					pTranscoder->setInt( "bitrate", mInt["bitrate"] );
					pTranscoder->setInt( "min-ts", mInt["min-ts"] );
					pTranscoder->setInt( "max-ts", mInt["max-ts"] );
				}
				/* Connect and init */
				pClient->setRoute( pDesc->route, pTranscoder );
				pTranscoder->setRoute( 0, pLink );
				pTranscoder->initProcessor();

				/* Wait until the transcoder is ready */
				SourceInterface* pTranscoderSource = dynamic_cast<SourceInterface*>( pTranscoder );
				while( not pTranscoderSource->ready() )
					oQuantum.sleep();

				/* Store the new descriptor */
				bTranscoding = true;
				const MediaDescriptor* pTranscodedDesc = &pTranscoderSource->getDescriptor()->front();
				oTranscodedContainer.addDescriptor( pTranscodedDesc );
			}
			else
			{
				/* No transcoding required */
				pClient->setRoute( pDesc->route, pLink );
				oTranscodedContainer.addDescriptor( pDesc );
			}
		}
		/* Change the descriptor if there was transcoding */
		if( bTranscoding )
			pContainer = &oTranscodedContainer;
	}

	/* What content is finally present? */
	bVideo = bAudio = true;
	if( bVideo and not pContainer->getVideoDescriptor() )
		bVideo = false;
	if( bAudio and not pContainer->getAudioDescriptor() )
		bAudio = false;
	if( not bVideo and not bAudio )
		RuntimeError( this, "Video nor audio avaiable for streaming" );

	/* Create the packetizers */
	char sName [64];
	for( int i = 0; i != pContainer->size(); ++i )
	{
		const MediaDescriptor* pDesc = &(*pContainer)[i];
		MediaProcessor* pPack;
		snprintf( sName, sizeof(sName), "packetizer-%d", i );

		if( bRawFile )
		{
			pPack = oProcessorManager.createProcessor( "dummy", sName );
		}
		else if( oFormat.first == mux_t::TS )
		{
			pPack = oProcessorManager.createProcessor( "pes-packetizer", sName );
			pPack->setInt( "delta", 0 );
			pPack->setBool( "insert-AUD", true );
			pPack->setBool( "zero-length", (pDesc->content == content_t::video) );
			pPack->setInt( "audio-per-PES", 15 );
			pPack->setBool( "debug", false );
		}
		else if( iMediaServer == MediaServer::HTTP )
		{
			pPack = oProcessorManager.createProcessor( "dummy", sName );
		}
		else if( iMediaServer == MediaServer::RTMP )
		{
			pPack = oProcessorManager.createProcessor( "RTMP-packetizer", sName );
			pPack->setBool( "debug", false );
			pPack->setInt( "stream-ID", mInt["RTMP-streamID"] );
			pPack->setInt( "chunk-ID", mInt["RTMP-video-chunkID"] + i );
			pPack->setInt("chunk-size", mInt["RTMP-chunk-size"] );
		}
		else
		{
			switch( pDesc->codec )
			{
			case codec_t::mp4v:
			case codec_t::mp4a:
				pPack = oProcessorManager.createProcessor( "mp4-packetizer", sName );
				break;

			case codec_t::mp1a:
			case codec_t::mp2a:
			case codec_t::mp1v:
			case codec_t::mp2v:
				pPack = oProcessorManager.createProcessor( "mp2-packetizer", sName );
				break;

			case codec_t::avc:
			case codec_t::svc:
			case codec_t::mvc:
				pPack = oProcessorManager.createProcessor( "avc-packetizer", sName );
				break;

			case codec_t::anb:
			case codec_t::awb:
				pPack = oProcessorManager.createProcessor( "AMR-packetizer", sName );
				break;

			case codec_t::ac3:
				pPack = oProcessorManager.createProcessor( "AC3-packetizer", sName );
				break;

			default:
				SirannonWarning( this,  "Packetizer for codec(%s) not supported", CodecToString(pDesc->codec) );
				pPack = oProcessorManager.createProcessor( "in", sName );
			}
		}
		pLink->setRoute( pDesc->route, pPack );
		pPack->initProcessor();
	}
}

inline void ProxyStreamer::connectPacketizers( const string& sDst )
{
	const ContainerDescriptor* pContainer = pSource->getDescriptor();
	char sSrc [64];
	for( int i = 0; i != pContainer->size(); ++i )
	{
		snprintf( sSrc, sizeof(sSrc), "packetizer-%d", i );
		oProcessorManager.setRoute( sSrc, sDst, 0 );
	}
}

void ProxyStreamer::createBuffer( void )
{
	/* Create the multiplexer */
	MediaProcessor* pMux;
	if( bRawFile )
	{
		pMux = oProcessorManager.createProcessor( "dummy", "multiplexer" );
	}
	else if( oFormat.first == mux_t::TS )
	{
		pMux = oProcessorManager.createProcessor( "ts-multiplexer", "multiplexer" );
		pMux->setBool( "delay", mInt["multiplexer-delay"] );
		pMux->setBool( "aggregate", true );
		if( iMediaServer == MediaServer::HTTP )
		{
			pMux->setInt( "mtu", 65535 );
		}
		pMux->setInt( "streams", pSource->getDescriptor()->size() );
		pMux->setBool( "debug", false );
	}
	else if( iMediaServer == MediaServer::HTTP )
	{
		if( oFormat.first == mux_t::ES )
		{
			pMux = oProcessorManager.createProcessor( "dummy", "multiplexer" );
		}
		else
		{
			/* Create the multiplexer with a specific format */
			pMux = oProcessorManager.createProcessor( "ffmpeg-multiplexer", "multiplexer" );
			pMux->setBool( "debug", true );
			pMux->setInt( "delay", mInt["multiplexer-delay"] );
			pMux->setInt( "streams", pSource->getDescriptor()->size() );
			pMux->setInt( "format", oFormat.first );
			pMux->setInt( "codec", oFormat.second );
			//pMux->setBool( "streamed", true );
		}
	}
	else if( pSource->getDescriptor()->size() > 1 )
	{
		pMux = oProcessorManager.createProcessor( "std-multiplexer", "multiplexer" );
		pMux->setInt( "delay", mInt["multiplexer-delay"] );
		pMux->setInt( "streams", pSource->getDescriptor()->size() );
	}
	else
	{
		pMux = oProcessorManager.createProcessor( "dummy", "multiplexer" );
	}
	/* Create the scheduler */
	pScheduler = oProcessorManager.createProcessor( "basic-scheduler", "scheduler" );
	pBuffer = dynamic_cast<BufferInterface*>( pScheduler );
	pScheduler->setInt( "delay", 0 );
	pScheduler->setInt("absolute-delay", SirannonTime::getUpTime().convertMsecs() );
	pScheduler->setInt( "buffer", mInt["scheduler-buffer"] );
	pScheduler->setBool( "pause", mBool["pause"] );
	pScheduler->setBool( "debug", true );

	/* Connect and init */
	connectPacketizers( "multiplexer" );
	pMux->setRoute( 0, pScheduler );
	pMux->initProcessor();
	pScheduler->initProcessor();

	/* Reset */
	pSource->findSchedulers();

	/* Out */
	out = oProcessorManager.createProcessor( "out", "out" );
	out->initProcessor();
	initOut();

	/* Add a sink when using HTTP */
	if( iMediaServer == MediaServer::HTTP )
	{
		MediaProcessor* pSink = oProcessorManager.createProcessor( "sink", "sink" );
		pSink->setBool( "audio-mode", bAudio );
		pSink->setBool( "video-mode", bVideo );
		pScheduler->setRoute( 0, pSink );
		pSink->setRoute( 0, out );
		pSink->initProcessor();
	}
	else if( iMediaServer == MediaServer::RTMP )
		pScheduler->setRoute( 0, out );

	/* Add an additional cache */
	const string& sCache = mString["cache"];
	if( not sCache.empty() )
	{
		debug( 1, "Setting cache %s", sCache.c_str() );
		MediaProcessor* pWriter = oProcessorManager.createProcessor( "basic-writer", "writer" );
		pWriter->setString( "filename", sCache );
		pWriter->setBool( "complete", true );
		pScheduler->setRoute( 0, pWriter );
		pWriter->initProcessor();
	}
	/* Metadata is ready */
	bReady = true;
}

void ProxyStreamer::createTransmitter( MediaDescriptor* pDesc )
{
	/* Ignore if NULL */
	if( not pDesc )
		return;

	/* Type of transmitter */
	char sTransmitterType [128];
	char sTransmitterName [128];
	char sDestination [128];
	snprintf( sTransmitterType, sizeof(sTransmitterType), "%s-transmitter", mString["transmitter"].c_str() );
	snprintf( sTransmitterName, sizeof(sTransmitterName), "%s-transmitter", pDesc->track.c_str() );
	snprintf( sDestination, sizeof(sDestination), "%s:%d", mString["destination"].c_str(), pDesc->clientPort );

	/* Create it */
	pDesc->serverPort = oPortManager.next();
	MediaProcessor* pProcessor = oProcessorManager.createProcessor( sTransmitterType, sTransmitterName );
	TransmitterInterface* pTransmitter = dynamic_cast<TransmitterInterface*>( pProcessor );
	if( not pTransmitter )
		ValueError( this, "Transmitter(%s) not of type(TransmitterInterface)", sTransmitterType );
	pProcessor->setInt( "port", pDesc->serverPort );
	pProcessor->setString( "destination", sDestination );
	pProcessor->setInt( "payload", pDesc->payload );
	pProcessor->setBool( "pts", MediaServer::RTSP and ( pDesc->codec & codec_t::H264 ) and oFormat.first != mux_t::TS );
	pProcessor->setBool( "extension", false );
	pProcessor->setBool( "debug", false );
	pProcessor->setString( "interface", mString["interface"] );
	pScheduler->setRoute( pDesc->route, pProcessor );
	pProcessor->setRoute( 0, out );
	pProcessor->initProcessor();

	if( pDesc->content == content_t::audio )
		pAudioTransmitter = pTransmitter;
	else
		pVideoTransmitter = pTransmitter;
}

int ProxyStreamer::play( double fSpeed ) synchronized
{
	bSchedule = true;
	return FileStreamer::play( fSpeed );
} end_synchronized

const ContainerDescriptor* ProxyStreamer::getDescriptor( void ) const
{
	if( bTranscoding )
		return &oTranscodedContainer;
	else
		return pSource->getDescriptor();
}

RTMPProxy::RTMPProxy( const string& sName, ProcessorManager* pScope )
	: ProxyStreamer(sName, pScope)
{
	mString["client"] = "RTMP-client";
}

RTMPTProxy::RTMPTProxy( const string& sName, ProcessorManager* pScope )
	: ProxyStreamer(sName, pScope)
{
	mString["client"] = "RTMPT-client";
}

RTSPProxy::RTSPProxy( const string& sName, ProcessorManager* pScope )
	: ProxyStreamer(sName, pScope)
{
	mString["client"] = "RTSP-client";
}

HTTPProxy::HTTPProxy( const string& sName, ProcessorManager* pScope )
	: ProxyStreamer(sName, pScope)
{
	mString["client"] = "HTTP-client";
}

FileProxy::FileProxy( const string& sName, ProcessorManager* pScope )
	: ProxyStreamer(sName, pScope)
{
	mString["client"] = "ffmpeg-reader";
}

FileHTTPProxy::FileHTTPProxy( const string& sName, ProcessorManager* pScope )
	: ProxyStreamer(sName, pScope)
{
	mString["client"] = "basic-reader";
}

LiveProxy::LiveProxy( const string& sName, ProcessorManager* pScope )
	: ProxyStreamer(sName, pScope)
{
	mString["client"] = "live-reader";
	mInt["scheduler-delay"] = 100;
}

H264Proxy::H264Proxy( const string& sName, ProcessorManager* pScope )
	: ProxyStreamer(sName, pScope)
{
	mString["client"] = "avc-reader";
}
