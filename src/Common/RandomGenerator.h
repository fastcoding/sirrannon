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
#ifndef RANDOMGENERATOR_H_
#define RANDOMGENERATOR_H_
#include "sirannon.h"

class RandomGenerator
{
public:
	RandomGenerator( int iSeed=-1 );

	uint32_t next( void );
	double frand( double a=0.0, double b=1.0 );
	int irand( int a=0, int b=100 );
	char crand( void );

	int getSeed( void ) const;
	void setSeed( int iSeed );

private:
	static const int iMask = 0x7fffffff;
	uint32_t iState;
};

/* Provide one global randomizer */
extern RandomGenerator oRandom;

/* Inline */
inline uint32_t RandomGenerator::next( void )
{
	return iState = ( iState * 69069 + 5 );
}

inline double RandomGenerator::frand( double a, double b )
{
	double t = next() * 1. / UINT32_MAX;
	return a + t * ( b - a );
}

inline int RandomGenerator::irand( int a, int b )
{
	int t = next();
	return a + ( t % ( b - a ) );
}

#endif /*RANDOMGENERATOR_H_*/
