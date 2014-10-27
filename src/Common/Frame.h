/*****************************************************************************
 * (c)2006-2010 Sirannon
 * Authors: Alexis Rombaut <alexis.rombaut@intec.ugent.be>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *****************************************************************************/
#ifndef FRAME_H_
#define FRAME_H_
#include "Utils.h"
#include "Bits.h"
#include "h264_avc.h"
#include "MediaPacket.h"

/* Duration in ms of a sequence */
static const timestamp_t TIMESTAMP_MIN = 90000;

void GetMetadata( const char* sMedia, const container_t* pContainer, MediaDescriptor* pDesc );

/* Transport streams */
uint8_t GetPesID( codec_t::type codec );
uint8_t GetPmtID( codec_t::type codec );
codec_t::type GetPmtCodec( int iPMT );

/* Conversions */
const char* ContentToString( content_t::type content );
const char* CodecToString( codec_t::type codec );
const char* MuxToString( mux_t::type iMux );
const char* TargetToString( target_t::type iMux );
mux_t::type StringToMux( const char* sMux );
codec_t::type StringToCodec( const char* sCodec );
container_t StringToContainer( const char* sExtension );
const char* FrameToString( frame_t::type frame);
char  FrameToChar( frame_t::type frame);
content_t::type CodecToContent( codec_t::type oCodec );
frame_t::type H264FrameToXFrame( int iNALType, int iSliceType );

/* Containers */
container_t ExtensionToContainer( const string& sMedia );
codec_t::type VerifyCodecForContainer( container_t iContainer, codec_t::type iCodec );

/* Frame tools */
bool IsIFrame( const MediaPacket* pckt );
bool IsSlice ( const MediaPacket* pckt );

/* Recurring media packet operations */
void convert_header_MP4( MediaPacket* pPckt );
int convert_header_ES( MediaPacket* pPckt );
MediaPacketPtrRef mergeFrameParts( deque_t& vBuffer, MediaPacketPtr& pMerged, bool bConvertMP4=false );
void H264_global_header( deque_t& vSPS, deque_t& vPPS,  uint8_t* pExtraData, uint32_t* iExtraSize );

/* Sample rates, profiles and channels */
int mp4aSampleRateIdx( int iSampleRate );
int mp4aProfileIdx( int iProfile );
int mp4aChannelsIdx( int iChannel );
int mp4aSampleRate( int iIdx );
int mp4aProfile( int iIdx );
int mp4aChannels( int iIdx );

/* Class to convert an ANNEX_B H.264 stream into MP4 stream or vice versa */
class MP4MediaConverter
{
public:
	MP4MediaConverter( bool bFixPTS=false, bool bSkipSEI=false, bool bSkipAID=false );
	~MP4MediaConverter();

	void skipNAL( bool bSkipSEI, bool bSkipAUD );
	timestamp_t getPOC( void ) const;
	const NAL_t* getNALParse( void ) const;

	static int ESDS2META( MediaDescriptor* pDesc );
	static int AVCC2META( MediaDescriptor* pDesc );

	MediaPacketPtrRef convertMP4( MediaPacketPtr& pPckt );
	MediaPacketPtrRef convertMP4Video( const uint8_t* pSource, int iSize, MediaDescriptor* );
	MediaPacketPtrRef convertMP4Audio( const uint8_t* pSource, int iSize, MediaDescriptor* );

	void convertES( MediaPacketPtr& pPckt, queue_t& oBuffer, bool bAddParameterSets=false );
	void convertESVideo( const uint8_t* pSource, int iSize, MediaDescriptor*, queue_t& oBuffer, bool bAddParameterSets=false );
	void convertESAudio( const uint8_t* pSource, int iSize, const MediaDescriptor*, queue_t& oBuffer );

	const uint8_t* getVideoExtraData( void ) const;
	uint32_t getVideoExtraSize( void ) const;
	const uint8_t* getAudioExtraData( void ) const;
	uint32_t getAudioExtraSize( void ) const;

	int buildVideoExtraData( const uint8_t* pBuffer, int iSize, MediaDescriptor* pDesc );
	int META2ESDS( MediaDescriptor* pDesc );
	int ADTS2ESDS( const uint8_t* pADTS, uint32_t iSize, MediaDescriptor* pDesc );

	NAL_t oNAL;

private:
	int ANNEXB2AVCC( MediaDescriptor* );
	int ADTS2ESDS( MediaDescriptor* );
	void clearParameterSets( void );
	void extractParameterSets( const MediaDescriptor* pDesc );
	void insertParameterSets( MediaPacketPtr& pPckt, queue_t& oBuffer );

	OBits oAudioExtra, oVideoExtra;
	deque_t vVideo;
	vector_t vSPS, vPPS;
	IBits oAudioHeader;
	int iUnit, iVideoParamVersion, iVideoHeaderVersion, iAudioHeaderVersion, iSub;
	bool bFirstVideo, bSkipAUD, bSkipSEI, bFixPTS;
};

inline const uint8_t* MP4MediaConverter::getVideoExtraData( void ) const
{
	return oVideoExtra.data();
}

inline uint32_t MP4MediaConverter::getVideoExtraSize( void ) const
{
	return oVideoExtra.size();
}

inline const uint8_t* MP4MediaConverter::getAudioExtraData( void ) const
{
	return oAudioExtra.data();
}

inline uint32_t MP4MediaConverter::getAudioExtraSize( void ) const
{
	return oAudioExtra.size();
}

inline const NAL_t* MP4MediaConverter::getNALParse( void ) const
{
	return &oNAL;
}

inline timestamp_t MP4MediaConverter::getPOC( void ) const
{
	return oNAL.pic_order_cnt;
}

#endif /*FRAME_H_*/
