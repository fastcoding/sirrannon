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
#include "sirannon.h"
#include "MediaPacket.h"
#include "Frame.h"
#include "SirannonException.h"
#include "RandomGenerator.h"
#include "MemoryManager.h"

static const int iBufferSpace = 32;

/* Standard constructor */
MediaPacket::MediaPacket( packet_t::type _type, content_t::type _content, uint32_t iMaxSize )
: 	framenumber(0), unitnumber(0), subframenumber(0), pts(0), dts(0), inc(0), T(0), Q(0), D(0),
	xroute(0), oSend(0), xstream(0),
  	framestart(true), frameend(true), error(false),
  	type(_type), content(_content), codec(codec_t::NO), mux(mux_t::ES),
  	frame(frame_t::no_frame), desc(NULL), key(false), sCstr(NULL)
{
	/* Assign dynamic memory */
	max_size = iMaxSize + 2 * iBufferSpace;
	_data = new uint8_t [max_size];
	begin = end = _data + iBufferSpace;
	//mem_register( max_size );
}

MediaPacket::MediaPacket( uint8_t* pBuffer, uint32_t iSize )
:	content(content_t::video), type(packet_t::media),
	framenumber(0), unitnumber(0), subframenumber(0), pts(0), dts(0), inc(0), T(0), Q(0), D(0),
	xroute(0), oSend(0), xstream(0),
  	framestart(true), frameend(true), error(false),
  	codec(codec_t::NO), mux(mux_t::ES),
  	frame(frame_t::no_frame), desc(NULL), key(false), sCstr(NULL)
{
	_data = begin = pBuffer;
	max_size = iSize;
	end = begin + max_size;
	//mem_register( max_size );
}

MediaPacket::MediaPacket( uint32_t iSize )
:	content(content_t::video), type(packet_t::media),
	framenumber(0), unitnumber(0), subframenumber(0), pts(0), dts(0), inc(0), T(0), Q(0), D(0),
	xroute(0), oSend(0), xstream(0),
  	framestart(true), frameend(true), error(false),
  	codec(codec_t::NO), mux(mux_t::ES),
  	frame(frame_t::no_frame), desc(NULL), key(false), sCstr(NULL)
{
	/* Assign dynamic memory */
	max_size = iSize + 2 * iBufferSpace;
	_data = new uint8_t [max_size];
	begin = end = _data + iBufferSpace;
	//mem_register( max_size );
}

/* Copy constructor */
MediaPacket::MediaPacket( const MediaPacket& other )
:	max_size(other.max_size),
	framenumber(other.framenumber), unitnumber(other.unitnumber), subframenumber(other.subframenumber),
  	xroute(other.xroute), xstream(other.xstream),
  	oSend(other.oSend),
  	dts(other.dts), pts(other.pts), inc(other.inc), T(other.T), Q(other.Q), D(other.D),
	framestart(other.framestart), frameend(other.frameend), error(other.error),
	type(other.type), content(other.content), mux(other.mux), codec(other.codec),
	frame(other.frame), desc(other.desc), key(other.key), sCstr(NULL)
{
	/* Assign dynamic memory */
	_data = new uint8_t [max_size];
	begin = _data + ( other.begin - other._data );
	end   = _data + ( other.end   - other._data );
	memcpy( begin, other.begin, end - begin );
	//mem_register( max_size );
}

MediaPacket::~MediaPacket( )
{
	delete [] _data;
	delete [] sCstr;
	//mem_unregister( max_size );
}

void MediaPacket::clear( void )
{
	begin = end = _data + iBufferSpace;
}

void MediaPacket::memalign( int iStep )
{
	/* Cannot align packets with data */
	if( size() )
		RuntimeError( "media-packet: alignment not allowed on packet with data" );

	/* Allign at iStep bytes */
	begin = end = data() - ( ((int64_t) data()) & iStep );

	/* Sanity */
	if( begin < _data )
		RuntimeError( "media-packet: alignment underflow" );
}

void MediaPacket::set_metadata( const MediaPacket* pckt )
{
	type = pckt->type;
	content = pckt->content;

	xroute	 = pckt->xroute;
	xstream  = pckt->xstream;

	framenumber	= pckt->framenumber;
	unitnumber	= pckt->unitnumber;
    subframenumber	= pckt->subframenumber;
    pts			= pckt->pts;
	dts			= pckt->dts;
	inc			= pckt->inc;
	oSend    = pckt->oSend;

	frame		= pckt->frame;
	framestart	= pckt->framestart;
	frameend	= pckt->frameend;
	error = pckt->error;
	key = pckt->key;

	Q = pckt->Q;
	D = pckt->D;
	T = pckt->T;

	mux   = pckt->mux;
	codec = pckt->codec;
	desc  = pckt->desc;
}

const char* MediaPacket::c_str_long( void )
{
	/* String memory */
	char sLabel [32];
	sLabel[0] = '\0';
	c_alloc();

	/* Label of the packet */
	if( content == content_t::audio )
		strcat( sLabel, "audio-" );
	else if( content == content_t::video )
		strcat( sLabel, "video-" );
	else
		strcat( sLabel, "mixed-" );
	if( type == packet_t::end )
		strcat( sLabel, "end-" );
	else if( type == packet_t::reset )
		strcat( sLabel, "reset-" );

	string s = FrameToString( frame );
	snprintf( sCstr, MAX_STRING_SIZE, "%spacket %06d.%06d.%02d dts(%08u) pts(%08u) inc(%04u) frame(%s) key(%d) size(%05u) codec(%s) mux(%s)",
				sLabel, (int)unitnumber, (int)framenumber, (int)subframenumber,
				(unsigned)dts, (unsigned)pts, (unsigned) inc, expand(s,3).c_str(), key, (unsigned)size(), CodecToString(codec), MuxToString(mux) );

	/* Route info */
	snprintf( sLabel, sizeof(sLabel), " x(%d)", (int)xroute );
	strcat( sCstr, sLabel );

	/* SVC tags */
	if( codec == codec_t::svc )
	{
		snprintf( sLabel, sizeof(sLabel), " (%d,%d,%d)", (int)T, (int) D, (int) Q );
		strcat( sCstr, sLabel );
	}
	/* Begin & End flags */
	char cB = framestart ? 'B' : ' ';
	char cE = frameend   ? 'E' : ' ';
	snprintf( sLabel, sizeof(sLabel), " %c%c", cB, cE );
	strcat( sCstr, sLabel );

	return sCstr;
}

const char* MediaPacket::c_str_full( uint32_t iBytes )
{
	c_str_long();

	strcat( sCstr, "\n" );
	strcat( sCstr, strArray( data(), MIN(size(),iBytes) ).c_str() );
	return sCstr;
}

const char* MediaPacket::c_str_data( uint32_t iBytes )
{
	c_alloc();
	strcat( sCstr, "\n" );
	strcat( sCstr, strArray( data(), MIN(size(),iBytes) ).c_str() );
	return sCstr;
}

const char* MediaPacket::c_str( void )
{
	/* String memory */
	char sLabel [32] = "";
	c_alloc();

	/* Label of the packet */
	if( content == content_t::audio )
		strcat( sLabel, "audio-" );
	else if( content == content_t::video )
		strcat( sLabel, "video-" );
	else
		strcat( sLabel, "mixed-" );
	if( type == packet_t::end )
		strcat( sLabel, "end-" );
	else if( type == packet_t::reset )
		strcat( sLabel, "reset-" );

	snprintf( sCstr, MAX_STRING_SIZE, "%spacket %06d.%06d.%02d dts(%08u) pts(%08u) inc(%04u) frame(%3s) size(%05u) codec(%s)",
				sLabel, (int)unitnumber, (int)framenumber, (int)subframenumber,
				(unsigned)dts, (unsigned)pts, (unsigned)inc, FrameToString(frame), (unsigned)size(), CodecToString(codec) );

	/* Route info */
	snprintf( sLabel, sizeof(sLabel), " x(%d)", (int)xroute );
	strcat( sCstr, sLabel );

	/* SVC tags */
	if( codec == codec_t::svc )
	{
		snprintf( sLabel, sizeof(sLabel), " (%d,%d,%d)", (int)T, (int) D, (int) Q );
		strcat( sCstr, sLabel );
	}
	/* Begin & End flags */
	char cB = framestart ? 'B' : ' ';
	char cE = frameend   ? 'E' : ' ';
	snprintf( sLabel, sizeof(sLabel), " %c%c", cB, cE );
	strcat( sCstr, sLabel );

	return sCstr;
}

const char* MediaPacket::c_str_short( void )
{
	/* String memory */
	char sLabel [32];
	sLabel[0] = '\0';
	c_alloc();

	/* Label of the packet */
	if( content == content_t::audio )
		strcat( sLabel, "audio-" );
	else if( content == content_t::video )
		strcat( sLabel, "video-" );
	else
		strcat( sLabel, "mixed-" );
	if( type == packet_t::end )
		strcat( sLabel, "end-" );
	else if( type == packet_t::reset )
		strcat( sLabel, "reset-" );

	string s = FrameToString( frame );
	snprintf( sCstr, MAX_STRING_SIZE, "%spacket %06d.%06d.%02d frame(%s) size(%05u)",
				sLabel, (int)unitnumber, (int)framenumber, (int)subframenumber,
				expand(s,3).c_str(), (unsigned)size() );
	return sCstr;
}

/* Function providing unique stream ID's */
static mutex oMediaPacketMutex;
int __iStreamID = 3;
int nextStreamID( void )
{
	int iRet;
	Lock_t oLock( oMediaPacketMutex );
	iRet = __iStreamID++;
	return iRet;
}

typedef struct
{
	deque_t vBuffer;
	double fStep;
} muxx_t;
typedef multimap<double,muxx_t*> map_t;
typedef pair<double,muxx_t*> pair_t;

void zip( deque_t& vBuffer,  int iNum, classify_t oClass, bool bRandomize )
{
	/* Allocate the Multiplexer and classes */
	map_t mMux;
	muxx_t* lMux = new muxx_t [iNum];

	/* Insert them into the Multiplexer */
	for( int i = 0; i < iNum; i++ )
		mMux.insert( pair_t(0.0,lMux+i) );

	/* Iterate over input vector & classify */
	while( vBuffer.size() )
	{
		lMux[oClass(vBuffer.front())].vBuffer.push_back( vBuffer.front() );
		vBuffer.pop_front();
	}
	/* Perfrom inner zip */
	if( bRandomize )
	{
		RandomGenerator oRandom;
		for( int i = 0; i < iNum; i++ )
		{
			muxx_t* pMux = lMux + i;
			for( int j = pMux->vBuffer.size(); j > 0; j-- )
			{
				int n = oRandom.next() % j;
				pMux->vBuffer.push_back( *(pMux->vBuffer.begin()+n) );
				pMux->vBuffer.erase( pMux->vBuffer.begin()+n );
			}
		}
	}
	/* Prepare the Multiplexer */
	for( int i = 0; i < iNum; i++ )
	{
		muxx_t* pMux = lMux+i;
		if( pMux->vBuffer.size() )
			pMux->fStep = 1.0 / pMux->vBuffer.size();
	}
	/* Mux */
	while( mMux.size() )
	{
		/* Fields */
		map_t::iterator oMux = mMux.begin();
		muxx_t* pMux = oMux->second;

		/* Add to the sendbuffer */
		if( pMux->vBuffer.size() )
		{
			//fprintf( stderr, "choose %d\n", pMux-lMux );
			vBuffer.push_back( pMux->vBuffer.front() );
			pMux->vBuffer.pop_front();
		}
		/* Readd the queue if needed */
		mMux.erase( oMux );
		if( pMux->vBuffer.size() )
			mMux.insert( pair_t( oMux->first + pMux->fStep, pMux ) );
	}

	/* Free */
	delete [] lMux;
}
