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
#include "Logger.h"

REGISTER_CLASS( Logger, "logger" );

Logger::Logger( const string& sName, ProcessorManager* pScope )
	: MediaProcessor(sName, pScope), oStart(0), bReal(false),
	iFirstTime(-1), iLastTime(-1), oStart2(0), iTotalSize(0), bEnd(false)
{
	mString["log-graph"] = "";
	mString["log-timing"] = "";
	mBool["relative"] = true;
	mString["tag"] = "test";
}

Logger::~Logger( )
{
}

void Logger::init( void )
{
	/* Base class */
	MediaProcessor::init();

	/* Open the log */
	oLog.open( mString["log-graph"].c_str(), "w" );

	/* Log start time */
	oStart = SirannonTime::getCurrentTime();
	bReal = not mBool["relative"];

	/* Plot a start point */
	log( 0 );
}

void Logger::log( int iTime )
{
	/* Difference from start */
	if( oLog.active() )
	{
		if( bReal )
		{
			/* Log the current time and the packet timestamp */
			double fCurr = SirannonTime::getCurrentTime().convertDouble() * 1000 * 1000;
			oLog.write( "%f,%d\n", fCurr, iTime );
		}
		else
		{
			/* Log the relative time and the packet timestamp */
			int iDiff = ( SirannonTime::getCurrentTime() - oStart ).convertUsecs();
			oLog.write( "%d,%d\n", iDiff, iTime );
		}
	}
}

void Logger::receive( MediaPacketPtr& pckt )
{
	/* Log */
	log( pckt->dts / 90 );

	/* Dts */
	if( iFirstTime < 0 )
	{
		iFirstTime = pckt->dts / 90;
		oStart2 = SirannonTime::getCurrentTime();
	}
	/* Total size */
	iTotalSize += pckt->size();

	/* Done */
	route( pckt );
}

void Logger::receive_end( MediaPacketPtr& pckt )
{
	/* Process only one end */
	if( bEnd )
		return route( pckt );

	/* Last time */
	iLastTime = pckt->dts / 90;

	/* Plot an end point */
	log( 0 );

	/* Difference */
	int iTimeDiff = (SirannonTime::getCurrentTime() - oStart).convertMsecs();
	int iTimeStampDiff = iLastTime - iFirstTime;
	int iStartDelay = (oStart2 - oStart).convertMsecs();
	double fBitrate = iTotalSize * 8. / 1000. / iTimeDiff;
	SirannonWarning( this,  "%s: %d / ( %d + %d = %d ) = %f @ %f Mbps",
			mString["tag"].c_str(), iTimeStampDiff,
			iStartDelay, iTimeDiff - iStartDelay, iTimeDiff,
			iTimeStampDiff * 1. / iTimeDiff, fBitrate );

	/* Log */
	fileLog oLog2;
	if( not oLog2.open( mString["log-timing"].c_str(), "a" ) )
	{
		oLog2.write( "%s: %d / ( %d + %d = %d ) = %f @ %f Mbps\n",
				mString["tag"].c_str(), iTimeStampDiff,
				iStartDelay, iTimeDiff - iStartDelay, iTimeDiff,
				iTimeStampDiff * 1. / iTimeDiff, fBitrate );
		oLog2.close();
	}
	/* Done */
	bEnd = true;
	route( pckt );
}

void Logger::receive_reset( MediaPacketPtr& pckt )
{
	receive( pckt );
}
