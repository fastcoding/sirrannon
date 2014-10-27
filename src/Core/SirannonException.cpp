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
#include "SirannonException.h"
#include "OSSupport.h"
#include <signal.h>

bool bUnhandled = false;

SirannonException::SirannonException( const char* sType, const char* sFmt, va_list oEllips )
{
	strcpy( sError, sType );
	strcat( sError, ": " );
	vsnprintf( sError+strlen(sError), sizeof(sError)-strlen(sError), sFmt, oEllips );
	va_end( oEllips );
}

SirannonException::SirannonException( const char* sType, const MediaProcessor* pProc, const char* sFmt, va_list oEllips )
{
	strcpy( sError, sType );
	strcat( sError, ": " );
	strcat( sError, pProc->c_str() );
	strcat( sError, ": " );
	vsnprintf( sError+strlen(sError), sizeof(sError)-strlen(sError), sFmt, oEllips );
	va_end( oEllips );
}

const char* SirannonException::what( void ) const throw()
{
	return sError;
}

void SirannonException::warn( void ) const throw()
{	
	fprintf( stderr, "[%06"LL"u] %s\n", SirannonTime::getUpTime().convertMsecs(), sError );
}

void SirannonException::unhandled( void )
{
	fprintf( stderr, "[%06"LL"u] Unhandled %s\n", SirannonTime::getUpTime().convertMsecs(), sError );
	exit( -1 );
}

int SirannonWarning( const char* sError, ... )
{
	va_list oEllipsis;
	va_start( oEllipsis, sError );
	SirannonException( "Warning", sError, oEllipsis ).warn();
	return -1;
}

int SirannonWarning( const MediaProcessor* pProc, const char* sError, ... )
{
	va_list oEllipsis;
	va_start( oEllipsis, sError );
	SirannonException( "Warning", pProc, sError, oEllipsis ).warn();
	return -1;
}

void RuntimeError( const char* sError, ... )
{
	va_list oEllipsis;
	va_start( oEllipsis, sError );
	throw SirannonException( "RuntimeError", sError, oEllipsis );
}

void RuntimeError( const MediaProcessor* pProc, const char* sError, ... )
{
	va_list oEllipsis;
	va_start( oEllipsis, sError );
	throw SirannonException( "RuntimeError", pProc, sError, oEllipsis );
}

void OutOfBoundsError( const char* sError, ... )
{
	va_list oEllipsis;
	va_start( oEllipsis, sError );
	throw SirannonException( "OutOfBoundsError", sError, oEllipsis );
}

void OutOfBoundsError( const MediaProcessor* pProc, const char* sError, ... )
{
	va_list oEllipsis;
	va_start( oEllipsis, sError );
	throw SirannonException( "OutOfBoundsError", pProc, sError, oEllipsis );
}

void TypeError( const char* sError, ... )
{
	va_list oEllipsis;
	va_start( oEllipsis, sError );
	throw SirannonException( "TypeError", sError, oEllipsis );
}

void TypeError( const MediaProcessor* pProc, const char* sError, ... )
{
	va_list oEllipsis;
	va_start( oEllipsis, sError );
	throw SirannonException( "TypeError", pProc, sError, oEllipsis );
}

void ValueError( const char* sError, ... )
{
	va_list oEllipsis;
	va_start( oEllipsis, sError );
	throw SirannonException( "ValueError", sError, oEllipsis );
}

void ValueError( const MediaProcessor* pProc, const char* sError, ... )
{
	va_list oEllipsis;
	va_start( oEllipsis, sError );
	throw SirannonException( "ValueError", pProc, sError, oEllipsis );
}

void SyntaxError( const char* sError, ... )
{
	va_list oEllipsis;
	va_start( oEllipsis, sError );
	throw SirannonException( "SyntaxError", sError, oEllipsis );
}

void SyntaxError( const MediaProcessor* pProc, const char* sError, ... )
{
	va_list oEllipsis;
	va_start( oEllipsis, sError );
	throw SirannonException( "SyntaxError", pProc, sError, oEllipsis );
}

void FFmpegError( const char* sError, ... )
{
	va_list oEllipsis;
	va_start( oEllipsis, sError );
	throw SirannonException( "FFmpegError", sError, oEllipsis );
}

void FFmpegError( const MediaProcessor* pProc, const char* sError, ... )
{
	va_list oEllipsis;
	va_start( oEllipsis, sError );
	throw SirannonException( "FFmpegError", pProc, sError, oEllipsis );
}

void IOError( const char* sError, ... )
{
	va_list oEllipsis;
	va_start( oEllipsis, sError );
	throw SirannonException( "IOError", sError, oEllipsis );
}

void IOError( const MediaProcessor* pProc, const char* sError, ... )
{
	va_list oEllipsis;
	va_start( oEllipsis, sError );
	throw SirannonException( "IOError", pProc, sError, oEllipsis );
}

void OSError( const char* sError, ... )
{
	va_list oEllipsis;
	va_start( oEllipsis, sError );
	throw SirannonException( "OSError", sError, oEllipsis );
}

void OSError( const MediaProcessor* pProc, const char* sError, ... )
{
	va_list oEllipsis;
	va_start( oEllipsis, sError );
	throw SirannonException( "OSError", pProc, sError, oEllipsis );
}

void PCAPError( const char* sError, ... )
{
	va_list oEllipsis;
	va_start( oEllipsis, sError );
	throw SirannonException( "PCAPError", sError, oEllipsis );
}

void PCAPError( const MediaProcessor* pProc, const char* sError, ... )
{
	va_list oEllipsis;
	va_start( oEllipsis, sError );
	throw SirannonException( "PCAPError", pProc, sError, oEllipsis );
}
