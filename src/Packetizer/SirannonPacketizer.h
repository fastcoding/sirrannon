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
#ifndef XSTREAM_PACKETIZER_H_
#define XSTREAM_PACKETIZER_H_
#include "Packetizer.h"

class SirannonPacketizer : public Packetizer
{
public:
	SirannonPacketizer( const string& sName, ProcessorManager* pScope );
	~SirannonPacketizer();

protected:
	void receive( MediaPacketPtr& pckt );
};

class SirannonUnpacketizer : public MediaProcessor
{
public:
	SirannonUnpacketizer( const string& sName, ProcessorManager* pScope );
	~SirannonUnpacketizer();

protected:
	void init( void );
	void unpack( void );
	void receive( MediaPacketPtr& pckt );
	void receive_end( MediaPacketPtr& pckt );
	void receive_reset( MediaPacketPtr& pckt );

	int iUnit, iFrame, iSubFrame, iNewUnit;
	deque_t vQueue;
	bool bFragment, bSync, bSeverity;
};

inline void SirannonUnpacketizer::receive_end( MediaPacketPtr& pckt )
{
	unpack();
	iUnit = pckt->unitnumber;
	debug( 1, "unpacked %s", pckt->c_str() );
	route( pckt );
}

inline void SirannonUnpacketizer::receive_reset( MediaPacketPtr& pckt )
{
	unpack();
	iUnit = pckt->unitnumber;
	debug( 1, "unpacked %s", pckt->c_str() );
	route( pckt );
}

#endif /* XSTREAM_PACKETIZER_H_ */
