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
#ifndef AVC_UNPACKETIZER_H
#define AVC_UNPACKETIZER_H
#include "Unpacketizer.h"
#include "h264_avc.h"

/*
*	avc-unPacketizer:
*		Rejoins fragmented avc-packets into the orignal nal units.
*
*	Parameters:
*		bool startcodes: if true add startcodes to the nals again (default: true)
*	
*	Properties:
*		avc-codec dependent
*		transmitter independent
*		single-input 		
*/
class AVCUnpacketizer : public Unpacketizer
{
public:
	AVCUnpacketizer( const string& sName, ProcessorManager* pScope );
	~AVCUnpacketizer();
	
protected:
	bool bStartcodes, bConformAnnexB;
	NAL_t oNAL;

	void init( void );
	void unpack( void );
	void unpackFU( uint32_t indicator );
	void unpackSTAPA( void );
	void startcodes( MediaPacketPtr& pckt );
	void receive( MediaPacketPtr& pckt );
};

#endif
