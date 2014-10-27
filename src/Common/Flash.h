#ifndef FLASH_H_
#define FLASH_H_
#include "MediaPacket.h"

const static uint8_t FLASH_VIDEO_H263 = 2;
const static uint8_t FLASH_VIDEO_SCREEN = 3;
const static uint8_t FLASH_VIDEO_VP6 = 4;
const static uint8_t FLASH_VIDEO_VP6_ALPHA = 5;
const static uint8_t FLASH_VIDEO_SCREEN_2 = 6;
const static uint8_t FLASH_VIDEO_H264 = 7;

const static uint8_t FLASH_VIDEO_KEY_FRAME = 1;
const static uint8_t FLASH_VIDEO_INTRA_FRAME = 2;
const static uint8_t FLASH_VIDEO_NON_INTRA_FRAME = 3;
const static uint8_t FLASH_VIDEO_GENERATED_FRAME = 4;
const static uint8_t FLASH_VIDEO_COMMAND_FRAME = 5;

const static uint8_t FLASH_VIDEO_KEY_MASK = 0xF0;
const static uint8_t FLASH_VIDEO_CODEC_ID_MASK = 0x0F;

const static uint8_t FLASH_AUDIO_ADPCM = 1;
const static uint8_t FLASH_AUDIO_MP3 = 2;
const static uint8_t FLASH_AUDIO_PCM = 3;
const static uint8_t FLASH_AUDIO_NELLY_16_MONO = 4;
const static uint8_t FLASH_AUDIO_NELLY_8_MONO = 5;
const static uint8_t FLASH_AUDIO_NELLY = 6;
const static uint8_t FLASH_AUDIO_G_711_A = 7;
const static uint8_t FLASH_AUDIO_G_771_M = 8;
const static uint8_t FLASH_AUDIO_AUDIO_RESERVED = 9;
const static uint8_t FLASH_AUDIO_AAC = 10;
const static uint8_t FLASH_AUDIO_SPEEX = 11;
const static uint8_t FLASH_AUDIO_MP3_8 = 14;
const static uint8_t FLASH_AUDIO_DEVICE_SPECIFIC = 15;

const static uint8_t FLASH_AUDIO_CODEC_ID_MASK = 0xF0;

const static uint8_t FLASH_AUDIO_5_KHZ = 0;
const static uint8_t FLASH_AUDIO_11_KHZ = 1;
const static uint8_t FLASH_AUDIO_22_KHZ = 2;
const static uint8_t FLASH_AUDIO_44_KHZ = 3;

const static uint8_t FLASH_AUDIO_8_BIT = 0;
const static uint8_t FLASH_AUDIO_16_BIT = 1;

const static uint8_t FLASH_AUDIO_MONO = 0;
const static uint8_t FLASH_AUDIO_STEREO = 1;

const static uint8_t FLASH_NO = 0;

codec_t::type FlashVideoToSirannon( uint8_t iFLVCodecID );
codec_t::type FlashAudioToSirannon( uint8_t iFLVCodecID );
codec_t::type FlashToSirannon( const char* sFLVCodecID );
uint8_t SirannonToFlash( codec_t::type );

#endif /* FLASH_H_ */
