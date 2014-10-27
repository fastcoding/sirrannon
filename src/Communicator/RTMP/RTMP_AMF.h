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
#ifndef RTMP_AMF_H_
#define RTMP_AMF_H_
#include "Utils.h"
#include "sirannon.h"
#include "Bits.h"

namespace AMF
{
	enum type
	{
		NUMBER = 0,
		BOOLEAN = 1,
		STRING = 2,
		OBJECT_START = 3,
		MOVIE_CLIP_MARKER = 4,
		AMF_NULL = 5,
		UNDEFINED = 6,
		ARRAY = 8,
		OBJECT_END = 9,
		STRICT_ARRAY = 0xA,
		SWITCH_AMF3= 0x11,
		REFERENCE = 7,
		DATE = 0xB,
		LONG_STRING = 0xC,
		UNSUPPORTED = 0xD,
		XML = 0x0F,
		TYPED_OBJECT = 0x10,

		OBJECT_STRING = 100,
		OBJECT_NUMBER = 101,
		OBJECT_KEY = 102,
		SKIP = 103
	};

	enum version
	{
		AMF0 = 0,
		AMF3 = 3
	};

	namespace AMF_3
	{
		enum type
		{
			UNDEFINED = 0,
			AMF_NULL = 1,
			AMF_FALSE = 2,
			AMF_TRUE = 3,
			INTEGER = 4,
			NUMBER = 5,
			STRING = 6,
			XML = 7,
			DATE = 8,
			ARRAY = 9,
			OBJECT = 0xA
		};
	}

	typedef union
	{
		int64_t i;
		double f;
	} MAY_ALIAS NumberConvert_t;
}

class AMFIBits : public IBits
{
public:
	AMFIBits( int iSize );
	AMFIBits( uint8_t* pData, int iSize );

	string readAMF_String( int iAMF, bool bMarker=true );
	string readAMF_StringLong( int iAMF );
	string readAMF_Key( int iAMF );
	uint64_t readAMF_Number( int iAMF );
	SirannonTime readAMF_Date( int iAMF );
	bool readAMF_Boolean( int iAMF );
	void readAMF_ObjectStart( int iAMF );
	bool readAMF_ObjectEnd( int iAMF );
	void readAMF_Undefined( int iAMF );
	void readAMF_Null( int iAMF );
	int readAMF_Skip( int iAMF );
	int readAMF_ArrayStart( int iAMF );
	int readAMF_StrictArrayStart( int iAMF );
	int readAMF( int iAMF, map<string,uint64_t>& mInt, map<string,string>& mString, map<string,bool>& mBool, const string& sKey = "" );
};

class AMFOBits : public OBits
{
public:
	AMFOBits( int iSize );

	void writeAMF_String( int iAMF, const char* sVal, bool bMarker=true );
	void writeAMF_Key( int iAMF, const char* sVal );
	void writeAMF_Number( int iAMF, uint64_t iVal, bool bMarker=true );
	void writeAMF_Double( int iAMF, double fVal, bool bMarker=true );
	void writeAMF_Boolean( int iAMF, bool bVal );
	void writeAMF_ObjectStart( int iAMF );
	void writeAMF_ObjectEnd( int iAMF );
	void writeAMF_Undefined( int iAMF );
	void writeAMF_Null( int iAMF );
	void writeAMF_ArrayStart( int iAMF, int iLen=0 );
	void writeAMF_StrictArrayStart( int iAMF, int iLen );
	void writeAMF_ObjectString( int iAMF, const char* sVal1, const char* sVal2 );
	void writeAMF_ObjectNumber( int iAMF, const char* sVal1, uint64_t iVal2 );
	void writeAMF_ObjectDouble( int iAMF, const char* sVal1, double fVal2 );
	void writeAMF_ObjectBoolean( int iAMF, const char* sVal1, bool bVal2 );
	void writeAMF_ObjectNull( int iAMF, const char* sVal1 );
	void writeAMF_SwitchToAMF3( int iAMF );
};

#endif // RTMP_AMF_H_
