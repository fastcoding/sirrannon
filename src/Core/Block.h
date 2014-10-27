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
#ifndef BLOCK_H_
#define BLOCK_H_
#include "sirannon.h"
#include "Interfaces.h"

class Block : public MediaProcessor, public NestedInterface
{
public:
	Block( const string& sName, ProcessorManager* pScope );

	/* Activators */
	virtual void init( void );
	virtual int flush( void );
	virtual void process( void );

	/* Exceptions */
	virtual void handleError( SirannonException* pException, ProcessorManager* pManager, MediaProcessor* pProcessor );

protected:
	/* Patch input through */
	void setRoute( uint32_t iRoute, MediaProcessor* pProc );
	virtual void initOut( void );
	virtual void receive( MediaPacketPtr& pckt );
	virtual	void receive_end( MediaPacketPtr& pckt );
	virtual void receive_reset( MediaPacketPtr& pckt );

	/* In & out */
	MediaProcessor *in, *out;
};

#endif /*BLOCK_H_*/
