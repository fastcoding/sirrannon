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
#ifndef TRANSCODER_H_
#define TRANSCODER_H_
#include "sirannon.h"
#include "SirannonSocket.h"
#include "Interfaces.h"
#include "Frame.h"
#include <signal.h>
#include "ffmpeg.h"
#include "h264_avc.h"

class TranscoderVideo : public MediaProcessor, public SourceInterface, public RateController
{
public:
	TranscoderVideo( const string& sName, ProcessorManager* pProc );
	virtual ~TranscoderVideo( void );

	virtual int seek( uint32_t iTimeIndex ) { return -1; }
	virtual bool ready( void ) const;
	virtual int flush( void );
	virtual bool bufferFull( void ) const;

protected:
	virtual void init( void );
	virtual void receive( MediaPacketPtr& pckt );
	virtual void receive_end( MediaPacketPtr& pckt );
	virtual void receive_reset( MediaPacketPtr& pckt );

	virtual void decoder_init( const MediaDescriptor* pDesc, bool bSourceMov=false );
	virtual void encoder_init( const MediaDescriptor* pDesc );
	virtual void decode_reset( bool bFinal=false );
	virtual void encode( MediaPacket* pSourcePckt );
	virtual void encode_reset( bool bFinal=false );
	virtual void encode_ctrl( MediaPacketPtr& pckt );

	/* Thread and Queue's */
	static const int iEncoderBuffer = 256 * 1024;
	uint8_t pEncoderBuffer [iEncoderBuffer];
	int iUnit, iFrame, iStream;
	int iW, iH, oW, oH;
	int iEncoded, iDecoded, iFrames;
	timestamp_t iEncodedDts, iMaxDts, iMinDts, iStartDts;
	bool bMovFrame;
	SirannonTime oEncoderStart;
	NAL_t oH264Decode;
	MediaDescriptor* pDesc;
	MP4MediaConverter oConverter;
	queue_t vBuffer;
	target_t::type iTarget;
	queue<int> vEncodeDTS, vDecodeDTS;
	deque_t vBuffer2;
	rational_t iReduce; int iFrac;
	double fAdaptiveFrameRate;

	/* FFmpeg data structures */
	struct SwsContext* pResampleCtx;
	AVFrame *pFrame, *pResampledFrame;
	AVCodec *pDecoder, *pEncoder;
	AVCodecContext *pDecoderCtx, *pEncoderCtx;
};

#endif /* TRANSCODER_H_ */
