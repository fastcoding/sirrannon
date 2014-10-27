#ifndef AMR_WBP_PACKETIZER_H_
#define AMR_WBP_PACKETIZER_H_
#include "Packetizer.h"

class AMR_WBP_Packetizer : public Packetizer
{
public:
	AMR_WBP_Packetizer( const string& sName, ProcessorManager* pScope );
	~AMR_WBP_Packetizer();

protected:
	void receive( MediaPacketPtr& pPckt );
	void pack( void );

	vector_t vBuffer;
	int iPack, iInternalSampleRate;
};

#endif /* AMR_WBP_PACKETIZER_H_ */
