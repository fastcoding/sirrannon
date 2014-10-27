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
#include "Utils.h"
#include "Network.h"
#include "SirannonTime.h"
#include "SirannonException.h"
#include "Bits.h"
#include <dirent.h>
#include "OSSupport.h"

/* Split a string in parts */
void split( const string& str, vector<string>& tokens, const string& delimiters)
{
    // Skip delimiters at beginning.
    string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    string::size_type pos     = str.find_first_of(delimiters, lastPos);

    while (string::npos != pos || string::npos != lastPos)
    {
        // Found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }
}

fileLog::fileLog( const char* s, const char* mode ) : fd(NULL)
{
	if( fd == NULL and s != NULL )
	{
		if( mode == NULL )
			fd = fopen( s, "a" );
		else
			fd = fopen( s, mode );
		if( not fd )
			IOError( "Could not open file(%s): %s", s, strError() );
	}
}

fileLog::~fileLog( )
{
	if( fd )
		fclose( fd );
}

bool fileLog::open( const char* s, const char* mode  )
{
	if( fd == NULL and s != NULL and strlen(s) )
	{
		if( mode == NULL )
			fd = fopen( s, "w" );
		else
			fd = fopen( s, mode );
		if( not fd )
			IOError( "Could not open file(%s): %s", s, strError() );
	}
	return (fd == NULL);
}

bool fileLog::active( void )
{
	return ( fd != NULL );
}

void fileLog::close( void )
{
	if( fd )
		fclose( fd );
	fd = NULL;
}

void fileLog::write( const char* fmt, ...)
{
	if( not fd )
		return;
	va_list ap;
	va_start(ap, fmt);
	vfprintf( fd, fmt, ap);
	va_end(ap);
}

void fileLog::flush( void )
{
	if( fd )
		fflush( fd );
}

string& expand( string& sIn, int iMax )
{
	int iIn = sIn.length();
	for( int i = iIn; i < iMax; i++ )
		sIn.append( " " );
	return sIn;
}

string strArray( const uint8_t* pData, uint32_t iSize )
{
	string sStr;
	char sTmp [8];
	for( uint32_t i = 0; i < iSize; i++ )
	{
		snprintf( sTmp, sizeof(sTmp), "%02X", (int)pData[i] );
		sStr.append( sTmp );
	}
	return sStr;
}

string path2str( const string& sStr, bool bExtension )
{
	int i = sStr.rfind( '/' );
	if( i == (int) string::npos )
		i = -1;
	int j = sStr.rfind( '.' );
	if( bExtension or j == (int) string::npos or j < i )
		j = sStr.size() + 1;
	return sStr.substr( i+1, j - i - 1 );
}

string dir2dir( const string& _sDir )
{
	string sDir = _sDir;
	int iLast = sDir.size() - 1;
	if( iLast >= 0 )
		if( sDir[iLast] == '\\' or sDir[iLast] == '/')
			sDir.erase( iLast );
	return sDir;
}

int getFileNames( vector<string>& vFiles, const string& _sDir, const string& sExtension )
{
	/* Open a directory */
	string sDir = _sDir;
	if( sDir[sDir.size()-1] == '/' )
		sDir.erase( sDir.size()-1, 1 );
	DIR* pDir = opendir( sDir.c_str() );
	if( not pDir )
		return -1;

	/* Iterate over the directory */
	struct dirent* pEntry;
	while ( ( pEntry = readdir( pDir ) ) )
	{
		/* Match with 264 */
		string sFile = pEntry->d_name;
		if( sFile[0] == '.' )
			continue;
		if( not sExtension.size() or sFile.find( sExtension ) != string::npos )
			vFiles.push_back( sFile );
	}
	closedir( pDir );
	return vFiles.size();
}

string getFileName( const string& sPath )
{
	int i = sPath.rfind( '/' );
	if( i == (int)string::npos )
		i = -1;
	return sPath.substr( i+1, sPath.size() - i - 1 );
}

string replace( const string& sIn, const string& sLabel, const string& sReplace )
{
	string sOut = sIn;
	int iRep = sReplace.size(), iLabel = sLabel.size(), iPos = 0;
	while( ( iPos = sOut.find( sLabel, iPos ) ) != (int)string::npos )
	{
		sOut.replace( iPos, iLabel, sReplace, 0, iRep );
		iPos += iRep;
	}
	return sOut;
}

int system_nonblocking( const char* sCmd )
{
//#ifdef replace
//	int iID = fork();
//	if( iID < 0 )
//	{
//		RuntimeError( this, "could not fork" );
//	}
//	else if( iID == 0 )
//	{
//		int iRet = ::system( sCmd );
//		exit(0);
//	}
//#endif
	return 0;
}

void str_lower_case( char* sText )
{
	int iSize = strlen( sText );
	for( int i = 0; i < iSize; ++i )
		sText[i] = tolower( sText[i] );
}
