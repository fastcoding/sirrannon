#ifndef EXAMPLE_H_
#define EXAMPLE_H_
#include "sirannon.h"

class Example : public MediaProcessor /* The common base class */
{
public:
	/**
	 * Constructor
	 * @param Name of the component
	 * @param Scope in which the component resides. The scope calls the cstor and dstor of the class.
	 */
	Example( const string& sName, ProcessorManager* pScope );

protected:
	/**
	 * The scope ensures that the destructor is not called while receive, reveive_reset, receive_end
	 * or process is active
	 */
	virtual ~Example();

	/**
	 * When init is called all parameter from XML file are available
	 */
	virtual void init( void );

	/**
	 * The component receives a packet which it must route forward or delete.
	 * @param Pointer to the packet wrapped in a C++ auto_ptr
	 */
	virtual void receive( MediaPacketPtr& pPckt );

	/**
	 * The component receives an empty reset-packet which it must route forward or delete and
	 * signals a reset of the video stream.
	 * @param Pointer to the packet wrapped in a C++ auto_ptr
	 */
	virtual void receive_reset( MediaPacketPtr& pPckt );

	/**
	 * The component receives an empty reset-packet which it must route forward or delete and
	 * signals the end of the video stream.
	 * @param Pointer to the packet wrapped in a C++ auto_ptr
	 */
	virtual void receive_end( MediaPacketPtr& pPckt );

	/**
	 * If the member bSchedule is true, call this function at regular intervals (typically every 40 ms)
	 */
	virtual void process( void );

	/** Local members */
	uint8_t pBuffer [2*KIBI];
};

#endif /* EXAMPLE_H_ */
