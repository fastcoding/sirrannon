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
#include "RandomGenerator.h"
#include <time.h>

RandomGenerator oRandom;

RandomGenerator::RandomGenerator( int iSeed )
{
	if( iSeed < 0 )
		iState = time(NULL);
	else
		iState = iSeed;				
}
	
int RandomGenerator::getSeed( void ) const
{
	return iState;
}

void RandomGenerator::setSeed( int iSeed )
{
	iState = iSeed; 
}

char RandomGenerator::crand( void )
{
	uint8_t iVal = next() & 0x3F;
	if( iVal < 10 )
		return (char) (iVal+48);
	else if( iVal < 36 )
		return (char) (iVal-10+65);
	else if( iVal < 62 )
		return (char) (iVal-36+97);
	else
		return 'Z';
}
