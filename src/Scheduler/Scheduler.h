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
#ifndef SCHED_H
#define SCHED_H
#include "sirannon.h"
#include "SirannonTime.h"
#include "Interfaces.h"

class Scheduler : public MediaProcessor, public BufferInterface
{
public:
	virtual ~Scheduler();

	bool bufferFull( void ) const;
	int iDelta, iBufferPackets;

	/* General commands */
	virtual void init( void );
	virtual int flush( void );

	/* Scheduler commands */
	virtual int pause( void );
	virtual int play( double fSpeed=-1.0 );

protected:
	/* Queue's */
	queue_t vBuffer;
	deque_t vSend;

	/* Params */
	bool bSending, bOverflow, bPause, bEnd;
	int32_t iLast, iMaxDelay, iBuffer, iLost, iMin, iError;
	timestamp_t iBase;
	uint32_t iUnderflow;
	double fCurrent, fSpeed;
	SirannonTime oStarttime, oPause, oFixedStartTime, oDrop, oLast, oPrev, oTimeOut,
	oStartDelay, oMaxDiff;

	/* Processing */
	void process( void );
	virtual void makeSchedule( void ) = 0;
	SirannonTime getOffset( void ) const;

	/* Packets */
	int getBufferDelta( void ) const;
	void receive( MediaPacketPtr& pckt);
	void receive_reset( MediaPacketPtr& pckt );
	void receive_end( MediaPacketPtr& pckt );

	Scheduler( const string& sName, ProcessorManager* pScope );
};


class basic_Scheduler : public Scheduler
{
	public:
	basic_Scheduler( const string& sName, ProcessorManager* pScope )
	: Scheduler(sName, pScope) {};

	protected:
	void makeSchedule( void );
};


class frame_Scheduler : public Scheduler
{
	public:
	frame_Scheduler( const string& sName, ProcessorManager* pScope )
	: Scheduler(sName, pScope), inc(1) {};

	protected:
	uint32_t inc;
	void makeSchedule( void );
};


class gop_Scheduler : public Scheduler
{
	public:
	gop_Scheduler( const string& sName, ProcessorManager* pScope )
	: Scheduler(sName, pScope) {};

	protected:
	void makeSchedule( void );
};

class svc_Scheduler : public Scheduler
{
	public:
	svc_Scheduler( const string& sName, ProcessorManager* pScope );

	protected:
	virtual void makeSchedule( void );
};

class qmatch_Scheduler : public svc_Scheduler
{
	public:
	qmatch_Scheduler( const string& sName, ProcessorManager* pScope );

	protected:
	/* Members */
	bool bInterleave1;
	bool bInterleave2;
	bool bInterleave3;

	/* Functions */
	void makeSchedule( void );
	static bool cmp_pts( const MediaPacket* p1, const MediaPacket* p2 );
	static uint32_t classify_size( const MediaPacket* );
	static uint32_t classify_route( const MediaPacket* );
	void init( void );
	void interleave( void );
	void interleave1( void );
	void interleave2( void );
	void interleave3( void );
};

class window_Scheduler : public Scheduler
{
public:
	window_Scheduler( const string& sName, ProcessorManager* pScope );

protected:
	void init( void );
	void makeSchedule( void );

	timestamp_t iWindow;
};

//class sliding_window_Scheduler : public Scheduler
//{
//	public:
//	sliding_window_Scheduler( const string& sName, ProcessorManager* pScope );
//
//	protected:
//	typedef enum { NORMAL, BEHIND, ADVANCE } mode_t;
//	mode_t iMode;
//	int32_t iMaxWindow, iLast, iSize;
//	double fSend, fMaxRate, fOldBandwidth, fGain;
//	double fMaxPlusDelay, fMaxMinusDelay, fInThMinus, fOutThMinus, fInThPlus, fOutThPlus;
//	void makeSchedule( void );
//	void init( void );
//	double fMIN( double a, double b );
//	double fmax( double a, double b );
//};

class shape_Scheduler : public Scheduler
{
	public:
	shape_Scheduler( const string& sName, ProcessorManager* pScope );

	protected:
	double fSend, fMaxRate, fMaxPlusDelay, fMaxMinusDelay;
	void makeSchedule( void );
	void init( void );
};

#endif
