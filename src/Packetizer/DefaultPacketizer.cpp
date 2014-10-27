#include "DefaultPacketizer.h"
#include "Frame.h"

/**
 * @component default-packetizer
 * @type packetizer
 * @param type, string, , if defined, force every packetizer to be of this type, useful for forcing PES-packetizer for example
 * @info For each new stream creates at runtime a fitting packetizer depending on the codec or the parameter type.
 */

REGISTER_CLASS( DefaultPacketizer, "default-packetizer" );

DefaultPacketizer::DefaultPacketizer( const string& sName, ProcessorManager* pScope )
 : Block(sName, pScope)
{ }

DefaultPacketizer::~DefaultPacketizer()
{ }

void DefaultPacketizer::init( void )
{
	MediaProcessor::init();

	out = oProcessorManager.createProcessor( "out", "out" );
	out->initProcessor();
	initOut();
}

void DefaultPacketizer::createPacketizer( MediaPacket* pPckt )
{
	char sType [256];
	if( not mString["type"].length() )
		snprintf( sType, size(sType), "%s-packetizer", CodecToString( pPckt->codec ) );
	else
		snprintf( sType, size(sType), "%s-packetizer", mString["type"].c_str() );

	char sName [256];
	snprintf(sName, sizeof(sName), "packetizer-%d", pPckt->xstream );

	MediaProcessor* pPacketizer = oProcessorManager.createProcessorDynamic(
			sType, sName, NULL, NULL, &mInt, &mDouble, &mString, &mBool, NULL );
	pPacketizer->setRoute( 0, out );

	mPacketizers[pPckt->xstream] = pPacketizer;
}

void DefaultPacketizer::receive( MediaPacketPtr& pPckt )
{
	MediaProcessor* pPacketizer = mPacketizers[pPckt->xstream];
	if( not pPacketizer )
	{
		createPacketizer( pPckt.get() );
		pPacketizer = mPacketizers[pPckt->xstream];
	}
	pPacketizer->receivePacket( pPckt );
}

void DefaultPacketizer::receive_end( MediaPacketPtr& pPckt )
{
	receive( pPckt );
}

void DefaultPacketizer::receive_reset( MediaPacketPtr& pPckt )
{
	receive( pPckt );
}
