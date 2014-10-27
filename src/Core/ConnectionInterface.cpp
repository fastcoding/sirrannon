#include "ConnectionInterface.h"
#include "SirannonException.h"
#include "OSSupport.h"
#include "SirannonTime.h"

const SirannonTime oDefaultTimeOut( 5000 );

ConnectionInterface::ConnectionInterface( void )
	: bBlocking(true), bThrow(true)
{ }

inline int ConnectionInterface::evaluateStatus( int iRet )
{
	if( iRet > 0 or not bThrow )
		return iRet;
	else if( iRet == COM_AGAIN )
	{
		if( bBlocking )
			IOError( "Connection time-out" );
		else
			return COM_AGAIN;
	}
	else if( iRet == 0 )
		IOError( "Socket closed" );
	else
		IOError( "Socket error: %s", strError() );
	return 0;
}

inline int ConnectionInterface::sendSmart( const uint8_t* pBuffer, uint32_t iSize )
{
	uint32_t iSend = 0;
	int iVal = 0;
	while( iSend < iSize )
	{
		iVal = send( pBuffer + iSend, iSize - iSend );
		if( iVal > 0 )
			iSend += iVal;
		else if( iVal == COM_AGAIN and bBlocking )
			oPollQuantum.sleep();
		else
			break;
	}
	return evaluateStatus( iVal );
}

inline int ConnectionInterface::sendSmart( const uint8_t* pBuffer, uint32_t iSize, const SirannonTime& oTimeOut )
{
	SirannonTime oInterval = SirannonTime::getCurrentTime();
	uint32_t iSend = 0;
	int iVal = 0;
	while( iSend < iSize and not oInterval.checkInterval( oTimeOut ) )
	{
		iVal = send( pBuffer + iSend, iSize - iSend );
		if( iVal > 0 )
			iSend += iVal;
		else if( iVal == COM_AGAIN and bBlocking )
			oPollQuantum.sleep();
		else
			break;
	}
	return evaluateStatus( iVal );
}

inline int ConnectionInterface::sendfileSmart( int iFile, uint64_t iSize )
{
	uint32_t iSend = 0;
	int iVal = 0;
	while( iSend < iSize )
	{
		iVal = sendfile( iFile, iSize - iSend );
		if( iVal > 0 )
			iSend += iVal;
		else if( iVal == COM_AGAIN and bBlocking )
			oPollQuantum.sleep();
		else
			break;
	}
	return evaluateStatus( iVal );
}

inline int ConnectionInterface::sendfileSmart( int iFile, uint64_t iSize, const SirannonTime& oTimeOut )
{
	SirannonTime oInterval = SirannonTime::getCurrentTime();
	uint32_t iSend = 0;
	int iVal = 0;
	while( iSend < iSize and not oInterval.checkInterval( oTimeOut ) )
	{
		iVal = sendfile( iFile, iSize - iSend );
		if( iVal > 0 )
			iSend += iVal;
		else if( iVal == COM_AGAIN and bBlocking )
			oPollQuantum.sleep();
		else
			break;
	}
	return evaluateStatus( iVal );
}

inline int ConnectionInterface::receiveRaw( uint8_t* pBuffer, uint32_t iSize )
{
	/* Receive one part */
	int iVal;
	while( true )
	{
		iVal = receive( pBuffer, iSize );
		if( iVal == COM_AGAIN and bBlocking )
		{
			oPollQuantum.sleep();
			continue;
		}
		return evaluateStatus( iVal );
	}
}

int ConnectionInterface::receiveRaw( uint8_t* pBuffer, uint32_t iSize, const SirannonTime& oTimeOut )
{
	SirannonTime oInterval = SirannonTime::getCurrentTime();
	int iVal = COM_AGAIN;
	while( not oInterval.checkInterval( oTimeOut ) )
	{
		iVal = receive( pBuffer, iSize );
		if( iVal == COM_AGAIN and bBlocking )
		{
			oPollQuantum.sleep();
			continue;
		}
		break;
	}
	return evaluateStatus( iVal );
}

int ConnectionInterface::receiveSmart( uint8_t* pBuffer, uint32_t iSize )
{
	uint32_t iRcv = 0;
	while( iSize - iRcv > 0 )
	{
		/* Receive one part */
		int iVal = receive( pBuffer + iRcv, iSize - iRcv );
		if( iVal == COM_AGAIN and bBlocking )
		{
			oPollQuantum.sleep();
			continue;
		}
		/* True data? */
		if( iVal > 0 )
			iRcv += iVal;
		else
			return evaluateStatus( iVal );
	}
	return iSize;
}

int ConnectionInterface::receiveSmart( uint8_t* pBuffer, uint32_t iSize, const SirannonTime& oTimeOut )
{
	SirannonTime oInterval = SirannonTime::getCurrentTime();
	uint32_t iRcv = 0;
	int iVal = -1;
	while( iSize - iRcv > 0 and not oInterval.checkInterval( oTimeOut ) )
	{
		/* Receive one part */
		iVal = receive( pBuffer + iRcv, iSize - iRcv );
		if( iVal == COM_AGAIN and bBlocking )
		{
			oPollQuantum.sleep();
			continue;
		}
		/* True data? */
		if( iVal > 0 )
			iRcv += iVal;
		else
			return evaluateStatus( iVal );
	}
	/* All data? */
	return evaluateStatus( iVal );
}
