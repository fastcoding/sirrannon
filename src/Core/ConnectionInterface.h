#ifndef SIRANNONCOMMUNICATOR_H_
#define SIRANNONCOMMUNICATOR_H_
#include <sirannon.h>
#include "SirannonTime.h"

const static int COM_AGAIN = -2;

class ConnectionInterface
{
public:
	virtual int sendfileSmart( int iFile, uint64_t iSize );
	virtual int sendfileSmart( int iFile, uint64_t iSize, const SirannonTime& oTimeOut );
	virtual int sendSmart( const uint8_t* pData, uint32_t iSize );
	virtual int sendSmart( const uint8_t* pData, uint32_t iSize, const SirannonTime& oTimeOut );
	virtual int receiveRaw( uint8_t* pData, uint32_t iSize );
	virtual int receiveSmart( uint8_t* pData, uint32_t iSize );
	virtual int receiveRaw( uint8_t* pData, uint32_t iSize, const SirannonTime& oTimeOut );
	virtual int receiveSmart( uint8_t* pData, uint32_t iSize, const SirannonTime& oTimeOut );
	virtual void setBlocking( void ) { bBlocking = true; }
	virtual void setNonBlocking( void ) { bBlocking = false; }
	virtual void setNonThrowing( void ) { bThrow = false; }
	virtual ~ConnectionInterface() { };

protected:
	virtual int sendfile( int iFile, size_t iSize ) { return -1; }
	virtual int send( const uint8_t* pData, uint32_t iSize ) = 0;
	virtual int receive( uint8_t* pData, uint32_t iSize ) = 0;

	bool bBlocking, bThrow;
	ConnectionInterface();

private:
	ConnectionInterface( const ConnectionInterface& );
	ConnectionInterface& operator=( const ConnectionInterface& );
	int evaluateStatus( int iRet );
};

extern const SirannonTime oDefaultTimeOut;

#endif /* SIRANNONCOMMUNICATOR_H_ */
