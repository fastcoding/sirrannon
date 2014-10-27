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
#include "sdp.h"
#include "h264_avc.h"
#include "ffmpeg.h"
#include "Base64.h"
#include "Frame.h"
#include "SirannonException.h"
#include "Utils.h"
#include "Network.h"
#define SIRANNON_USE_BOOST_REGEX
#define SIRANNON_USE_BOOST_TOKENIZER
#include "Boost.h"

static void AACStrop( char* sStrop, int iStrop, const uint8_t* data, int size )
{
	/* Check */
	if( size < 2 )
		RuntimeError( "sdp: AAC extradata too short (%d < 2)", size );

	/* Output */
	snprintf( sStrop, iStrop, "profile-level-id=15;mode=AAC-hbr;sizelength=13;"
			"indexlength=3;indexdeltalength=3;config=%02X%02X", (int)data[0], (int)data[1] );
}

static void mp4vStrop( char* sStrop, int iStrop, const uint8_t* data, int size )
{
	/* Output */
	snprintf( sStrop, iStrop, "profile-level-id=3;mode=generic;sizelength=13;indexlength=3;indexdeltalength=3;config=" );

	/* Add data */
	char sHexa [4];
	for( int i = 0; i < size; i++ )
	{
		snprintf( sHexa, sizeof(sHexa), "%02X", data[i] );
		strcat( sStrop, sHexa );
	}
}

static void AudioTimestamp( char* sTimestamp, int iTimestamp, AVCodecContext* pCodec )
{
	snprintf( sTimestamp, iTimestamp, "%d/%d", pCodec->sample_rate, pCodec->channels );
}

static bool AVCRawStrop( char* sStrop, int iStrop, const uint8_t* pData, int iSize )
{
	string sStr;
	const uint8_t* pSps = NULL;
	bool bSVC = false;
	for( int iPos=0, iOffset=1; iPos < iSize; iPos += iOffset )
	{
		/* Read length part */
		iOffset = H264_find_NAL( pData+iPos, iSize-iPos );

		/* Skip startcode */
		int iHeader = H264_is_start_code( pData+iPos );
		if( iOffset < iHeader )
			RuntimeError( "sdp: NAL unit too small, (%d<%d)", iOffset, iHeader );

		/* Type of nal */
		int iType = H264_NAL_type( pData+iPos );
		if( iType == NAL_UNIT_TYPE_SEI )
			continue;
		else if( 	iType != NAL_UNIT_TYPE_SPS and
					iType != NAL_UNIT_TYPE_PPS and
					iType != NAL_UNIT_TYPE_SPS_EXT and
					iType != NAL_UNIT_TYPE_SPS_SVC )
			break;

		/* Detect svc */
		if( iType == NAL_UNIT_TYPE_SPS_SVC )
			bSVC = true;

		/* Base64 encode the nal */
		char* sNal = Base64Encode( pData+iPos+iHeader, iOffset-iHeader );
		sStr.append( sNal );
		sStr.append( "," );
		delete [] sNal;

		/* Check if the unit is an SPS, if so save the profile */
		if( pSps == NULL and iType == NAL_UNIT_TYPE_SPS )
			pSps = pData + iPos + iHeader + 1;
	}

	/* Profile */
	char sProfile[16];
	if( pSps == NULL )
		RuntimeError( "sdp: avc raw: no SPS found" );
	snprintf( sProfile, sizeof(sProfile), "%02X%02X%02X", pSps[0], pSps[1], pSps[2] );

	/* Remove the last ' */
	if( sStr.at(sStr.size()-1) == ',' )
		sStr.erase( sStr.size()-1, 1 );

	/* Output */
	snprintf( sStrop, iStrop, "packetization-mode=1;profile-level-id=%s;sprop-parameter-sets=%s", sProfile, sStr.c_str() );
	return bSVC;
}

static void AVCMovStrop( char* sStrop, int iStrop, const uint8_t* data, int size )
{
	/* Check minimum size */
	if( size < 7 )
		RuntimeError( "sdp: AVC extradata too short (%d < 7)", size );

	/* Data */
	int spsSize = 0, ppsSize = 0, i = 0;

	/* Skip one byte */
	i++;

	/* Sprop */
	char sProfile[16];
	snprintf( sProfile, sizeof(sProfile), "%02X%02X%02X", data[i], data[i+1], data[i+2] );
	i += 3;

	/* Skip one byte */
	i++;

	/* Number of SPS */
	string sStr;
	uint32_t i_sps = data[i++] & 0x1F;
	//fprintf( stderr, "detected (%d) SPS\n", i_sps );
	for( uint32_t j = 0; j < i_sps; j++ )
    {
		/* Size of this SPS */
		spsSize = ntoh16(*(uint16_t*)(data+i));
		i += 2;
		//fprintf( stderr, "\tsps size(%d)\n", spsSize );

		/* SDP */
		char* sSps = Base64Encode( data+i, spsSize );
		i += spsSize;
		sStr.append( sSps );
		sStr.append( "," );
		delete [] sSps;
    }

    /* Number of SPS */
	uint32_t i_pps  = data[i++] & 0x1F;
	//fprintf( stderr, "detected (%d) PPS\n", i_pps );
	for( uint32_t j = 0; j < i_pps; j++ )
    {
		/* Size of this PPS */
		ppsSize = ntoh16(*(uint16_t*)(data+i));
		i += 2;
		//fprintf( stderr, "\tpps size(%d)\n", ppsSize );

		/* SDP */
		char* sPps = Base64Encode( data+i, ppsSize );
		i += ppsSize;
		sStr.append( sPps );
		sStr.append( "," );
		delete [] sPps;
    }
	/* Remove the last ' */
	if( sStr.at(sStr.size()-1) == ',' )
		sStr.erase( sStr.size()-1, 1 );

	/* Check maximum size */
	if( size < i )
		RuntimeError( "sdp: AVC extradata too long (%d > %d)", i, size );

	/* Output */
	snprintf( sStrop, iStrop, "packetization-mode=1;profile-level-id=%s;sprop-parameter-sets=%s", sProfile, sStr.c_str() );
}

void SDPString( const ContainerDescriptor* pSDP, string& sSDP )
{
	sSDP.append( "v=0\r\n"
				 "c=IN IP4 127.0.0.1\r\n"
				 "s= \r\n"
				 "t=0 0\r\n" );

	for( ContainerDescriptor::const_iterator pDesc = pSDP->begin(); pDesc != pSDP->end(); ++pDesc )
		sSDP.append( pDesc->sdp );
}

int SDPConstruct( const ContainerDescriptor* pSrcContainer, int iPort, bool bMP2TS, ContainerDescriptor* pContainer )
{
	/* Intial values */
	int iPayload = -1, iStream = 1, iDynamicPayload = 96;
	codec_t::type iCodec;
	string sSDP;
	char sTmp[1024];
	char sType [16];
	char sCodec [64];
	char sStrop [512];
	char sTimestamp [16];
	pContainer->clear();
	using namespace codec_t;

	/* Iterate over the streams */
	if( not bMP2TS )
	{
		for( ContainerDescriptor::const_iterator pSrcDesc = pSrcContainer->begin();
				pSrcDesc != pSrcContainer->end(); ++pSrcDesc )
		{
			/* Next track */
			MediaDescriptor* pDesc = pContainer->addDescriptor( &*pSrcDesc );
			sSDP.clear();

			/* Analyze */
			if( pDesc->content == content_t::video and pDesc->codec != NO )
			{
				strcpy( sType, "video" );
				strcpy( sTimestamp, "90000" );

				switch( pDesc->codec )
				{
				case mp4v:
					strcpy( sCodec, "MP4V-ES" );
					iPayload = iDynamicPayload++;
					mp4vStrop( sStrop, sizeof(sStrop), pDesc->getExtraData(), pDesc->getExtraSize() );
					break;

				case avc:
					strcpy( sCodec, "H264" );
					iPayload = iDynamicPayload++;
					//if( pSrcContainer->mux != mux_t::ES ) Always MP4 structure
						AVCMovStrop( sStrop, sizeof(sStrop), pDesc->getExtraData(), pDesc->getExtraSize() );
					//else
					//	AVCRawStrop( sStrop, sizeof(sStrop), pDesc->getExtraData(), pDesc->getExtraSize() );
					break;

				case mp1v:
				case mp2v:
					strcpy( sCodec, "MPV" );
					iPayload = 32;
					strcpy( sStrop, "" );
					break;

				default:
					SirannonWarning( "TypeError: Codec(%s) not supported with SDP", CodecToString(pDesc->codec) );
					continue;
				}
			}
			else if( pDesc->content == content_t::audio and pDesc->codec != NO )
			{
				strcpy( sType, "audio" );
				snprintf( sTimestamp, sizeof(sTimestamp), "%d/%d", pDesc->samplerate, pDesc->channels );

				switch( pDesc->codec )
				{
				case mp4a:
					strcpy( sCodec, "mpeg4-generic" );
					iPayload = iDynamicPayload++;
					AACStrop( sStrop, sizeof(sStrop), pDesc->getExtraData(), pDesc->getExtraSize() );
					break;

				case mp2a:
				case mp1a:
					strcpy( sCodec, "MPA" );
					iPayload = 14;
					strcpy( sStrop, "" );
					break;

				case anb:
					strcpy( sCodec, "AMR" );
					iPayload = iDynamicPayload++;
					strcpy( sStrop, "" );
					break;

				case awb:
					strcpy( sCodec, "AMR-WB" );
					iPayload = iDynamicPayload++;
					strcpy( sStrop, "" );
					break;

				case ac3:
					strcpy( sCodec, "ac3" );
					iPayload = iDynamicPayload++;
					strcpy( sStrop, "" );
					break;

				default:
					SirannonWarning( "TypeError: Codec(%s) not supported with SDP", CodecToString(pDesc->codec) );
					continue;
				}
			}
			else
				continue;

			/* Generate SDP text */
			snprintf( sTmp, sizeof(sTmp), "m=%s %d RTP/AVP %d\r\n", sType, iPort, iPayload );
			sSDP.append( sTmp );
			snprintf( sTmp, sizeof(sTmp), "a=rtpmap:%d %s/%s\r\n", iPayload, sCodec, sTimestamp );
			sSDP.append( sTmp );
			snprintf( sTmp, sizeof(sTmp), "a=control:track%d\r\n", iStream );
			sSDP.append( sTmp );

			/* Optional strop field */
			if( strlen(sStrop) )
			{
				snprintf( sTmp, sizeof(sTmp), "a=fmtp:%d %s\r\n", iPayload, sStrop );
				sSDP.append( sTmp );
			}
			/* Set */
			pDesc->payload = iPayload;
			pDesc->clientPort = iPort;
			pDesc->sdp = sSDP;
			snprintf( sTmp, sizeof(sTmp), "track%d", iStream );
			pDesc->track = sTmp;

			/* Increase */
			if( iPort > 0 ) iPort += 2;
			iStream++;
		}
	}
	else
	{
		/* Descriptor */
		MediaDescriptor* pDesc = pContainer->addDescriptor();

		/* Fixed SDP header */
		snprintf( sTmp, sizeof(sTmp), "m=video %d RTP/AVP 33\r\n", iPort );
		sSDP.append( sTmp );
		snprintf( sTmp, sizeof(sTmp), "a=rtpmap:33 MP2T/90000\r\n" );
		sSDP.append( sTmp );
		snprintf( sTmp, sizeof(sTmp), "a=control:track1\r\n" );
		sSDP.append( sTmp );

		/* Parameters */
		pDesc->content = content_t::mixed;
		pDesc->payload = 33;
		pDesc->clientPort = iPort;
		pDesc->sdp = sSDP;
		pDesc->track = "";
		pDesc->route = 0;
	}

	/* Succes */
	return 0;
}

int SDPConstruct( const string& sFile, int iPort, bool bMP2TS,
		ContainerDescriptor* pContainer, mux_t::type& oMux )
{
	/* Open video file */
	AVFormatContext* pFormatCtx = NULL;
	if( av_open_input_file( &pFormatCtx, sFile.c_str(), NULL, 0, NULL )!= 0 )
		return -1;

	/* Retrieve stream information */
	if( av_find_stream_info( pFormatCtx ) < 0 )
		return -1;

	/* Intial values */
	int iPayload = -1, iStream = 1, iDynamicPayload = 96;
	codec_t::type iCodec;
	content_t::type iType;
	string sSDP;
	char sTmp[1024];
	char sType [16];
	char sCodec [64];
	char sStrop [512];
	char sTimestamp [16];
	pContainer->clear();

	/* Container */
	oMux = FFFormatToSirannonFormat( pFormatCtx->iformat ).first;

	/* Iterate over the streams */
	if( not bMP2TS )
	{
		/* Fixed video, audio order */
		AVCodecContext* oCodecs [2] = { NULL, NULL };
		for( uint32_t i = 0; i < pFormatCtx->nb_streams; i++ )
		{
			AVCodecContext* c = pFormatCtx->streams[i]->codec;
			if( c->codec_type == AVMEDIA_TYPE_VIDEO )
				oCodecs[0] = c;
			else if( c->codec_type == AVMEDIA_TYPE_AUDIO )
				oCodecs[1] = c;
		}

		for( int i = 0; i < 2; i++ )
		{
			/* Defined? */
			AVCodecContext* c = oCodecs[i];
			if( c == NULL )
				continue;

			/* Add to stream */
			MediaDescriptor* pDesc = pContainer->addDescriptor();
			sSDP.clear();

			/* For each stream fill out codec name, timestamp, type,
			 * payload and optional strop */
			if( c->codec_type == AVMEDIA_TYPE_VIDEO )
			{
				strcpy( sType, "video");
				strcpy( sTimestamp, "90000");
				iType = content_t::video;

				switch( c->codec_id )
				{
				case CODEC_ID_MPEG4:
					strcpy( sCodec, "MP4V-ES" );
					iPayload = iDynamicPayload++;
					mp4vStrop( sStrop, sizeof(sStrop), c->extradata, c->extradata_size );
					break;

				case CODEC_ID_H264:
					if( oMux != mux_t::ES )
					{
						strcpy( sCodec, "H264");
						iPayload = iDynamicPayload++;
						AVCMovStrop( sStrop, sizeof(sStrop), c->extradata, c->extradata_size );
					}
					else
					{
						AVPacket oFrame, *pFrame = &oFrame;
						pFrame->data = NULL;
						av_read_frame( pFormatCtx, pFrame );
						strcpy( sCodec, "H264" );
						iPayload = iDynamicPayload++;
						bool bSVC = AVCRawStrop( sStrop, sizeof(sStrop), pFrame->data, pFrame->size );
						if( pFrame->data != NULL )
							av_free_packet( pFrame );
					}
					break;

				case CODEC_ID_MPEG2VIDEO:
				case CODEC_ID_MPEG1VIDEO:
					strcpy( sCodec, "MPV" );
					iPayload = 32;
					strcpy( sStrop, "" );
					break;

				default:
					SirannonWarning( "sdp: video codec \"%s\" not supported in \"%s\"",
							CodecToString(FFCodecToSirannonCodec(c->codec_id)), sFile.c_str() );
					continue;
				}
			}
			else if( c->codec_type == AVMEDIA_TYPE_AUDIO )
			{
				strcpy( sType, "audio" );
				iType = content_t::audio;
				AudioTimestamp( sTimestamp, sizeof(sTimestamp), c );

				switch( c->codec_id )
				{
				case CODEC_ID_AAC:
					strcpy( sCodec, "mpeg4-generic" );
					iPayload = iDynamicPayload++;
					AACStrop( sStrop, sizeof(sStrop), c->extradata, c->extradata_size );
					break;

				case CODEC_ID_MP2:
				case CODEC_ID_MP3:
					strcpy( sCodec, "MPA" );
					iPayload = 14;
					strcpy( sStrop, "" );
					break;

				case CODEC_ID_AMR_NB:
				case CODEC_ID_AMR_WB:
					strcpy( sCodec, (c->codec_id == CODEC_ID_AMR_NB) ? "AMR" : "AMR-WB" );
					iPayload = iDynamicPayload++;
					strcpy( sStrop, "" );
					break;

				case CODEC_ID_AC3:
					strcpy( sCodec, "ac3" );
					iPayload = iDynamicPayload++;
					strcpy( sStrop, "" );
					break;

				default:
					SirannonWarning( "SDP: Audio codec(%s) not supported in (%s)",
							CodecToString(FFCodecToSirannonCodec(c->codec_id)), sFile.c_str() );
					continue;
				}
			}
			else
				continue;

			/* Generate SDP text */
			snprintf( sTmp, sizeof(sTmp), "m=%s %d RTP/AVP %d\r\n", sType, iPort, iPayload );
			sSDP.append( sTmp );
			snprintf( sTmp, sizeof(sTmp), "a=rtpmap:%d %s/%s\r\n", iPayload, sCodec, sTimestamp );
			sSDP.append( sTmp );
			snprintf( sTmp, sizeof(sTmp), "a=control:track%d\r\n", iStream );
			sSDP.append( sTmp );

			/* Optional strop field */
			if( strlen(sStrop) )
			{
				snprintf( sTmp, sizeof(sTmp), "a=fmtp:%d %s\r\n", iPayload, sStrop );
				sSDP.append( sTmp );
			}
			/* Set */
			pDesc->content = iType;
			pDesc->codec = FFCodecToSirannonCodec( c->codec_id );
			pDesc->payload = iPayload;
			pDesc->clientPort = iPort;
			pDesc->sdp = sSDP;
			snprintf( sTmp, sizeof(sTmp), "track%d", iStream );
			pDesc->track = sTmp;
			pDesc->route = iStream * 100;

			/* Increase */
			if( iPort > 0 ) iPort += 2;
			iStream++;
		}
	}
	else
	{
		/* Descriptor */
		MediaDescriptor* pDesc = pContainer->addDescriptor();

		/* Fixed SDP header */
		snprintf( sTmp, sizeof(sTmp), "m=video %d RTP/AVP 33\r\n", iPort );
		sSDP.append( sTmp );
		snprintf( sTmp, sizeof(sTmp), "a=rtpmap:33 MP2T/90000\r\n" );
		sSDP.append( sTmp );

		/* Parameters */
		pDesc->content = content_t::mixed;
		pDesc->payload = 33;
		pDesc->clientPort = iPort;
		pDesc->sdp = sSDP;
		pDesc->track = "";
		pDesc->route = 0;
	}
	/* Close the video file */
	av_close_input_file( pFormatCtx );

	/* Succes */
	return 0;
}

int SDPParse( const string& oSDP, mux_t::type& oMux, ContainerDescriptor* pContainer )
{
	const char* sSDP = oSDP.c_str();
	cmatch oParse;
	string sMode, sCodec;

	/* Check until the first 'm=' */
	int iPos = 0;
	int iStream = 0;
	while( (iPos = oSDP.find( "m=", iPos )) != string::npos )
	{
		/* Add a discriptor */
		MediaDescriptor* pDesc = pContainer->addDescriptor();
		pDesc->route = ++iStream * 100;

		/* Parse the media expression */
		static const regex oMediaExpression( "m=(\\w+) (\\d+) ([\\w/]+) (\\d+)\\r\\n" );
		if( not regex_search( sSDP + iPos, oParse, oMediaExpression, match_continuous ) )
			return SirannonWarning( "SDP: Invalid media description" );
		iPos += ( oParse.suffix().first - oParse.prefix().first );
		pDesc->clientPort = atoi( oParse[2].first );

		/* Content type */
		if( strncmp( oParse[1].first, "video", 5 ) == 0 )
			pDesc->content = content_t::video;
		else if( strncmp( oParse[1].first, "audio", 5 ) == 0 )
			pDesc->content = content_t::audio;
		else
			pDesc->content = content_t::mixed;

		/* Parse additional expressions */
		while( sSDP[iPos] == 'a' )
		{
			/* Parse the additional expression start */
			static const regex oAddExpression( "a=(\\w+):(\\w+)" );
			if( not regex_search( sSDP + iPos, oParse, oAddExpression, match_continuous ) )
				return SirannonWarning( "SDP: Invalid additional description" );
			iPos += ( oParse.suffix().first - oParse.prefix().first );
			sMode.assign( oParse[1].first, oParse[1].second );

			/* Match the additional field */
			if( sMode == "fmtp" )
			{
				/* Different formats for each codec */
				if( pDesc->codec == codec_t::mp4a )
				{
					/* Parse the mp4a format parameter expression */
					static const regex oFmtpExpression( " profile-level-id=\\d+;mode=[\\w\\-]+;sizelength=\\d+;indexlength=\\d;indexdeltalength=\\d;config=(\\w+)\\r\\n");
					if( not regex_search( sSDP + iPos, oParse, oFmtpExpression, match_continuous ) )
						return SirannonWarning( "SDP: Invalid format parameter description (%s)", sCodec.c_str() );
					iPos += ( oParse.suffix().first - oParse.prefix().first );

					/* Write directly into the extra data */
					OBits oHeader( pDesc->extra_data, 2 );
					pDesc->extra_size = 2;
					oHeader.write( 16, strtoul( oParse[1].first, NULL, 16 ) );

					/* Deduce some info from the data */
					MP4MediaConverter::ESDS2META( pDesc );
				}
				else if( pDesc->codec & codec_t::H264 )
				{
					/* Parse the avc format parameter expression */
					static const regex oFmtpExpression( " packetization-mode=(\\d);profile-level-id=(\\w+);sprop-parameter-sets=([\\w\\+/=,\\-]+)\\r\\n");
					if( not regex_search( sSDP + iPos, oParse, oFmtpExpression, match_continuous ) )
						return SirannonWarning( "SDP: Invalid format parameter description (%s)", sCodec.c_str() );
					iPos += ( oParse.suffix().first - oParse.prefix().first );

					/* Write directly into the extra data */
					OBits oVideoExtra( pDesc->extra_data, oSDP.length() );

					/* Profile */
					oVideoExtra.write( 8, 0x01 ); /* version */
					oVideoExtra.write( 24, strtoul( oParse[2].first, NULL, 16 ) );
					oVideoExtra.write( 8, 0xFF ); /* 6 bits reserved (111111) + 2 bits nal size length - 1 (11) */

					/* Parameter sets */
					string sParams( oParse[3].first, oParse[3].second );
					Delim_t oDelim( "," );
					Tokenizer_t oToken( sParams, oDelim );
					for( Tokenizer_t::iterator i = oToken.begin(); i != oToken.end(); ++i )
					{
						uint32_t iSize = i->length();
						uint8_t* pParameterSet = Base64Decode( (char*)i->c_str(), iSize );
						oVideoExtra.write( 8, 0xE0 | 1 );
						oVideoExtra.write( 16, iSize );
						oVideoExtra.write_buffer( pParameterSet, iSize );
						delete [] pParameterSet;
					}
					pDesc->extra_size = oVideoExtra.size();

					MP4MediaConverter::AVCC2META( pDesc );
				}
				else if( pDesc->codec == codec_t::mp4v )
				{
					/* Parse the mp4a format parameter expression */
					static const regex oFmtpExpression( " profile-level-id=\\d+;mode=[\\w\\-]+;sizelength=\\d+;indexlength=\\d;indexdeltalength=\\d;config=([0-9A-Fa-f]+)\\r\\n");
					if( not regex_search( sSDP + iPos, oParse, oFmtpExpression, match_continuous ) )
						return SirannonWarning( "SDP: Invalid format parameter description (%s)", sCodec.c_str() );
					iPos += ( oParse.suffix().first - oParse.prefix().first );

					/* Write directly into the extra data */
					int iHeader = oParse[1].second - oParse[1].first;
					if( iHeader % 2 )
						return SirannonWarning( "SDP: MP4V fmtp config contains an odd number of hexdeicimal symbols" );
					IBits oHeader( (const uint8_t*) oParse[1].first, iHeader );
					pDesc->extra_size = iHeader / 2;
					for( int i = 0; i < pDesc->extra_size; ++i )
						pDesc->extra_data[i] = oHeader.read_hex();


					SirannonWarning( strArray( pDesc->extra_data, pDesc->extra_size ).c_str() );

					/* Deduce some info from the data */
					//MP4MediaConverter::ESDS2META( pDesc );
				}
				else
				{
					int iOff = oSDP.find( '\n', iPos );
					if( iOff != string::npos )
						return SirannonWarning( "SDP: Invalid format parameter description (%s)", sCodec.c_str() );
					iPos += iOff + 1;
				}
			}
			else if( sMode == "rtpmap" )
			{
				pDesc->payload = atoi( oParse[2].first );

				/* Parse the line */
				static const regex oRtpExpression( " ([\\w\\-]+)/(\\d+)(?:/(\\d+))?\\r\\n" );
				if( not regex_search( sSDP + iPos, oParse, oRtpExpression, match_continuous ) )
					return SirannonWarning( "SDP: Invalid rtpmap description" );
				iPos += ( oParse.suffix().first - oParse.prefix().first );

				/* Match the codec */
				sCodec.assign( oParse[1].first, oParse[1].second );
				using namespace codec_t;
				oMux = mux_t::RTP;
				if( sCodec == "H264" )
					pDesc->codec = avc;
				else if( sCodec == "MP4V-ES" )
					pDesc->codec = mp4v;
				else if( sCodec == "MPV" )
					pDesc->codec = mp2v;
				else if( sCodec == "mpeg4-generic" )
					pDesc->codec = mp4a;
				else if( sCodec == "MPA" )
					pDesc->codec = mp2a;
				else if( sCodec == "MP2T" )
					oMux = mux_t::TS;
				else if( sCodec == "ac3" )
					pDesc->codec = ac3;
				else if( sCodec == "AMR" )
					pDesc->codec = anb;
				else if( sCodec == "AMR-WB" )
					pDesc->codec = awb;
				else
					return SirannonWarning( "SDP: Unsupported codec(%s)", sCodec.c_str() );

				/* Sample rate & channels */
				if( oParse[3].matched )
				{
					pDesc->samplerate = atoi( oParse[2].first );
					pDesc->channels = atoi( oParse[3].first );
				}
			}
			else if( sMode == "control" )
			{
				/* Match the track */
				pDesc->track.assign( oParse[2].first, oParse[2].second );

				if( strncmp( sSDP + iPos, "\r\n", 2 ) != 0 )
					return SirannonWarning( "SDP: Invalid control description" );
				iPos += 2;
			}
		}
	}
	return 0;
}
