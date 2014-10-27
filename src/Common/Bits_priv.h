#ifndef BITS_PRIV_H_
#define BITS_PRIV_H_
#include "Bits.h"

inline OBits::OBits( uint8_t* pBytes, size_t iBytes )
	: iBuff(8), bMem(false), iBytes(iBytes)
{
	v0 = v = pBytes;
	if( iBytes )
		v[0] = 0;
}

inline OBits::OBits( size_t iBytes )
	: iBuff(8), bMem(true), iBytes(iBytes)
{
	v0 = v = new uint8_t [iBytes];
	if( iBytes )
		memset( v, 0, iBytes );
}

inline OBits::~OBits( )
{
	if( bMem )
		delete [] v0;
}

inline void OBits::clear( void )
{
	v = v0;
	v[0] = 0;
	iBuff = 8;
}

inline size_t OBits::size( ) const
{
	if( iBuff == 8 )
		return v - v0;
	else
		return v - v0 + 1;
}

inline size_t OBits::size_bits( ) const
{
	return (v - v0 + 1) * 8 - iBuff;
}

inline uint8_t* OBits::data( ) const
{
	return v0;
}

inline void OBits::write_buffer( const uint8_t* pData, size_t iSize )
{
	memcpy( v, pData, iSize );
	v += iSize;
	if( v - v0 < iBytes )
		*v = 0;
}

inline IBits::IBits( void )
	: v(NULL), v0(NULL), v1(0), iBuf(0), iBytes(0), bMem(false)
{ }

inline size_t IBits::rem( void ) const
{
	return ( iBytes  - ( v - v0 ) ) * 8 + iBuf;
}

inline size_t IBits::size ( void ) const
{
	return v - v0;
}

inline uint8_t* IBits::data( void ) const
{
	return v0;
}

inline void IBits::seek( size_t iBytes )
{
	if( aligned() )
	{
		v += iBytes;
	}
	else
	{
		for( int i = 0; i < iBytes; i++ )
			read( 8 );
	}
}

inline int32_t IBits::read_sev( void )
{
	uint32_t iCodeNum = read_uev( );
	if( iCodeNum & 0x1 )
		return ( iCodeNum + 1 ) >> 1;
	else
		return ( 0 - iCodeNum ) >> 1;
}

inline int32_t IBits::peek( uint8_t IBits )
{
	/* Save state */
	uint8_t tmp_v1 = v1, *tmp_v = v;
	uint8_t tmp_iBuf = iBuf;

	/* Get */
	int ret = read( IBits );

	/* Reset state */
	v = tmp_v;
	v1 = tmp_v1;
	iBuf = tmp_iBuf;

	return ret;
}

inline uint8_t* IBits::cur( void ) const
{
	return v;
}

#endif /* BITS_PRIV_H_ */
