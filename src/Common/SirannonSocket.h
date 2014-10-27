#ifndef _SIRANNON_SOCKET_H_
#define _SIRANNON_SOCKET_H_
#include "ConnectionInterface.h"
#include "Network.h"
#include "Utils.h"
#include "IPAddress.h"

class SirannonSocket : public ConnectionInterface
{
public:
	virtual ~SirannonSocket();
	void setLocalPort( uint16_t localPort );
	void setLocalAddressAndPort( const string &localAddress,  uint16_t localPort = 0 );
	string getLocalAddress( void ) const;
	uint16_t getLocalPort( void ) const;
	void setBufferSize( int size );
	int connect( const string &foreignAddress, uint16_t foreignPort, SirannonTime oTimeOut=5000 );
	int sendfile( int iFile, size_t iBytes );

private:
   SirannonSocket( const SirannonSocket &oSocket );
   void operator=( const SirannonSocket &oSocket );

protected:
   int iSocket;
   int send( const uint8_t* buffer, uint32_t iBuffer );
   int receive( uint8_t* buffer, uint32_t iBuffer );
   SirannonSocket( int iType, int iProtocol, bool bBlock );
   SirannonSocket( int iSocket, bool bBlock );
};

class TCPSocket : public SirannonSocket
{
public:
   TCPSocket();
   TCPSocket( const string &sForeignAddress, uint16_t iForeignPort, SirannonTime oTimeOut=5000 );
   ~TCPSocket();
   string getForeignAddress();
   uint16_t getForeignPort();
   int makeConnection( const string &sForeignAddress, uint16_t iForeignPort );

private:
   friend class TCPServerSocket;
   TCPSocket( int newConnSD );
   void setNonLingering( void );
};

class TCPServerSocket : public SirannonSocket
{
public:
	TCPServerSocket( uint16_t localPort, uint32_t queueLen=5 );
	TCPServerSocket( const string &localAddress, uint16_t localPort, uint32_t queueLen=5 );

	TCPSocket* accept( void );

private:
	void setListen( int queueLen );
};

class UDPSocket : public SirannonSocket
{
public:
	UDPSocket( uint16_t localPort );
	UDPSocket( uint16_t localPort, sockaddr_in oAddr );

	int setDestination( const IPAddress& oAddr );
	int send( const uint8_t* pBuffer, uint32_t iBuffer );
	int recvFrom( uint8_t* pBuffer, uint32_t iBuffer, string &oSourceAddress, uint16_t &iSourcePort );

	void setMulticastTTL( uint8_t multicastTTL );
	void joinGroup( const string &multicastGroup );
	void leaveGroup( const string &multicastGroup );

private:
	void setBroadcast();
	void disconnect();
	sockaddr_in oAddr;
};

#endif
