#ifndef LIVEREADER_H_
#define LIVEREADER_H_
#include "Frame.h"
#include "Link.h"
#include "Interfaces.h"
#include <set>

class LiveReader : public InLink, public ClientInterface
{
public:
	LiveReader( const string& sName, ProcessorManager* pScope );
	virtual ~LiveReader();

	void init( void );
	virtual int seek( uint32_t iTimeIndex ) { return -1; } /* Seeking is not allowed */
	virtual bool ready( void ) const;
	virtual int play( double fSpeed );
	virtual int pause( void );
	virtual void findSchedulers( void );

protected:
	void link( SourceInterface* pSource );
	void send( MediaPacketPtr& pPckt );
	void enqueue( MediaPacketPtr& pPckt );
	void process( void );
	void receive( MediaPacketPtr& pPckt );
	void receive_reset( MediaPacketPtr& pPckt ) { receive( pPckt ); }
	void receive_end( MediaPacketPtr& pPckt ) { receive( pPckt); }

	SourceInterface* pSource;
	queue_t vQueue;
	map<int, vector_t> dStreams;
	set<int> dSync;
	timestamp_t iBaseDts, iStartDts;
	bool bSync, bMp4;
	MP4MediaConverter oMP4;
};

#endif /* LIVEREADER_H_ */
