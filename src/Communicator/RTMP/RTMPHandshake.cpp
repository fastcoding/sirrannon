#include "OSSupport.h"
#include "RTMPHandshake.h"
#include "RandomGenerator.h"
#include "hmac_sha2.h"
#include "SirannonException.h"
#include "RTMPChunkStream.h"
#include "ffmpeg.h"
/*extern "C"
{
#include "libavutil/sha.h"
}

//TODO: Move HMAC code somewhere. Eventually.
#define HMAC_IPAD_VAL 0x36
#define HMAC_OPAD_VAL 0x5C

static void rtmp_calc_digest(const uint8_t *src, int len, int gap,
                             const uint8_t *key, int keylen, uint8_t *dst)
{
    struct AVSHA *sha;
    uint8_t hmac_buf[64+32] = {0};
    int i;

    sha = (AVSHA*) av_mallocz(av_sha_size);

    if (keylen < 64) {
        memcpy(hmac_buf, key, keylen);
    } else {
        av_sha_init(sha, 256);
        av_sha_update(sha,key, keylen);
        av_sha_final(sha, hmac_buf);
    }
    for (i = 0; i < 64; i++)
        hmac_buf[i] ^= HMAC_IPAD_VAL;

    av_sha_init(sha, 256);
    av_sha_update(sha, hmac_buf, 64);
    if (gap <= 0) {
        av_sha_update(sha, src, len);
    } else { //skip 32 bytes used for storing digest
        av_sha_update(sha, src, gap);
        av_sha_update(sha, src + gap + 32, len - gap - 32);
    }
    av_sha_final(sha, hmac_buf + 64);

    for (i = 0; i < 64; i++)
        hmac_buf[i] ^= HMAC_IPAD_VAL ^ HMAC_OPAD_VAL; //reuse XORed key for opad
    av_sha_init(sha, 256);
    av_sha_update(sha, hmac_buf, 64+32);
    av_sha_final(sha, dst);

    av_free(sha);
}*/

static uint8_t GENUINE_FMS_KEY [] = {
	0x47, 0x65, 0x6e, 0x75, 0x69, 0x6e, 0x65, 0x20,
	0x41, 0x64, 0x6f, 0x62, 0x65, 0x20, 0x46, 0x6c,
	0x61, 0x73, 0x68, 0x20, 0x4d, 0x65, 0x64, 0x69,
	0x61, 0x20, 0x53, 0x65, 0x72, 0x76, 0x65, 0x72,
	0x20, 0x30, 0x30, 0x31, // Genuine Adobe Flash Media Server 001
	0xf0, 0xee, 0xc2, 0x4a, 0x80, 0x68, 0xbe, 0xe8,
	0x2e, 0x00, 0xd0, 0xd1, 0x02, 0x9e, 0x7e, 0x57,
	0x6e, 0xec, 0x5d, 0x2d, 0x29, 0x80, 0x6f, 0xab,
	0x93, 0xb8, 0xe6, 0x36, 0xcf, 0xeb, 0x31, 0xae };

//static uint8_t GENUINE_FP_KEY[] = {
//  0x47, 0x65, 0x6E, 0x75, 0x69, 0x6E, 0x65, 0x20, 0x41, 0x64, 0x6F, 0x62,
//    0x65, 0x20, 0x46, 0x6C,
//  0x61, 0x73, 0x68, 0x20, 0x50, 0x6C, 0x61, 0x79, 0x65, 0x72, 0x20, 0x30,
//    0x30, 0x31,			/* Genuine Adobe Flash Player 001 */
//  0xF0, 0xEE,
//  0xC2, 0x4A, 0x80, 0x68, 0xBE, 0xE8, 0x2E, 0x00, 0xD0, 0xD1, 0x02, 0x9E,
//    0x7E, 0x57, 0x6E, 0xEC,
//  0x5D, 0x2D, 0x29, 0x80, 0x6F, 0xAB, 0x93, 0xB8, 0xE6, 0x36, 0xCF, 0xEB,
//    0x31, 0xAE
//};

static const uint8_t GENUINE_FP_KEY[] = {
    'G', 'e', 'n', 'u', 'i', 'n', 'e', ' ', 'A', 'd', 'o', 'b', 'e', ' ',
    'F', 'l', 'a', 's', 'h', ' ', 'P', 'l', 'a', 'y', 'e', 'r', ' ', '0', '0', '1',

    0xF0, 0xEE, 0xC2, 0x4A, 0x80, 0x68, 0xBE, 0xE8, 0x2E, 0x00, 0xD0, 0xD1, 0x02,
    0x9E, 0x7E, 0x57, 0x6E, 0xEC, 0x5D, 0x2D, 0x29, 0x80, 0x6F, 0xAB, 0x93, 0xB8,
    0xE6, 0x36, 0xCF, 0xEB, 0x31, 0xAE
};

RTMPHandshake::RTMPHandshake( )
	: iClientScheme(1), iServerScheme(2)
{ }

RTMPHandshake::~RTMPHandshake()
{ }

bool RTMPHandshake::isClientCrypto( void ) const
{
	return (iClientScheme > 0);
}

bool RTMPHandshake::isServerCrypto( void ) const
{
	return (iServerScheme > 0);
}

const uint8_t* RTMPHandshake::generateC1( bool bCrypto )
{
	/* Determine scheme */
	iClientScheme = 0;
	if( bCrypto )
		iClientScheme = 2;

	/* Generate C1 */
	OBits oBuffer( C1, HANDSHAKE_SIZE );
	oBuffer.write( 32, SirannonTime::getUpTime().convertMsecs() );

	if( iClientScheme > 0 )
	{
		oBuffer.write( 8, 0x80 );
		oBuffer.write( 8, 0x00 );
		oBuffer.write( 8, 0x03 );
		oBuffer.write( 8, 0x02 );
	}
	else
	{
		oBuffer.write( 32, 0 );
	}
	for( uint32_t i = 0; i < HANDSHAKE_SIZE - 8; i++ )
		C1[i+8] = oRandom.next();

	/* Insert a hash at a specific offset */
	if( iClientScheme > 0 )
	{
		/* Join the spliced 1504 bytes and hash and insert into the random sequence */
		int iOffset = getDigestOffset( C1, iClientScheme );
		//rtmp_calc_digest( C1, HANDSHAKE_SIZE, iOffset, (const uint8_t*)GENUINE_FP_KEY, 30, C1 + iOffset );
		uint8_t pLocal [HANDSHAKE_SIZE - 32];
		memcpy( pLocal, C1, iOffset );
		memcpy( pLocal + iOffset, C1 + iOffset + 32, HANDSHAKE_SIZE - 32 - iOffset );
		hmac_sha256( (uint8_t*)GENUINE_FP_KEY, 30, pLocal, HANDSHAKE_SIZE - 32, C1 + iOffset, 32 );
	}
	return C1;
}

int RTMPHandshake::validateC1( const uint8_t* pC1, uint32_t iSize )
{
	/* Copy */
	memcpy( C1, pC1, HANDSHAKE_SIZE );

	/* Determine the scheme */
	if( pC1[4] == 0 )
		iClientScheme = 0;
	else if( validateC1Scheme( pC1, iSize, 2 ) )
		iClientScheme = 2;
	else if( validateC1Scheme( pC1, iSize, 1 ) )
		iClientScheme = 1;
	else
		return -1;
	return iClientScheme;
}

inline bool RTMPHandshake::validateC1Scheme( const uint8_t* pC1, uint32_t iSize, int iTestScheme ) const
{
	int iOffset = 0;
	if( iTestScheme == 1 )
		iOffset = getDigestOffset1( pC1 );
	else
		iOffset = getDigestOffset2( pC1 );

	uint8_t pLocal [HANDSHAKE_SIZE];
	memcpy( pLocal, pC1, iOffset );
	memcpy( pLocal + iOffset, pC1 + iOffset + 32, HANDSHAKE_SIZE - 32 - iOffset );
	hmac_sha256( (uint8_t*)GENUINE_FP_KEY, 30, pLocal, HANDSHAKE_SIZE - 32, pLocal + HANDSHAKE_SIZE - 32, 32 );

	const uint8_t* pHash = pC1 + iOffset;
	const uint8_t* pLocalHash = pLocal + HANDSHAKE_SIZE - 32;
	for( int i = 0; i < 32; i++ )
	{
		if( pHash[i] != pLocalHash[i] )
			return false;
	}
	return true;
}

const uint8_t* RTMPHandshake::generateS1( const uint8_t* pC1, uint32_t iSize )
{
	/* Determine what we will use */
	if( validateC1( pC1, iSize ) < 0 )
		ValueError( "Illegal C1 Handshake bytes" );
	iServerScheme = iClientScheme; // Follow the scheme of the client

	/* First 8 bytes */
	OBits oBuffer( S1, HANDSHAKE_SIZE );
	oBuffer.write( 32, SirannonTime::getUpTime().convertMsecs() );
	if( iServerScheme > 0 )
	{
		S1[4] = 0x03;
		S1[5] = 0x05;
		S1[6] = 0x02;
		S1[7] = 0x01;
	}
	else
	{
		S1[4] = 0x00;
		S1[5] = 0x00;
		S1[6] = 0x00;
		S1[7] = 0x00;
	}
	/* Generate a random sequence */
	for( uint32_t i = 0; i < HANDSHAKE_SIZE - 8; i++ )
		S1[i+8] = oRandom.next();

	/* Insert a hash at a specific offset */
	if( iServerScheme > 0 )
	{
		/* Join the spliced 1504 bytes and hash and insert into the random sequence */
		int iOffset = getDigestOffset( S1, iServerScheme );
		uint8_t pLocal [HANDSHAKE_SIZE - 32];
		memcpy( pLocal, S1, iOffset );
		memcpy( pLocal + iOffset, S1 + iOffset + 32, HANDSHAKE_SIZE - 32 - iOffset );
		hmac_sha256( GENUINE_FMS_KEY, 36, pLocal, HANDSHAKE_SIZE - 32, S1 + iOffset, 32 );
	}
	return S1;
}

int RTMPHandshake::validateS1( const uint8_t* pS1, uint32_t iSize )
{
	/* Copy */
	memcpy( S1, pS1, HANDSHAKE_SIZE );

	/* Determine the scheme */
	if( S1[4] == 0 )
		iServerScheme = 0;
	else if( S1[4] == 1 and S1[5] == 2 and S1[6] == 3 and S1[7] == 4 ) // Hack for RED5
		iServerScheme = 0;
	else if( validateS1Scheme( pS1, iSize, 2 ) )
		iServerScheme = 2;
	else if( validateS1Scheme( pS1, iSize, 1 ) )
		iServerScheme = 1;
	else
		iServerScheme = -1;
	return iServerScheme;
}

inline bool RTMPHandshake::validateS1Scheme( const uint8_t* pS1, uint32_t iSize, int iTestScheme ) const
{
	int iOffset = 0;
	if( iTestScheme == 1 )
		iOffset = getDigestOffset1( pS1 );
	else
		iOffset = getDigestOffset2( pS1 );

	uint8_t pLocal [HANDSHAKE_SIZE];
	memcpy( pLocal, pS1, iOffset );
	memcpy( pLocal + iOffset, pS1 + iOffset + 32, HANDSHAKE_SIZE - 32 - iOffset );
	hmac_sha256( GENUINE_FMS_KEY, 36, pLocal, HANDSHAKE_SIZE - 32, pLocal + HANDSHAKE_SIZE - 32, 32 );

	const uint8_t* pHash = pS1 + iOffset;
	const uint8_t* pLocalHash = pLocal + HANDSHAKE_SIZE - 32;
	for( int i = 0; i < 32; i++ )
	{
		if( pHash[i] != pLocalHash[i] )
			return false;
	}
	return true;
}

const uint8_t* RTMPHandshake::generateS2( void )
{
	/* Scheme was determined by C1 */
	if( iClientScheme > 0 )
	{
		/* Crypto response */
		int iOffset = getDigestOffset( C1, iClientScheme );

		uint8_t oCryptoKey [32];
		memset( oCryptoKey, 0, 32 );

		for( uint32_t i = 0; i < HANDSHAKE_SIZE - 32; i++ )
			S2[i] = oRandom.next();

		hmac_sha256( GENUINE_FMS_KEY, 68, C1 + iOffset, 32, oCryptoKey, 32 );
		hmac_sha256( oCryptoKey, 32, S2, HANDSHAKE_SIZE - 32, S2 + HANDSHAKE_SIZE - 32, 32 );
		return S2;
	}
	else
	{
		/* Response is simply C1 */
		return C1;
	}
}

const uint8_t* RTMPHandshake::generateC2( const uint8_t* pS1, uint32_t iSize )
{
	/* Check S1 */
	if( validateS1( pS1, iSize ) < 0 )
		SirannonWarning( "Illegal S1 Handshake bytes, scheme(%d)", iServerScheme );

	/* Scheme was chosen by us */
	if( iServerScheme > 0 )
	{
		/* Crypto response */
		int iOffset = getDigestOffset( S1, iServerScheme );

		uint8_t oCryptoKey [32];
		memset( oCryptoKey, 0, 32 );

		for( uint32_t i = 0; i < HANDSHAKE_SIZE - 32; i++ )
			C2[i] = oRandom.next();

//		rtmp_calc_digest( S1 + iOffset, 32, 0, GENUINE_FP_KEY, sizeof(GENUINE_FP_KEY), oCryptoKey);
//		rtmp_calc_digest( C2, HANDSHAKE_SIZE - 32, 0, oCryptoKey, 32, C2 + HANDSHAKE_SIZE - 32 );
//		fprintf( stderr, "%d %d\n", iClientScheme, iServerScheme );
		hmac_sha256( (uint8_t*)GENUINE_FP_KEY, sizeof(GENUINE_FP_KEY), S1 + iOffset, 32, oCryptoKey, 32 );
		hmac_sha256( oCryptoKey, 32, C2, HANDSHAKE_SIZE - 32, C2 + HANDSHAKE_SIZE - 32, 32 );
		return C2;
	}
	else
	{
		/* Response is simply S1 */
		return S1;
	}
}

bool RTMPHandshake::validateC2( const uint8_t* C2, uint32_t iSize )
{
	if( iServerScheme > 0 )
	{
		/* Calcute the hash ourselves to verify */
		uint8_t oCryptoKey [32], oHashed [32];
		memset( oHashed, 0, 32 );
		int iOffset = getDigestOffset( S1, iServerScheme );
		hmac_sha256( (uint8_t*)GENUINE_FP_KEY, 62, S1 + iOffset, 32, oCryptoKey, 32 );
		hmac_sha256( oCryptoKey, 32, (uint8_t*)C2, HANDSHAKE_SIZE - 32, oHashed, 32 );

		/* Check if they match */
		for( int i = 0; i < 32; i++ )
		{
			if( oHashed[i] != C2[1504+i] )
				return false;
		}
		return true;
	}
	else
	{
		/* Check if they match */
		for( int i = 8; i < HANDSHAKE_SIZE; i++ )
		{
			if( C2[i] != S1[i] )
				return false;
		}
		return true;
	}
}

bool RTMPHandshake::validateS2( const uint8_t* S2, uint32_t iSize )
{
	if( iClientScheme > 0 )
	{
		/* Calcute the hash ourselves to verify */
		uint8_t oCryptoKey [32], oHashed [32];
		int iOffset = getDigestOffset( C1, iClientScheme );
		hmac_sha256( GENUINE_FMS_KEY, 68, C1 + iOffset, 32, oCryptoKey, 32 );
		hmac_sha256( oCryptoKey, 32, (uint8_t*)S2, HANDSHAKE_SIZE - 32, oHashed, 32 );

		/* Check if they match */
		for( int i = 0; i < 32; i++ )
		{
			if( oHashed[i] != S2[1504+i] )
				return false;
		}
		return true;
	}
	else
	{
		/* Check if they match */
		for( int i = 8; i < HANDSHAKE_SIZE; i++ )
		{
			if( S2[i] != C1[i] )
				return false;
		}
		return true;
	}
}

inline uint32_t RTMPHandshake::getDigestOffset( const uint8_t* pBuffer, int iScheme ) const
{
	uint32_t iOffset = 0;
	switch( iScheme )
	{
	case 1:
		return getDigestOffset1( pBuffer );

	case 2:
		return getDigestOffset2( pBuffer );
	}
	return 0;
}

inline uint32_t RTMPHandshake::getDigestOffset2( const uint8_t* pBuffer ) const
{
	uint32_t iOffset = 0;
	iOffset = (pBuffer[772] & 0x0ff) + (pBuffer[773] & 0x0ff) + (pBuffer[774] & 0x0ff) + (pBuffer[775] & 0x0ff);
	iOffset = iOffset % 728;
	iOffset = iOffset + 776;
	return iOffset;
}

inline uint32_t RTMPHandshake::getDigestOffset1( const uint8_t* pBuffer ) const
{
	uint32_t iOffset = 0;
	iOffset = (pBuffer[8] & 0x0ff) + (pBuffer[9] & 0x0ff) + (pBuffer[10] & 0x0ff) + (pBuffer[11] & 0x0ff);
	iOffset = iOffset % 728;
	iOffset = iOffset + 12;
	return iOffset;
}
