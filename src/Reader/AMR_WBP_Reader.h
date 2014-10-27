#ifndef AMR_WBP_READER_H_
#define AMR_WBP_READER_H_
#include "Reader/Reader.h"

class AMR_WBP_Reader : public Reader
{
public:
	AMR_WBP_Reader( const string& sName, ProcessorManager* pScope );
	~AMR_WBP_Reader();

protected:
	void init( void );
	void openBuffer( void );
	bool doEnd( void );
	bool doStep( void );

	MediaDescriptor* pDesc;
	int iFrame, iRoute, iStream;
	timestamp_t iDTS;
};

#endif /* AMR_WBP_READER_H_ */
