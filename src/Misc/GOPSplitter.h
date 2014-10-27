#ifndef GOPSPLITTER_H_
#define GOPSPLITTER_H_
#include "sirannon.h"

class GOP_Splitter : public MediaProcessor
{
public:
	GOP_Splitter( const string& sName, ProcessorManager* pScope );

protected:
	virtual void init( void );
	void receive_reset( MediaPacketPtr& pckt );
	void receive_end( MediaPacketPtr& pckt );
	virtual void receive( MediaPacketPtr& pckt );
	void split( MediaPacket* pckt );

	int iCount, iSplit, iFrame;
	map<int,int> mUnit;
	bool bSync;
};

class TimeSplitter : public GOP_Splitter
{
public:
	TimeSplitter( const string& sName, ProcessorManager* pScope );

protected:
	void receive( MediaPacketPtr& pckt );
	void init( void );

	timestamp_t iDts, iInterval;
	bool bKey;
};

#endif /* GOPSPLITTER_H_ */
