#include "Bits.h"
#include "Utils.h"
#include "SirannonException.h"

void OBits::write( uint8_t iBits, uint32_t iVal )
{
	if( v - v0 >= iBytes )
		OutOfBoundsError( "OBits: write of (%d) bits would exceed buffer (%d bytes)", iBits, iBytes );

	while( iBits > 0 )
	{
		/* Number of bits to write in this step */
		int iWrite = MIN( iBuff, iBits );
		iBits -= iWrite;
		iBuff -= iWrite;

		/* Write to array */
		*v |= ( ( iVal & mask( iWrite, iBits ) ) >> iBits ) << iBuff;

		/* Jump to the next position when the bit buffer is empty */
		if( iBuff == 0 )
		{
			v++;
			if( v - v0 < iBytes )
				*v = 0;
			else if( iBits )
				OutOfBoundsError( "OBits: write of (%d) bits would exceed buffer (%d bytes)", iBits, iBytes );
			iBuff = 8;
		}
	}
}

void OBits::seek( int iDelta, int iMode )
{
	if( iMode == SEEK_SET )
		v = v0 + iDelta;
	else
		v += iDelta;
	iBuff = 8;
}

void OBits::pad( uint8_t iAllign, uint8_t iPattern )
{
	if( iBuff < 8 )
		write( iBuff, iPattern );
	while( (uint64_t) v % iAllign )
		write( 8, iPattern );
}

IBits::IBits( const uint8_t* data, size_t iBytes )
	: v((uint8_t*)data), v0((uint8_t*)data), v1(0), iBuf(0), iBytes(iBytes), bMem(false)
{ }

IBits::IBits( size_t iBytes )
	:  v1(0), iBuf(0), iBytes(iBytes), bMem(true)
{
	v = v0 = new uint8_t [iBytes];
	memset( v, 0, iBytes );
}

IBits::~IBits()
{
	if( bMem )
		delete [] v0;
}

void IBits::assign( uint8_t* pData, size_t iBytes )
{
	if( bMem )
	{
		delete [] v0;
		bMem = false;
	}
	v = v0 = pData;
	this->iBytes = iBytes;
	v1 = 0;
	iBuf = 0;
}

void IBits::clear( void )
{
	v = v0;
	v1 = 0;
	iBuf = 0;
}

bool IBits::aligned( void ) const
{
	return iBuf == 0;
}

uint8_t IBits::read_hex( void )
{
	char sHexa [3] = { read(8), read(8), '\0' };
	return strtoul( sHexa, NULL, 16 );
}

uint32_t IBits::read_uev( void )
{
	/* Table to translate a number into the amount of leading zeroes */
	static const uint8_t leadingZeroes [256] =
	{
		8,
		7,
		6, 6,
		5, 5, 5, 5,
		4, 4, 4, 4, 4, 4, 4, 4,
		3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
		2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	};

	/* Count the number of leading zero bits */
	int iLeadingZerOBits = 0, iZerOBits = 0;
	do
	{
		int iPeek = MIN( rem(), 8 );
		iLeadingZerOBits += iZerOBits = leadingZeroes[ peek(iPeek) << (8-iPeek) ];
		read( iZerOBits );
	} while( iZerOBits == 8 );

	/* Read the result */
	return read( iLeadingZerOBits + 1 ) - 1;
}

uint32_t IBits::read( uint8_t iBits )
{
	int iRet = 0;
	/* Trivial case */
	if( iBits <= 0 )
		return iRet;

	/* Mask */
	int iMask = mask( iBits );

	/* Read the bits */
	if( iBits <= iBuf )
	{
		/* Read from IBits from the remaning iBuf bits in v0 */
		iBuf -= iBits;
		iRet = v1 >> iBuf;
	}
	else
	{
		/* Read the iBuf front bits from v0*/
		iBits -= iBuf;
		if( iBits < 32 )
			iRet |= v1 << iBits;

		/* Read multitudes of 8 */
		while( iBits > 8 )
		{
			iBits -= 8;
			iRet |= *v++ << iBits;
		}

		/* Read the last IBits bits from the new v0 */
		v1 = *v++;
		iBuf = 8 - iBits;
		iRet |= v1 >> iBuf;
	}
	/* Return the masked result */
	return iRet & iMask ;
}

char IBits::read_UTF8( void )
{
	uint8_t c = read( 8 );
	uint8_t f = c & 0xF0;
	if( ( c & 0x80 ) == 0 )
		return c;
	else if( f == 0xC0 )
		return read( 8 );
	else if( f == 0xD0  )
		seek( 1 );
	else if( f == 0xE0 )
		seek( 2 );
	else if( f == 0xF0 )
	{
		f = c & 0x0F;
		if( ( f & 0x80 ) == 0 )
			seek( 3 );
		else
		{
			if( ( f & 0x40 ) == 0 )
				seek( 4 );
			else
			{
				if( ( f & 0x20 ) == 0 )
					seek( 5 );
				else
				{
					seek( 5 );
					ValueError( "invalid UTF-8 value %02hhX", c );
				}
			}
		}
	}
	return '?';
}

void IBits::read_buffer( uint8_t* pDst, size_t iSize )
{
	if( aligned() )
	{
		memcpy( pDst, v, iSize );
		v += iSize;
	}
	else
		RuntimeError( "buffer not aligned when calling read_buffer" );
}

void IBits::find( uint32_t iPattern, uint8_t iBits )
{
	while( rem() >= MAX(iBits,8) )
	{
		if( peek( iBits ) == iPattern )
			break;
		seek( 1 );
	}
}

string IBits::str( size_t iMax ) const
{
	string sRet;
	int iPos1 = MIN( iMax, size() );
	sRet.append( strArray( data(), iPos1 ) );
	sRet.append( ">" );
	int iPos2 = MIN( iMax, iBytes );
	sRet.append( strArray( data() + iPos1, iPos2 - iPos1 ) );
	return sRet;
}

string OBits::str( size_t iMax ) const
{
	string sRet;
	int iPos1 = MIN( iMax, size() );
	sRet.append( strArray( data(), iPos1 ) );
	sRet.append( ">" );
	int iPos2 = MIN( iMax, iBytes );
	sRet.append( strArray( data() + iPos1, iPos2 - iPos1 ) );
	return sRet;
}
