#ifndef BASICREADER_H_
#define BASICREADER_H_
#include "Reader.h"

class BasicReader : public Reader
{
public:
	BasicReader( const string& sName, ProcessorManager* pScope );
	virtual void init( void );

protected:
	virtual ~BasicReader();
	virtual bool doStep( void );
	void createEnd( packet_t::type iType );

	int iMaxBuffer, iUnit, iDuration;
	MediaDescriptor* pDesc;
};

#endif /* BASICREADER_H_ */
