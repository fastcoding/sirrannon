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
#include "sirannon.h"
#include "SirannonTime.h"
#include "SirannonException.h"
#include "SirannonPrint.h"
#include <time.h>

SirannonTime SirannonTime::oTime = 0, SirannonTime::oStep = 0;
bool SirannonTime::bRealtime = true;
SirannonTime SirannonTime::oUpTime = SirannonTime::getCurrentTime();
const SirannonTime oPollQuantum( 40 );

#ifdef WIN32
SirannonTime SirannonTime::getCurrentTime( void )
{
	if( bRealtime )
	{
		/* Detect which clock we will use */
		static int64_t iFreq = -1, iStart = -1;

		if( iStart < 0 )
			QueryPerformanceCounter( (LARGE_INTEGER*) &iStart );

		if( iFreq < 0 )
		{
			QueryPerformanceFrequency( (LARGE_INTEGER*) &iFreq );

			/* Validity of the clock */
			if( iFreq != 1193182LL and iFreq != 3579545LL )
				iFreq = 0;
			else
			{
				/* Get windows version */
				OSVERSIONINFOEX oVersion;
				memset( &oVersion, 0, sizeof(OSVERSIONINFOEX) );
				oVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
				if( not GetVersionEx ( (OSVERSIONINFO*) &oVersion ) )
					iFreq = 0;

				/* Minimally Windows7 */
				if( oVersion.dwMajorVersion < 6 ) // Windows 7 is 6
					iFreq = 0;
			}
			if( iFreq == 0 )
				SirannonPrint( 1, "info @ SirannonTime: Can not use microsecond precision clock, time accuracy will suffer" );
		}
		/* If the frequency is larger than 0 use the QueryPerformanceCounter */
		if( iFreq > 0 )
		{
			__int64 iTime = -1;
			QueryPerformanceCounter( (LARGE_INTEGER*) &iTime );
			iTime -= iStart;
			iTime *= 1000000000LL;
			iTime /= iFreq;
			return SirannonTime( iTime / 1000000000, iTime % 1000000000 );
		}
		else
		{
			__int64 iTime = timeGetTime();
			return SirannonTime( iTime / 1000, (iTime % 1000) * MEGA );
		}
	}
	else
	{
		return oTime;
	}
}
#endif
