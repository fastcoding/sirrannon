#ifndef AMRPACKETIZER_H_
#define AMRPACKETIZER_H_
#include "Packetizer.h"

class AMRPacketizer : public Packetizer
{
public:
	AMRPacketizer( const string& sName, ProcessorManager* pScope );
	void init( void );

protected:
	~AMRPacketizer();

	void pack( void );
	void receive( MediaPacketPtr& pPckt );

	static int encodedBits( const MediaPacket* pPckt );

protected:
	vector_t vBuffer;
	int iPack, iMaxDuration;
};

#endif /* AMRPACKETIZER_H_ */
