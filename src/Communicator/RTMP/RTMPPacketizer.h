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
#ifndef RTMP_SERVER_H_
#define RTMP_SERVER_H_
#include "Packetizer/Packetizer.h"
#include "Frame.h"
#include "RTMPChunkStream.h"

class RTMPPacketizer : public Packetizer
{
public:
	RTMPPacketizer( const char* sName, ProcessorManager* pProc );
	virtual ~RTMPPacketizer();

	virtual int flush( void );

protected:
	virtual void init( void );
	virtual void receive( MediaPacketPtr& pPckt );
	virtual void pack( MediaPacketPtr& pPckt, uint32_t iSubFrame );
	virtual uint8_t getFlag( const MediaPacketPtr& pPckt );

	OBits oMessageHeader, oChunkHeader, oFlag;
	bool bFirstChunkEver, bReset, bGlobalHeader;
	timestamp_t iOldTimestampDelta, iSummedTime;
	int iSize, iUnit, iChunkSize, iStreamID, iChunkID;
	deque_t vBuffer;
	queue_t vPPS, vSPS;
	MP4MediaConverter oMp4Convertor;
};

#endif /* RTMP_SERVER_H_ */
