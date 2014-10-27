#ifndef AC3PACKETIZER_H_
#define AC3PACKETIZER_H_
#include "Packetizer.h"

class AC3Packetizer : public Packetizer
{
public:
	AC3Packetizer( const string& sName, ProcessorManager* pScope );
	void init( void );

protected:
	~AC3Packetizer();

	void receive( MediaPacketPtr& pPckt );
	void fillHeader( MediaPacket* pPckt, int iFrags, bool bFirst, bool bFiveEights );

	bool bDraft;
};

#endif /* AC3PACKETIZER_H_ */
