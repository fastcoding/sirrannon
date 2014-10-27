#include "RTSPServer.h"

/**
 * RTSP SERVER
 * @component RTSP-server
 * @type media-server
 * @param port, int, 554, listen to this port for incoming connections
 * @info Listens for RTSP connections. Supports pausing, but not seeking.
 **/

REGISTER_CLASS( RTSPServer, "RTSP-server" );

RTSPServer::RTSPServer( const string& sName, ProcessorManager* pScope )
	: MediaServer(sName, pScope)
{
	sSession = "RTSP-session";
}
