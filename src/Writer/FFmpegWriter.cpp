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
#include "FFmpegWriter.h"
#include "Frame.h"
#include "h264_avc.h"

/*
 * FFMPEG WRITER
 * @component ffmpeg-writer
 * @properties scheduled, buffered
 * @type writer
 * @param filename, string, , the path of the file where to write to
 * @param initial-delay, int, 0, in ms, the minimal delay between the first received packet and first multiplexed packet
 * @param delay, int, 1000, in ms, the minimal amount of data present for each stream of the multiplex before the next packet is written
 * @param streams, int, -1, the number of different streams required before multiplexing, -1 omits this requirement
 * @param format, string, , if defined, overrules the format determined by the extension in the filename
 * @param fragmented, bool, false, if true, after each reset, close the container and open a new container with a name of the form e.g. demo-0.avi, demo-1.avi, demo-2.avi, etc.
 * @info Joins packets from different source into a container format supported by
 * FFMPEG (eg. FLV, WEBM, MP4). The container is written to a file.
 **/
REGISTER_CLASS( FFmpegWriter, "ffmpeg-writer" );

FFmpegWriter::FFmpegWriter( const string& sName, ProcessorManager* pScope )
: FFmpegMultiplexer(sName, pScope), iVersion(0)
{
	mString["filename"] = "test.mov";
	mString["format"] = "";
}

void FFmpegWriter::init( void )
{
	FFmpegMultiplexer::init();

	oVersionedFilename = createVersionTemplate( mString["filename"].c_str() );
}

void FFmpegWriter::setFormat( void )
{
	if( mString["format"].length() )
		pFmt = av_guess_format( mString["format"].c_str(), NULL, NULL );
	if( not pFmt )
		pFmt = av_guess_format( NULL, mString["filename"].c_str(), NULL );
	if( not pFmt )
	   RuntimeError( this, "Could not find suitable output format" );
}

void FFmpegWriter::openBuffer( void )
{
	/* The file name */
	if( not bFragmented )
		snprintf( pCtx->filename, sizeof(pCtx->filename), "%s", mString["filename"].c_str() );
	else
		snprintf( pCtx->filename, sizeof(pCtx->filename), oVersionedFilename.native_file_string().c_str(), iVersion++ );

	/* Open the file */
	if( url_fopen( &pCtx->pb, pCtx->filename, URL_WRONLY ) < 0 )
		RuntimeError( this, "Could not open (%s)", pCtx->filename );
}
