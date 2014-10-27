#ifndef MP2UNPACKETIZER_H_
#define MP2UNPACKETIZER_H_
#include "Unpacketizer.h"
#include "Bits.h"

class MP2Unpacketizer : public Unpacketizer
{
public:
	MP2Unpacketizer( const string& sName, ProcessorManager* pScope );

protected:
	virtual ~MP2Unpacketizer();

	void unpack( void );
	void receive( MediaPacketPtr& pPckt );

	IBits oHeader;
};

#endif /* MP2UNPACKETIZER_H_ */
