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
#ifndef COMMUNICATOR_H_
#define COMMUNICATOR_H_
#include "sirannon.h"
#include "Bits.h"
#include "Interfaces.h"
#include "OSSupport.h"

class MediaTransmitter : public MediaProcessor
{
public:
	const static uint32_t HEADER_SIZE = 20;
	const static uint32_t LENGTH_SIZE = 2;

protected:
	MediaTransmitter( const string& sName, ProcessorManager* pScope );

	virtual void init( void );
	int encode_length( uint8_t* header, uint16_t size );
	int encode_length( MediaPacketPtr& pckt );
	int encode_header( const MediaPacketPtr& pckt, uint8_t* pHeader );
	void encode_header( MediaPacketPtr& pckt );

	int iStream, iVideo, iAudio;
	OBits oHeader;
};

class MediaReceiver : public MediaProcessor, public SourceInterface
{
public:
	const static uint32_t LENGTH_SIZE = 2;

protected:
	MediaReceiver( const string& sName, ProcessorManager* pScope );

	virtual void init( void );
	virtual int seek( uint32_t iTimeIndex ) { return -1; }
	virtual bool ready( void ) const { return true; }

	int decode_length( uint8_t* header );
	int decode_header( MediaPacketPtr& pckt, const uint8_t* pHeader );
	void decode_additional( MediaPacketPtr& pckt );
	void decodeDescriptor( MediaPacket* pPckt );

	MediaDescriptor* pDesc;
	int iStream, iVideo, iAudio;
	OBits oHeader;
};

#endif /*COMMUNICATOR_H_*/
