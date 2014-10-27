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
#ifndef PROCESSORMANAGER_PRIV_H_
#define PROCESSORMANAGER_PRIV_H_
#include "ProcessorManager.h"

/* Global seed, quantum & simulation  (unique definition) */
extern int iSeed, iVerbose;
extern SirannonTime oQuantum, oSimulation;

inline const string& ProcessorManager::str( void ) const
{
	return sParent;
}

inline const char* ProcessorManager::c_str( void ) const
{
	return str().c_str();
}

inline int ProcessorManager::getSeed( void ) const
{
	return iSeed;
}

inline const SirannonTime& ProcessorManager::getQuantum( void ) const
{
	return oQuantum;
}

inline const SirannonTime& ProcessorManager::getSimulation( void ) const
{
	return oSimulation;
}

inline int ProcessorManager::getVerbose( void ) const
{
	return iVerbose;
}

inline void ProcessorManager::setSeed( int x )
{
	if( not pParent )
		iSeed = x;
}

inline void ProcessorManager::setQuantum( const SirannonTime& _oQuantum )
{
	if( not pParent )
		oQuantum = _oQuantum;
}

inline void ProcessorManager::setSimulation( const SirannonTime& _oSimulation )
{
	if( not pParent )
		oSimulation = _oSimulation;
}

inline void ProcessorManager::setVerbose( int x )
{
	if( not pParent )
		iVerbose = x;
}

inline int ProcessorManager::getXMLErrors( void ) const
{
	return iErrors;
}

inline ProcessorManager* ProcessorManager::getParent( void ) const
{
	return pParent;
}

#endif /*PROCESSORMANAGER_PRIV_H_*/
