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
#include "PortManager.h"
#include "SirannonException.h"
#include "SirannonSocket.h"

PortManager oPortManager( 49152, 65534 );

PortManager::PortManager( uint16_t iBase, uint16_t iMax )
	: iBase(iBase), iMax(iMax), iPort(iBase)
{ }

uint16_t PortManager::next()
{
	/* Find the next free ephemeral port pair */
	Lock_t oLock( oMutex );
	while( true )
	{
		try
		{
			/* Non guaranteed solution since we release the ports... So another process
			 * including another copy of sirannon could hijack in the time between "return" and the actual
			 * "bind", however the possibility should be very low. In worst case an IOError will be thrown.
			 */
			UDPSocket oFirst( iPort ), oSecond( iPort + 1 );
			iPort += 2;
			return iPort - 2;
		}
		catch( SirannonException oException )
		{
			iPort += 2;
			if( iPort >= iMax )
				iPort = iBase;
			continue;
		}
	}
}
