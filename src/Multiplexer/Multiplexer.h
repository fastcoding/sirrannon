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
#ifndef MUXER_H_
#define MUXER_H_
#include "sirannon.h"
#include "Frame.h"

class Multiplexer : public MediaProcessor
{
public:
	Multiplexer( const string& sName, ProcessorManager* pScope );
	virtual ~Multiplexer();

	virtual int flush( void );

protected:
	/* Members */
	typedef struct
	{
		deque_t vQueue;
		int iMarker, iPID, iCC, iOrigSize;
		codec_t::type codec;
		timestamp_t iOrigDTS;
	} muxx_t;
	int streamID, iUnit, iLast, iDelay, iMaxPID, iStreams;
	SirannonTime oStart;
	map<int,muxx_t> mMux;

	/* Methods */
	virtual void init( void );
	virtual void receive( MediaPacketPtr& pckt );
	virtual void receive_end( MediaPacketPtr& pckt );
	virtual void receive_reset( MediaPacketPtr& pckt );
	virtual void process( void );
	virtual bool checkMux( void );
	virtual int selectMux( void );
	virtual MediaPacketPtrRef resetMux( void );
	virtual void printMux( void );
	virtual void mux( MediaPacketPtr& pckt );
};

class UnitMultiplexer : public Multiplexer
{
public:
	UnitMultiplexer( const string& sName, ProcessorManager* pScope );

protected:
	virtual int selectMux( void );
};

#endif /*MUXER_H_*/
