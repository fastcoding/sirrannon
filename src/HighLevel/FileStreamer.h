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
#ifndef STREAMER_H_
#define STREAMER_H_
#include "Block.h"
#include "Interfaces.h"

class FileStreamer : public Block, public StreamerInterface
{
public:
	FileStreamer( const string& sName, ProcessorManager* pScope );

	virtual const ContainerDescriptor* getDescriptor( void ) const;
	virtual void createSource( void );
	virtual void createBuffer( void );
	virtual void createTransmitter( MediaDescriptor* pDesc = NULL );
	virtual bool ready( void ) const;
	virtual int play( double fSpeed );
	virtual int pause( void );
	virtual int seek( uint32_t iTimeIndex );
	virtual void process( void );

protected:
	virtual void init( void );

	bool bVideo, bAudio;
};

#endif /*STREAMER_H_*/
