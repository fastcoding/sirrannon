#ifndef MP4UNPACKETIZER_H_
#define MP4UNPACKETIZER_H_
#include "Unpacketizer.h"

class MP4Unpacketizer : public Unpacketizer
{
public:
	MP4Unpacketizer( const string& sName, ProcessorManager* pScope );
	virtual ~MP4Unpacketizer();

protected:
	void unpack( void );
};

#endif /* MP4UNPACKETIZER_H_ */
