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
#ifndef AVC_READER_H
#define AVC_READER_H
#include "Reader.h"
#include "h264_avc.h"
#include "Frame.h"

/*
*	avc-Reader:
*		Reads in an avc-videofile and creates a videopacket for each nal
*
*	Parameters:
*		string filename: the name of the mp4-file, if omitted it waits
* 			for a packet containing a string (default: '')
* 		int xroute: assign this xroute to the packets (default: 100)
*		bool iLoop: jump to the beginning of the video again after reaching the end (default: false)
*
*	Properties:
*		avc-codec dependant
*		transmitter independant
*		single-input
* 		threaded
*/
class AVCReader : public Reader
{
public:
	AVCReader( const string& sName, ProcessorManager* pScope );
	~AVCReader();

protected:
	int iMaxBuffer; /* 256 kB */
	uint8_t*  pBuffer;
	vector<MediaPacket*> vStack;
	int iBound, streamID, iRoute, iSubFrame;
	uint32_t iIndex, iEndIndex;
	uint32_t iUnit, iFrame, iPOC;
	timestamp_t dts, iRefDts, iBaseDts, iCurDts, iLastDts;
	frame_t::type old_type;
	bool bSkipSEI, bSkipAUD, bKey, bMOV;
	NAL_t oH264Decode;
	rational_t oInc;
	MediaDescriptor* pVideoDesc;
	MP4MediaConverter oConvertor;

	virtual void init( void );
	virtual bool doStep( void );
	virtual void prepareBuffer( void );
	virtual void send_frame( void );
	virtual bool classify( MediaPacketPtr& pckt );
	virtual void classify_ctrl( MediaPacketPtr& pckt );
	virtual void printFrame( MediaPacketPtr& pckt );
};
#endif
