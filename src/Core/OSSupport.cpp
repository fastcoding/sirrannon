#include "Network.h"
#include "OSSupport.h"
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#ifdef WIN32

int setNonBlocking( int fd )
{
	 u_long val = 1;
	 return ioctlsocket(fd, FIONBIO, &val);
}

char* strError( void )
{
	static char sError[512];
	snprintf( sError, sizeof(sError), "%d: ", WSAGetLastError() );
	FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, 0, WSAGetLastError(), 0, sError + strlen(sError), 511 - strlen(sError), NULL );
	return sError;
}

#else

int setNonBlocking( int fd )
{
	int flags = fcntl( fd, F_GETFL, 0 );
	return fcntl( fd, F_SETFL, flags | O_NONBLOCK );
}

char* strError( void )
{
	return strerror( errno );
}

#endif
