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
#ifndef PES_UNPACKETIZER_H_
#define PES_UNPACKETIZER_H_
#include "sirannon.h"

class PESUnpacketizer : public MediaProcessor
{
public:
	PESUnpacketizer( const string& sName, ProcessorManager* pScope );
	~PESUnpacketizer( void );

protected:
	void receive( MediaPacketPtr& pckt );
	void receive_end( MediaPacketPtr& pckt );
	void receive_reset( MediaPacketPtr& pckt );
	int unpack( void );

	int iUnit, iFrame;
	timestamp_t iLastDTS, iFirstDTS, iBaseDTS;
	deque_t vQueue;
	bool bSync;
};

#endif /*PES_UNPACKETIZER_H_*/
