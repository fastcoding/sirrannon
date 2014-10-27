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
#ifndef RTMPPROXY_H_
#define RTMPPROXY_H_
#include "RTMPChunkStream.h"
#include "Interfaces.h"
#include "RTMPHandshake.h"
#include "Frame.h"

class RTMPClient : public MediaProcessor, public RTMPChunkStream, public ClientInterface
{
public:
	RTMPClient( const char* sName, ProcessorManager* pProc );
	virtual ~RTMPClient();

	virtual void process( void );
	virtual void init( void );
	virtual void mainThreadA( void );

	/* Commands */
	virtual int seek( uint32_t );
	virtual int play( double );
	virtual int pause( void );
	virtual int flush( void );
	virtual bool ready( void ) const;

protected:
	virtual int handleCommunication( void );
	virtual int handleHandshake( void );
	virtual int handleApplicationConnect( void );
	virtual int handlePlay( void );

	virtual int sendCommandConnect( void );
	virtual int sendCommandCreateStream( void );
	virtual int sendCommandPlay( void );
	virtual int sendCommandSeek( uint32_t iSeek );
	virtual int sendCommandPauseRaw( bool bPause, uint32_t iPause );

	virtual int receiveResponse( const string& sCommand, int iMessageType, const char* sMode=NULL );

	virtual int handleCommandAMF0( message_t* );
	virtual int handleMetaDataAMF0( message_t* );
	virtual int handleVideo( message_t* );
	virtual int handleAudio( message_t* );
	virtual int handleEnd( void );

	virtual void printState( void );

	string sUrl, sStatus, sCode, sDesc, sServer, sApp, sMedia, sProtocol;
	int iPort;
	map<string,uint64_t> mResponseInt;
	map<string,string> mResponseString;
	map<string,bool> mResponseBool;
	int iAMF, iVideoRoute, iAudioRoute, iVideoStream, iAudioStream, iVideoUnit, iAudioUnit;
	timestamp_t iTimestamp, iLastVideoDts, iLastAudioDts;
	bool bFlush, bPause, bVideo, bAudio, bVideoMetaData, bAudioMetaData, bWait, bAnnexB, bVideoMov, bAudioMov;
	RTMPHandshake oHandshake;
	queue_t mBuffer;
	MP4MediaConverter oAnnexB;
	MediaDescriptor *pVideoDesc, *pAudioDesc;
};

#endif /* RTMPPROXY_H_ */
