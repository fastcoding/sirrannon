/*
 * IPAddress.h
 *
 *  Created on: Sep 15, 2010
 *      Author: arombaut
 */

#ifndef IPADDRESS_H_
#define IPADDRESS_H_
#include "sirannon.h"
#include "Network.h"
#include "Endianness.h"

class IPAddress
{
public:
	IPAddress( const string& s, uint16_t iPort=0 );
	IPAddress( uint8_t* _vIP, uint16_t iPort=0 );
	IPAddress( uint32_t _vIP=0, uint16_t iPort=0 );

	bool valid( void ) const;
	void setPort( uint16_t iPort );
	uint16_t getPort( void ) const;
	string getPortStr( void ) const;
	string getAddressStr( void ) const;
	string getIPStr( void ) const;
	uint32_t getIP( void ) const;
	uint8_t* getIPArray( void );
	sockaddr_in getSockAddr( void ) const;

public:
	uint8_t vIP [4];
	uint16_t iPort;
};
#endif /* IPADDRESS_H_ */
