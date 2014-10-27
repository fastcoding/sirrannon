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
#ifndef XTIME_H_
#define XTIME_H_
#define SIRANNON_USE_BOOST_FUNCTION
#include "Boost.h"

/* Includes */
#ifdef WIN32
	#ifndef _TIMESPEC_DEFINED
		#define _TIMESPEC_DEFINED
		struct timespec
		{
			long int tv_sec;		/* Seconds.  */
			long int tv_nsec;		/* Nanoseconds.  */
		};
	#endif
	#include <time.h>
#else
	#include <time.h>
	#include <sys/types.h>
#endif

class SirannonTime
{
public:
	/* Constructors */
	SirannonTime( uint32_t iSeconds, uint32_t iNanoSeconds );
	SirannonTime( uint32_t iMilliSeconds );
	SirannonTime( );

	/* Get the time difference since the start from the Sirannon */
	static SirannonTime getCurrentTime( void );
	static SirannonTime getUpTime( void );

	/* This function makes time simulated with a fixed step */
	static bool isSimulatedTime( void );
	static void setSimulatedTime( const SirannonTime& oSim );
	static void setRealtime( void );
	static void tick( void );

	/* Conversion functions */
	static SirannonTime fromUSecs( uint64_t iMicro);
	static SirannonTime fromNSecs( uint64_t iNano );
	static SirannonTime fromMPEG( uint64_t iVal );
	struct timespec convertStruct( void ) const;
	uint64_t convertNsecs( void ) const;
	uint64_t convertUsecs( void ) const;
	uint64_t convertMsecs( void ) const;
	uint32_t convertSecs( void ) const;
	double convertDouble( void ) const;

	/* Handy functions */
	bool checkInterval( const SirannonTime& oInterval );
	bool zero( void ) const;
	void synchronizeInterval( const SirannonTime& oStep );
	void sleep( void ) const;
	void sleepUntil( function0<bool> oPredicate ) const;
	string asctime( void ) const;

	/* Operators */
	SirannonTime operator+( const SirannonTime& ) const;
	SirannonTime operator-( const SirannonTime& ) const;
	SirannonTime& operator-=( const SirannonTime& );
	SirannonTime& operator+=( const SirannonTime& );
	bool operator<( const SirannonTime& ) const;
	bool operator>( const SirannonTime& ) const;
	bool operator<=( const SirannonTime& ) const;
	bool operator>=( const SirannonTime& ) const;

private:
	uint32_t nsec, sec;
	static bool bRealtime;
	static SirannonTime oTime, oStep, oUpTime;
};

extern const SirannonTime oPollQuantum;

#include "SirannonTime_priv.h"

#endif /*TIME_H_*/

