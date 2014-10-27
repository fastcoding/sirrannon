/*****************************************************************************
 * (c)2006-2009 xTranscoderSession
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
#ifndef TranscoderSession_H_
#define TranscoderSession_H_
#include "FileStreamer.h"

class TranscoderSession : public FileStreamer
{
	public:
	TranscoderSession( const string& sName, ProcessorManager* pScope );
	const ContainerDescriptor* getDescriptor( void ) const;

	int play( double fSpeed=-1.0 );
	int pause( void );
	int seek( uint32_t iTimeIndex );

	protected:
	void init( void );

};

#endif /*TranscoderSession_H_*/

