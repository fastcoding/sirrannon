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
#ifndef PORTMANAGER_H_
#define PORTMANAGER_H_
#include "sirannon.h"

class PortManager
{
public:
	PortManager( uint16_t iBasePort=49152, uint16_t iMax=65534 );
	uint16_t next();

private:
	uint16_t iBase, iMax, iPort;
	mutex oMutex;
};

extern PortManager oPortManager;

#endif /*PORTMANAGER_H_*/
