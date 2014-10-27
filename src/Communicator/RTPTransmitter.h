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
#ifndef RTP_TRANS_H
#define RTP_TRANS_H
#include "Communicator.h"
#include "Interfaces.h"
#include "Utils.h"

/* Dirty trick to access private members */
#define private public

/* Jrtplib */
#ifndef WIN32
#include <rtpsession.h>
#include <rtpsessionparams.h>
#include <rtpudpv4transmitter.h>
#include <rtpipv4address.h>
#include <rtptimeutilities.h>
using namespace jrtplib;
#else
#include <jrtplib3/rtpsession.h>
#include <jrtplib3/rtpsessionparams.h>
#include <jrtplib3/rtpudpv4transmitter.h>
#include <jrtplib3/rtpipv4address.h>
#include <jrtplib3/rtptimeutilities.h>
#endif

/* Enough dirtyness */
#undef private

const static uint8_t EXT_META = 0x0A;
const static uint8_t EXT_FILLER = 0xFF;

class SirannonRTPTransmitter : public MediaTransmitter, public TransmitterInterface
{
private:
	RTPSession pSession;
	int iPayload, iPrevTiming;
	uint8_t pHeader[128];
	bool bPts, bExtension, bForce;
	int iForce;
	fileLog oTrace;

	void receive( MediaPacketPtr& pckt );
	void receive_reset( MediaPacketPtr& pckt );
	void receive_end( MediaPacketPtr& pckt );
	void process( void );
	int calculate_payload( MediaPacketPtr& pckt );
	uint32_t pckts;

public:
	SirannonRTPTransmitter( const string& sName, ProcessorManager* pScope );

	virtual uint32_t getSequenceNumber( void ) const;
	virtual uint32_t getTimestamp( void ) const;
	virtual uint32_t getPort( void ) const;
	void init( void );
};
#endif
