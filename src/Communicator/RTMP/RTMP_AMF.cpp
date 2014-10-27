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
#include "RTMP_AMF.h"

void AMFError( const char* sError, ... )
{
	va_list oEllipsis;
	va_start( oEllipsis, sError );
	throw SirannonException( "AMFError", sError, oEllipsis );
}

void AMFError( const MediaProcessor* pProc, const char* sError, ... )
{
	va_list oEllipsis;
	va_start( oEllipsis, sError );
	throw SirannonException( "AMFError", pProc, sError, oEllipsis );
}

AMFIBits::AMFIBits( int iBytes )
	: IBits( iBytes )
{ }

AMFIBits::AMFIBits( uint8_t* pData, int iSize )
	: IBits(pData, iSize)
{ }

bool AMFIBits::readAMF_Boolean( int iAMF )
{
	switch( iAMF )
	{
	case AMF::AMF0:
		seek( 1 );
		return read( 8 );

	case AMF::AMF3:
		switch( read ( 8 ) )
		{
		case AMF::AMF_3::AMF_TRUE:
			return true;

		case AMF::AMF_3::AMF_FALSE:
			return false;
		}

	default:
		return false;
	}
}

uint64_t AMFIBits::readAMF_Number( int iAMF )
{
	uint64_t iTmp;
	uint8_t iVal;
	AMF::NumberConvert_t oConv;

	switch( iAMF )
	{
	case AMF::AMF0:
		oConv.i = ntoh64( *(uint64_t*)(data() + size() + 1) );
		seek( 9 );
		return (uint64_t)oConv.f;

	case AMF::AMF3:
		seek( 1 );
		iVal = read( 8 );
		iTmp = iVal & 0x7F;

		if( iVal & 0x80 )
		{
			iTmp <<= 7;
			iVal = read( 8 );
			iTmp |= iVal & 0x7F;

			if( iVal & 0x80 )
			{
				iTmp <<= 7;
				iVal = read( 8 );
				iTmp |= iVal & 0x7F;

				if( iVal & 0x80 )
				{
					iTmp <<= 8;
					iVal = read( 8 );
					iTmp |= iVal;
				}
			}
		}
	default:
		return -1;
	}
}

SirannonTime AMFIBits::readAMF_Date( int iAMF )
{
	AMF::NumberConvert_t oConv;

	switch( iAMF )
	{
	case AMF::AMF0:
		oConv.i = ntoh64( *(uint64_t*)(data() + size() + 1) );
		seek( 11 );
		return SirannonTime( oConv.f / 1000. );

	default:
		return -1;
	}
}

string AMFIBits::readAMF_String( int iAMF, bool bMarker )
{
	int iLen, i;
	string sVal;
	switch( iAMF )
	{
	case AMF::AMF0:
		if( bMarker )
			if( read( 8 ) == AMF::UNDEFINED )
				return "";
		iLen = read( 16 );
		for( i = 0; i < iLen; i++ )
			sVal.append( 1, read_UTF8() );
		return sVal;

	case AMF::AMF3:

	default:
		return "";
	}
}

string AMFIBits::readAMF_StringLong( int iAMF )
{
	int iLen, i;
	string sVal;
	switch( iAMF )
	{
	case AMF::AMF0:
		seek( 1 );
		iLen = read( 32 );
		for( i = 0; i < iLen; i++ )
			sVal.append( 1, read_UTF8() );
		return sVal;

	case AMF::AMF3:

	default:
		return "";
	}
}

string AMFIBits::readAMF_Key( int iAMF )
{
	return readAMF_String( iAMF, false );
}

void AMFIBits::readAMF_Null( int iAMF )
{
	switch( iAMF )
	{
	case AMF::AMF0:
		read( 8 );
		return;

	case AMF::AMF3:

	default:
		return;
	}
}

void AMFIBits::readAMF_Undefined( int iAMF )
{
	switch( iAMF )
	{
	case AMF::AMF0:
		read( 8 );
		return;

	case AMF::AMF3:

	default:
		return;
	}
}

int AMFIBits::readAMF_ArrayStart( int iAMF )
{
	switch( iAMF )
	{
	case AMF::AMF0:
		read( 8 );
		return read( 32 );

	case AMF::AMF3:
		return -1;

	default:
		return -1;
	}
}

int AMFIBits::readAMF_StrictArrayStart( int iAMF )
{
	switch( iAMF )
	{
	case AMF::AMF0:
		read( 8 );
		return read( 32 );

	case AMF::AMF3:
		return -1;

	default:
		return -1;
	}
}

void AMFIBits::readAMF_ObjectStart( int iAMF )
{
	switch( iAMF )
	{
	case AMF::AMF0:
		read( 8 );
		return;

	case AMF::AMF3:

	default:
		return;
	}
}

bool AMFIBits::readAMF_ObjectEnd( int iAMF )
{
	switch( iAMF )
	{
	case AMF::AMF0:
		if( peek( 24 ) == AMF::OBJECT_END )
		{
			read( 24 );
			return true;
		}
		return false;

	case AMF::AMF3:

	default:
		return false;
	}
}

int AMFIBits::readAMF( int iAMF, map<string,uint64_t>& mInt, map<string,string>& mString, map<string,bool>& mBool, const string& sBaseKey )
{
	int iFields = 0;
	int iType = 0;
	string sKey, sObject;
	char sVal [32];

	if( iAMF == AMF::AMF0 )
	{
		switch( iType = peek( 8 ) )
		{
		case AMF::TYPED_OBJECT:
		case AMF::ARRAY:
		case AMF::OBJECT_START:
			/* All associative arrays with different headers */
			if( iType == AMF::OBJECT_START )
				seek( 1 );
			else if(iType == AMF::ARRAY )
				seek( 5 );
			else
			{
				seek( 1 );
				readAMF_String( iAMF, 0 );
			}
			/* Parse the series of key-value pairs */
			while( not readAMF_ObjectEnd( iAMF ) )
			{
				sKey = readAMF_String( iAMF, false );
				if( readAMF( iAMF, mInt, mString, mBool, sKey ) < 0 )
					return -1;
			}
			break;

		case AMF::STRICT_ARRAY:
			seek( 1 );
			iFields = read( 32 );

			/* A fixed number of singletons */
			for( int i = 0; i < iFields; i++ )
			{
				snprintf( sVal, sizeof(sVal), "%06d", i );
				if( readAMF( iAMF, mInt, mString, mBool, sVal ) < 0 )
					return -1;
			}
			break;

		case AMF::BOOLEAN:
			mBool[sBaseKey] = readAMF_Boolean( iAMF );
			break;

		case AMF::NUMBER:
			mInt[sBaseKey] = readAMF_Number( iAMF );
			break;

		case AMF::STRING:
			mString[sBaseKey] = readAMF_String( iAMF );
			break;

		case AMF::DATE:
			mInt[sBaseKey] = readAMF_Date( iAMF ).convertMsecs();
			break;

		case AMF::LONG_STRING:
			mString[sBaseKey] = readAMF_StringLong( iAMF );
			break;

		case AMF::XML:
			mString[sBaseKey] = readAMF_StringLong( iAMF );
			break;

		case AMF::UNDEFINED:
		case AMF::AMF_NULL:
		case AMF::UNSUPPORTED:
			seek( 1 );
			break;

		case AMF::REFERENCE:
			seek( 3 );
			break;

		case AMF::SWITCH_AMF3:
		case AMF::MOVIE_CLIP_MARKER:
		default:
			return -1;
		}
		return iType;
	}
	return -1;
}

int AMFIBits::readAMF_Skip( int iAMF )
{
	map<string,uint64_t> mInt;
	map<string,string> mString;
	map<string,bool> mBool;
	return readAMF( iAMF, mInt, mString, mBool, "value" );
}

AMFOBits::AMFOBits( int iBytes )
	: OBits( iBytes )
{
}

void AMFOBits::writeAMF_Boolean( int iAMF, bool bVal )
{
	switch( iAMF )
	{
	case AMF::AMF0:
		write( 8, AMF::BOOLEAN );
		if( bVal )
			write( 8, 1 );
		else
			write( 8, 0 );
		return;

	case AMF::AMF3:
		if( bVal )
			write( 8, AMF::AMF_3::AMF_TRUE );
		else
			write( 8, AMF::AMF_3::AMF_FALSE );
		return;
	}
}

void AMFOBits::writeAMF_Number( int iAMF, uint64_t iVal, bool bMarker )
{
	AMF::NumberConvert_t oConv;
	uint64_t iMem;

	switch( iAMF )
	{
	case AMF::AMF0:
		if( bMarker )
			write( 8, AMF::NUMBER );

		oConv.f = (double)(int64_t) iVal;
		iMem = hton64( oConv.i );
		write_buffer( (uint8_t*)&iMem, 8 );
		return;

	case AMF::AMF3:
		if( bMarker )
			write( 8, AMF::AMF_3::INTEGER );
		if( iVal < 0x00000080 )
			write( 8, iVal );
		else if( iVal < 0x00004000 )
		{
			write( 8, (iVal >> 7) | 0x80 );
			write( 8, iVal & 0x7F );
		}
		else if( iVal < 0x00200000 )
		{
			write( 8, (iVal >> 14) | 0x80 );
			write( 8, (iVal >> 7) | 0x80 );
			write( 8, iVal & 0x7F );
		}
		else if( iVal < 0x40000000 )
		{
			write( 8, (iVal >> 22) | 0x80 );
			write( 8, (iVal >> 15) | 0x80 );
			write( 8, (iVal >> 8) | 0x80 );
			write( 8, iVal );
		}
		else
			AMFError( "Integer out of bounds" );
		return;
	}
}

void AMFOBits::writeAMF_Double( int iAMF, double fVal, bool bMarker )
{
	uint64_t iTmp;
	AMF::NumberConvert_t oConv;

	switch( iAMF )
	{
	case AMF::AMF0:
		if( bMarker)
			write( 8, AMF::NUMBER );
		oConv.f = fVal;
		iTmp = hton64( oConv.i );
		write_buffer( (uint8_t*)&iTmp, 8 );
		return;

	case AMF::AMF3:
		if( bMarker )
			write( 8, AMF::AMF_3::NUMBER );
		oConv.f = fVal;
		iTmp = hton64( oConv.i );
		write_buffer( (uint8_t*)&iTmp, 8 );
		return;
	}
}

void AMFOBits::writeAMF_String( int iAMF, const char* sVal, bool bMarker )
{
	switch( iAMF )
	{
	case AMF::AMF0:
		if( bMarker)
			write( 8, AMF::STRING );
		write( 16, strlen( sVal ) );
		write_buffer( (uint8_t*) sVal, strlen( sVal ) );
		return;

	case AMF::AMF3:
		if( bMarker )
			write( 8, AMF::AMF_3::STRING );
		writeAMF_Number( AMF::AMF3, (strlen(sVal) << 1) | 1, false );
		write_buffer( (uint8_t*) sVal, strlen( sVal ) );
		return;
	}
}

void AMFOBits::writeAMF_Key( int iAMF, const char* sVal )
{
	writeAMF_String( iAMF, sVal, false );
}

void AMFOBits::writeAMF_Null( int iAMF )
{
	switch( iAMF )
	{
	case AMF::AMF0:
		write( 8, AMF::AMF_NULL );
		return;

	case AMF::AMF3:
		write( 8, AMF::AMF_3::AMF_NULL );
		return;
	}
}

void AMFOBits::writeAMF_Undefined( int iAMF )
{
	switch( iAMF )
	{
	case AMF::AMF0:
		write( 8, AMF::UNDEFINED );
		return;

	case AMF::AMF3:
		write( 8, AMF::AMF_3::UNDEFINED );
		return;
	}
}

void AMFOBits::writeAMF_ArrayStart( int iAMF, int iLen )
{
	switch( iAMF )
	{
	case AMF::AMF0:
		write( 8, AMF::ARRAY );
		write( 32, iLen );
		return;

	case AMF::AMF3:
		return;
	}
}

void AMFOBits::writeAMF_StrictArrayStart( int iAMF, int iLen )
{
	switch( iAMF )
	{
	case AMF::AMF0:
		write( 8, AMF::STRICT_ARRAY );
		write( 32, iLen );
		return;

	case AMF::AMF3:
		return;
	}
}

void AMFOBits::writeAMF_ObjectStart( int iAMF )
{
	switch( iAMF )
	{
	case AMF::AMF0:
		write( 8, AMF::OBJECT_START );
		return;

	case AMF::AMF3:
		write( 8, AMF::AMF_3::OBJECT );
		write( 8, 0xB ); // 0 static members
		write( 8, 1 ); // Empty string
		return;
	}
}

void AMFOBits::writeAMF_ObjectEnd( int iAMF )
{
	switch( iAMF )
	{
	case AMF::AMF0:
		write( 24, AMF::OBJECT_END );
		return;

	case AMF::AMF3:
		write( 8, 1 ); // Empty string
		return;
	}
}

void AMFOBits::writeAMF_ObjectDouble( int iAMF, const char* sVal1, double fVal2 )
{
	writeAMF_Key( iAMF, sVal1 );
	writeAMF_Double( iAMF, fVal2 );
}

void AMFOBits::writeAMF_ObjectString( int iAMF, const char* sVal1, const char* sVal2 )
{
	writeAMF_Key( iAMF, sVal1 );
	writeAMF_String( iAMF, sVal2 );
}

void AMFOBits::writeAMF_ObjectNumber( int iAMF, const char* sVal1, uint64_t iVal2 )
{
	writeAMF_Key( iAMF, sVal1 );
	writeAMF_Number( iAMF, iVal2 );
}

void AMFOBits::writeAMF_ObjectBoolean( int iAMF, const char* sVal1, bool bVal2 )
{
	writeAMF_Key( iAMF, sVal1 );
	writeAMF_Boolean( iAMF, bVal2 );
}

void AMFOBits::writeAMF_ObjectNull( int iAMF, const char* sVal1 )
{
	writeAMF_Key( iAMF, sVal1 );
	writeAMF_Null( iAMF );
}

void AMFOBits::writeAMF_SwitchToAMF3( int iAMF )
{
	write( 8, AMF::SWITCH_AMF3 );
}
