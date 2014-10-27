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
#ifndef FFMEG_WRITER_H_
#define FFMEG_WRITER_H_
#include "Multiplexer/Multiplexer.h"
#include "ffmpeg.h"

typedef struct stream_t
{
	int id;
	timestamp_t last;
};

class FFmpegMultiplexer : public Multiplexer
{
public:
	FFmpegMultiplexer( const string& sName, ProcessorManager* pScope );
	~FFmpegMultiplexer();

protected:
	virtual void init( void );
	virtual void receive( MediaPacketPtr& pckt );
	virtual void openContext( MediaPacket* pPckt );
	void closeContext( void );
	virtual void setFormat( void );
	virtual void openBuffer( void );
	static int processBuffer0( void* pThis, uint8_t* pBuffer, int iSize );
	virtual int processBuffer( uint8_t* pBuffer, int iSize );
	static int64_t seekBuffer0( void *opaque, int64_t offset, int whence );
	virtual int64_t seekBuffer( int64_t offset, int whence );
	virtual void add_audio_stream( int streamID, const MediaDescriptor* desc );
	virtual void add_video_stream( int streamID, const MediaDescriptor* desc );
	virtual void mux( MediaPacketPtr& pckt );

	map<int,stream_t> dStreams;
	AVFormatContext* pCtx;
	AVOutputFormat* pFmt;
	timestamp_t iDts, iBaseDts;
	int maxID;
	static AVRational x_time_base;
	mux_t::type iMux;
	MP4MediaConverter oMP4Convertor;
	uint8_t* pBuffer;
	uint32_t iBuffer;
	int64_t iDelta, iPos;
	MediaPacket* pCurrent;
	bool bBackwards, bIgnore, bFragmented;
	deque_t oBuffer;
};

#endif /*FFMEG_WRITER_H_*/
