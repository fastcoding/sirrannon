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
#ifndef STAT_H
#define STAT_H
#include "sirannon.h"
#include "Utils.h"

class streamInfo_t
{
public:
	streamInfo_t( void )
		: iSizeCount(0), iRateCount(0), iDelayCount(0), iRateSize(0), iUnit(-1), iLost(0),
		  fMinSize(1.0e+30), fMaxSize(0.), fMeanSize(0.), fSqrSize(0.),
		  fMinRate(1.0e+30), fMaxRate(0.), fMeanRate(0.), fSqrRate(0.),
		  fMinDelay(1.0e+30), fMaxDelay(0.), fMeanDelay(0.), fSqrDelay(0.),
		  fPrev(0.), pDesc(NULL)
	{ }

	int64_t iSizeCount, iRateCount, iDelayCount, iRateSize, iUnit, iLost;
	double fMinSize, fMaxSize, fMeanSize, fSqrSize;
	double fMinRate, fMaxRate, fMeanRate, fSqrRate;
	double fMinDelay, fMaxDelay, fMeanDelay, fSqrDelay;
	double fPrev;
	MediaDescriptor* pDesc;
};

class Statistics : public MediaProcessor
{
	protected:
	int iInterval, iUnit, iLost, iFirstUnit, iOverhead;
	double fCurr, fLast, fStart, fDelay, fPrev;
	fileLog oLog;
	map<int,streamInfo_t> oMatrix;
	string sLabel;

	void receive_end( MediaPacketPtr& pckt );
	void receive_reset( MediaPacketPtr& pckt );
	void receive( MediaPacketPtr& pckt );
	void log( MediaPacket* pckt );
	void show( void );

	public:
	Statistics( const string& sName, ProcessorManager* pScope );
	void init( void );
};
#endif
