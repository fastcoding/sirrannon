#include "MediaDescriptor.h"
#include "Frame.h"

MediaDescriptor::MediaDescriptor()
: 	height(0), width(0), bitrate(0), codec(codec_t::NO),
	extra_size(0), inc(3600), content(content_t::video),
	gopsize(0), lengthSize(0), profile(0), samplerate(0),
	channels(0), level(0), duration(0), bytesize(0), framerate(0.), framesize(0),
	route(100)
{
	timebase.num = 1;
	timebase.den = 90000;
}

void MediaDescriptor::setExtraData( const uint8_t* pData, uint32_t iSize )
{
	if( iSize > sizeof(extra_data) )
		OutOfBoundsError( "Extra data too large: %d > %d (byte)", iSize, sizeof(extra_data) );
	memcpy( extra_data, pData, iSize );
	extra_size = iSize;
}

string MediaDescriptor::str( void ) const
{
	char sStr [1024] = "";
	switch( content )
	{
	case content_t::audio:
		snprintf( sStr, sizeof(sStr), "%s %dHz %dkbps %d-channels profile-%d",
			CodecToString(codec), samplerate, bitrate/1000, channels, profile );
		break;

	case content_t::video:
		snprintf( sStr, sizeof(sStr), "%s %dx%d %dkbps %f-fps extra(%d)",
			CodecToString(codec), width, height, bitrate/1000, framerate, extra_size );
		break;

	default:
		snprintf( sStr, sizeof(sStr), "%s %dkbps", CodecToString(codec), bitrate/1000 );
	}
	return sStr;
}

ContainerDescriptor::ContainerDescriptor()
	: mux(mux_t::ES), bytesize(0), bitrate(0), duration(0)
{ }

MediaDescriptor* ContainerDescriptor::getVideoDescriptor( void )
{
	for( iterator i = begin(); i != end(); ++i )
	{
		if( i->content == content_t::video )
			return &(*i);
	}
	return NULL;
}

MediaDescriptor* ContainerDescriptor::getAudioDescriptor( void )
{
	for( iterator i = begin(); i != end(); ++i )
	{
		if( i->content == content_t::audio )
			return &(*i);
	}
	return NULL;
}

const MediaDescriptor* ContainerDescriptor::getVideoDescriptor( void ) const
{
	for( const_iterator i = begin(); i != end(); ++i )
	{
		if( i->content == content_t::video )
			return &(*i);
	}
	return NULL;
}

const MediaDescriptor* ContainerDescriptor::getAudioDescriptor( void ) const
{
	for( const_iterator i = begin(); i != end(); ++i )
	{
		if( i->content == content_t::audio )
			return &(*i);
	}
	return NULL;
}

MediaDescriptor* ContainerDescriptor::addDescriptor( const MediaDescriptor* pDesc )
{
	if( pDesc )
		push_back( *pDesc );
	else
		push_back( MediaDescriptor() );
	return &back();
}
