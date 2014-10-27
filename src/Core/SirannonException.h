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
#ifndef EXCEPTIONS_H_
#define EXCEPTIONS_H_
#include "sirannon.h"
#include "Boost.h"

extern bool bUnhandled;

class SirannonException : public std::exception
{
public:
	SirannonException( const char* sType, const char* sError, va_list oEllips );
	SirannonException( const char* sType, const MediaProcessor* pProc, const char* sError, va_list oEllips );
	SirannonException( const std::exception& oException );
	~SirannonException() throw() { };

	virtual const char* what( void ) const throw();
	virtual void unhandled( void );
	virtual void warn( void ) const throw();

private:
	char sError [1024];
};

int SirannonWarning( const char* sError, ... );
int SirannonWarning( const MediaProcessor* pProc, const char* sError, ... );
void RuntimeError( const char* sError, ... );
void RuntimeError( const MediaProcessor* pProc, const char* sError, ... );
void OutOfBoundsError( const char* sError, ... );
void OutOfBoundsError( const MediaProcessor* pProc, const char* sError, ... );
void TypeError( const char* sError, ... );
void TypeError( const MediaProcessor* pProc, const char* sError, ... );
void ValueError( const char* sError, ... );
void ValueError( const MediaProcessor* pProc, const char* sError, ... );
void SyntaxError( const char* sError, ... );
void SyntaxError( const MediaProcessor* pProc, const char* sError, ... );
void FFmpegError( const char* sError, ... );
void FFmpegError( const MediaProcessor* pProc, const char* sError, ... );
void IOError( const char* sError, ... );
void IOError( const MediaProcessor* pProc, const char* sError, ... );
void OSError( const char* sError, ... );
void OSError( const MediaProcessor* pProc, const char* sError, ... );
void PCAPError( const char* sError, ... );
void PCAPError( const MediaProcessor* pProc, const char* sError, ... );

#endif /* EXCEPTIONS_H_ */
