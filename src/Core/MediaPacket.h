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
#ifndef MEDIA_PACKET_H
#define MEDIA_PACKET_H
#include "sirannon.h"
#include "SirannonTime.h"

namespace packet_t  { enum type { media=0, reset, end, /*start*/ }; }
namespace content_t { enum type { audio=0, video, mixed }; }
namespace mux_t     { enum type { ES=0, MOV, TS, PES, RTMP, RTP, FLV, WEBM, M3U, PS, AVI, WMV, MKV, OGG, NO };   }
namespace codec_t   {
	enum family { MPEG1 = 0x0010, MPEG2 = 0x0020, MPEG4 = 0x0040, H264 = 0x0080, WMV = 0x0100, AMR = 0x0200, GOOGLE = 0x0400, FLASH = 0x0800, OTHER = 0x1000 };
	enum type { NO=0, mp1v=MPEG1, mp1a, mp2v=MPEG2, mp2a, mp4v=MPEG4, mp4a, avc=H264, svc, mvc, wmv1=WMV, wmv2, wmv3, wmvc, wma1, wma2, anb=AMR, awb, awbp, vp6=GOOGLE, vp6f, theora, vorbis, vp8, flv1=FLASH, swfa, yuv=OTHER, ac3, pcm, h263, other }; }
namespace target_t  {
	enum family { SMART = 0x0010, TV = 0x0020, PC = 0x0030, OTHER = 0x0040 };
	enum type { NO=0, IPAD1=SMART, IPAD2, IPHONE3, IPHONE4, IPHONE5, IPOD, ANDROID, HDTV=TV, SDTV, YOUTUBE=PC, FLASH }; }
typedef pair<mux_t::type, codec_t::type> container_t;

namespace frame_t
{
	enum type { I=0 /** I-frame */,
				IDR /** I-frame with Instanious Decoder Refresh allowing random acces*/,
				SPS /** Sequence Parameter Set: avc/svc unit describing the structure*/,
				SI /** Switching I-slice: avc/sec unit gracefuly */,
				B /** B-slice */,
				P /** P-slice */,
				SP /** Switch P-slice: avc/svc unit */,
				E /** Prefix NAL: svc unit */ , EI, EP, EB, BI, BP, BB, CI, CP, CB, ESPS, PPS, SEI, AUD, AUDH, other, no_frame }; // #24
}

class rational_t
{
public:
	rational_t() : num(0), den(1) { }
	rational_t( int w, int h ) : num(w), den(h) { }
	void simplify( void );
	bool operator<( const rational_t& other ) const;
	bool operator==( const rational_t& other ) const;
	string str( void ) const;
	double fraction( void ) const;
	int num, den;
};

typedef int32_t timestamp_t;

class MediaDescriptor;
class MediaPacket
{

public:
	/* Type members */
	packet_t::type 	type;
	content_t::type content;
	mux_t::type mux;
	codec_t::type codec;
	frame_t::type frame;
	MediaDescriptor* desc;
	int32_t framenumber, unitnumber, subframenumber, xroute, xstream;
	timestamp_t dts, pts, inc;
	uint8_t T, D, Q;
	bool frameend, framestart, error, key;
	SirannonTime oSend;

	/* Constructors & destructor */
	MediaPacket( packet_t::type type, content_t::type content, uint32_t iSize );
	MediaPacket( const MediaPacket& other );
	MediaPacket( uint8_t* pBuffer, uint32_t iSize );
	MediaPacket( uint32_t iSize );
	~MediaPacket();

	/* Copy member info */
	void set_metadata( const MediaPacket* pckt );

	/* Information */
	const char* c_str( void );
	const char* c_str_long( void );
	const char* c_str_short( void );
	const char* c_str_full( uint32_t iBytes );
	const char* c_str_data( uint32_t iBytes );
	uint8_t* data( void ) const;
	uint32_t size( void ) const;

	/* Data control */
	void push_back( const uint8_t* buffer, uint32_t len );
	void push_back( uint32_t len );
	void push1_back( uint8_t x );
	void push_front( const uint8_t* buffer, uint32_t len );
	void push_front( uint32_t len );
	void push1_front( uint8_t x );
	void pop_back( uint32_t len );
	void pop_front( uint32_t len );
	void clear( void );

	/* Optimalization */
	void memalign( int iStep );

private:
	inline void c_alloc( void );
	uint8_t *_data, *begin, *end;
	char* sCstr;
	int	max_size;
	const static uint32_t MAX_STRING_SIZE = 4096;

	MediaPacket& operator=( const MediaPacket& oOther );
};

typedef auto_ptr<MediaPacket> MediaPacketPtr;
typedef auto_ptr_ref<MediaPacket> MediaPacketPtrRef;

/* Function providing unique streamID's */
int nextStreamID( void );

/* Typedefs */
#include <queue>
#include <deque>
#include <vector>
#include <list>
typedef queue<MediaPacket*> queue_t;
typedef deque<MediaPacket*> deque_t;
typedef deque_t::iterator deque_it;
typedef vector<MediaPacket*> vector_t;
typedef vector_t::iterator vector_it;
typedef list<MediaPacket*> list_t;
typedef list_t::iterator list_it;

/* Very handy function to zip vectors of packets together */
typedef uint32_t (*classify_t) (const MediaPacket*);
void zip( deque_t& vBuffer, int iNum, classify_t oClass, bool bRandomize=false );

/* Inline function */
#include "MediaPacket_priv.h"
#include "MediaDescriptor.h"

#endif
