#include "Network.h"
#include "SirannonSocket.h"
#include "SirannonException.h"
#include "Endianness.h"
#include <errno.h>             // For errno
#include <iostream>
#include "OSSupport.h"

/* Error codes differ in WIN32 and Linux */
#ifndef WIN32
const static int SOCK_AGAIN = EAGAIN;
const static int SOCK_INPROGRESS = EINPROGRESS;
const static int SOCK_ALREADY = EALREADY;

inline bool checkError( int iError )
{
	return errno == iError;
}
#else
const static int SOCK_AGAIN = WSAEWOULDBLOCK;
const static int SOCK_INPROGRESS = WSAEINPROGRESS;
const static int SOCK_ALREADY = WSAEALREADY;

inline bool checkError( int iError )
{
	return WSAGetLastError( ) == iError;
}
#endif

/* Must activate the Winsock 2.0 library */
#ifdef WIN32
int win_sock_loader( void )
{
	WORD wVersionRequested;
	WSADATA wsaData;
	wVersionRequested = MAKEWORD(2, 0);                // Request WinSock v2.0
	WSAStartup(wVersionRequested, &wsaData);
	return 0;
}
static int iLoadWinSock = win_sock_loader();
#endif

// Function to fill in address structure given an address and port
static void fillAddr(const string &address, uint16_t port,
                     sockaddr_in &addr) {
  memset(&addr, 0, sizeof(addr));  // Zero out address structure
  addr.sin_family = AF_INET;       // Internet address

  hostent *host;  // Resolve name
  if ((host = gethostbyname(address.c_str())) == NULL) {
    // strError(  "RuntimeError", ) will not work for gethostbyname() and hstrError(  "RuntimeError", )
    // is supposedly obsolete
    return;
  }
  addr.sin_addr.s_addr = *((unsigned long *) host->h_addr_list[0]);

  addr.sin_port = hton16(port);     // Assign port in network byte order
}

#ifndef WIN32
SirannonSocket::SirannonSocket( int type, int protocol, bool bBlock )
{
	// Make a new socket
	iSocket = socket( PF_INET, type, protocol );
	if( iSocket < 0 )
		IOError( "Socket unsupported: %s", strError() );

	/* Raw sockets are always non blocking */
	::setNonBlocking( iSocket );
	bBlocking = bBlock;
}
#else
SirannonSocket::SirannonSocket( int type, int protocol, bool bBlock )
{
	// Make a new socket
	iSocket = socket( PF_INET, type, protocol );
	if( iSocket < 0 )
		IOError( "Socket unsupported: %s", strError( ) );

	/* Raw sockets are always non blocking */
	::setNonBlocking( iSocket );
	bBlocking = bBlock;
}
#endif

SirannonSocket::SirannonSocket( int iSocket, bool bBlock )
{
	this->iSocket = iSocket;

	/* Raw sockets are always non blocking */
	::setNonBlocking( iSocket );
	bBlocking = bBlock;
}

#ifndef WIN32
SirannonSocket::~SirannonSocket()
{
    ::close( iSocket );
}
#else
SirannonSocket::~SirannonSocket()
{
    ::closesocket( iSocket );
}
#endif

void SirannonSocket::setLocalPort( uint16_t _iPort )
{
  /* Fill out the address structure */
  sockaddr_in oLocalAddr;
  memset( &oLocalAddr, 0, sizeof(oLocalAddr) );
  oLocalAddr.sin_family = AF_INET;
  oLocalAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  oLocalAddr.sin_port = hton16( _iPort );

  /* Acquire the port */
  if( ::bind( iSocket, (sockaddr*) &oLocalAddr, sizeof(sockaddr_in) ) < 0 )
	  IOError( "Socket would not bind on port %hu", _iPort );
}

void SirannonSocket::setLocalAddressAndPort( const string &sLocalAddr, uint16_t iPort )
{
	sockaddr_in oLocalAddr;
	fillAddr( sLocalAddr, iPort, oLocalAddr );

  if( ::bind( iSocket, (sockaddr*) &oLocalAddr, sizeof(sockaddr_in) ) < 0 )
	  IOError( "Socket would not bind on port %s:%hu", sLocalAddr.c_str(), iPort );
}

string SirannonSocket::getLocalAddress() const
{
	sockaddr_in oLocalAddr;
	uint32_t iLocalAddr = sizeof(oLocalAddr);

	if( getsockname( iSocket, (sockaddr*) &oLocalAddr, (socklen_t*) &iLocalAddr ) < 0 )
		return "";
	else
		return inet_ntoa( oLocalAddr.sin_addr );
}

uint16_t SirannonSocket::getLocalPort() const
{
	sockaddr_in oLocalAddr;
	uint32_t iLocalAddr = sizeof(oLocalAddr);

	if( getsockname( iSocket, (sockaddr*) &oLocalAddr, (socklen_t*) &iLocalAddr ) < 0 )
		return 0;
	else
		return ntoh16( oLocalAddr.sin_port );
}

void SirannonSocket::setBufferSize( int iSize )
{
	::setsockopt( iSocket, SOL_SOCKET, SO_RCVBUF, (const char*)&iSize, sizeof(int) );
}

int SirannonSocket::connect( const string &sForeignAddr, uint16_t iForeignPort, SirannonTime oTimeOut )
{
	/* Create the address */
	sockaddr_in oForeignAddr;
	fillAddr( sForeignAddr, iForeignPort, oForeignAddr );

	/* Connect blocking or non blocking and sleep before retrying when EINPROGRESS or ELREADY */
	SirannonTime oInterval = SirannonTime::getCurrentTime();
	int iRet;
	while( (iRet = ::connect( iSocket, (sockaddr*) &oForeignAddr, sizeof(oForeignAddr) )) < 0 )
	{
		if( not bBlocking )
			return iRet;
		if( oInterval.checkInterval( oTimeOut ) )
			return -2;
		if( checkError( SOCK_AGAIN ) or checkError( SOCK_INPROGRESS ) or checkError( SOCK_ALREADY ) )
			oPollQuantum.sleep();
		else
			return iRet;
	}
	return iRet;
}

int SirannonSocket::send( const uint8_t* pBuffer, uint32_t iBuffer )
{
	int iVal = ::send( iSocket, (raw_type*) pBuffer, iBuffer, 0 );
	if( iVal >= 0 )
		return iVal;
	else if( checkError( SOCK_AGAIN ) )
		return -2;
	else
		return -1;
}
#ifndef WIN32
#include <sys/sendfile.h>
int SirannonSocket::sendfile( int iFile, size_t iBytes )
{
	off_t iOffset = 0;
	int iVal = ::sendfile( iSocket, iFile, NULL, iBytes );
	if( iVal >= 0 )
		return iVal;
	else if( checkError( SOCK_AGAIN ) )
		return -2;
	else
		return -1;
}
#else
int SirannonSocket::sendfile( int iFile, size_t iBytes )
{
	return -1;
}
#endif

int SirannonSocket::receive( uint8_t* pBuffer, uint32_t iBuffer )
{
	int iVal = ::recv( iSocket, (raw_type*) pBuffer, iBuffer, 0 );
	if( iVal >= 0 )
		return iVal;
	else if( checkError( SOCK_AGAIN ) )
		return -2;
	else
		return -1;
}

TCPSocket::TCPSocket( )
	: SirannonSocket(SOCK_STREAM, IPPROTO_TCP, true)
{
	setNonLingering();
}

TCPSocket::~TCPSocket( )
{
	::shutdown( iSocket, SHUT_RDWR );
}

TCPSocket::TCPSocket( const string &sForeignAddress, uint16_t iForeignPort, SirannonTime oTimeOut )
	: SirannonSocket(SOCK_STREAM, IPPROTO_TCP, true)
{
	setNonLingering();
	int iStatus = connect( sForeignAddress, iForeignPort, oTimeOut );
	if( iStatus < 0 )
	{
		IOError( "Could not connect to (%s:%hu): %s", sForeignAddress.c_str(), iForeignPort,
				(iStatus==-2) ? "Time-out" : strError() );
	}
}

TCPSocket::TCPSocket( int newConnSD )
	: SirannonSocket(newConnSD, true)
{
	setNonLingering();
}

int TCPSocket::makeConnection( const string &sForeignAddress, uint16_t iForeignPort )
{
	int iStatus = connect( sForeignAddress, iForeignPort );
	if( iStatus < 0 and bThrow )
	{
		IOError( "Could not connect to (%s:%hu): %s", sForeignAddress.c_str(), iForeignPort,
					(iStatus==-2) ? "Time-out" : strError() );
	}
	return iStatus;
}

void TCPSocket::setNonLingering( void )
{
	/* Set the socket to no linger, so it does not block */
	struct linger oLinger;
	oLinger.l_onoff = 1;
	oLinger.l_linger = 0;
	::setsockopt( iSocket, SOL_SOCKET, SO_LINGER, (const char*)&oLinger, sizeof(struct linger) );
}

uint16_t TCPSocket::getForeignPort( void )
{
	sockaddr_in oAddr;
	uint32_t iAddr = sizeof(oAddr);

	if( getpeername( iSocket, (sockaddr*) &oAddr, (socklen_t*) &iAddr ) < 0 )
		return 0;
	return ntoh16( oAddr.sin_port );
}

string TCPSocket::getForeignAddress()
{
	sockaddr_in oAddr;
		uint32_t iAddr = sizeof(oAddr);;

	if( getpeername( iSocket, (sockaddr*) &oAddr, (socklen_t*) &iAddr ) < 0 )
		return "";
	return inet_ntoa( oAddr.sin_addr );
}

TCPServerSocket::TCPServerSocket( uint16_t iLocalPort, uint32_t iQueue )
     : SirannonSocket(SOCK_STREAM, IPPROTO_TCP, true)
{
	setLocalPort( iLocalPort );
	setListen( iQueue );
}

TCPServerSocket::TCPServerSocket( const string &oLocalAddress, uint16_t iLocalPort, uint32_t iQueue )
	: SirannonSocket(SOCK_STREAM, IPPROTO_TCP, true)
{
	setLocalAddressAndPort( oLocalAddress, iLocalPort );
	setListen( iQueue );
}

TCPSocket *TCPServerSocket::accept( void )
{
	int iNewSocket;
	while( (iNewSocket = ::accept( iSocket, NULL, 0) ) < 0 )
	{
		if( checkError( SOCK_AGAIN ) and bBlocking )
			oPollQuantum.sleep();
		else
			return NULL;
	}
	return new TCPSocket( iNewSocket );
}

void TCPServerSocket::setListen( int iQueue )
{
	listen( iSocket, iQueue );
}

//UDPSocket::UDPSocket()
//	: SirannonSocket(SOCK_DGRAM, IPPROTO_UDP, true)
//{
//	setBroadcast();
//}

UDPSocket::UDPSocket( uint16_t iPort, sockaddr_in oDestAddr )
	: SirannonSocket(SOCK_DGRAM, IPPROTO_UDP, true), oAddr(oDestAddr)
{
	setLocalPort( iPort );
	setBroadcast();
}

UDPSocket::UDPSocket( uint16_t iPort )
	: SirannonSocket(SOCK_DGRAM, IPPROTO_UDP, true)
{
	setLocalPort( iPort );
	setBroadcast();
}

//UDPSocket::UDPSocket( const string &oLocalAddress, uint16_t iPort,  )
//	: SirannonSocket(SOCK_DGRAM, IPPROTO_UDP, true)
//{
//	setLocalAddressAndPort( oLocalAddress, iPort );
//	setBroadcast();
//}

void UDPSocket::setBroadcast()
{
	  // If this fails, we'll hear about it when we try to send.  This will allow
	  // system that cannot broadcast to continue if they don't plan to broadcast
	  int broadcastPermission = 1;
	  setsockopt(iSocket, SOL_SOCKET, SO_BROADCAST,
				 (raw_type *) &broadcastPermission, sizeof(broadcastPermission));
}

void UDPSocket::disconnect( void )
{
	sockaddr_in oNullAddr;
	memset( &oNullAddr, 0, sizeof(oNullAddr) );
	oNullAddr.sin_family = AF_UNSPEC;
	::connect( iSocket, (sockaddr*) &oNullAddr, sizeof(oNullAddr) );
}

int UDPSocket::setDestination( const IPAddress& oAddr )
{
	this->oAddr = oAddr.getSockAddr();
	return 0;
}

int UDPSocket::send( const uint8_t* pBuffer, uint32_t iBuffer )
{
	int iVal = ::sendto( iSocket, (raw_type*) pBuffer, iBuffer, 0, (sockaddr*) &oAddr, sizeof(oAddr) );
	if( iVal >= 0 )
		return iVal;
	else if( checkError( SOCK_AGAIN ) )
		return -2;
	else
		return -1;
}

int UDPSocket::recvFrom( uint8_t* pBuffer, uint32_t iBuffer, string &oSourceAddress, uint16_t &iSourcePort )
{
	sockaddr_in oClientAddr;
	socklen_t iClientAddr = sizeof(oClientAddr);

	int iVal = recvfrom( iSocket, (raw_type*) pBuffer, iBuffer, 0, (sockaddr*) &oClientAddr, (socklen_t*) &iClientAddr );

	oSourceAddress = inet_ntoa( oClientAddr.sin_addr );
	iSourcePort = ntoh16( oClientAddr.sin_port );

	if( iVal >= 0 )
		return iVal;
	else if( checkError( SOCK_AGAIN ) )
		return -2;
	else
		return -1;
}

void UDPSocket::setMulticastTTL(uint8_t multicastTTL)  {
  if (setsockopt(iSocket, IPPROTO_IP, IP_MULTICAST_TTL,
                 (raw_type *) &multicastTTL, sizeof(multicastTTL)) < 0) {
	  IOError( "setMulticastTTL failed" );
  }
}

void UDPSocket::joinGroup(const string &multicastGroup)
{
	struct ip_mreq multicastRequest;
	multicastRequest.imr_multiaddr.s_addr = inet_addr(multicastGroup.c_str());
	multicastRequest.imr_interface.s_addr = htonl(INADDR_ANY);
	if( setsockopt( iSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (raw_type*) &multicastRequest, sizeof(multicastRequest) ) < 0 )
		IOError( "Join multicast(%s) failed", multicastGroup.c_str() );
}

void UDPSocket::leaveGroup(const string &multicastGroup)  {
  struct ip_mreq multicastRequest;

  multicastRequest.imr_multiaddr.s_addr = inet_addr(multicastGroup.c_str());
  multicastRequest.imr_interface.s_addr = htonl(INADDR_ANY);
  if (setsockopt(iSocket, IPPROTO_IP, IP_DROP_MEMBERSHIP,
                 (raw_type *) &multicastRequest,
                 sizeof(multicastRequest)) < 0) {
	  IOError( "Leave multicast(%s) failed", multicastGroup.c_str() );
  }
}
