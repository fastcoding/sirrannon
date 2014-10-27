#ifdef WITH_LIBPCAP
#ifndef PCAPWRITER_H_
#define PCAPWRITER_H_
#define SIRANNON_USE_BOOST_FILESYSTEM
#include "Boost.h"
#include "sirannon.h"
extern "C"
{
#include <pcap.h>
}

class PcapWriter: public MediaProcessor
{
public:
	PcapWriter( const string& sName, ProcessorManager* pScope );
	virtual ~PcapWriter();

protected:
	void init( void );
	void pcapLoop( void );

	char sError [1024];
	pcap_t* pHandle;
	pcap_dumper_t* pFile;
};

#endif /* PCAPWRITER_H_ */
#endif
