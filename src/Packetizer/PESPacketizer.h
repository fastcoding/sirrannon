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
#ifndef PES_H_
#define PES_H_
#include "Packetizer.h"
#include "Bits.h"

class PESPacketizer : public Packetizer
{
public:
	PESPacketizer( const string& sName, ProcessorManager* pScope );
	~PESPacketizer();

	void init( void );

protected:
	/* Attributes */
	deque_t oBuffer, oSend;
	int iSize, iMaxSize, iDelta, iAudio;
	bool bAUD, bZero;
	OBits oHeader;
	timestamp_t iFirst, iLast;

	/* Methods */
	void receive( MediaPacketPtr& pPckt );
	void receive_reset( MediaPacketPtr& pPckt );
	void receive_end( MediaPacketPtr& pPckt );
	size_t createHeader( MediaPacket* pPckt, size_t iPayload );
};

#endif /*PES_H_*/
