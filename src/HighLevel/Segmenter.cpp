/*
 * Segmenter.cpp
 *
 *  Created on: Mar 29, 2011
 *      Author: arombaut
 */

#include "Segmenter.h"
#include "Reader/FFmpegReader.h"
#define SIRANNON_USE_BOOST_FILESYSTEM
#include "Boost.h"

/** TS-segmenter
 * @component TS-segmenter
 * @type high-level
 * @param media, string, dat/media, path of the folder in which to search for requested files
 * @param filename, string, , the path of the container to stream (relative to the folder specified by media)
 * @param target, string, , string indicating the target display (iphone, ipad, youtube)
 * @param duration, int, 10, in seconds the maximum duration of one segment
 * @param bitrate-0, int, 0, in kbps, the 1st bit rate set point, if 0, do not transcode and only segment and do not check for more bitrates
 * @param width-0, int, 0, the 1st width set point
 * @param height-0, int, 0, the 1st height set point
 * @param bitrate-1, int, 0, in kbps, the 2nd bit rate set point, if 0, do not transcode and only segment and do not check for more bitrates
 * @param width-1, int, 0, the 2nd width set point
 * @param height-1, int, 0, the 2nd height set point
 * @param bitrate-2, int, 0, in kbps, the 3rd bit rate set point, if 0, do not transcode and only segment and do not check for more bitrates
 * @param width-2, int, 0, the 3rd width set point
 * @param height-2, int, 0, the 3rd height set point
 * @info This component ingest a video container and generates a series of transport streams
 * at varies bit rates
 **/
REGISTER_CLASS( Segmenter, "TS-segmenter" );

Segmenter::Segmenter( const string& sName, ProcessorManager* pScope )
	: MediaProcessor(sName, pScope), NestedInterface(sName, pScope)
{
	mString["target"] = "";
	mString["filename"] = "";
	mInt["duration"] = 10;
	mString["media"] = "dat/media";
}

Segmenter::~Segmenter()
{ }

void Segmenter::handleError( SirannonException* pException, ProcessorManager* pManager, MediaProcessor* pProcessor )
{
	pScope->handleError( pException, pScope, pProcessor );
}

void Segmenter::init( void )
{
	MediaProcessor::init();

	/* Names */
	char sName [1024];
	const filesystem::path sMedia( mString["media"] );
	const filesystem::path sFile( mString["filename"] );
	const filesystem::path sStem( sFile.branch_path() / sFile.stem() );
	const filesystem::path sFull( sMedia / sStem );
	const filesystem::path sTotal( sMedia / sFile );

	/* Create the reader */
	MediaProcessor* pReader = oProcessorManager.createProcessor( "ffmpeg-reader", "ffmpeg-reader" );
	SourceInterface* pSource = dynamic_cast<SourceInterface*>( pReader );
	pReader->setString( "filename", sTotal.native_file_string().c_str() );
	pReader->setBool( "repeat-parameter-sets", true );
	pReader->initProcessor();

	/* Create the sink */
	MediaProcessor* pSink = oProcessorManager.createProcessor( "sink", "sink" );

	/* Create M3U */
	snprintf( sName, sizeof(sName), "%s.m3u", sFull.native_file_string().c_str() );
	fileLog oGlobal( sName, "w" );
	oGlobal.write( "#EXTM3U\n" );

	/* Loop over all bitrates */
	int i = 0;
	bool bTranscode = true;
	for( i = 0; bTranscode; ++i )
	{
		/* Bitrate & dimensions */
		char sVar [64];
		snprintf( sVar, sizeof(sVar), "bitrate-%d", i );
		int iBitrate = mInt[sVar] * 1000;
		snprintf( sVar, sizeof(sVar), "width-%d", i );
		int iWidth = mInt[sVar];
		snprintf( sVar, sizeof(sVar), "height-%d", i );
		int iHeight = mInt[sVar];

		const ContainerDescriptor* pCont = pSource->getDescriptor();
		bTranscode = (iBitrate > 0);
		if( iBitrate == 0 )
			iBitrate = pCont->bitrate;
		if( iHeight <= 0 or iWidth <= 0 )
		{
			if( pCont->getVideoDescriptor() )
			{
				iWidth = pCont->getVideoDescriptor()->width;
				iHeight = pCont->getVideoDescriptor()->height;
			}
		}
		/* Create the transport stream multiplexer */
		snprintf( sName, sizeof(sName), "TS-multiplexer-%d", i );
		MediaProcessor* pTS = oProcessorManager.createProcessor( "TS-multiplexer", sName );
		pTS->setInt( "streams", pCont->size() );
		pTS->setBool( "continuous-timestamps", false );
		pTS->setBool( "aggregate", false );
		pTS->setBool( "psi-on-key", true );
		pTS->setBool( "interleave", false );

		/* Transcoder and packetizer for each content */
		for( int j = 0; j < pCont->size(); ++j )
		{
			const MediaDescriptor* pDesc = &(*pCont)[j];

			/* Transcoder */
			snprintf( sName, sizeof(sName), "transcoder-%d-%d", iBitrate, j );
			MediaProcessor* pTranscoder = oProcessorManager.createProcessor(
					(pDesc->content == content_t::video and bTranscode) ? "transcoder-video" : "dummy", sName );
			pTranscoder->setInt( "bitrate", iBitrate );
			pTranscoder->setInt( "width", iWidth );
			pTranscoder->setInt( "height", iHeight );
			pTranscoder->setString( "target", mString["target"] );
			pTranscoder->setBool( "debug", bDebug );

			/* PES-packetizer */
			snprintf( sName, sizeof(sName), "packetizer-%d-%d", iBitrate, j );
			MediaProcessor* pPacketizer = oProcessorManager.createProcessor( "PES-packetizer", sName );
			pPacketizer->setBool( "insert-AUD", true );
			pPacketizer->setBool( "zero-length", (pDesc->content == content_t::video) );
			pPacketizer->setBool( "aggregate-audio", true );

			/* Connect */
			pReader->setRoute( pDesc->route, pTranscoder );
			pTranscoder->setRoute( 0, pPacketizer );
			pPacketizer->setRoute( 0, pTS );

			/* Init */
			pTranscoder->initProcessor();
			pPacketizer->initProcessor();
		}
		/* Splitter */
		snprintf( sName, sizeof(sName), "splitter-%d", i );
		MediaProcessor* pSplitter = oProcessorManager.createProcessor( "time-splitter", sName );
		pSplitter->setInt( "interval", mInt["duration"] * 1000 );
		pSplitter->setBool( "key", true );
		//pSplitter->setBool( "debug", true );
		pTS->setRoute( 0, pSplitter );

		/* Write directly to a file */
		snprintf( sName, sizeof(sName), "writer-%d", i );
		MediaProcessor* pWriter = oProcessorManager.createProcessor( "basic-writer", sName );
		snprintf( sName, sizeof(sName), "%s-%d.ts", sFull.native_file_string().c_str() , iBitrate );
		pWriter->setString( "filename", sName );
		pWriter->setBool( "fragmented", true );
		pSplitter->setRoute( 0, pWriter );
		pWriter->setRoute( 0, pSink );

		/* Init */
		pTS->initProcessor();
		pSplitter->initProcessor();
		pWriter->initProcessor();

		/* Create the local M3U */
		snprintf( sName, sizeof(sName), "%s-%d.m3u", sFull.native_file_string().c_str() , iBitrate );
		fileLog oLocal( sName, "w" );

		const int iMaxDur = pCont->duration / 1000000;
		const int iStep = mInt["duration"];

		oLocal.write( "#EXTM3U\n" );
		oLocal.write( "#EXT-X-TARGETDURATION:%d\n", iStep );
		oLocal.write( "#EXT-X-MEDIA-SEQUENCE:0\n" );
		if( pCont->duration )
		{
			for( int iTime = 0, k = 0; iTime < iMaxDur; iTime += iStep, k++ )
			{
				const int iDur = MIN( iStep, iMaxDur - iTime );
				oLocal.write( "#EXTINF:%d,\n", iDur );
				oLocal.write( "/HTTP/%s-%d-%d.ts\n", sStem.native_file_string().c_str() , iBitrate, k );
			}
		}
		else
		{
			oLocal.write( "#EXTINF:%d,\n", 1000 );
			oLocal.write( "%s-%d-0.ts\n", sStem.native_file_string().c_str(), iBitrate );
		}
		oLocal.write( "#EXT-X-ENDLIST\n" );
		oLocal.close();

		/* Global M3U */
		oGlobal.write( "#EXT-X-STREAM-INF:PROGRAM-ID=1, BANDWIDTH=%d\n", iBitrate );
		oGlobal.write( "/M3U-FILE/%s-%d.m3u\n", sStem.native_file_string().c_str(), iBitrate );
	}
	/* Now we now the number of streams */
	pSink->setInt( "count", i );
	pSink->initProcessor();
	pSource->findSchedulers();

	/* End global M3U */
	oGlobal.close();

	/* Ready & able */
	bSchedule = true;
}

void Segmenter::process( void )
{
	/* Schedule the processors */
	if( not oProcessorManager.scheduleProcessors() )
	{
		/* Terminate */
		pScope->stopProcessors();
	}
}
