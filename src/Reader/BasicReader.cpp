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
#include "BasicReader.h"
#include "Utils.h"

/**
 * BASIC READER
 * @component basic-reader
 * @type reader
 * @param chunk-size, int, 64000, in bytes, the maximum size of a chunk
 * @param length, int, 0, in ms, if > 0, provides the reader with the duration of the file so it can guess the DTS
 * @info Reads in any container in large chunks. Its main use is the HTTP transmission
 * in chunks of a container.
 **/

REGISTER_CLASS( BasicReader, "basic-reader" );

BasicReader::BasicReader( const string& sName, ProcessorManager* pScope )
	: Reader(sName, pScope), iUnit(0), iMaxBuffer(64*KIBI-1), pDesc(NULL), iDuration(0)
{
	mInt["chunk-size"] = 64*KIBI-1;
	mInt["length"] = 0;
}

BasicReader::~BasicReader()
{ }

void BasicReader::init( void )
{
	iMaxBuffer = MAX( mInt["chunk-size"], 1 );
	iDuration = mInt["length"] * 90;
	Reader::init();

	/* File size */
	fseek( oFile, 0L, SEEK_END );
	oContainer.bytesize = ftell( oFile );
	fseek( oFile, 0L, SEEK_SET );

	/* One medium */
	pDesc = addMedia();
	pDesc->route = 100;
}

bool BasicReader::doStep( void )
{
	/* Create a new packet */
	MediaPacketPtr pPckt( new MediaPacket( iMaxBuffer ) );
	pPckt->unitnumber = iUnit++;
	pPckt->desc = pDesc;
	pPckt->xroute = pDesc->route;
	if( iDuration > 0 )
		pPckt->dts = (uint64_t) ftell( oFile ) * iDuration / oContainer.bytesize ;

	/* Fill our pBuffer */
	int iBuffer = fread( pPckt->data(), sizeof(uint8_t), iMaxBuffer, oFile );
	if( iBuffer > 0 )
	{
		pPckt->push_back( iBuffer );

		/* Send packet */
		debug( 1, "parsed %s", pPckt->c_str() );
		route( pPckt );
	}
	/* Verify if the pBuffer is filled correctly */
	if( feof( oFile ) )
	{
		closeBuffer();

		if( mInt["loop"] < 0 or iLoop++ < mInt["loop"] )
		{
			openBuffer();
			createEnd( packet_t::reset );
		}
		else
		{
			bSchedule = false;
			createEnd( packet_t::end );
		}
		return false;
	}
	else if( ferror( oFile ) )
		RuntimeError( this, "Couldn't read from file(%s)", mString["filename"].c_str() );

	return true;
}

void BasicReader::createEnd( packet_t::type iType )
{
	MediaPacketPtr pPckt( new MediaPacket( 0 ) );
	pPckt->type = iType;
	pPckt->content = content_t::mixed;
	pPckt->unitnumber = iUnit++;
	pPckt->xroute = pDesc->route;
	if( iDuration > 0 )
		pPckt->dts = iDuration;
	debug( 1, "parsed %s", pPckt->c_str() );
	route( pPckt );
}
