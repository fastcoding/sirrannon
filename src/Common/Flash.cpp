#include "sirannon.h"
#include "Flash.h"

codec_t::type FlashVideoToSirannon( uint8_t iFLVCodecID )
{
	switch( iFLVCodecID )
	{
	case FLASH_VIDEO_H263:
	case FLASH_VIDEO_SCREEN:
	case FLASH_VIDEO_SCREEN_2:
		return codec_t::other;

	case FLASH_VIDEO_VP6:
	case FLASH_VIDEO_VP6_ALPHA:
		return codec_t::vp6;

	case FLASH_VIDEO_H264:
		return codec_t::avc;

	default:
		return codec_t::NO;
	}
}

codec_t::type FlashToSirannon( const char* sFLVCodecID )
{
	if( sFLVCodecID[0] == '.' )
		sFLVCodecID++;
	if( strncasecmp( sFLVCodecID, "mp3", 3 ) == 0 )
		return codec_t::mp1a;
	if( strncasecmp( sFLVCodecID, "vp6", 3 ) == 0 )
		return codec_t::vp6;
	if( strncasecmp( sFLVCodecID, "h264", 4 ) == 0 or strncasecmp( sFLVCodecID, "avc", 3 ) == 0 )
		return codec_t::avc;
	if( strncasecmp( sFLVCodecID, "mp4a", 4 ) == 0 or strncasecmp( sFLVCodecID, "aac", 3 ) == 0 )
		return codec_t::mp4a;
	return codec_t::NO;
}

codec_t::type FlashAudioToSirannon( uint8_t iFLVCodecID )
{
	switch( iFLVCodecID )
	{
	case FLASH_AUDIO_ADPCM:
	case FLASH_AUDIO_PCM:
	case FLASH_AUDIO_NELLY_16_MONO:
	case FLASH_AUDIO_NELLY_8_MONO:
	case FLASH_AUDIO_NELLY:
	case FLASH_AUDIO_G_711_A:
	case FLASH_AUDIO_G_771_M:
	case FLASH_AUDIO_AUDIO_RESERVED:
	case FLASH_AUDIO_SPEEX:
	case FLASH_AUDIO_DEVICE_SPECIFIC:
		return codec_t::other;

	case FLASH_AUDIO_AAC:
		return codec_t::mp4a;

	case FLASH_AUDIO_MP3:
	case FLASH_AUDIO_MP3_8:
		return codec_t::mp1a;

	default:
		return codec_t::NO;
	}
}

uint8_t SirannonToFlash( codec_t::type iCodec )
{
	switch( iCodec )
	{
	case codec_t::mp1a:
	case codec_t::mp2a:
		return FLASH_AUDIO_MP3;

	case codec_t::mp4a:
		return FLASH_AUDIO_AAC;

	case codec_t::avc:
		return FLASH_VIDEO_H264;

	case codec_t::vp6:
	case codec_t::vp6f:
		return FLASH_VIDEO_VP6;

	default:
		return 0;
	}
}
