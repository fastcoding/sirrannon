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
#include "Packetizer.h"
#include "Frame.h"

/**
 * PACKETIZER
 * @component packetizer
 * @properties abstract
 * @type core
 * @param tracefile, string, , if defined, the path of trace where to log information about the split packets
 * @param trace-pts, bool, false, if true, write the PTS instead of the DTS in the log file
 **/

Packetizer::Packetizer( const string& sName, ProcessorManager* pScope )
	: MediaProcessor(sName,pScope), bTracePTS(false), iUnit(0), iMTU(1500)
{
	mString["tracefile"] = "";
	mBool["trace-pts"] = false;
	mInt["mtu"] = 1500;
}

void Packetizer::init( void )
{
	/* Base class */
	MediaProcessor::init();

	/* Open the tracefile if specified */
	if( mString["tracefile"].size() )
	{
		oTrace.open( mString["tracefile"].c_str(), "w" );
		if( not oTrace.active() )
			RuntimeError( this, "while trying to open %s", mString["tracefile"].c_str() );
	}
	bTracePTS = mBool["trace-pts"];
	iMTU = mInt["mtu"] - 20 /*IP*/ - 20 /*TCP/UDP*/ - 18 /*LTP*/ - 12 /*RTP*/;
}

void Packetizer::trace( MediaPacketPtr& pckt, int iParts )
{
	if( oTrace.active() )
	{
		timestamp_t iTime = bTracePTS ? pckt->pts : pckt->dts;
		oTrace.write( "%d\t%c\t%d\t%d\t%f\n", pckt->framenumber, FrameToChar(pckt->frame), pckt->size(), iParts, iTime * 1. / 90000 );
	}
}

void Packetizer::send( MediaPacketPtr& pPckt )
{
	pPckt->unitnumber = iUnit++;
	pPckt->mux = mux_t::RTP;
	debug( 1, "packed(%s)", pPckt->c_str() );
	route( pPckt );
}

void Packetizer::receive_end( MediaPacketPtr& pPckt )
{
	receive_reset( pPckt );
}

void Packetizer::receive_reset( MediaPacketPtr& pPckt )
{
	/* Send the current aggregate away */
	pack();

	/* Directly send the end/reset packet */
	send( pPckt );
}

void Packetizer::pack( void )
{ }
