#include "GigabitTransmitter.h"
#include "SirannonSocket.h"
#include "IPAddress.h"
#include "RandomGenerator.h"
#include "OSSupport.h"
#ifndef WINDOWS
#include <sys/types.h>

/**
 * @component gigabit-transmitter
 * @type miscellaneous
 * @param destination, string, 10.10.0.1:1234, the destination
 * @param port, int, 4000, the source port
 * @param mtu, int, 1450, the size of each generated packet (including headers)
 * @param bitrate, int, 100, in megabits per seconds, bitrate of the generated stream
 * @param fps, int, 25, number of frames per second
 */

REGISTER_CLASS( GigabitTransmitter, "Gigabit-transmitter" );

GigabitTransmitter::GigabitTransmitter( const string& sName, ProcessorManager* pScope )
	: MediaProcessor(sName, pScope), oFake(NULL)
{
	mString["destination"] = "127.0.0.1:1234";
	mInt["port"] = 4000;
	mInt["mtu"] = 1450;
	mInt["fps"] = 25;
	mInt["bitrate"] = MEGA;
}

GigabitTransmitter::~GigabitTransmitter()
{
	debug( 1, "Last delay was: %"LL"d ns", iDiff );
	if( oFake )
		fclose( oFake );
}

void GigabitTransmitter::init( void )
{
	MediaProcessor::init();
	createThread( bind( &GigabitTransmitter::mainThread, this ) );
}

void GigabitTransmitter::mainThread( void )
{
	/* Create a fake temporary file */
	const uint32_t iMTU = mInt["mtu"];
	oFake = tmpfile();
	const int iFake = fileno( oFake );
	for( int i = 0; i < iMTU; ++i )
	{
		uint8_t iVal = oRandom.next();
		fwrite( &iVal, 1, 1, oFake );
	}
	rewind( oFake );

	/* Create socket */
	IPAddress oIPAddr ( mString["destination"] );
	if( not oIPAddr.valid() )
		ValueError( this, "Translated ip(%s) from address(%s) invalid",
				oIPAddr.getAddressStr().c_str(), mString["destination"].c_str() );
	UDPSocket oUDP( mInt["port"] );
	if( oUDP.connect( oIPAddr.getIPStr(), oIPAddr.getPort() ) < 0 )
		IOError("Could not connect to %s: %s", oIPAddr.getAddressStr().c_str(), strError() );

	/* Big loop */
	const SirannonTime oStep = SirannonTime::fromNSecs( (int64_t)iMTU * 8 * GIGA / ( mInt["bitrate"] * MEGA ));
	SirannonTime oSend = SirannonTime::getCurrentTime();
	debug( 1, "bitrate(%d Mbps) interpacket time(%"LL"d ns)", mInt["bitrate"], oStep.convertNsecs() );

	while( true )
	{
		/* Interval */
		oSend += oStep;
		while( SirannonTime::getCurrentTime() < oSend )
			this_thread::interruption_point();

		/* Send a packet */
		while( true )
		{
			/* Interrupt */
			this_thread::interruption_point();

			/* Send */
			int iVal = oUDP.sendfileSmart( iFake, iMTU );
			if( iVal == COM_AGAIN )
				continue;
			else if( iVal <= 0 )
				IOError( "Send_file failed: %d(%s)", iVal, strError() );
			else
				break;
		}
		/* In time? */
		iDiff = (SirannonTime::getCurrentTime() - oSend).convertNsecs();
		if( iDiff > GIGA )
			RuntimeError( "System too slow: delay(%"LL"d ns)", iDiff );
	}
	fclose( oFake );
	oFake = NULL;
}

#endif
