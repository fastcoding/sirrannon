/*****************************************************************************
 * (c)2006-2010 Sirannon
 * Authors: Alexis Rombaut <alexis.rombaut@intec.ugent.be>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *****************************************************************************/
#include "RTMPServer.h"

/**
 * RTMP SERVER
 * @component RTMP-server
 * @type media-server
 * @param port, int, 1935, listen to this port for incoming connections
 * @info Listens for incoming RTMP connections. Supports pausing and seeking.
 * Performs a cryptographical handshake to enable Flash Player to play H.264/AVC and
 * MPEG4-Audio.
 **/

REGISTER_CLASS( RTMPServer, "RTMP-server" );

RTMPServer::RTMPServer( const char* sName, ProcessorManager* pScope )
	: MediaServer(sName, pScope)
{
	sSession = "RTMP-session";
}
