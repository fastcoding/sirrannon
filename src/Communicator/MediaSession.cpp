#include "MediaSession.h"

MediaSession::MediaSession( const string& sName, ProcessorManager* pScope )
	: MediaProcessor(sName, pScope), NestedInterface(sName, pScope),
	  pServer(NULL), pConnection(NULL), iTarget(target_t::NO), oFormat(mux_t::NO, codec_t::NO)
{ }

MediaSession::~MediaSession()
{
	pServer->endSession( this, pConnection );
}

void MediaSession::init( void )
{
	MediaProcessor::init();

	pConnection = (TCPSocket*)( mPrivate["connection"] );
	pServer = (MediaServer*)( mPrivate["server"] );
}

void MediaSession::process( void )
{
	oProcessorManager.scheduleProcessors();
}

void MediaSession::handleError( SirannonException* pException, ProcessorManager* pManager, MediaProcessor* pProcessor )
{
	SirannonWarning( this,  "Handling: %s", pException->what() );

	/* This session cannot continue */
	end();
}
