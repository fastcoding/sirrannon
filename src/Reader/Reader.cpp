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
#include "Reader.h"
#include "MemoryManager.h"
#include "SirannonPrint.h"

/**
 * READER
 * @component reader
 * @properties abstract, scheduled
 * @type core
 * @param filename, string, , the path of the file to open
 * @param loop, int, 1, the number of times to play the video, -1 being infinite, 0 interpreted as 1
 * @param video-route, int, 100, the xroute that will be assigned to packets containing video
 * @param audio-route, int, 200, the xroute that will be assigned to packets containing audio
 * @param dts-start, int, 1000, in ms, the timestamp that will be added to the DTS & PTS of each frame
 * @info Readers form the primary source of data in the sirannon. When the
 * file reaches its end, a reader either closes the file, generating an end-packet or
 * loops creating a reset-packet. A reader checks if no buffers downstream
 * (typically a scheduler buffer) are full before processing the next frame.
 **/
Reader::Reader( const string& sName, ProcessorManager* pScope )
	: MediaProcessor(sName, pScope), oFile(NULL), oStepStart(0), iLoop(1)
{
	mInt["loop"] = 1;
	mString["filename"] = "";
	mInt["video-route"] = 0;
}

Reader::~Reader()
{
	if( oFile )
		fclose( oFile );
}

void Reader::findSchedulers( void ) synchronized
{
	vector<MediaProcessor*> vProcs;
	getDownstream( vProcs );
	for( int i = 0; i < vProcs.size(); ++i )
		vSchedulers.push_back( dynamic_cast<RateController*>(vProcs[i]) );
} end_synchronized

void Reader::openBuffer( void )
{
	/* Open the videostream */
	oFile = fopen( mString["filename"].c_str(), "rb" );

	/* Check for error while reading the stream */
	if( oFile == NULL )
		RuntimeError( this, "couldn't open file \"%s\"", mString["filename"].c_str() );
	debug( 1, "opened file %s", mString["filename"].c_str() );
}

void Reader::closeBuffer( void )
{
	debug( 1, "closed file %s", mString["filename"].c_str() );
	fclose( oFile );
	oFile = NULL;
}

void Reader::init( void )
{
	/* Base class */
	MediaProcessor::init();

	/* Buffer scheduling */
	findSchedulers();

	/* Activate */
	bSchedule = true;

	/* Open our file */
	openBuffer();
}

void Reader::process( void )
{
	oStepStart = SirannonTime::getCurrentTime();

	/* Continue loop until the buffer is full or our quantum has expired.
	 * When in an active loop do just 1 step.  */
	while( bSchedule )
	{
		/* Do not produce more than the scheduler permits */
		if( checkBufferFull() )
		{
			return;
		}

		for( int i = 0; i < 10; ++ i )
		{
			if( not doStep() )
			{
				return;
			}

			/* Time slot */
			if( oStepStart.checkInterval( pScope->getQuantum() ) )
			{
				//debug( 1, "quantum elapsed: %d %lld", pScope->getQuantum(), oStepStart.convertMsecs() ) ;
				return;
			}
		}
	}
}

inline bool Reader::checkBufferFull( void )
{
	if( vSchedulers.empty() )
		return false;

	for( uint32_t i = 0; i < vSchedulers.size(); ++i )
	{
		if( vSchedulers[i]->bufferFull() )
		{
			MediaProcessor* pProc = dynamic_cast<MediaProcessor*>( vSchedulers[i] );
			//SirannonPrint( 2, "%s was full", pProc->c_str() );
			return true;
		}
	}
	//SirannonPrint( 2, "going ahead" );
	return false;
}
