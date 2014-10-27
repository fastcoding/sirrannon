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
#ifndef TRANSCODER_AUDIO_H_
#define TRANSCODER_AUDIO_H_
#include "sirannon.h"
#include <signal.h>
#include "ffmpeg.h"
#include "Interfaces.h"
#include "Frame.h"

class TranscoderAudio : public MediaProcessor, public SourceInterface
{
public:
	TranscoderAudio( const string& sName, ProcessorManager* pProc );
	virtual ~TranscoderAudio( void );

	virtual int seek( uint32_t iTimeIndex ) { return -1; }
	virtual bool ready( void ) const;
	virtual bool bufferFull( void ) const;
	virtual int flush( void );

protected:
	virtual void init( void );
	virtual void receive( MediaPacketPtr& pckt );
	virtual void receive_end( MediaPacketPtr& pckt );
	virtual void receive_reset( MediaPacketPtr& pckt );

	virtual void decoder_init( const MediaDescriptor* pDesc );
	virtual void encoder_init( const MediaDescriptor* pDesc );
	virtual void decode_reset( void );
	virtual void encode( MediaPacket* );
	virtual void encode_reset( void );
	virtual void encode_ctrl( MediaPacketPtr& pckt );
	void generateDescriptor( MediaDescriptor* pSourceDesc );

	/* Thread and Queue's */
	uint8_t pEncoderBuffer [256 * 1024];
	int16_t pSamplesNonAligned [196000];
	int16_t pSamplesAlignedBuffer [196000];
	int16_t *pSamplesAligned;
	int iFrame, iUnit, iStream, iSamplesEnd, iSamplesStart, iMaxSamples, iSamplesPerStep;
	int64_t iSamplesTotal;
	int iEncoded, iDecoded;
	timestamp_t iMinDts, iMaxDts, iStartDts;
	bool bMovFrame, bBypass;
	SirannonTime oEncoderStart, oStart, oEncoderEnd;
	MediaDescriptor* pDesc;
	MP4MediaConverter oConverter;
	queue_t vBuffer;

	/* FFmpeg data structures */
	AVCodec *pDecoder, *pEncoder;
	AVCodecContext *pDecoderCtx, *pEncoderCtx;
};

#endif /* TRANSCODER_AUDIO_H_ */
