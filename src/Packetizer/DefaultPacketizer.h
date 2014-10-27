/*
 * DefaultPacketizer.h
 *
 *  Created on: May 30, 2011
 *      Author: arombaut
 */

#ifndef DEFAULTPACKETIZER_H_
#define DEFAULTPACKETIZER_H_

#include "Block.h"

class DefaultPacketizer: public Block
{
public:
	DefaultPacketizer( const string& sName, ProcessorManager* pScope );
	virtual ~DefaultPacketizer();

protected:
	void init( void );
	void createPacketizer( MediaPacket* pPckt );
	void receive( MediaPacketPtr& pPckt );
	void receive_end( MediaPacketPtr& pPckt );
	void receive_reset( MediaPacketPtr& pPckt );

	map<int, MediaProcessor*> mPacketizers;
};

#endif /* DEFAULTPACKETIZER_H_ */
