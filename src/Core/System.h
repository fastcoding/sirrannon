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
#ifndef SINK_H_
#define SINK_H_
#include "sirannon.h"

class Sink : public MediaProcessor
{
public:
	Sink( const string& sName, ProcessorManager* pScope );

protected:
	bool bVideo, bAudio;
	int iCount;
	void receive_end( MediaPacketPtr& pckt );
};

class TimeOut : public MediaProcessor
{
	public:
	TimeOut( const string& sName, ProcessorManager* pScope );
	void init( void );

	protected:
	SirannonTime iTime, iStep;
	void receive( MediaPacketPtr& pckt );
	void process( void );
};

class discard : public MediaProcessor
{
public:
	discard( const string& sName, ProcessorManager* pScope );

protected:
	void receive( MediaPacketPtr& pckt ) { };
	void receive_end( MediaPacketPtr& pckt ) { };
	void receive_reset( MediaPacketPtr& pckt ) { };
};


#endif /*SINK_H_*/
