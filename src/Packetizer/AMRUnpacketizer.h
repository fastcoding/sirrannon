#ifndef AMRUNPACKETIZER_H_
#define AMRUNPACKETIZER_H_
#include "Unpacketizer.h"

class AMRUnpacketizer : public Unpacketizer
{
public:
	AMRUnpacketizer( const string& sName, ProcessorManager* pScope );

protected:
	~AMRUnpacketizer();

	void unpack( void );
};

#endif /* AMRUNPACKETIZER_H_ */
