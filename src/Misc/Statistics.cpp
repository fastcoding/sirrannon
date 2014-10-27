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
#include "Statistics.h"
#include "SirannonTime.h"
#include "OSSupport.h"
#include <math.h>

/** MISC
 * @component miscellaneous
 * @properties abstract
 * @type core
 **/

/** STATISTICS
 * @component statistics
 * @type miscellaneous
 * @param interval, int, 5000, in ms, the amount of time between two reports, -1 disables the reports
 * @param log, string, , if defined, the path where to log information about the passing packets
 * @param label, string, , if defined, label used for each entry in the log, this is needed when sending multiple statistics output to one single file
 * @param append, bool, true, if true, append data to the log, if false, overwrite the log
 * @param overhead, int, 0, the amount of header overhead from network headers to add to the packet size
 * @param draw, bool, false, if true, when an end-packet is received, draw a graph of the bandwidth, requires that \textit{log} is defined
 * @info This component generates at regular intervals information about the passing stream
 **/
REGISTER_CLASS( Statistics, "statistics");

Statistics::Statistics( const string& sName, ProcessorManager* pScope )
  : MediaProcessor(sName, pScope), iInterval(5000), iUnit(-1), iLost(0), iFirstUnit(-1),
    fCurr(-1), fLast(-1), fStart(-1), fDelay(-1), fPrev(0), iOverhead(0)
{
	mInt["interval"] = 5000;
	mString["log"] = "";
	mString["label"] = "";
	mBool["draw"] = false;
	mBool["append"] = true;
	mInt["overhead"] = 0;
}

void Statistics::init( void )
{
	/* Base class */
	MediaProcessor::init();

	iInterval = mInt["interval"];
	sLabel = mString["label"];
	iOverhead = mInt["overhead"];
	/* Open the log */
	if( mString["log"].size() )
	{
		if( mBool["append"] )
			oLog.open( mString["log"].c_str(), "a" );
		else
			oLog.open( mString["log"].c_str(), "w" );
		if( not oLog.active() )
			RuntimeError( this, "while opening file %s", mString["log"].c_str() );
	}
}

void Statistics::log( MediaPacket* pckt )
{
	/* Relative */
	if( fStart < 0 )
	{
		SirannonTime oCurrent = SirannonTime::getCurrentTime();
		fStart = oCurrent.convertDouble() * 1000.;
		fLast = 0;
	}
	/* Store information */
	streamInfo_t* pMatrix = &oMatrix[pckt->xstream];

	/* Check unit number */
	if( pMatrix->iUnit >= 0 and  pckt->unitnumber != pMatrix->iUnit + 1 )
	{
		/* We lost some packets */
		pMatrix->iLost += pckt->unitnumber - ( pMatrix->iUnit + 1 );
	}
	pMatrix->iUnit = pckt->unitnumber;

	/* Don't log reset / end packets */
	if( pckt->type != packet_t::media )
		return;

	/* Timing info */
	fCurr = SirannonTime::getCurrentTime().convertDouble() * 1000. - fStart;

	/* Size running average */
	pMatrix->pDesc = pckt->desc;
	pMatrix->iSizeCount++;
	pMatrix->fMinSize = MIN( pckt->size(), pMatrix->fMinSize );
	pMatrix->fMaxSize = MAX( pckt->size(), pMatrix->fMaxSize );
	pMatrix->fMeanSize = ( ( pMatrix->iSizeCount - 1 ) * pMatrix->fMeanSize + pckt->size() ) / pMatrix->iSizeCount;
	pMatrix->fSqrSize = ( ( pMatrix->iSizeCount - 1 ) * pMatrix->fSqrSize + pckt->size() * pckt->size() ) / pMatrix->iSizeCount;

	/* Bandwidth running average */
	pMatrix->iRateSize += pckt->size() + iOverhead;
	if( fCurr - pMatrix->fPrev >= 100.0 )
	{
		pMatrix->iRateCount++;
		double fRate = pMatrix->iRateSize * 8. / ( fCurr - pMatrix->fPrev );
		pMatrix->fPrev = fCurr;
		pMatrix->iRateSize = 0;
		pMatrix->fMinRate = MIN( fRate, pMatrix->fMinRate );
		pMatrix->fMaxRate = MAX( fRate, pMatrix->fMaxRate );
		pMatrix->fMeanRate = ( ( pMatrix->iRateCount - 1 ) * pMatrix->fMeanRate + fRate ) / pMatrix->iRateCount;
		pMatrix->fSqrRate = ( ( pMatrix->iRateCount - 1 ) * pMatrix->fSqrRate + fRate * fRate ) / pMatrix->iRateCount;
	}
	/* Log line to file */
	if( oLog.active() )
		oLog.write( "%s,%d,%d,%d\n",
			sLabel.c_str(),	(int)fCurr, pckt->size(), pckt->xroute%100 );

	/* Show after x ms */
	if( iInterval >= 0 and fCurr - fLast >= iInterval )
	{
		show();
		fLast = fCurr;
	}
}

void Statistics::show( void )
{
	/* Gather super aggregates */
	print( 0, "Statistics: time(%d ms)", (int)fCurr );
	int64_t iCount = 0, iLost = 0;
	double fMeanSize = 0., fMeanRate = 0., fTotalSize = 0., fMeanDelay = 0.;
	for( map<int,streamInfo_t>::iterator i = oMatrix.begin(); i != oMatrix.end(); i++ )
	{
		streamInfo_t* pStream = &i->second;
		fMeanRate += pStream->fMeanRate;
		fTotalSize += pStream->iSizeCount * pStream->fMeanSize;
		iCount += pStream->iSizeCount;
		iLost += pStream->iLost;
	}
	fMeanSize = fTotalSize / iCount;

	/* Handle each stream */
	int iStream = 0;
	for( map<int,streamInfo_t>::iterator i = oMatrix.begin(); i != oMatrix.end(); i++ )
	{
		streamInfo_t* pStream = &i->second;

		/* Now calculate the sigmas */
		double fSizeSigma = sqrt( MAX( pStream->fSqrSize - pStream->fMeanSize * pStream->fMeanSize, 0. ) );
		double fRateSigma = sqrt( MAX( pStream->fSqrRate - pStream->fMeanRate * pStream->fMeanRate, 0. ) );
		int64_t iTotal = pStream->iSizeCount + pStream->iLost;

		/* Avoid 0 devision */
		if( iTotal <= 0 )
			iTotal = 1;

		/* Print stream info */
		print( 0, "stream (%d):", ++iStream );
		print( 0, "\tpackets: total(%"LL"d) recv(%"LL"d) lost(%"LL"d) loss(%1.2E)", iTotal, pStream->iSizeCount, pStream->iLost, pStream->iLost * 1. / iTotal );
		print( 0, "\tpacket size: mean(%04d B)     min(%04d B)     max(%04d B)     sigma(%04d B)     total(%06d kB)",
				(int)pStream->fMeanSize, (int)pStream->fMinSize, (int)pStream->fMaxSize,
				(int) fSizeSigma, (int)(pStream->iSizeCount*pStream->fMeanSize/1024) );
		print( 0, "\tbit rate:    mean(%05d kb/s) min(%05d kb/s) max(%05d kb/s) sigma(%05d kb/s)",
				(int)pStream->fMeanRate, (int)pStream->fMinRate, (int)pStream->fMaxRate,
				(int) fRateSigma );
	}
	/* Print total info */
	if( oMatrix.size() > 1 )
	{
		print( 0, "total:" );
		print( 0, "\tpackets: total(%"LL"d) recv(%"LL"d) lost(%"LL"d) prob(%1.2E)", iLost+iCount, iCount, iLost, iLost*1./(iLost+iCount) );
		print( 0, "\tsize: total(%d kB) mean(%d B)", int(fTotalSize/1000.), int(fMeanSize) );
		print( 0, "\trate: mean(%d kbps)", int(fMeanRate) );
	}
}

void Statistics::receive_end( MediaPacketPtr& pckt )
{
	/* Show Statistics */
	show();
	route( pckt );

	/* Close the file */
	oLog.close();

	/* Launch the UI */
	if( mBool["draw"] and mString["log"].size() )
	{
		string sCmd = "python python/stats.py " + mString["log"];
		if( system( sCmd.c_str() ) < 0 )
			SirannonWarning( this,  "could not draw Statistics" );
	}
}

void Statistics::receive_reset( MediaPacketPtr& pckt )
{
	receive( pckt );
}

void Statistics::receive( MediaPacketPtr& pckt )
{
	log( pckt.get() );
	route(pckt);
}
