#ifndef MEDIADESCRIPTOR_H_
#define MEDIADESCRIPTOR_H_
#include "sirannon.h"
#include "MediaPacket.h"

class SourceInterface;
class MediaDescriptor
{
public:
	MediaDescriptor();
	~MediaDescriptor() { };

	string str( void ) const;
	const uint8_t* getExtraData( void ) const;
	uint32_t getExtraSize( void ) const;
	void setExtraData( const uint8_t* pData, uint32_t iSize );

	/* Fields */
	static const uint32_t frequency = 90000;
	uint32_t inc;
	content_t::type content;
	codec_t::type codec;
	int lengthSize;
	int width, height, bitrate, samplerate, framesize, channels, gopsize, profile, level, duration,
	bytesize, route;
	double framerate;
	rational_t timebase;
	string sdp, track;
	int payload, serverPort, clientPort;
	uint8_t extra_data [5*KIBI];
	uint32_t extra_size;
	SourceInterface* source;
 };

class ContainerDescriptor : public vector<MediaDescriptor>
{
public:
	ContainerDescriptor();
	~ContainerDescriptor() { };

	MediaDescriptor* getVideoDescriptor( void );
	MediaDescriptor* getAudioDescriptor( void );
	const MediaDescriptor* getVideoDescriptor( void ) const;
	const MediaDescriptor* getAudioDescriptor( void ) const;
	MediaDescriptor* addDescriptor( const MediaDescriptor* pDesc = NULL );

	/* Fields */
	mux_t::type mux;
	int bytesize, bitrate, duration;
};

inline const uint8_t* MediaDescriptor::getExtraData( void ) const
{
	return extra_data;
}

inline uint32_t MediaDescriptor::getExtraSize( void ) const
{
	return extra_size;
}

#endif /* MEDIADESCRIPTOR_H_ */
