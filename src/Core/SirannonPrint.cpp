/*
 * SirannonPrint.cpp
 *
 *  Created on: Nov 8, 2010
 *      Author: arombaut
 */
#include "SirannonPrint.h"
#include "SirannonTime.h"
#include "OSSupport.h"

mutex oPrintMutex;

void SirannonPrint( int iLevel, const char* fmt, ... )
{
	if( iVerbose >= iLevel )
	{
		va_list ap;
		va_start(ap, fmt);
		Lock_t oLock( oPrintMutex );
		fprintf( stderr, "info [%08"LL"u]: ", SirannonTime::getUpTime().convertMsecs() );
		vfprintf( stderr, fmt, ap );
		fprintf( stderr, "\n" );
		va_end( ap );
		fflush( stderr );
	}
}
