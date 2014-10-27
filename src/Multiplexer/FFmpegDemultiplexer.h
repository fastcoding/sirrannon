#ifndef FFMPEGDEMULTIPLEXER_H_
#define FFMPEGDEMULTIPLEXER_H_
#include "Reader/FFmpegReader.h"

class FFmpegDemultiplexer : public FFmpegReader
{
public:
	FFmpegDemultiplexer( const string& sName, ProcessorManager* pScope );
	virtual void init( void );
	virtual bool ready( void ) const;

protected:
	virtual ~FFmpegDemultiplexer();
	virtual void openBuffer( void );
	virtual void closeBuffer( void );

	static int readPacket0( void* pSelf, uint8_t* pBuffer, int iSize );
	int readPacket( uint8_t* pBuffer, int iSize );
	static int64_t seekPacket0( void* pSelf, int64_t iOffset, int iMode );
	int64_t seekPacket( int64_t iOffset, int iMode );

	virtual void process( void );
	virtual void receive( MediaPacketPtr& pPckt );
	virtual void receive_end( MediaPacketPtr& pPckt );
	virtual void receive_reset( MediaPacketPtr& pPckt );

	queue_t vBuffer;
	uint8_t pBuffer [32*KIBI];
	condition_variable oBufferCondition;
	bool bReady;
	mutex oBufferMutex;
	AVIOContext* pFFMPEGBuffer;
};

#endif /* FFMPEGDEMULTIPLEXER_H_ */
