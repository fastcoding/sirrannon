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
#include "Writer.h"
#include "OSSupport.h"

/**
 * WRITER
 * @component writer
 * @properties abstract
 * @type core
 **/

/**
 * BASIC WRITER
 * @component basic-writer
 * @type writer
 * @param filename, string, , the path of the file where to write to
 * @param flush, bool, false, if true, flush the IO buffer after each write
 * @param fragmented, bool, false, if true, after each reset, close the container and open a new container with a name of the form e.g. demo-0.avi, demo-1.avi, demo-2.avi, etc.
 * @param complete, bool, false, if true, if the file is not closed when entering the destructor, delete the file
 * @info Writes the content of each received packet to a file and deletes the packet.
 * When an end-packet is received, the file is closed.
 **/

REGISTER_CLASSES( Writer, "writer", 1 );
REGISTER_CLASSES( Writer, "basic-writer", 2 );

Writer::Writer( const string& sName, ProcessorManager* pScope )
	: MediaProcessor(sName, pScope), oFile(NULL), bFragmented(false), iVersion(0)
{
	mString["filename"] = "";
	mBool["flush"] = false;
	mBool["fragmented"] = false;
	mBool["complete"] = false;
}

Writer::~Writer()
{
	if( oFile )
	{
		close();
		if( mBool["complete"] )
			if( remove( mString["filename"].c_str() ) < 0 )
				IOError( "Could not delete file(%s): %s", mString["filename"].c_str(), strError() );
	}
}

void Writer::init( void )
{
	/* Base class */
	MediaProcessor::init();

	/* Vars */
	bFragmented = mBool["fragmented"];
	bFlush = mBool["flush"];
	if( bFragmented )
		oVersionedFilename = createVersionTemplate( mString["filename"].c_str() );
}

void Writer::open( void )
{
	/* The file name */
	if( not bFragmented )
		snprintf( sFile, sizeof(sFile), "%s", mString["filename"].c_str() );
	else
		snprintf( sFile, sizeof(sFile), oVersionedFilename.native_file_string().c_str(), iVersion++ );

	/* Open the file */
	oFile = fopen( sFile, "wb" );

	/* Check */
	if( oFile == NULL )
		IOError( this, "Could not open file(%s)", sFile );
	debug( 1, "opened stream(%s)", sFile );
}

void Writer::close( void )
{
	if( oFile )
		fclose( oFile );
	oFile = NULL;
	debug( 1, "closed stream(%s)", sFile );
}

void Writer::receive( MediaPacketPtr& pckt )
{
	/* Open when needed */
	if( not oFile )
		open();

	/* Write the data from the packt away */
	debug( 2, "wrote: %s", pckt->c_str() );
	fwrite( (char*) pckt->data(), 1, pckt->size(), oFile );
	if( ferror( oFile ) )
		IOError( this, "Could not write to file(%s)", sFile );

	/* Flush if needed */
	if( bFlush )
		fflush( oFile );

	/* Still pass the object */
	route( pckt );
}

void Writer::receive_reset( MediaPacketPtr& pckt )
{
	/* Write the data from the packt away */
	if( bFragmented )
		close();

	/* Still pass the object */
	route( pckt );
}

void Writer::receive_end( MediaPacketPtr& pckt )
{
	/* Close the file */
	close();

	/* Still pass the object */
	route( pckt );
}
