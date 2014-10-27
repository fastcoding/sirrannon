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
#ifndef LOGGER_H_
#define LOGGER_H_
#include "sirannon.h"
#include "Utils.h"

class Logger : public MediaProcessor
{
public:
	Logger( const string& sName, ProcessorManager* pScope );
	~Logger();

protected:
	void log( int );
	void init( void );
	void receive( MediaPacketPtr& pckt );
	void receive_reset( MediaPacketPtr& pckt );
	void receive_end( MediaPacketPtr& pckt );

	uint64_t iTotalSize;
	fileLog oLog;
	SirannonTime oStart, oStart2;
	bool bReal, bEnd;
	timestamp_t iFirstTime, iLastTime;
};

#endif /* LOGGER_H_ */
