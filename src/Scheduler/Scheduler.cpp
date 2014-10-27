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
#include "Scheduler.h"
#include "Frame.h" /* some handy functions */
#include "OSSupport.h"

/**
 * SCHEDULER
 * @component scheduler
 * @properties abstract, buffered, scheduled
 * @type core
 * @param delay, int, 100, in ms, the scheduler starts working delay ms after the first packet
 * @param buffer, int, 1000, in ms, the size in time of the buffer
 * @param absolute-delay, int, 0, in ms, start scheduling absolule-delay ms after the start of Sirannon, 0 disables this
 * @param pause, bool, false, if true, pause scheduling until the function play is called
 * @param speed, double, 1.0, the factor at which to schedule slower or faster than real-time
 * @param precise, bool, false, if true, run the scheduler in a seperate thread with a quantum of 1 ms for precise timings
 * @info Schedulers introduce real-time behavior to the stream (readers generate packets
 * at an arbitrary speed). It also introduces correct real-time behavior to streams
 * coming from several receivers. It buffers all incoming packets and releases them at
 * the rate set by the packet's decoding time stamp (DTS). Some schedulers introduce
 * shifts to these time stamps order to obtain for example a smoother bandwidth usage.
 **/

REGISTER_CLASS( basic_Scheduler, "basic-scheduler" );
REGISTER_CLASS( frame_Scheduler, "frame-scheduler" );
REGISTER_CLASS( gop_Scheduler, "gop-scheduler" );
REGISTER_CLASS( svc_Scheduler, "svc-scheduler" );
REGISTER_CLASS( window_Scheduler, "window-scheduler" );
REGISTER_CLASS( qmatch_Scheduler, "qmatch-scheduler" );

/* Constructor */
Scheduler::Scheduler( const string& sName, ProcessorManager* pScope )
  : MediaProcessor(sName, pScope),
    iDelta(0), bSending(false), bOverflow(false), bPause(false),
    iBase(-1), iMin(0), iLast(-1),
    iBuffer(0), iLost(0), oPrev(0), oMaxDiff(0),
    iUnderflow(-1), oStartDelay(0), oTimeOut(1000),
	fCurrent(0.0), fSpeed(-1.),
	oStarttime(0), oPause(0), oFixedStartTime(0), iError(0)
{
	mInt["delay"] = 100;
	mInt["buffer"] = 1000;
	mInt["absolute-delay"] = 0;
	mBool["pause"] = false;
	mDouble["speed"] = 1.0;
	mBool["precise"] = false;
}

Scheduler::~Scheduler( )
{
	while( not vBuffer.empty() )
	{
		delete vBuffer.front();
		vBuffer.pop();
	}
	while( not vSend.empty() )
	{
		delete vSend.front();
		vSend.pop_front();
	}
}

void Scheduler::init( void )
{
	/* Base class */
	MediaProcessor::init();

	if( not mInt.count( "buffer" ) ) // Legacy
		iBuffer = mInt["delay"];
	else
		iBuffer = mInt["buffer"];
	oStartDelay = mInt["delay"];
	bPause = mBool["pause"];
	oFixedStartTime = mInt["absolute-delay"];
	if( not oFixedStartTime.zero() )
	{
		oStarttime = oPause = SirannonTime::getCurrentTime();
		oStartDelay = oFixedStartTime - SirannonTime::getUpTime();
		oStarttime += oStartDelay;
	}
	if( mDouble["speed"] != 1.0 )
	{
		fSpeed = mDouble["speed"];
		iBuffer = (int) (iBuffer * fSpeed);
	}
	if( mBool["precise"] )
		forcePriviligedThread( 1*MEGA );
}

int Scheduler::getBufferDelta( void ) const
{
	FlowLock_t oLock( oFlowMutex );
	if( not vBuffer.empty() )
		return vBuffer.back()->dts - vBuffer.front()->dts;
	else
		return 0;
}

bool Scheduler::bufferFull( void ) const
{
	return getBufferDelta() > iBuffer * 90;
}

int Scheduler::flush( void ) synchronized
{
	/* Super */
	MediaProcessor::flush();

	/* Delete all media packets in the buffer */
	while( not vBuffer.empty() )
	{
		delete vBuffer.front();
		vBuffer.pop();
	}
	/* Delete the sendbuffer */
	while( not vSend.empty() )
	{
		delete vSend.front();
		vSend.pop_front();
	}
	/* Reset flags */
	bSending = false;
	iBase = -1;
	bSchedule = false;
	iMin = -1;
	iDelta = 0;
	bPause = false;
	oDrop = 0;
	iLost = 0;
	oPrev = 0;
	iLast = -1;
	iUnderflow = -1;
	fCurrent = 0.;
	fSpeed = -1.;
	oStarttime = SirannonTime( 0 );
	oFixedStartTime = SirannonTime( 0 );
	oStartDelay = mInt["delay"];
	return 0;
} end_synchronized

void Scheduler::receive( MediaPacketPtr& pPckt )
{
	/* Start our thread if needed */
	if( not bSchedule )
	{
		bSchedule = true;
		if( oFixedStartTime.zero() )
		{
			oStarttime = oPause = SirannonTime::getCurrentTime();
			oStarttime += oStartDelay;
		}
	}
	/* Basic dts */
	if( iBase < 0 )
		iBase = pPckt->dts;
	else if( pPckt->dts < iBase )
		iBase = pPckt->dts;

	/* Store */
	//debug( 3, "buffering %s", pPckt->c_str() );
	vBuffer.push( pPckt.release() );
	iDelta = getBufferDelta();
}

void Scheduler::receive_reset( MediaPacketPtr& pPckt )
{
	receive( pPckt );
}

void Scheduler::receive_end( MediaPacketPtr& pPckt )
{
	receive( pPckt );
}

void Scheduler::process( void )
{
	/* Pauzed? */
	if( bPause )
		return;

	/* Current time */
	SirannonTime oCurrent = SirannonTime::getCurrentTime();

	/* Wait at start point */
	if( oCurrent < oStarttime )
		return;

	/* Relative timing */
	oCurrent -= oStarttime;

	/* Speed scaling */
	if( fSpeed > 0.0 )
	{
		fCurrent += ( oCurrent - oPrev ).convertNsecs() * fSpeed;
		oPrev = oCurrent;
		oCurrent = SirannonTime::fromNSecs( fCurrent );
	}
	/* Keep repeating until sufficient data is processed */
	while( true )
	{
		/* makeSchedule the frames */
		if( not bSending )
		{
			if( vBuffer.empty() )
				return;

			/* Load in a series of packets depending on the specific type of Scheduler */
			makeSchedule();
			iDelta = getBufferDelta();

			/* Check if there is data in the sendbuffer */
			if( vSend.empty() )
				RuntimeError( this, "Sendbuffer underflow" );

			/* We are sending */
			bSending = true;
		}
		/* Send the scheduled frames */
		if( bSending )
		{
			while( not vSend.empty() )
			{
				/* Get the packet */
				MediaPacket* pFrontPckt = vSend.front();

				/* Time yet? */
				if( oCurrent < pFrontPckt->oSend )
					return;

				/* How accurate are we? */
				SirannonTime oDiff = oCurrent - pFrontPckt->oSend;
				if( pScope->getVerbose() > 0 and pFrontPckt->unitnumber % 1 == 0 )
					debug( 1, "schedule(%"LL"d ms) delay(%"LL"d ms) size-packets(%d+%d) size-ms(%d) full(%d): %s",
							pFrontPckt->oSend.convertMsecs(), oDiff.convertMsecs(),
							(int)vBuffer.size(), (int)vSend.size(),
							getBufferDelta() / 90, (int)bufferFull(), pFrontPckt->c_str() );

				/* Warning */
//				if( oDiff > oMaxDiff )
//					oMaxDiff = oDiff;
//				if( oDiff.convertMsecs() > 1000 )
//				{
//					iError++;
//					if( iError % 200000 == 1 )
//					{
//						debug( 1, "schedule(%"LL"d ns) delay(%"LL"d ns / %"LL"d ms) size-packets(%d+%d) size-ms(%d) full(%d)",
//												pFrontPckt->oSend.convertNsecs(), oDiff.convertNsecs(), oDiff.convertMsecs(),
//												(int)vBuffer.size(), (int)vSend.size(), getBufferDelta() / 90, (int)bufferFull() );
//						SirannonWarning( "System too slow or input feed too slow: delay(%"LL"d ms), message repeated %d times, buffer %d",
//								oDiff.convertMsecs(), iError, (int)vBuffer.size() );
//					}
//				}
//				else
//					iError = 0;

				/* Send our packet */
				MediaPacketPtr pPckt( pFrontPckt );
				vSend.pop_front();
				route( pPckt );

				/* Last packet of group? */
				if( vSend.empty() )
					bSending = false;
			}
		}
	}
}

int Scheduler::pause( void ) synchronized
{
	/* We pauze! */
	if( not bPause )
	{
		debug( 1,"pausing");
		bPause = true;

		/* Save time */
		oPause = SirannonTime::getCurrentTime();
	}
	return 0;
} end_synchronized

int Scheduler::play( double fFactor ) synchronized
{
	/* Log */
	if( fFactor != -1.0 or fSpeed > 0.0 )
	{
		/* Handle the case where in the midst of streaming a SPEED is added.
		 * This probably why some streamers cannot handle this, but we can :-) */
		if( not oStarttime.zero() and fSpeed <= 0.0 )
		{
			SirannonTime oCurrent = SirannonTime::getCurrentTime();
			if( oCurrent >= oStarttime )
			{
				oCurrent -= oStarttime;
				oPrev = oCurrent;
				fCurrent = oCurrent.convertNsecs();
			}
		}
		/* Set speed */
		fSpeed = fFactor;
	}
	/* Only play when paused */
	if( bPause )
	{
		/* Pause is over */
		bPause = false;

		/* Prevent to play before the first packet */
		if( oStarttime.convertMsecs() == 0 )
		{
			debug( 1, "play before first packet in buffer" );
			return 0;
		}
		/* Pause time */
		SirannonTime oCurrent = SirannonTime::getCurrentTime();
		oCurrent -= oPause;

		/* Initial pause? */
		if( oPause > oStarttime )
		{
			/* Standard pause */
			debug( 1, "paused duration (%"LL"d ms)", oCurrent.convertMsecs() );
			oStarttime += oCurrent;
		}
	}
	debug( 1, "play" );
	return 0;
} end_synchronized

inline SirannonTime Scheduler::getOffset( void ) const
{
	uint64_t iDelta = vSend.front()->dts - iBase;
	return SirannonTime::fromNSecs( iDelta * MEGA / 90 );
}

/** BASIC SCHEDULER
 * @component basic-scheduler
 * @type scheduler
 * @info Simplest form in which the packets are sent at the time solely indicated by the
 *  DTS. This leads to a burst of packets for each frame since all the packets have the
 *  same DTS.
 **/
void basic_Scheduler::makeSchedule( void )
{
	/* Basic makeSchedule in which we read one frame in */
	int32_t dts = -1;

	/* Select one frame */
	while( not vBuffer.empty() )
	{
		MediaPacket* pPckt = vBuffer.front();
		if( dts < 0 )
			dts = pPckt->dts;

		if( pPckt->dts == dts  )
		{
			vSend.push_back( pPckt );
			vBuffer.pop();
		}
		else
			break;
	}
	/* We send everything from one frame at once */
	SirannonTime oOffset = getOffset();
	for( deque_it i = vSend.begin(); i != vSend.end(); i++ )
		(*i)->oSend = oOffset;
}

/**
 * FRAME SCHEDULER
 * @component frame-scheduler
 * @type scheduler
 * @info The packets belonging to a frame are smoothed in time over the duration of the
 * frame (instead of sending an entire frame in one burst). Caveat, make sure the
 * delay is longer than the duration of one frame!
 **/

void frame_Scheduler::makeSchedule( void )
{
	/* We read an entire dts sequence and spread it over time */
	bool start = true;
	int frame = 0;

	/* Select one frame */
	int iTotal = 0;
	while( not vBuffer.empty() )
	{
		MediaPacket* pPckt = vBuffer.front();
		if( start )
		{
			frame = pPckt->framenumber;
			start = false;
		}
		if( pPckt->framenumber == frame )
		{
			vSend.push_back( pPckt );
			vBuffer.pop();
			iTotal += pPckt->size();
		}
		else
		{
			break;
		}
	}
	/* Number of packets in this frame */
	if( not iTotal )
		iTotal = 1;

	/* Time increment per packet */
	uint64_t iInc = (uint64_t)vSend.back()->inc * MEGA / 90;

  	/* Offset in seconds from the start */
	SirannonTime oOffsettime = getOffset();

	/* Iterate over the entire frame */
	int iSize = 0;
	for( deque_it i = vSend.begin(); i != vSend.end(); i++ )
	{
		MediaPacket* pPckt = (*i);
		pPckt->oSend = oOffsettime;
		pPckt->oSend += SirannonTime::fromNSecs( iInc * iSize / iTotal );
		iSize += pPckt->size();
	}
}

/**
 * GOP SCHEDULER
 * @component gop-scheduler
 * @type scheduler
 * @info The packets belonging to a GOP are smoothed in time over the entire duration
 * of that GOP. Caveat, make sure the delay is longer than the duration of one GOP!
 **/

void gop_Scheduler::makeSchedule( void )
{
	/* We read an entire GOP and spread it over time */
	int iTotal = 0, iFrame = -1;
	while( not vBuffer.empty() )
	{
		MediaPacket* pPckt = vBuffer.front();

		/* First frame number */
		if( iFrame < 0 )
			iFrame = pPckt->framenumber;

		/* Stop? */
		if( IsIFrame( pPckt ) and pPckt->framenumber != iFrame )
		{
			/* Stop at the next I frame */
			break;
		}
		else
		{
			/* Enlist this packet */
			vSend.push_back( pPckt );
			vBuffer.pop();
			iTotal += pPckt->size();
		}
	}
	/* Number of packets in this frame */
	if( iTotal == 0 )
		iTotal = 1;

	/* Time increment per packet */
	MediaPacket* last  = vSend.back();
	MediaPacket* first = vSend.front();
	int iInc = MAX( 1, ( last->dts + last->inc - first->dts ) / 90 );

  	/* Offset in seconds from the start */
	int iOffsettime = ( first->dts - iBase ) / 90;

	/* Iterate over the entire frame */
	int iSize = 0;
	for( deque_it i = vSend.begin(); i != vSend.end(); i++ )
	{
		MediaPacket* pPckt = (*i);
		pPckt->oSend = int( iOffsettime + (int64_t) iInc * iSize / iTotal );
		iSize += pPckt->size();
	}
}

/**
 * SVC SCHEDULER
 * @component svc-scheduler
 * @type scheduler
 * @info The packets belonging to one temporal pyramid are smoothed in time over the
 * entire duration of that pyramid. Caveat, make sure the delay is longer than
 * the duration of one pyramid!
 **/

svc_Scheduler::svc_Scheduler( const string& sName, ProcessorManager* pScope )
: Scheduler(sName, pScope)
{ }

void svc_Scheduler::makeSchedule( void )
{
	/* Collect a full GOP */
	int iFrame = -1;
	int iTotal = 0;
	while( not vBuffer.empty() )
	{
		MediaPacket* pPckt = vBuffer.front();

		/* Stop at the start of the next GOP */
		if( iFrame >= 0 and pPckt->framenumber > iFrame and pPckt->T == 0 )
			break;
		iFrame = pPckt->framenumber;

		/* Put the packet in another vBuffer */
		vSend.push_back( pPckt );
		vBuffer.pop();

		/* Collect size */
		iTotal += pPckt->size();
	}
	/* Sanity check */
	if( vSend.empty() )
		return;

	/* Time increment per packet */
	if( not iTotal )
		iTotal = 1;
	MediaPacket *last = vSend.back();
	MediaPacket *first = vSend.front();
	int iInc = MAX( 1, ( last->dts + last->inc - first->dts ) / 90 );
	int iOffsettime = ( first->dts - iBase ) / 90;

	/* Iterate over the entire frame */
	int iSize = 0;
	for( deque_it i = vSend.begin(); i != vSend.end(); i++ )
	{
		MediaPacket* pPckt = *i;
		pPckt->oSend = int( iOffsettime + (int64_t) iInc * iSize / iTotal );
		iSize += pPckt->size();
	}
}

/**
 * QMATCH SCHEDULER
 * @component qmatch-scheduler
 * @properties private
 * @type scheduler
 * @param interleave1, bool, false, ???
 * @param interleave2, bool, false, ???
 * @param interleave3, bool, false, ???
 **/

qmatch_Scheduler::qmatch_Scheduler( const string& sName, ProcessorManager* pScope )
:   svc_Scheduler(sName, pScope),
	bInterleave1(false), bInterleave2(false), bInterleave3(false)
{
	 mBool["interleave1"] = false;
	 mBool["interleave2"] = false;
	 mBool["interleave3"] = false;
}

void qmatch_Scheduler::init( void )
{
	/* Base class */
	svc_Scheduler::init();

	/* Store in variable (more efficient) */
	bInterleave1 = mBool["interleave1"];
	bInterleave2 = mBool["interleave2"];
	bInterleave3 = mBool["interleave3"];
}

bool qmatch_Scheduler::cmp_pts( const MediaPacket* p1, const MediaPacket* p2 )
{
	return p1->pts < p2->pts;
}

void qmatch_Scheduler::interleave( void )
{
	if( bInterleave1 )
		interleave1();
	else if( bInterleave2 )
		interleave2();
	else if( bInterleave3 )
		interleave3();
}

void qmatch_Scheduler::interleave1( void )
{
	/* Sort the vSend */
	sort( vSend.begin(), vSend.end(), cmp_pts );
}

void qmatch_Scheduler::interleave2( void )
{
	zip( vSend, 5, &classify_route );
}

void qmatch_Scheduler::interleave3( void )
{
	zip( vSend, 5, &classify_route, true );
}

uint32_t qmatch_Scheduler::classify_route( const MediaPacket* pPckt )
{
	return pPckt->xroute % 100;
}

uint32_t qmatch_Scheduler::classify_size( const MediaPacket* pPckt )
{
	return pPckt->size() / 200;
}

void qmatch_Scheduler::makeSchedule( void )
{
	MediaPacket* pPckt;

	/* Collect a full GOP */
	int iFrame = -1;
	int iTotal = 0;
	while( not vBuffer.empty() )
	{
		MediaPacket* pPckt = vBuffer.front();

		/* Stop at the start of the next GOP */
		if( iFrame >= 0 and pPckt->framenumber > iFrame and pPckt->T == 0 )
			break;
		iFrame = pPckt->framenumber;

		/* Put the packet in another vBuffer */
		vSend.push_back( pPckt );
		vBuffer.pop();

		/* Collect size */
		iTotal += pPckt->size();
	}
	/* Sanity check */
	if( vSend.empty() )
		return;

	/* Time increment per packet */
	if( not iTotal )
		iTotal = 1;
	MediaPacket *last = vSend.back();
	MediaPacket *first = vSend.front();
	int iInc = MAX( 1, ( last->dts + last->inc - first->dts ) / 90 );
	int iOffsettime = ( first->dts - iBase ) / 90;

	/* Reorder */
	interleave();

	/* Iterate over the entire frame */
	int iSize = 0;
	for( deque_it i = vSend.begin(); i != vSend.end(); i++ )
	{
		pPckt = *i;
		pPckt->oSend = int( iOffsettime + (int64_t) iInc * iSize / iTotal );
		iSize += pPckt->size();
	}
}

/**
 * WINDOW SCHEDULER
 * @component window-scheduler
 * @type scheduler
 * @param window, int, 900, in ms, the size of the fixed window
 * @info The packets are smoothed in time over a fixed non-sliding window.
 * Caveat, make sure the delay is longer than the duration of one window!
 **/

window_Scheduler::window_Scheduler( const string& sName, ProcessorManager* pScope )
	: Scheduler(sName, pScope), iWindow(900)
{
	mInt["window"] = 900;
}

void window_Scheduler::init( void )
{
	/* Base class */
	Scheduler::init();

	/* Quick params */
	iWindow = mInt["window"];
}

void window_Scheduler::makeSchedule( void )
{
	int iTotal = 0;
	timestamp_t iStart = vBuffer.front()->dts;

	/* Collect the number of packets for this window */
	while( not vBuffer.empty() )
	{
		MediaPacket* pPckt = vBuffer.front();

		/* Stop if we exceed the window */
		if( pPckt->dts - iStart >= iWindow * 90 )
			break;

		/* Transfer packet from buffer to send */
		vBuffer.pop();
		vSend.push_back( pPckt );
		iTotal += pPckt->size();
	}
	/* Sanity */
	if( vSend.empty() )
		return;

	/* Time difference */
	if( not iTotal )
		iTotal = 1;
	MediaPacket* pBack = vSend.back();
	MediaPacket* pFront = vSend.front();
	timestamp_t iActualWindow = ( pBack->dts + pBack->inc - pFront->dts ) / 90;
	SirannonTime oOffsettime ( ( pFront->dts - iBase ) / 90 );

	/* Iterate over the entire frame */
	int iSize = 0;
	for( deque_it i = vSend.begin(); i != vSend.end(); i++ )
	{
		MediaPacket* pPckt = *i;
		pPckt->oSend = oOffsettime + SirannonTime::fromNSecs( iActualWindow * iSize * MEGA / iTotal );
		iSize += pPckt->size();
	}
}
