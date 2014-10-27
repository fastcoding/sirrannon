/*
 * FakeReader.h
 *
 *  Created on: May 16, 2011
 *      Author: arombaut
 */

#ifndef FAKEREADER_H_
#define FAKEREADER_H_
#include "Reader.h"

class FakeReader : public Reader
{
public:
	FakeReader( const string& sName, ProcessorManager* pScope );
	~FakeReader();

protected:
	void receive( MediaPacketPtr& pPckt );
	void receive_reset( MediaPacketPtr& pPckt );
	void receive_end( MediaPacketPtr& pPckt );
	void init( void );
	void openBuffer( void ){ };
	void closeBuffer( void ){ };
	bool doStep( void );

	int iMTU;
	uint8_t* pFrame;
	int iFrameSize, iFrame, iDuration, iUnit;
	double fFPS;
	bool bRandom;
	MediaDescriptor* pDesc;
};

#endif /* FAKEREADER_H_ */
