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
#ifndef MEDIA_PACKET_PRIV_H_
#define MEDIA_PACKET_PRIV_H_
#include "MediaPacket.h"
#include "SirannonException.h"
class SirannonException;

inline void MediaPacket::push1_back( uint8_t x )
{
	push_back( &x, 1 );
}

inline void MediaPacket::push1_front( uint8_t x )
{
	push_front( &x, 1 );
}	
	
inline void MediaPacket::push_front( const uint8_t* buffer, uint32_t len )
{
	/* Check */
 	if( begin - len < _data )
 		OutOfBoundsError( "MediaPacket underflow" );
 	
 	/* Copy */
 	begin -= len;
 	memcpy( begin, buffer, len );
}

inline void MediaPacket::push_front( uint32_t len )
{
	/* Check */
 	if( begin - len < _data )
 		OutOfBoundsError( "MediaPacket underflow" );
 	begin -= len;
}

inline void MediaPacket::push_back( const uint8_t* buffer, uint32_t len )
{
 	/* Check */
 	if( end + len >= _data + max_size )
 		OutOfBoundsError( "MediaPacket overflow" );
 	
 	/* Copy */
 	memcpy( end, buffer, len );
 	end += len;
}

inline void MediaPacket::push_back( uint32_t len )
{
 	/* Check */
 	if( end + len >= _data + max_size )
 		OutOfBoundsError( "MediaPacket overflow" );
 	end += len;
}

inline void MediaPacket::pop_front( uint32_t len )
{
	/* Check */
	if( begin + len > end )
		OutOfBoundsError( "Cannot pop an empty packet" );
 	
	/* Pop */
	begin += len;	
}

inline void MediaPacket::pop_back( uint32_t len )
{
	/* Check */
	if( end - len < begin ) 
		OutOfBoundsError( "Cannot pop an empty packet" );

	/* Pop */
 	end -= len;
}

inline uint8_t* MediaPacket::data( void ) const
{
 	return begin;
}

inline uint32_t MediaPacket::size( void ) const
{
	return end - begin;
}

inline void MediaPacket::c_alloc( void )
{
	if( not sCstr )
		sCstr = new char [MAX_STRING_SIZE];
	sCstr[0] = 0;
}
 
#endif /*MEDIA_PACKET_PRIV_H_*/
