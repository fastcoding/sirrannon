#include "Bits.h"
#include "SirannonException.h"
#include "Base64.h"

static const int8_t fDecoder[256] = {
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 00-0F */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 10-1F */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,  /* 20-2F */
        52,53,54,55,56,57,58,59,60,61,-1,-1,-1,0,-1,-1,  /* 30-3F */
        -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,  /* 40-4F */
        15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,  /* 50-5F */
        -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,  /* 60-6F */
        41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,  /* 70-7F */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 80-8F */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 90-9F */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* A0-AF */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* B0-BF */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* C0-CF */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* D0-DF */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* E0-EF */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1   /* F0-FF */
};

static const uint8_t fEncoder[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char* Base64Encode( const uint8_t* pSource, uint32_t iSize )
{
	/* Create the memory */
	const uint32_t iSteps = iSize / 3;
	const uint32_t iPad = iSize - iSteps * 3;
	const uint32_t iEncoded = 4 * iSteps + (iPad ? 4 : 0) + 1;
	char* pEncoded = new char[iEncoded];

	/* Bit steam utils */
	IBits oSource( pSource, iSize );
	OBits oEncoded( (uint8_t*)pEncoded, iEncoded );

	/* Encoding */
	for( int i = 0; i < iSteps; ++i )
	{
		oEncoded.write( 8, fEncoder[ oSource.read( 6 ) ] );
		oEncoded.write( 8, fEncoder[ oSource.read( 6 ) ] );
		oEncoded.write( 8, fEncoder[ oSource.read( 6 ) ] );
		oEncoded.write( 8, fEncoder[ oSource.read( 6 ) ] );
	}
	/* Padding */
	if( iPad > 0 )
	{
		oEncoded.write( 8, fEncoder[ oSource.read( 6 ) ] );

		if( iPad == 1 )
		{
			oEncoded.write( 8, fEncoder[ oSource.read( 2 ) << 4 ] );
			oEncoded.write( 8, '=' );
		}
		else
		{
			oEncoded.write( 8, fEncoder[ oSource.read( 6 ) ] );
			oEncoded.write( 8, fEncoder[ oSource.read( 4 ) << 2 ] );
		}
		oEncoded.write( 8, '=' );
	}
	oEncoded.write( 8, 0 );
	return pEncoded;
}

uint8_t* Base64Decode( const char* pData, uint32_t& iSize )
{
	/* Create the memory */
	IBits oSource( (const uint8_t*)pData, iSize );
	int iDecoded = iSize * 3 / 4 + 3 + 2;
	uint8_t* pDecoded = new uint8_t[iDecoded];
	OBits oDecoded( pDecoded, iDecoded );

	/* Decode */
	while( oSource.size() < iSize )
	{
		int iKey = oSource.read( 8 );
		int iVal = fDecoder[ iKey ];
		if( iVal < 0 )
			ValueError( "Illegal base64 character(0x%x)", iKey );
		oDecoded.write( 6, iVal );
	}

	/* Remove trailing zeroes */
	iSize = oDecoded.size();
	while( iSize > 0 and pDecoded[iSize-1] == 0 )
		iSize--;

	return pDecoded;
}
