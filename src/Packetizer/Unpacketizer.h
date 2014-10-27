#ifndef UNPACKETIZER_H_
#define UNPACKETIZER_H_
#include "sirannon.h"

class Unpacketizer: public MediaProcessor
{
public:
	Unpacketizer( const string& sName, ProcessorManager* pScope );
	virtual ~Unpacketizer();

protected:
	virtual void unpack( void ) = 0;
	virtual void receive( MediaPacketPtr& pckt );
	virtual void receive_end( MediaPacketPtr& pckt );
	virtual void receive_reset( MediaPacketPtr& pckt );
	void drop( void );

	deque_t vBuffer;
	int iUnit;
};

#endif /* UNPACKETIZER_H_ */
