#include "AMR_WBP_Reader.h"
#include "Frame.h"

REGISTER_CLASS(AMR_WBP_Reader, "AMR-WBP-reader");

/**
 * @component AMR-WBP-reader
 * @type reader
 * @properties beta, private
 * @info Reads in an AMR-WB+ encoded sequence in file storage format. The component releases one superframe
 * per packet.
 */

AMR_WBP_Reader::AMR_WBP_Reader( const string& sName, ProcessorManager* pScope )
	: Reader(sName, pScope), iFrame(0), iDTS(0), iStream(0), iRoute(200)
{ }

AMR_WBP_Reader::~AMR_WBP_Reader()
{ }

const uint8_t dSize [48] = {
		17, 23, 32, 36, 40, 46, 50, 58,
		60, 5, 34, 45, 60, 60, 0, 0,
		26, 30, 34, 38,	42, 48, 52, 60,
		31, 32, 35, 36, 38, 40, 41, 43,
		45, 46, 48, 50, 51, 53, 56, 58,
		60, 64, 65, 67, 72, 74, 75, 80 };

const uint8_t dChannels [48] = {
		1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 2, 1, 2, 0, 0,
		1, 1, 1, 1, 1, 1, 1, 1,
		2, 2, 2, 2, 2, 2, 2, 2,
		2, 2, 2, 2, 2, 2, 2, 2,
		2, 2, 2, 2, 2, 2, 2, 2 };

const uint8_t dFrames[48] = {
		1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 4, 4, 4, 4, 1, 1,
		4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4
};

const int dFreq [14] = { 25600, 12800, 14400, 16000, 17067, 19200, 21333, 24000, 25600, 28800, 32000, 34133, 36000, 38400 };

void AMR_WBP_Reader::init( )
{
	Reader::init();

	/* Values */
	iRoute = mInt["audio-route"];
	iStream = nextStreamID();
	iDTS = mInt["dts-start"] * 90;

	/* Add a media descriptor */
	pDesc = addMedia();

	/* We must peek into the first header to know some values */
	uint8_t pHeader [2];
	if( fread( pHeader, sizeof(uint8_t), 2, oFile ) < 2 )
		IOError( this, "Could not read enough bytes (2) from file" );
	fseek( oFile, -2, SEEK_CUR );

	/* Secure check of frame type and internal sample rate */
	uint8_t iFrameType = pHeader[0] & 0x7F;
	if( iFrameType > 47 )
		ValueError( this, "Invalid FT (0x%hhX)", iFrameType );
	int iFrameSize = dSize[iFrameType];
	uint8_t iInternalSampleRateIdx = pHeader[1] & 0x1F;
	if( iInternalSampleRateIdx > 13 )
		ValueError( this, "Invalid ISF (0x%hhX)", iInternalSampleRateIdx );
	int iInternalSampleRate = dFreq[iInternalSampleRateIdx];

	/* Fill out the descriptor */
	pDesc->codec = codec_t::awbp;
	pDesc->content =  content_t::audio;
	pDesc->samplerate = 72000;
	pDesc->inc = 90 * 20 * 25600 / iInternalSampleRate;
	pDesc->framesize = pDesc->samplerate * pDesc->inc / 90000;
	pDesc->bitrate = iFrameSize * iInternalSampleRate / 64;
	pDesc->channels = dChannels[iFrameType];
	pDesc->route = iRoute;

	debug( 1, "audio: codec(%s) samples(%d/%d) inc(%d) channels(%d) bitrate(%d)",
			CodecToString(pDesc->codec), pDesc->samplerate, pDesc->framesize, pDesc->inc,
			pDesc->channels, pDesc->bitrate );
}

void AMR_WBP_Reader::openBuffer( void )
{
	Reader::openBuffer();

	/* Skip the magic number */
	fseek( oFile, 4, SEEK_CUR );
}

bool AMR_WBP_Reader::doEnd( void )
{
	/* End or reset? */
	packet_t::type iType = packet_t::end;
	if( mInt["loop"] < 0 or iLoop++ < mInt["loop"] )
	{
		fseek( oFile, 4, SEEK_SET );
		iType = packet_t::reset;
	}
	else
		closeBuffer();

	/* Packet */
	MediaPacketPtr pPckt( new MediaPacket( 0 ) );

	/* Meta */
	pPckt->type = iType;
	pPckt->content = content_t::audio;
	pPckt->codec = codec_t::awbp;
	pPckt->mux = mux_t::ES;
	pPckt->frame = frame_t::AUD;
	pPckt->framenumber = iFrame;
	pPckt->unitnumber = iFrame++;
	pPckt->inc = pDesc->inc;
	pPckt->dts = pPckt->pts = iDTS;
	iDTS += pDesc->inc;

	return false;
}

bool AMR_WBP_Reader::doStep( void )
{
	/* End? */
	if( feof( oFile ) )
		return doEnd();

	/* Read the two byte header */
	uint8_t pHeader [2];
	if( fread( pHeader, sizeof(uint8_t), 2, oFile ) < 2)
		IOError( this, "Could not read enough bytes (2) from file" );

	/* Secure check of frame type and internal sample rate */
	uint8_t iFrameType = pHeader[0] & 0x7F;
	if( iFrameType > 47 )
		ValueError( this, "Invalid FT (0x%hhX)", iFrameType );
	uint8_t iInternalSampleRateIdx = pHeader[1] & 0x1F;
	if( iInternalSampleRateIdx > 13 )
		ValueError( this, "Invalid ISF (0x%hhX)", iInternalSampleRateIdx );
	int iInternalSampleRate = dFreq[iInternalSampleRateIdx];
	int iFrameSize = dSize[iFrameType];
	int iFrameCount = dFrames[iFrameType];

	/* Create one or four packets */
	for( int i = 0; i < iFrameCount; ++i )
	{
		/* Packet */
		MediaPacketPtr pPckt( new MediaPacket( iFrameSize ) );

		/* Read directly into the frame */
		if( iFrameSize )
		{
			if( fread( pPckt->data(), sizeof(uint8_t), iFrameSize, oFile ) < iFrameSize )
				IOError( this, "Could not read enough bytes (%d) from file", iFrameSize );
			pPckt->push_back( iFrameSize );
		}
		/* Modify TFI in header */
		pHeader[1] &= 0x3F;
		pHeader[1] |= i << 6;

		/* Add the header */
		pPckt->push_front( pHeader, 2 );

		/* Meta */
		pPckt->type = packet_t::media;
		pPckt->content = content_t::audio;
		pPckt->codec = codec_t::awbp;
		pPckt->mux = mux_t::ES;
		pPckt->frame = frame_t::AUD;
		pPckt->framenumber = iFrame;
		pPckt->unitnumber = iFrame++;
		pPckt->inc = pDesc->inc;
		pPckt->dts = pPckt->pts = iDTS;
		pPckt->xstream = iStream;
		iDTS += pDesc->inc;

		/* Send */
		debug( 1, "read(%s)", pPckt->c_str() );
		route( pPckt );
	}
	return true;
}
