#include "Example.h"

/* Below comment block uses a doxy like syntax to declare components for
 * documentation and GUI
 * 1. @component: string name of the class used in REGISTER_CLASS
 * 2. @type: category to which the component belongs (eg. reader, packetizer, miscellaneous...)
 * 3. @param: name, type, default, description of a parameter
 * 4. @info: description of a component
 *
 * Run python/doc.py to update the documentation and GUI after making changes to a comment block
 */

/** EXAMPLE
 * @component example
 * @type miscellaneous
 * @param test, int, 5, input variable
 * @param foo, bool, false, an example flag
 * @info Example component demonstrating the basic API of a functional component
 **/

/* REGISTERCLASS( X, Y )
 * Registers the class X under the name Y. This way a string Y in a configuration file can be
 * mapped to class X. Because of this system, we do not need a central file which registers all
 * the components.
 */
REGISTER_CLASS( Example, "example" );

int32_t myTransformFunction( const uint8_t*, uint32_t, uint8_t*, uint32_t );

const static uint32_t FEC_SIZE = 128;

/* Constructor */
Example::Example( const string& sName, ProcessorManager* pScope )
	: MediaProcessor(sName, pScope)
{
	/* Load the default values of parameters */
	mInt["test"] = 5;
}

/* Destructor */
Example::~Example()
{ }

/* This function will be called before the calls to receive and process begin */
void Example::init( void )
{
	/* call the base class */
	MediaProcessor::init();

	/* Set bSchedule to true if you want process to be called */
	bSchedule = false;
}

/* The component receives MediaPckt. This packet must either be deleted or routed. */
void Example::receive( MediaPacketPtr& pPckt )
{
	/* Transform this packet and write to a buffer */
	int iNewSize = myTransformFunction( pPckt->data(), pPckt->size(), pBuffer, sizeof(pBuffer) );
	if( iNewSize < 0 )
		RuntimeError( this, "Problem is..." );

	/* Create a new packet with the same meta data but no data */
	MediaPacketPtr pNewPckt( new MediaPacket( packet_t::media, pPckt->content, pPckt->size() + FEC_SIZE ) );
	pNewPckt->set_metadata( pPckt.get() );

	/* Add the transformed data */
	pNewPckt->push_back( pBuffer, iNewSize );

	/* Delete the old packet */
	delete pPckt.release(); // Can be omitted since we work with auto_ptr

	/* Send the new one away */
	route( pNewPckt ); // CAVEAT after calling this function the pointer pNewPckt is invalidated
}

void Example::receive_reset( MediaPacketPtr& pPckt )
{
	/* Have to route the pPckt at least */
	route( pPckt );
}

void Example::receive_end( MediaPacketPtr& pPckt )
{
	/* Have to route the pPckt at least */
	route( pPckt );
}

/* This function will be reguarly called (controlled by the global parameter quantum [-q]),
 * typically every 40 ms, if bSchedule is true
 */
void Example::process( void )
{ }

int myTransformFunction( const uint8_t*, uint32_t, uint8_t*, uint32_t )
{
	return 0;
}
