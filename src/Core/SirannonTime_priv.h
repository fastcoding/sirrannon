#ifndef SIRANNONTIME_PRIV_H_
#define SIRANNONTIME_PRIV_H_
#define SIRANNON_USE_BOOST_THREAD
#define SIRANNON_USE_BOOST_TIME
#include "Boost.h"

inline SirannonTime::SirannonTime( uint32_t iSeconds, uint32_t iNanoSeconds )
	: nsec(iNanoSeconds), sec(iSeconds)
{
	if( nsec >= 1000000000 )
	{
		sec += nsec / 1000000000;
		nsec = nsec % 1000000000;
	}
}

inline SirannonTime::SirannonTime( uint32_t iMilliSeconds )
{
	sec = iMilliSeconds / 1000;
	nsec = (iMilliSeconds % 1000) * 1000000;
}

inline SirannonTime::SirannonTime( void )
	: nsec(0), sec(0)
{ }

inline SirannonTime SirannonTime::fromNSecs( uint64_t iNano )
{
	uint64_t iSec = iNano / 1000000000;
	return SirannonTime( iSec, iNano - iSec * 1000000000 );
}

inline SirannonTime SirannonTime::fromMPEG( uint64_t iVal )
{
	return fromNSecs( iVal * 1000000 / 90 );
}

inline SirannonTime SirannonTime::fromUSecs( uint64_t iVal )
{
	return fromNSecs( iVal * 1000 );
}

inline SirannonTime& SirannonTime::operator-=( const SirannonTime &t )
{
	sec -= t.sec;
	if( t.nsec > nsec )
	{
		sec--;
		nsec += 1000000000;
	}
	nsec -= t.nsec;
	return *this;
}

inline SirannonTime &SirannonTime::operator+=( const SirannonTime &t )
{
	sec += t.sec;
	nsec += t.nsec;
	if( nsec >= 1000000000 )
	{
		sec++;
		nsec -= 1000000000;
	}
	return *this;
}

inline bool SirannonTime::operator<( const SirannonTime &t ) const
{
	if (sec < t.sec)
		return true;
	if (sec > t.sec)
		return false;
	if (nsec < t.nsec)
		return true;
	return false;
}

inline bool SirannonTime::operator>( const SirannonTime &t ) const
{
	if (sec > t.sec)
		return true;
	if (sec < t.sec)
		return false;
	if (nsec > t.nsec)
		return true;
	return false;
}

inline bool SirannonTime::operator<=( const SirannonTime &t ) const
{
	if (sec < t.sec)
		return true;
	if (sec > t.sec)
		return false;
	if (nsec <= t.nsec)
		return true;
	return false;
}

inline bool SirannonTime::operator>=( const SirannonTime &t ) const
{
	if (sec > t.sec)
		return true;
	if (sec < t.sec)
		return false;
	if (nsec >= t.nsec)
		return true;
	return false;
}

inline SirannonTime SirannonTime::operator+( const SirannonTime& other ) const
{
	SirannonTime oSum = *this;
	oSum += other;
	return oSum;
}

inline SirannonTime SirannonTime::operator-( const SirannonTime& other ) const
{
	SirannonTime oDiff = *this;
	oDiff -= other;
	return oDiff;
}

inline bool SirannonTime::checkInterval( const SirannonTime& oInterval )
{
	const SirannonTime oCurrent = getCurrentTime();
	const SirannonTime oCheck =  *this + oInterval;
	if( oCheck >= oCurrent )
		return false;
	*this = oCheck;
	return true;
}

inline void SirannonTime::synchronizeInterval( const SirannonTime& oStep )
{
	SirannonTime oCurrent = getCurrentTime();
	*this += oStep;
	if( oCurrent < *this )
	{
		SirannonTime oDiff = *this - oCurrent;
		oDiff.sleep();
	}
}

inline uint64_t SirannonTime::convertNsecs( void ) const
{
	return (uint64_t)nsec + (uint64_t)sec * 1000000000;
}

inline uint64_t SirannonTime::convertUsecs( void ) const
{
	return (uint64_t)nsec / 1000 + (uint64_t)sec * 1000000;
}

inline uint64_t SirannonTime::convertMsecs( void ) const
{
	return (uint64_t)nsec / 1000000 + (uint64_t)sec * 1000;
}

inline uint32_t SirannonTime::convertSecs( void ) const
{
	return sec;
}

inline double SirannonTime::convertDouble( void ) const
{
	return convertNsecs() / 1000000000.;
}

inline struct timespec SirannonTime::convertStruct( void ) const
{
	struct timespec oTime;
	oTime.tv_sec  = sec;
	oTime.tv_nsec = nsec;
	return oTime;
}

inline void SirannonTime::sleep( void ) const
{
	this_thread::sleep( posix_time::microseconds( convertUsecs() ) );
}

inline bool SirannonTime::zero( void ) const
{
	return (nsec == 0) and (sec == 0);
}

inline void SirannonTime::setSimulatedTime( const SirannonTime& oSim )
{
	bRealtime = false;
	oStep = oSim;
	oUpTime = 0;
}

inline void SirannonTime::setRealtime( void )
{
	bRealtime = true;
}

inline void SirannonTime::tick( void )
{
	oTime += oStep;
}

inline SirannonTime SirannonTime::getUpTime( void )
{
	SirannonTime oCurrent = getCurrentTime();
	oCurrent -= oUpTime;
	return oCurrent;
}

inline bool SirannonTime::isSimulatedTime( void )
{
	return not bRealtime;
}

inline void SirannonTime::sleepUntil( function0<bool> oPredicate ) const
{
	while( not oPredicate() )
		sleep();
}

inline string SirannonTime::asctime( void ) const
{
	char sTime [32];
	time_t oTime = convertSecs();
#ifdef __USE_POSIX
	asctime_r( localtime( &oTime ), sTime );
#else
    static const char wday_name[7][4] = {
	        "Sun", "Mon", "Tue", "Wed",
	        "Thu", "Fri", "Sat"
	    };

	static const char mon_name[12][4] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};

	struct tm* timeptr = localtime( &oTime );
	sprintf(sTime, "%.3s %.3s%3d %.2d:%.2d:%.2d %d\n",
		wday_name[timeptr->tm_wday], mon_name[timeptr->tm_mon],
		timeptr->tm_mday, timeptr->tm_hour, timeptr->tm_min,
		timeptr->tm_sec, 1900 + timeptr->tm_year);
#endif
	sTime[strlen(sTime)-1] = '\0';
	return sTime;
}

#ifndef WIN32
inline SirannonTime SirannonTime::getCurrentTime( void )
{
	if( bRealtime )
	{
		struct timespec oTime;
		if( clock_gettime( CLOCK_REALTIME, &oTime ) < 0 )
			return 0;
		return SirannonTime( oTime.tv_sec, oTime.tv_nsec );
	}
	else
		return oTime;
}
#endif

#endif /* SIRANNONTIME_PRIV_H_ */
