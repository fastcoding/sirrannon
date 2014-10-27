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
#include "Proxy.h"

REGISTER_CLASS( Proxy, "proxy" );

Proxy::Proxy( const string& sName, ProcessorManager* pScope )
	: Block(sName, pScope)
{
	mInt["base-port"] = 5000;
	mInt["streams"] = 1;
	mString["receiver"] = "rtp";
	mString["unPacketizer"] = "xstream";
	mInt["delay"] = 1000;
	mInt["initial-delay"] = 0;
}

void Proxy::init( void )
{
	/* Base class, skip Block base class */
	MediaProcessor::init();

	/* Check parameters */
	char sTransType [256] = "";
	char sTransName [256] = "";
	char sUnpackType [256] = "";
	char sUnpackName [256] = "";
	snprintf( sTransType, sizeof(sTransType), "%s-receiver", mString["receiver"].c_str() );
	if( mString["unPacketizer"].size() )
		snprintf( sUnpackType, sizeof(sUnpackType), "%s-unPacketizer", mString["unPacketizer"].c_str() );

	int iStreams = mInt["streams"];
	if( iStreams <= 0 )
		RuntimeError( this, "invalid number of streams: %d", iStreams );

	/* In & out */
	in = oProcessorManager.createProcessor( "in", "in" );
	out = oProcessorManager.createProcessor( "out", "out" );

	/* Multiplexer */
	oProcessorManager.createProcessor( "std-multiplexer", "Multiplexer" );
	oProcessorManager.setInt( "Multiplexer", "delay", mInt["delay"] );
	oProcessorManager.setInt( "Multiplexer", "intial-delay", mInt["intial-delay"] );
	oProcessorManager.setInt( "Multiplexer", "streams", iStreams );
	oProcessorManager.setBool( "Multiplexer", "debug", mBool["debug"] );
	oProcessorManager.setBool( "Multiplexer", "thread", mBool["thread"] );
	oProcessorManager.setRoute( "Multiplexer", "out", 0 );

	/* Create the iStreams number of receivers */
	for( int i = 0; i < iStreams; i++ )
	{
		/* Names */
		snprintf( sTransName, sizeof(sTransName), "receiver-%02d", i );

		/* Create a receiver */
		oProcessorManager.createProcessor( sTransType, sTransName );
		oProcessorManager.setInt( sTransName, "port", mInt["base-port"] + i * 2 );
		oProcessorManager.setBool( sTransName, "extension", true );
		oProcessorManager.setBool( sTransName, "debug", false );

		/* Create an unPacketizer */
		if( sUnpackType[0] )
		{
			snprintf( sUnpackName, sizeof(sUnpackName), "unPacketizer-%02d", i );
			oProcessorManager.createProcessor( sUnpackType, sUnpackName );
			oProcessorManager.setBool( sUnpackName, "debug", false );
			oProcessorManager.setRoute( sTransName, sUnpackName, 0 );
			oProcessorManager.setRoute( sUnpackName, "Multiplexer", 0 );
		}
		else
		{
			/* Connect */
			oProcessorManager.setRoute( sTransName, "Multiplexer", 0 );
		}
	}
	/* Init */
	oProcessorManager.initProcessors();
	initOut();

	/* We always schedule */
	bSchedule = true;
}
