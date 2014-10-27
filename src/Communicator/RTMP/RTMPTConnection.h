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
#ifndef RTMPT_CONNECTION_H_
#define RTMPT_CONNECTION_H_
#include "sirannon.h"
#include "ConnectionInterface.h"
#include "Boost.h"

static const uint32_t MAX_HTTP_SIZE = 16*1024*1024;
static const uint32_t HTTP_REPLY_SIZE = 1024;

class RTMPTConnection : public ConnectionInterface
{
public:
	RTMPTConnection();
	virtual ~RTMPTConnection();

	virtual int send( const uint8_t* pData, uint32_t iSize );
	virtual int receive( uint8_t* pData, uint32_t iSize );

public:
	typedef enum { INVALID, OPEN, IDLE, SEND, HAIL, CLOSE } mode_t;

public:
	uint8_t* const buffer_in;
	uint8_t* const buffer_out;
	uint8_t* const buffer_in_end;
	uint8_t* const buffer_out_end;
	uint8_t* offset_in;
	uint8_t* current_in;
	uint8_t* offset_out;
	string hash;
	mutex oMutex;
	condition_variable_any oCondition;
	bool terminateExternal, terminateInternal;
	SirannonTime active;

private:
	uint8_t priv_buffer_out [MAX_HTTP_SIZE];
	uint8_t priv_buffer_in [MAX_HTTP_SIZE];
};

#endif /* RTMPT_CONNECTION_H_ */
