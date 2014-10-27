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
#include "System.h"

/**
 * SYSTEM
 * @component system
 * @type core
 * @properties abstract
 **/

/** SINK
 * @component sink
 * @type system
 * @param video-mode, bool, true, if true, a video end-packet must be received
 * @param audio-mode, bool, false, if true, an audio end-packet must be received
 * @param count, int, 0, if larger than 0, the number of end-packets which must be received
 * @info When this component receives an end-packet the program will terminate gracefully.
 * In case of both an audio and a video stream, an end-packet for both audio and video needs
 * to be received (if set).
 **/

/* MediaProcessorFactory */
REGISTER_CLASSES( Sink, "sink", 1 );

Sink::Sink( const string& sName, ProcessorManager* pScope )
	: MediaProcessor(sName, pScope), bVideo(false), bAudio(false), iCount(0)
{
	mBool["video-mode"] = true;
	mBool["audio-mode"] = false;
	mInt["count"] = 0;
}

void Sink::receive_end( MediaPacketPtr& pPckt )
{
	/* Did we got both video and audio? */
	if( pPckt->content == content_t::video or not mBool["video-mode"] )
		bVideo = true;
	if( pPckt->content == content_t::audio or not mBool["audio-mode"] )
		bAudio = true;
	if( pPckt->content == content_t::mixed )
	{
		bAudio = true;
		bVideo = true;
	}
	/* The end? */
	if( bAudio and bVideo and ( not mInt["count"] or ++iCount >= mInt["count"] ) )
	{
		/* Terminate */
		debug( 1, "terminating" );
		pScope->stopProcessors();
	}
	else
		debug( 1, "NOT terminating" );
	route( pPckt );
}

/** TIMEOUT
 * @component time-out
 * @type system
 * @param time-out, int, 1000, in ms, the maximum interval in which no packets are received
 * @param kill, bool, true, if true, terminate the program, if false, generate an end-packet *
 * @info If this component does not receive any packet within a given interval after
 * the last packet, forcibly terminate the program or generate an end-packet.
 **/
REGISTER_CLASS( TimeOut, "time-out" );

TimeOut::TimeOut( const string& sName, ProcessorManager* pScope )
	: MediaProcessor(sName, pScope), iTime(0), iStep(0)
{
	mInt["time-out"] = 1000;
	mBool["kill"] = true;
}

void TimeOut::init( void )
{
	/* Base class */
	MediaProcessor::init();

	/* Log activity */
	iStep = mInt["time-out"];
	iTime = SirannonTime::getCurrentTime() + iStep;

	/* Activate */
	bSchedule = true;
}

void TimeOut::process( void )
{
	if( SirannonTime::getCurrentTime() > iTime )
	{
		if( mBool["kill"] )
			RuntimeError( this, "Time-out of %d ms exceeded", iStep.convertMsecs() );
		else
		{
			/* Generate an end packet ourselves */
			MediaPacketPtr pPckt ( new MediaPacket( packet_t::end, content_t::mixed, 0 ) );
			route( pPckt );
			bSchedule = false; /* stop */
		}
	}
}

void TimeOut::receive( MediaPacketPtr& pPckt )
{
	/* Log activity */
	iTime = SirannonTime::getCurrentTime() + iStep;

	/* Dont touch */
	route( pPckt );
}

REGISTER_CLASS( discard, "discard" );

/**
 * DISCARD
 * @component discard
 * @type system
 * @info Any received packet will be deleted.
 **/
discard::discard( const string& sName, ProcessorManager* pScope )
	: MediaProcessor(sName, pScope)
{ }
