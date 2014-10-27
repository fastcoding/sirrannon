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
#ifndef PACKETIZER_H_
#define PACKETIZER_H_
#include "sirannon.h"
#include "Utils.h"

class Packetizer : public MediaProcessor
{
protected:
	Packetizer( const string& name, ProcessorManager* pManager );

	void trace( MediaPacketPtr& pckt, int iParts );
	void send( MediaPacketPtr& pPckt );

	virtual void receive_end( MediaPacketPtr& pPckt );
	virtual void receive_reset( MediaPacketPtr& pPckt );

	virtual void init( void );
	virtual void pack( void );

	fileLog oTrace;
	bool bTracePTS;
	int iUnit, iMTU;
};

#endif /* PACKETIZER_H_ */
