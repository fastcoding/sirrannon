#ifndef FrameTransformer_H_
#define FrameTransformer_H_
#include "sirannon.h"
#include "Frame.h"

class FrameTransformer : public MediaProcessor
{
public:
	FrameTransformer( const string& sName, ProcessorManager* pScope );
	~FrameTransformer();

	void init( void );

protected:
	void receive( MediaPacketPtr& pPckt );

	bool bToES;
	queue_t oBuffer;

	MP4MediaConverter oConvertor;
};

#endif /* FrameTransformer_H_ */
