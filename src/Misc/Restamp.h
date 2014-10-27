/*
 * Restamp.h
 *
 *  Created on: Dec 21, 2011
 *      Author: arombaut
 */

#ifndef RESTAMP_H_
#define RESTAMP_H_
#include "MediaProcessor.h"
#include <set>

class Restamp : public MediaProcessor
{
public:
	Restamp( const string& sName, ProcessorManager* pScope );
	virtual ~Restamp();

protected:
	virtual void init( void );
	virtual void receive( MediaPacketPtr& pPckt );
	virtual void receive_reset( MediaPacketPtr& pPckt );
	virtual void receive_end( MediaPacketPtr& pPckt );
	virtual void extract_lowest( void );

	set<timestamp_t> vTime;
	queue_t vPackets;
	int iDelay;
};

#endif /* RESTAMP_H_ */
