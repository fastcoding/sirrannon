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
#ifndef ENDIANNESS_H_
#define ENDIANNESS_H_
#include "Network.h"

inline uint16_t hton16( uint16_t n )
{
	return htons(n);
}

inline uint16_t ntoh16( uint16_t n )
{
	return ntohs(n);
}

inline uint32_t hton32( uint32_t n )
{
	return htonl(n);
}

inline uint32_t hton32( uint8_t* p )
{
	return htonl(*(uint32_t*)p);
}

inline uint32_t ntoh32( uint32_t n )
{
	return ntohl(n);
}

inline uint64_t hton64( uint64_t n )
{
	return ((uint64_t)hton32( n ) << 32) + (uint64_t)hton32( n>>32 );
}

inline uint64_t ntoh64( uint64_t n )
{
	return ((uint64_t)ntoh32( n ) << 32) + (uint64_t)ntoh32( n>>32 );
}

#endif /* ENDIANNESS_H_ */
