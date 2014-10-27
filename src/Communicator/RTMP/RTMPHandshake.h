#ifndef RTMPHANDSHAKE_H_
#define RTMPHANDSHAKE_H_
#include "sirannon.h"

class RTMPHandshake
{
public:
	RTMPHandshake();
	virtual ~RTMPHandshake();

	const uint8_t* generateC1( bool bCrypto );
	const uint8_t* generateS1( const uint8_t* pC1, uint32_t iSize );
	const uint8_t* generateS2( void );
	const uint8_t* generateC2( const uint8_t* pS1, uint32_t iSize );
	bool validateS2( const uint8_t* S2, uint32_t iSize );
	bool validateC2( const uint8_t* C2, uint32_t iSize );
	bool isClientCrypto( void ) const;
	bool isServerCrypto( void ) const;
	const static uint32_t HANDSHAKE_SIZE = 1536;

public:
	int validateC1( const uint8_t* pData, uint32_t iSize );
	bool validateC1Scheme( const uint8_t* pC1, uint32_t iSize, int iTestScheme ) const;
	int validateS1( const uint8_t* pData, uint32_t iSize );
	bool validateS1Scheme( const uint8_t* pC1, uint32_t iSize, int iTestScheme ) const;

	uint32_t getDigestOffset( const uint8_t* pBuffer, int iScheme ) const;
	uint32_t getDigestOffset1( const uint8_t* pBuffer ) const;
	uint32_t getDigestOffset2( const uint8_t* pBuffer ) const;

	uint8_t C1 [HANDSHAKE_SIZE], S1 [HANDSHAKE_SIZE], C2 [HANDSHAKE_SIZE], S2 [HANDSHAKE_SIZE];
	int iClientScheme, iServerScheme;
};

#endif /* RTMPHANDSHAKE_H_ */
