#include "FakeReader.h"
#include "RandomGenerator.h"
#include "Utils.h"

/**
 * @component fake-reader
 * @type miscellaneous
 * @param width, int, 1920, the width of the fake frame
 * @param height, int, 1080, the height of the fake frame
 * @param bits-per-pixel, int, 12, the number of bits per pixel of the fake frame
 * @param fps, double, 25., the number of frames per second
 * @param bitrate, int, -1, if defined, overwrite the bitrate implied by the combination of fps and frame dimensions
 * @param duration, int, -1, in ms, if defined the duration of the sequence, otherwise generate frames indefinately
 * @param mtu, int, -1, if > 0, divide each frame into packets of maximum mtu size
 * @info Generates random YUV frames with specified dimensions and/or targetted bitrate.
 */
REGISTER_CLASS( FakeReader, "fake-reader" );

FakeReader::FakeReader( const string& sName, ProcessorManager* pScope )
	: Reader(sName, pScope), iFrame(0), pFrame(NULL), bRandom(false), iDuration(-1),
	  iUnit(0), iMTU(-1)
{
	mInt["width"] = 1920;
	mInt["height"] = 1080;
	mInt["bits-per-pixel"] = 12;
	mInt["bitrate"] = -1;
	mInt["mtu"] = -1;
	mDouble["fps"] = 25;
	mBool["randomize"] = false;
	mInt["duration"] = -1;
}

FakeReader::~FakeReader()
{
	delete [] pFrame;
}

void FakeReader::init( void )
{
	Reader::init();

	/* Parse input params */
	iMTU = mInt["mtu"];
	fFPS = mDouble["fps"];
	if( fFPS <= 0 )
		ValueError( "Invalid fps(%f)", fFPS );

	bRandom = mBool["randomize"];
	iDuration = mInt["duration"];
	int iBitrate = mInt["bitrate"];
	if( iBitrate > 0 )
		iFrameSize = mInt["bitrate"] / 8 / fFPS;
	else
	{
		iFrameSize = mInt["width"] * mInt["height"] * mInt["bits-per-pixel"];
		iBitrate = iFrameSize * fFPS;
	}
	if( iFrameSize <= 0 )
		ValueError( "Invalid framesize(%d)", iFrameSize );

	/* Generate the fake frame */
	pFrame = new uint8_t [iFrameSize];
	for( int i = 0; i < iFrameSize; ++i )
		pFrame[i] = oRandom.next();

	/* Generate descriptor */
	pDesc = addMedia();
	pDesc->bitrate = iBitrate;
	pDesc->codec = codec_t::yuv;
	pDesc->framerate = fFPS;
	pDesc->route = 100;
	pDesc->width = mInt["width"];
	pDesc->height = mInt["height"];
	debug( 1, "fake reader: %s", pDesc->str().c_str() );
}

void FakeReader::receive_reset( MediaPacketPtr& pPckt )
{
	receive( pPckt );
}

void FakeReader::receive_end( MediaPacketPtr& pPckt )
{
	receive( pPckt );
}

void FakeReader::receive( MediaPacketPtr& pPckt )
{
	/* Looping */
	pPckt->framenumber = iFrame;
	pPckt->unitnumber = iUnit++;
	pPckt->dts = pPckt->pts = (timestamp_t)iFrame * 90000 / fFPS;
	if( pPckt->frameend )
		iFrame++;
	debug( 1, "recycled %s", pPckt->c_str() );
	route( pPckt );
}

bool FakeReader::doStep( void )
{
	/* Create and fill a new packet(s) */
	const timestamp_t iDTS = (timestamp_t)iFrame * 90000 / fFPS;
	if( iMTU < 0 )
		iMTU = iFrameSize;
	for( int i = 0; i < iFrameSize; i += iMTU )
	{
		/* Create the packet */
		int iSize = MIN( iMTU, iFrameSize - i );
		MediaPacketPtr pPckt( new MediaPacket( packet_t::media, content_t::video, iSize ) );
		pPckt->push_back( iSize );

		/* Meta fields */
		pPckt->codec = codec_t::yuv;
		pPckt->unitnumber = iUnit++;
		pPckt->framenumber = iFrame;
		pPckt->desc = pDesc;
		pPckt->dts = pPckt->pts = iDTS;
		pPckt->inc = 90000 / fFPS;
		pPckt->framestart = (i == 0);
		pPckt->frameend = ( iFrameSize - i ) <= iMTU;

		debug( 1, "generated %s", pPckt->c_str() );
		route( pPckt );
	}
	/* Stop? */
	if( iDuration >= 0 and iDTS / 90 >= iDuration )
		bSchedule = false;
	iFrame++;
	return true;
}
