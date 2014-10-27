/*****************************************************************************
 * (c)2006-2010 Sirannon
 * Authors: Alexis Rombaut <alexis.rombaut@intec.ugent.be>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *****************************************************************************/
#ifndef UTILS_H
#define UTILS_H
#include "Network.h"
#include "sirannon.h"
#include "Endianness.h"
#include <vector>
#include <string>
#include <map>


/* Split a string in parts */
void split( const string& str, vector<string>& tokens, const string& delimiters);

/* Min & Max */
#undef MIN
#define MIN(x,y) ((x)<(y)?(x):(y))
#undef MAX
#define MAX(x,y) ((x)>(y)?(x):(y))

/* Masks */
inline int mask( int IBits, int iShift=0 )
{
	if( IBits >= 32 )
		return 0xFFFFFFFF;
	else
		return ( ( 1 << IBits ) - 1 ) << iShift;
}

/* Logging (files) */
class fileLog
{
public:
	explicit fileLog( const char* s=NULL, const char* mode=NULL );
	~fileLog();

	bool open( const char* s, const char* mode=NULL );
	bool active( void );
	void close( void );
	void write( const char* fmt, ... );
	void flush( void );

private:
	fileLog( const fileLog& );
	FILE* fd;
};

/* Acquire the keys of a map */
template< typename key_t, typename ele_t >
void get_keys( map<key_t,ele_t>& maps, vector<key_t>& keys )
{
	for( typename map<key_t,ele_t>::iterator i = maps.begin(); i != maps.end(); i++ )
	{
		keys.push_back( i->first );
	}
}

/* Acquire the keys of a map */
template< typename key_t, typename val_t >
void get_values( map<key_t,val_t>& maps, vector<val_t>& values )
{
	for( typename map<key_t,val_t>::iterator i = maps.begin(); i != maps.end(); i++ )
	{
		values.push_back( i->second );
	}
}

/* Random numbers */
inline double frand( double a=0.0, double b=1.0 )
{
	double t = rand() * 1. / RAND_MAX;
	return a + t * ( b - a );
}

inline int irand( int a=0, int b=100 )
{
	int t = rand();
	return a + ( t % ( b - a ) );
}

/* Make the string have a fixed size by appending spaces */
string& expand( string& sIn, int iMax );

/* Replace parts of a string */
string replace( const string& sIn, const string& sLabel, const string& sReplace );

/* Print an array in a hex editor style represenation */
string strArray( const uint8_t* pData, uint32_t iSize );

/* Truncate a pathname to the filename without extension
 * dat/xml/test.xml -> test */
string path2str( const string& sStr, bool bExtension=true );

/* Get the file names in a folder */
int getFileNames( vector<string>& vFiles, const string& sDir, const string& sExtension="" );
string getFileName( const string& sPath );

/* Remove the trailing / or \\ from a string */
string dir2dir( const string& sDir );

/* Execute a command without blocking */
int system_nonblocking( const char* sCmd );

/* Startcodes */
inline int find_next_start_code( const uint8_t* pData, int iSize, int iStartcode )
{
	int iVal = 0xFFFFFFFF, iPos = 0;
	while( true )
	{
		iVal <<= 8;
		iVal |= pData[iPos++];
		if( iVal == iStartcode )
			return iPos - 4;
		if( iPos > iSize )
			return -1;
	}
}

/* Align a buffer */
inline uint8_t* mem_align( uint8_t* pBuffer, int iAlign )
{
	uint8_t* pAlign = pBuffer - (int64_t) pBuffer % iAlign;
	if( pAlign != pBuffer )
		pAlign += iAlign;
	return pAlign;
}

inline uint32_t hash( void* pObject )
{
	return (uint64_t) pObject;
}

/* Lower caps */
void str_lower_case( char* sText );

#endif /* UTILS_H */
