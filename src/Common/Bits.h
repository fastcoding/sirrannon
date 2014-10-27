#ifndef BITS_H_
#define BITS_H_
#include "sirannon.h"
#include <stdio.h>

/* Writing bitstreams */
class OBits
{
public:
	/**
	 * Constructor
	 * @param Size of the memory to dynamically allocate
	 */
	explicit OBits( size_t iBytes );

	/**
	 * Constructor
	 * @param Pointer to the buffer to write to
	 * @param Size of the buffer to write to
	 */
	OBits( uint8_t* pBytes, size_t iBytes );

	/**
	 * Destructor
	 */
	virtual ~OBits();

	/**
	 * Write bits to the buffer
	 * @param Number of bits to write
	 * @param Value to write
	 */
	void write( uint8_t iBits, uint32_t iVal );

	/**
	 * Write padding bits into the buffer until the bit size is a multiple of 8
	 * @param Pad until the buffer is iAllign-bytes alligned (default: 1)
	 * @param Pattern to write as padding bits (default: 0xFF)
	 */
	void pad( uint8_t iAllign = 1, uint8_t iPattern = 0xFF );

	/**
	 * Writes an array to the buffer, the current write position must be bytes aligned
	 * @param Pointer to the array
	 * @param Number of bytes to write
	 */
	void write_buffer( const uint8_t* pData, size_t iSize );

	/**
	 * @return How many bytes or bits written so far (ceiled: 9 bits is 2 bytes)
	 */
	size_t size( void ) const;
	size_t size_bits( void ) const;

	/**
	 * @return Pointer to the buffer
	 */
	uint8_t* data( void ) const;

	/**
	 * Reset write location to the start of the buffer
	 */
	void clear( void );

	/**
	 * Seek by iDelta absolute (SEEK_SET) or relatively(SEEK_CUR)
	 */
	void seek( int iDelta, int iMode = SEEK_SET );

	/**
	 * Return a hex string of the buffer
	 * @param Maximum number of data bytes to print
	 * @return The string
	 */
	string str( size_t iMax ) const;

	/**
	 * Assign a new area where to write data to, resets the internal state
	 */
	void assign( uint8_t* pData, size_t iBytes );

private:
	OBits();
	OBits( const OBits& );
	OBits operator=( const OBits& );
	uint8_t iBuff;
	uint32_t iBytes;
	uint8_t *v, *v0;
	bool bMem;
};


class IBits
{
public:
	/**
	 * Constructor
	 * @param Pointer of the buffer to read
	 * @param Size of the buffer to read
	 */
	IBits( const uint8_t* data, size_t iBytes );

	/**
	 * Constructor
	 * @param Size of the buffer to dynamically allocate
	 */
	IBits( size_t iBytes );

	/**
	 * Default Constructor
	 * Use assign to set the buffer
	 */
	IBits( void );

	~IBits( );

	uint32_t read( uint8_t IBits );
	uint32_t read_uev( void );
	int32_t read_sev( void );
	char read_UTF8( void );
	void read_buffer( uint8_t* pDst, size_t iSize );
	uint8_t read_hex( void );
	int32_t peek( uint8_t IBits );

	void seek( size_t iBytes );

	void assign( uint8_t* pData, size_t iBytes );
	size_t size( void ) const;
	uint8_t* data( void ) const;
	bool aligned( void ) const;
	size_t rem( void ) const;
	uint8_t* cur( void ) const;
	void clear( void );
	string str( size_t iMax ) const;
	void find( uint32_t iPattern, uint8_t iBits );

private:
	IBits( const IBits& );
	IBits operator=( const IBits& );
	uint8_t *v, *v0, v1, iBuf;
	int iBytes;
	bool bMem;
};

#include "Bits_priv.h"

#endif /* BITS_H_ */
