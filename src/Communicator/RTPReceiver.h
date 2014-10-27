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
#ifndef RTP_RECEIVE_H
#define RTP_RECEIVE_H
#include "Communicator.h"
#include "RTPTransmitter.h"

/* Jrtplib */
#ifndef WIN32
#include <rtpsession.h>
#include <rtpsessionparams.h>
#include <rtpudpv4transmitter.h>
#include <rtpipv4address.h>
#include <rtpaddress.h>
#include <rtptimeutilities.h>
#include <rtppacket.h>
#include <rtpsourcedata.h>
using namespace jrtplib;
#else
#include <jrtplib3/rtpsession.h>
#include <jrtplib3/rtpsessionparams.h>
#include <jrtplib3/rtpudpv4transmitter.h>
#include <jrtplib3/rtpipv4address.h>
#include <jrtplib3/rtpaddress.h>
#include <jrtplib3/rtptimeutilities.h>
#include <jrtplib3/rtppacket.h>
#include <jrtplib3/rtpsourcedata.h>
#endif

class RTPReceiver : public MediaReceiver
{
public:
	RTPReceiver( const string& sName, ProcessorManager* pScope );
	~RTPReceiver();

protected:
	RTPSession					session;
	RTPSessionParams			sessionparams;
	RTPUDPv4TransmissionParams	transparams;
	fileLog oTrace;
	int iFrame, iSubFrame, iFirstUnit;
	timestamp_t iLastDts, iFirstDts, iBaseDts;
	bool bFirst;

	void process( void );
	void init( void );
};

#endif
