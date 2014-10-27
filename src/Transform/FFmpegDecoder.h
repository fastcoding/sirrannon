#ifndef FFMPEGDECODER_H_
#define FFMPEGDECODER_H_
#include "sirannon.h"
#include "Interfaces.h"
#include "ffmpeg.h"
#include "Frame.h"

class FFmpegDecoder : public MediaProcessor, public SourceInterface
{
public:
	FFmpegDecoder( const string& sName, ProcessorManager* pScope );
	~FFmpegDecoder();

	int seek( uint32_t iTimeIndex ) { return -1; }
	bool ready( void ) const { return true; }

	void init( void );
	void receive( MediaPacketPtr& pPckt );
	void receive_end( MediaPacketPtr& pPckt );
	void receive_reset( MediaPacketPtr& pPckt );
	void output( void );
	void outputControl( packet_t::type iType );
	void decoder_init( MediaPacket* pPckt );
	void decoder_reset();

protected:
	int iFrames, iFrame, iDecoded, iStreamID, iUnit, iSize, iLast;
	bool bReset, bCopy;
	MediaDescriptor* pDesc;
	SirannonTime oStart;
	MP4MediaConverter oMP4;

	/* FFmpeg data structures */
	AVFrame *pFrame;
	AVCodec *pDecoder;
	AVCodecContext *pDecoderCtx;
};

#endif /* FFMPEGDECODER_H_ */
