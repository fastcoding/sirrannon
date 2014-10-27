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
/*
 * h264_avc_priv.h
 *
 *  Created on: 12-jan-2009
 *      Author: arombaut
 */

#ifndef H264_AVC_PRIV_H_
#define H264_AVC_PRIV_H_

inline bool H264_is_SVC( const uint8_t *pBuffer )
{
	uint8_t iType = H264_NAL_type( pBuffer );
	if( iType == NAL_UNIT_TYPE_SLICE_SVC or
		iType == NAL_UNIT_TYPE_PREFIX )
		return true;
	else
		return false;
}

inline int32_t H264_is_start_code( const uint8_t *pBuffer )
{
	if( pBuffer[0] == 0 and pBuffer[1] == 0 )
	{
		if( pBuffer[2] == 1 )
			return 3;
		else if( pBuffer[2] == 0 and pBuffer[3] == 1 )
			return 4;
	}
	return 0;
}

inline int32_t H264_find_NAL( const uint8_t* pBuffer, int iSize )
{
	uint32_t iVal = 0xFFFFFFFF;
	int32_t iPos = H264_is_start_code( pBuffer );

	while( iPos < iSize )
	{
		iVal = ( iVal << 8 ) | pBuffer[iPos++];
		if( ( iVal & 0x00FFFFFF ) == 0x00000001 )
		{
			if( iVal & 0xFF000000 )
				return iPos - 3;
			else
				return iPos - 4;
		}
	}
	return iSize;
}

inline uint8_t H264_NAL_type( const uint8_t* pBuffer )
{
	return pBuffer[ H264_is_start_code(pBuffer) ] & 0x1F;
}

inline uint8_t H264_NAL_idc( const uint8_t* pBuffer )
{
	return ( pBuffer[ H264_is_start_code(pBuffer) ] >> 5 ) & 0x03;
}

inline int32_t H264_header_size( const uint8_t* pBuffer )
{
	int iHeader = 0;
	if( pBuffer[0] == 0 and pBuffer[1] == 0 )
	{
		if( pBuffer[2] == 1 )
			iHeader = 3;
		else if( pBuffer[2] == 0 and pBuffer[3] == 1 )
			iHeader = 4;
	}

	int iType = pBuffer[iHeader] & 0x1F;
	if( iType == NAL_UNIT_TYPE_SLICE_SVC or
		iType == NAL_UNIT_TYPE_PREFIX )
		iHeader += 3;

	return iHeader + 1;
}

#endif /* H264_AVC_PRIV_H_ */
