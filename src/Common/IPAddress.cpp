#include "sirannon.h"
#include "IPAddress.h"
#include "Bits.h"
#include <cstdio>

IPAddress::IPAddress( const string& sAddress, uint16_t iInputPort )
	: iPort(iInputPort)
{
	/* Init */
	memset( vIP, 0, sizeof(vIP) );

	/* Index */
	int32_t iIndex = sAddress.find(':');

	/* Resolve name */
	hostent* pHost;
	if( ( pHost = gethostbyname( sAddress.substr(0,iIndex).c_str() ) ) == NULL )
		return;

	/* Copy to IP structure */
	OBits oIP( vIP, 4 );
	oIP.write( 32, ntoh32(*((uint32_t*)pHost->h_addr_list[0])) );

	/* Find iPort */
	if( iIndex != string::npos and iPort == 0 )
		iPort = atoi( sAddress.substr(iIndex+1).c_str() );
}

IPAddress::IPAddress( uint8_t* _vIP, uint16_t _port )
	: iPort(0)
{
	for( int k = 0; k < 4; k++ )
		vIP[k] = _vIP[k];
	iPort = _port;
}

IPAddress::IPAddress( uint32_t _vIP, uint16_t _port )
	: iPort(0)
{
	for( int k = 0; k < 4; k++ )
		vIP[k] = (_vIP & (0xFF << (8*k))) >> (8*k) ;
	iPort = _port;
}

string IPAddress::getPortStr( void ) const
{
	char sBuffer [32];
	snprintf( sBuffer, sizeof(sBuffer), "%hu", iPort );
	return sBuffer;
}

string IPAddress::getAddressStr( void ) const
{
	string sAddr( getIPStr() );
	sAddr.append( ":" );
	sAddr.append( getPortStr() ) ;
	return sAddr;
}

string IPAddress::getIPStr( void ) const
{
	char sBuffer [64];
	snprintf( sBuffer, sizeof(sBuffer), "%d.%d.%d.%d", (int)vIP[0], (int)vIP[1], (int)vIP[2], (int)vIP[3] );
	return sBuffer;
}

uint32_t IPAddress::getIP( void ) const
{
	IBits oIP( vIP, sizeof(vIP) );
	return oIP.read( 32 );
}

sockaddr_in IPAddress::getSockAddr( void ) const
{
	sockaddr_in addr;
	memset( &addr, 0, sizeof(addr) );
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = hton32( getIP() );
	addr.sin_port = hton16( iPort );
	return addr;
}

bool IPAddress::valid( void ) const
{
	return iPort > 0 and ( vIP[0] or vIP[1] or vIP[2] or vIP[3] );
}

uint16_t IPAddress::getPort( void ) const
{
	return iPort;
}

uint8_t* IPAddress::getIPArray( void )
{
	return vIP;
}

void IPAddress::setPort( uint16_t iPort )
{
	this->iPort = iPort;
}
