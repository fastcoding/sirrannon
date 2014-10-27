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
#ifndef TSMultiplexer_H
#define TSMultiplexer_H
#include "Multiplexer.h"
/*		A ts-packtizer produces nt-packets of around the mtu size filled with H264_several ts-packets
 * 		drawn from the original packet. If the option aggrgate is set it will not send a nt-packet
 * 		untill it is filled completely. If a packet doesnt completely fill one or more packets,
 * 		the packtizer will wait for another packet before sending the nt-pckt.
 * 		This implies that packet might spread be out over 1 or more packets or that H264_several
 * 		packets are present within one nt-packet(hence aggregation).
 *
 * CAVEAT:
 * 		When sending both audio/video or streams with different xroutes through the same
 * 		ts-Packetizer packets of different streams will end up in the same nt-packets. This
 * 		poses normally no problem but you need to be aware of it
 */
/*
*	ts-Packetizer:
*		Reads in videopackets and multiplexes them into transport stream packets
*
*	Parameters:
*		int mtu: the treshold for fragmentation in bytes (default: 1500 bytes)
*
*	Properties:
*		codec independent
*		transmitter independent
*		single-input
*/
class TSMultiplexer : public Multiplexer
{
public:
	TSMultiplexer( const string& sName, ProcessorManager* pScope );
	~TSMultiplexer();

	void init( void );
	void process( void );

protected:
	const static int iSDTInterval = 500;
	const static int iPATInterval = 100;
	const static int iPCRInterval = 20;

	typedef enum { SDT=0, PAT, PMT, PCR, FILL, PES } packet_t;
	typedef struct{ int version, cycle, count, interval; uint16_t PID; uint8_t cc, size; } SI_t;
	int32_t iMTU, iRefStream, audioStream, videoStream, iMuxRate, iSources, iLocalRefStream;
	timestamp_t iPCR, iPCRDelay, iShape;
	OBits oHeader;
	MediaPacketPtr pTSPckt;
	bool bCont, bAggr, bInterleave, bPSIonKey;
	SI_t SI [32];
	deque_t vOut;

	void pushSDT( MediaPacket* pckt );
	void pushPMT( MediaPacket* pckt );
	void pushPAT( MediaPacket* pckt );
	void pushFILL( MediaPacket* pckt );
	uint32_t sizePMT( void );
	uint32_t sizeSDT( void );
	static const char* strType( packet_t iType );
	bool step( int iStream );
	int createTSPacket( muxx_t* pMux, packet_t iType, bool bStart, bool bPCR );
	void setPCR( MediaPacket* ts_pckt, int32_t pcr );
	bool flagPCR( const MediaPacket* ts_pckt ) const;
	void fillTSHeader( MediaPacket* ts_pckt, int iSize, int iPID, int iCount, bool do_start, bool do_pcr, bool bDoRandom );
	void findRefStream( void );
	void mux( MediaPacketPtr& pckt );
};

#endif
