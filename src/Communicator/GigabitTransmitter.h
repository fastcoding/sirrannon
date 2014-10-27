#ifndef GIGABITTRANSMITTER_H_
#define GIGABITTRANSMITTER_H_
#include "MediaProcessor.h"

class GigabitTransmitter : public MediaProcessor
{
public:
	GigabitTransmitter( const string& sName, ProcessorManager* pScope );
	~GigabitTransmitter();

protected:
	void init( void );
	void mainThread( void );

	FILE* oFake;
	int64_t iDiff;
};

#endif /* GIGABITTRANSMITTER_H_ */
