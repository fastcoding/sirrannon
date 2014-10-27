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
#include "RTMPTConnection.h"
#include "Utils.h"
#include "sirannon.h"

RTMPTConnection::RTMPTConnection( void )
	: buffer_in(priv_buffer_in + HTTP_REPLY_SIZE), buffer_out(priv_buffer_out + HTTP_REPLY_SIZE),
	  buffer_in_end(priv_buffer_in + sizeof(priv_buffer_in)),
	  buffer_out_end(priv_buffer_out + sizeof(priv_buffer_out)), terminateExternal(false),
	  terminateInternal(false), active(SirannonTime::getCurrentTime())
{
	offset_in = current_in = buffer_in;
	offset_out = buffer_out;
}

RTMPTConnection::~RTMPTConnection()
{ }

int RTMPTConnection::send( const uint8_t* pData, uint32_t iSize )
{
	/* Simply copy to the out buffer */
	Lock_t oLock( oMutex );
	if( terminateExternal or terminateInternal )
		return -1;
	else if( offset_out + iSize > buffer_out_end )
	{
		SirannonWarning( "RTMPT-connection: output buffer overflow: client is not polling frequently enough" );
		return -1;
	}
	else
	{
		memcpy( offset_out, pData, iSize );
		offset_out += iSize;
		return iSize;
	}
}

int RTMPTConnection::receive( uint8_t* pData, uint32_t iSize )
{
	/* Extract data from the buffer, but sleep if there is no data */
	Lock_t oLock( oMutex );
	uint32_t iCurrent = offset_in - current_in;

	/* Messages can never be split over multiple HTTP messages */
	if( terminateExternal or terminateInternal )
		return -1;
	else if( iCurrent == 0 )
		return -2;
	else
	{
		/* Copy data */
		uint32_t iPayload = min( iSize, iCurrent );
		memcpy( pData, current_in, iPayload );
		current_in += iPayload;
		return iPayload;
	}
}
