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
#ifndef TS_DEMUXER_H_
#define TS_DEMUXER_H_
#include "sirannon.h"

static const int iMaxFrame = 1024*1024;

typedef struct
{
	uint8_t pData [iMaxFrame];
	int iPos;
	int iStream;
	int iFrame;
	int iUnit;
	codec_t::type iCodec;
	content_t::type iContent;
	MediaDescriptor desc;
	int iRoute;
	int iXstream;
} STREAM_t;

typedef struct
{
	map<int,int> mStreams;
	int iChannel;
	int iVersion;
	int iPCR;
} PMT_t;

typedef struct
{
	map<int,STREAM_t*> mStreams;
	map<int,PMT_t> mChannels;
	map<int,int> mCC;
	int iVersion;
	int iID;
} PAT_t;

class TSDemultiplexer : public MediaProcessor
{
public:
	TSDemultiplexer( const string& sName, ProcessorManager* pScope );
	~TSDemultiplexer();

protected:
	void init( void );
	void receive( MediaPacketPtr& pckt );
	void receive_end( MediaPacketPtr& pckt );
	void receive_reset( MediaPacketPtr& pckt );

	int parseTS( MediaPacketPtr& pckt, uint8_t* pData );
	int parsePAT( uint8_t* pData, int iSize );
	int parsePMT( uint8_t* pData, int iSize, int iPMT );
	int parsePKT( uint8_t* pData, int iSize, int iPID, bool bNew, bool bSkip );

	PAT_t oPAT, *pPAT;
	int iVideoRoute, iAudioRoute;
};


#endif /*TS_DEMUXER_H_*/
