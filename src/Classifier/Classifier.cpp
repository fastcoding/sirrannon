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
#include "Classifier.h"
#include "SirannonTime.h"
#include "Frame.h"
#include "RandomGenerator.h"
#include "OSSupport.h"

/**
 * @component classifier
 * @properties abstract
 * @type core
 * @param discard, bool, false, if true, delete the packet instead if the condition is met
 * @param sender-trace, string, , if defined, the path where to log information about the packets entering the classifier
 * @param receiver-trace, string, , if defined, the path where to log information about the packets exiting the classifier, implies discard is true
 * @info Classifiers add an offset to the xroute of a packet if it meets a certain
 * condition. See section \ref{sec:diff} for an example. Several classifiers can be
 * chained to obtain a more precision classification.
 **/

Classifier::Classifier( const string& sName, ProcessorManager* pScope )
: MediaProcessor(sName, pScope), bDiscard(false), iID(0)
{
	mInt["xroute"] = 1;
	mBool["discard"] = false;
	mString["sender-trace"] = "";
	mString["receiver-trace"] = "";
}

void Classifier::init( void )
{
	/* Base class */
	MediaProcessor::init();

	/* Open logs */
	oSendLog.open( mString["sender-trace"].c_str(), "w" );
	oRcvLog.open( mString["receiver-trace"].c_str(), "w" );

	/* Quick parameters */
	iRoute = mInt["xroute"];
	bDiscard = mBool["discard"];

	/* Force discard is case of a defined receiver trace */
	if( mString["receiver-trace"].size() )
		bDiscard = true;
}

void Classifier::classify( MediaPacketPtr& pPckt, bool bClassify, int iRoad )
{
	/* Log sender */
	if( oSendLog.active() )
	{
		oSendLog.write( "%lf\tid %d\tudp %d\n",
			SirannonTime::getCurrentTime().convertDouble(), iID, pPckt->size() );
	}

	/* Classify it or not */
	if( bClassify )
	{
		if( bDiscard )
		{
			debug( 1, "discarding %s", pPckt->c_str_short() );
			pPckt.reset();
		}
		else
		{
			if( iRoad < 0 )
				iRoad = iRoute;
			pPckt->xroute += iRoad;
			debug( 2, "classifying %s", pPckt->c_str_short() );
			route( pPckt );
		}
	}
	else
	{
		/* Log receiver */
		if( oRcvLog.active() )
		{
			oRcvLog.write( "%lf\tid %d\tudp %d\n",
			SirannonTime::getCurrentTime().convertDouble(), iID, pPckt->size() );
		}
		/* Leave the packet untouched */
		if( iRoad > 0 )
			pPckt->xroute += iRoad;
		route( pPckt );
	}
	/* Increase iID */
	iID++;
}

/**
 * FRAME CLASSIFIER
 * @component frame-classifier
 * @type classifier
 * @param I, int, 0, the offset if the media-packet is an I slice or frame
 * @param B, int, 0, the offset if the media-packet is an B slice or frame
 * @param P, int, 0, the offset if the media-packet is an P slice or frame
 * @param default, int, 0, the offset if the frame type is none of the above
 * @info Classifies packets based on the type of frame they belong to. The parameter
 * I stands for all sort of I frames (I, IDR, EI, SI...), the same holds for other
 * frame types.
 **/
REGISTER_CLASS( frame_Classifier, "frame-classifier" );

frame_Classifier::frame_Classifier( const string& sName, ProcessorManager* pScope )
: Classifier(sName, pScope)
{
	/* Default values */
	mInt["I"]       = 0;
	mInt["B"]       = 0;
	mInt["P"]       = 0;
	mInt["default"] = 0;
}

void frame_Classifier::receive( MediaPacketPtr& pPckt )
{
	int iRoute = 0;
	switch( pPckt->frame )
	{
	case frame_t::I:
	case frame_t::IDR:
	case frame_t::EI:
	case frame_t::BI:
	case frame_t::CI:
	case frame_t::SI:
		iRoute = mInt["I"];
		break;

	case frame_t::P:
	case frame_t::BP:
	case frame_t::CP:
	case frame_t::EP:
	case frame_t::SP:
		iRoute = mInt["P"];
		break;

	case frame_t::B:
	case frame_t::EB:
	case frame_t::BB:
	case frame_t::CB:
		iRoute = mInt["B"];
		break;

	default:
		iRoute = mInt["default"];
	}
	/* Classify */
	classify( pPckt, true, iRoute );
}

/**
 * TIMESTAMP CLASSIFIER
 * @component timestamp-classifier
 * @type classifier
 * @param start, int, 0, in DTS clock ($90kHz$), the minimum of the interval
 * @param stop, int, -1, in DTS clock ($90kHz$), the maximum of the interval, -1 being unlimited
 * @param xroute, int, 1, offset if the condition is met
 * @info Basic classifier based solely on the decoding time stamp (DTS) interval,
 * relative to the first packet.
 **/
REGISTER_CLASS( timestamp_Classifier, "timestamp-classifier" );

/* Time dropper */
timestamp_Classifier::timestamp_Classifier( const string& sName, ProcessorManager* pScope )
	: Classifier(sName, pScope), iDTS(-1), iStart(0), iStop(-1)
{
	mInt["start"] = 0;
	mInt["stop"] = -1;
}

void timestamp_Classifier::init( void )
{
	/* Base class */
	Classifier::init();

	/* Quick params */
	iStart = mInt["start"];
	iStop = mInt["stop"];
}

void timestamp_Classifier::receive( MediaPacketPtr& pPckt )
{
	/* Offset? */
	if( iDTS < 0 )
		iDTS = pPckt->dts;

	/* Classify the packet if it within the interval */
	timestamp_t iTime = pPckt->dts - iDTS;
	if( iStart >= 0 and iTime >= iStart and ( iStop < 0 or iTime < iStop ) )
		classify( pPckt, true );
	else
		classify( pPckt, false );
}

/**
 * COUNT CLASSIFIER
 * @component count-classifier
 * @type classifier
 * @param cycle, int, 10, do not classify every 'cycle * n'th frame, e.g. if cycle is 10, do not classify frames 0, 10, 20, 30, etc.
 * @info This component classifies or discards every nth frame if 'n % cycle' is non zero. By setting discard to true, this component will essentially
 * discard all but every nth frame.
 **/
REGISTER_CLASS( CountClassifier, "count-classifier" );

/* Time dropper */
CountClassifier::CountClassifier( const string& sName, ProcessorManager* pScope )
	: Classifier(sName, pScope), iCycle(10), iFrame(0)
{
	mInt["cycle"] = 10;
}

void CountClassifier::init( void )
{
	/* Base class */
	Classifier::init();

	/* Quick params */
	iCycle = mInt["cycle"];
}

void CountClassifier::receive( MediaPacketPtr& pPckt )
{
	if( pPckt->framestart )
		iFrame++;

	if( (iFrame-1) % iCycle )
		classify( pPckt, true );
	else
		classify( pPckt, false );
}

/**
 * TIME CLASSIFIER
 * @component time-classifier
 * @type classifier
 * @param start, int, 0, in ms, the minimum of the range
 * @param stop, int, -1, in ms, the maximum of the range, -1 being unlimited
 * @param step, int, -1, in ms, the time between two steps, -1 disabling the step effect
 * @param delta, int, -1, in ms, the duration of one step, -1 being unlimited
 * @param xroute, int, 1, offset if the condition is met
 * @info Basic classifier based solely on the real time interval, relative to the
 * first packet. The four time parameters combine to create an interval of the form:
 * $t\in[start,stop[$, when step is defined, it must also hold that:
 * $$t\in\bigcup_{n=1}^{+\infty}[n\cdot step,n\cdot step+delta[$$.
 **/
REGISTER_CLASS( time_Classifier, "time-classifier" );

/* Time dropper */
time_Classifier::time_Classifier( const string& sName, ProcessorManager* pScope )
	: Classifier(sName, pScope), iBegin(-1), iStep(-1), iDelta(0), iStart(0), iStop(-1)
{
	mInt["start"] = 0;
	mInt["stop"] = -1;
	mInt["step"] = -1;
	mInt["delta"] = -1;
}

void time_Classifier::init( void )
{
	/* Base class */
	Classifier::init();

	/* Quick params */
	iStart = mInt["start"];
	iStop = mInt["stop"];
	iStep = mInt["step"];
	iDelta = MAX( 0, mInt["delta"] );
}

void time_Classifier::receive( MediaPacketPtr& pPckt )
{
	/* Current time */
	int iCurrent = SirannonTime::getCurrentTime().convertMsecs();
	if( iBegin < 0 )
		iBegin = iCurrent;
	iCurrent -= iBegin;

	/* Mark all packets inside [begin,end[ */
	if( iCurrent >= iStart )
	{
		if( iStop < 0 or iCurrent < iStop )
		{
			if( iStep <= 0 )
			{
				return classify( pPckt, true );
			}
			else
			{
				int iMod = ( iCurrent - iStart ) % iStep;
				if( iDelta < 0 or iMod < iDelta )
					return classify( pPckt, true );
			}
		}
	}
	/* Default */
	return classify( pPckt, false );
}

/**
 * RANDOM CLASSIFIER
 * @component random-classifier
 * @type classifier
 * @param P(loss), double, 0.01, $\in[0,1]$, probability to classify the packet
 * @param xroute, int, 0, offset if the condition is met
 * @info This component has a random chance of classifying a packet using a uniform
 * distribution.
 **/
REGISTER_CLASS( random_Classifier, "random-classifier" );

random_Classifier::random_Classifier( const string& sName, ProcessorManager* pScope )
	: Classifier(sName, pScope)
{
	mDouble["P(loss)"] = 0.0;
}

void random_Classifier::receive( MediaPacketPtr& pPckt )
{
	/* Classify the packet */
	double f = frand();
	if( f < mDouble["P(loss)"] )
		classify( pPckt, true );
	else
		classify( pPckt, false );
}

/**
 * GILBERT CLASSIFIER
 * @component gilbert-classifier
 * @type classifier
 * @param alpha, double, 0.01, $\in[0,1]$, probability to transit from the GOOD to the BAD state
 * @param beta, double, 0.1, $\in[0,1]$, probability to transit from the BAD to the GOOD state
 * @param gamma, double, 0.75, $\in[0,1]$, probability to classify a packet in the BAD state
 * @param delta, double, 0.01, $\in[0,1]$, probability to classify a packet in the GOOD state
 * @param xroute, int, 1, offset if the condition is met
 * @info This component has a random chance of classifying a packet using the Gilbert method.
 **/
REGISTER_CLASSES( gilbert_Classifier, "qsac-classifier", 1 );
REGISTER_CLASSES( gilbert_Classifier, "gilbert-classifier", 2 );

gilbert_Classifier::gilbert_Classifier( const string& sName, ProcessorManager* pScope )
	: Classifier(sName, pScope), fAlpha(0.9), fBeta(0.1), fGamma(0.0), fDelta(1.0),
	oState(GOOD)
{
	mDouble["alpha"] = 0.01;
	mDouble["beta"] = 0.10;
	mDouble["gamma"] = 0.75;
	mDouble["delta"] = 0.01;
}

void gilbert_Classifier::init( void )
{
	/* Base class */
	Classifier::init();

	/* Quick params */
	fAlpha = mDouble["alpha"];
	fBeta = mDouble["beta"];
	fGamma = mDouble["gamma"];
	fDelta = mDouble["delta"];
}

void gilbert_Classifier::receive( MediaPacketPtr& pPckt )
{
	/* Process */
	if( oState == BAD )
	{
		/* Transition chance */
		if( oRandom.frand() < fBeta )
			oState = GOOD;
		else
			oState = BAD;

		/* Loss chance */
		if( oRandom.frand() < fGamma )
			return classify( pPckt, true );
	}
	else if( oState == GOOD )
	{
		/* Transition chance */
		if( oRandom.frand() < fAlpha )
			oState = BAD;
		else
			oState = GOOD;

		/* Loss chance */
		if( oRandom.frand() < fDelta )
			return classify( pPckt, true );
	}
	/* Normal */
	return classify( pPckt, false );
}

/**
 * AVC CLASSIFIER
 * @component avc-classifier
 * @type classifier
 * @param I, int, 0, the offset if the media-packet is an I slice
 * @param SI, int, 0, the offset if the media-packet is an SI slice
 * @param EI, int, 0, the offset if the media-packet is an EI slice
 * @param I(B), int, 0, the offset if the media-packet is an I (data partition B) slice
 * @param I(C), int, 0, the offset if the media-packet is an I (data partition C) slice
 * @param P, int, 0, the offset if the media-packet is a P slice
 * @param SP, int, 0, the offset if the media-packet is an SP slice
 * @param EP, int, 0, the offset if the media-packet is an EP slice
 * @param P(B), int, 0, the offset if the media-packet is a P (data partition B) slice
 * @param P(C), int, 0, the offset if the media-packet is a P (data partition C) slice
 * @param B, int, 0, the offset if the media-packet is a B slice
 * @param EB, int, 0, the offset if the media-packet is an EB slice
 * @param B(B), int, 0, the offset if the media-packet is a B (data partition B) slice
 * @param B(C), int, 0, the offset if the media-packet is a B (data partition C) slice
 * @param E, int, 0, the offset if the media-packet is a prefix NAL
 * @param PPS, int, 0, the offset if the media-packet is a PPS unit
 * @param SPS, int, 0, the offset if the media-packet is an SPS unit
 * @param ESPS, int, 0, the offset if the media-packet is an extended SPS unit
 * @param SEI, int, 0, the offset if the media-packet is a SEI unit
 * @param default, int, 0, the offset if the packet does not belong to any of the above
 * @info This components classifies the many different types of NAL units present in
 * H264, far the beyond the common I, B and P frames.
 **/
REGISTER_CLASS( avc_Classifier, "avc-classifier" );

avc_Classifier::avc_Classifier( const string& sName, ProcessorManager* pScope )
	: Classifier(sName, pScope)
{
	/* Default values */
	mInt["E"]     = 0;
	mInt["I"]     = 0;
	mInt["EI"]    = 0;
	mInt["SI"]    = 0;
	mInt["I(B)"]  = 0;
	mInt["I(C)"]  = 0;
	mInt["P"]     = 0;
	mInt["EP"]    = 0;
	mInt["SP"]    = 0;
	mInt["P(B)"]  = 0;
	mInt["P(C)"]  = 0;
	mInt["B"]     = 0;
	mInt["EB"]    = 0;
	mInt["B(B)"]  = 0;
	mInt["B(C)"]  = 0;
	mInt["PPS"]   = 0;
	mInt["SPS"]   = 0;
	mInt["ESPS"]  = 0;
	mInt["SEI"]   = 0;
	mInt["default"] = 0;
}

void avc_Classifier::receive( MediaPacketPtr& pPckt )
{
	int iRoute = 0;
	switch( pPckt->frame )
	{
	case frame_t::E:
		iRoute = mInt["E"];
		break;
	case frame_t::I:
	case frame_t::IDR:
		iRoute = mInt["I"];
		break;
	case frame_t::EI:
		iRoute = mInt["EI"];
		break;
	case frame_t::BI:
		iRoute = mInt["I(B)"];
		break;
	case frame_t::CI:
		iRoute = mInt["I(C)"];
		break;
	case frame_t::SI:
		iRoute = mInt["SI"];
		break;
	case frame_t::P:
		iRoute = mInt["P"];
		break;
	case frame_t::BP:
		iRoute = mInt["P(B)"];
		break;
	case frame_t::CP:
		iRoute = mInt["P(C)"];
		break;
	case frame_t::EP:
		iRoute = mInt["EP"];
		break;
	case frame_t::SP:
		iRoute = mInt["SP"];
		break;
	case frame_t::B:
		iRoute = mInt["B"];
		break;
	case frame_t::EB:
		iRoute = mInt["EB"];
		break;
	case frame_t::BB:
		iRoute = mInt["B(B)"];
		break;
	case frame_t::CB:
		iRoute = mInt["B(C)"];
		break;
	case frame_t::SEI:
		iRoute = mInt["SEI"];
		break;
	case frame_t::PPS:
		iRoute = mInt["PPS"];
		break;
	case frame_t::SPS:
		iRoute = mInt["SPS"];
		break;
	case frame_t::ESPS:
		iRoute = mInt["ESPS"];
		break;
	default:
		iRoute = mInt["default"];
	}
	/* Classify */
	classify( pPckt, true, iRoute );
}

/* MediaProcessorFactory */
REGISTER_CLASS( avc_framedrop_Classifier, "avc-framedrop-classifier" );

avc_framedrop_Classifier::avc_framedrop_Classifier( const string& sName, ProcessorManager* pScope )
	: Classifier(sName, pScope), bDecodIdx(false), dFrameRate(25.0), dPtsBase(1.0)
{
	dropFile = NULL;
	mString["drop-trace"] = "";
	mBool["decode-idx"] = false;
	mDouble["fps"] = 25.0;
}

void avc_framedrop_Classifier::init( void )
{
	/* Base class */
	Classifier::init();

	bDecodIdx = mBool["decode-idx"];
	dFrameRate = mDouble["fps"];

	/* Open file with drop indices */
	if( dropFile == NULL and mString["drop-trace"].c_str() != NULL )
	{
		debug( 1, "Trying to open file '%s'", mString["drop-trace"].c_str() );
		/* Try to open the file */
		dropFile = fopen(mString["drop-trace"].c_str(), "r");
		if(dropFile == NULL)
			IOError( this, "Unable to open drop_trace file(%s)", mString["drop-trace"].c_str() );	/* File could not be opened */

		/* Read the contents of the file and add all frames indices to the vector */
		int iDropidx;
		while(!feof(dropFile) && dropFile != NULL){
			if (fscanf(dropFile, "%d", &iDropidx) > 0)
			{
				debug( 1, "Drop frame '%d'", iDropidx);
				vDropFrames.push_back(iDropidx);
			}
		}

		if( dropFile )
			fclose( dropFile );
	}
}

void avc_framedrop_Classifier::receive( MediaPacketPtr& pPckt )
{
	/* Check if the packet must be dropped or not, depending on the frame number and frame type */
	int idx_to_drop = getFramenumber(pPckt);
	if(FrameToChar(pPckt->frame) != 'H' && hasToDrop(idx_to_drop))
	{
		return classify( pPckt, true );
	}

	/* Normal */
	return classify( pPckt, false );
}

int avc_framedrop_Classifier::getFramenumber( MediaPacketPtr& pPckt)
{
	/* If the decoding order must be used as indices for dropping the frames */
	if(bDecodIdx) return pPckt->framenumber;

	/* Calculate the frame number based on the PTS */
	if(pPckt->framenumber == 0)
	{
		//Determine the base pts
    dPtsBase = pPckt->pts * 1. / 90000;

		return 0;
	}

	/* Return the framenumber based on the PTS */
	return (int) ((((pPckt->pts * 1. / 90000) - dPtsBase) * dFrameRate) + 0.5);
}

bool avc_framedrop_Classifier::hasToDrop(int framenumber)
{
	if(vDropFrames.size() <= 0) return false;

	for( uint32_t i = 0; i < vDropFrames.size(); i++)
	{
		if(vDropFrames[i] == framenumber) return true;
	}

	return false;
}

avc_framedrop_Classifier::~avc_framedrop_Classifier( )
{
	vDropFrames.clear();
}

/**
 * NALU DROP CLASSIFIER
 * @component nalu-drop-classifier
 * @type classifier
 * @param drop-trace, string, , path to file which contains information on which NAL units to drop. A sample drop-trace file is located in the src/Misc folder.
 * @param udpmode, bool, false, set to true in case a UDP receiver is used instead of an RTP receiver. Sequence numbers are retained in UDP mode whereas in case of an RTP receiver and transmitter new sequence numbers are generated.
 * @info Drops specific NAL units from an H.264/AVC encoded video stream. The indices of the NAL units to drop are provided in a tracefile.
 * The first NAL unit of the stream has index 1.
 **/

/* MediaProcessorFactory */
REGISTER_CLASS( nalu_drop_Classifier, "nalu-drop-classifier" );

nalu_drop_Classifier::nalu_drop_Classifier( const string& sName, ProcessorManager* pScope )
	: Classifier(sName, pScope), udpmode(false),  iCurrNalNumber(0), iSub(-1), iFrame(-1)
{
	dropFile = NULL;
	mString["drop-trace"] = "";
	mBool["udpmode"] = false;
}

void nalu_drop_Classifier::init( void )
{
	/* Base class */
	Classifier::init();

	udpmode = mBool["udpmode"];

	/* Open file with drop indices */
	if( dropFile == NULL and mString["drop-trace"].c_str() != NULL )
	{
		debug( 1, "Trying to open file '%s'", mString["drop-trace"].c_str() );
		/* Try to open the file */
		dropFile = fopen(mString["drop-trace"].c_str(), "rt");
		if(dropFile == NULL)
			IOError( "Unable to open drop_trace file(%s)", mString["drop-trace"].c_str() );	/* File could not be opened */

		/* Read the contents of the file and add all frames indices to the vector */
		int iDropIdx;
		char sStatus[256];
		char buffer[256];

		while(!feof(dropFile) && dropFile != NULL){
			if(fgets(buffer, 256, dropFile) != NULL)	//Read every line
			{
				if (sscanf(buffer, "%d\t%*d\t%*c\t%s\t%*d", &iDropIdx, sStatus)> 0)	//Process the line
				{
					//Check the NALU must be dropped
					if(0 == strcmp(sStatus, "Lost") || 0 == strcmp(sStatus, "lost"))
					{
						debug( 1, "avc_framedrop_Classifier: Drop NALU '%d'", iDropIdx);
						vDropNALUs.push_back(iDropIdx);
					}
				}
			}
		}

		debug(1, "avc_framedrop_Classifier: %d NALUs to drop", (int) vDropNALUs.size());
		debug(1, "avc_framedrop_Classifier: Total NALU count: %d", iDropIdx);

		if( dropFile )
			fclose( dropFile );
	}
}

void nalu_drop_Classifier::receive( MediaPacketPtr& pPckt )
{
	if(udpmode)
		receive_udpmode(pPckt);
	else
		receive_rtpmode(pPckt);
}

void nalu_drop_Classifier::receive_udpmode( MediaPacketPtr& pPckt )
{
	IBits oHeader( pPckt->data(), pPckt->size() );
	uint8_t* data = pPckt->data();

	oHeader.read( 3 );	//Ver and P
	bool bExtension = oHeader.read( 1 );	//X
	int iHeader = 12 + oHeader.read( 4 ) * 4;	//12 byte fixed header + CSRC
	oHeader.read(8);	//M and PT
	int iSeqNumber = oHeader.read(16);
	oHeader.seek( iHeader - 4 );	//-4 because already 4 bytes read
	if( bExtension )
	{
		oHeader.seek( 2 );	//skip 'defined by profile'
		int iExtension = oHeader.read( 16 ) * 4;	//length
		iHeader += iExtension;
		oHeader.seek(iExtension);
		iHeader += 4; //'defined by profile' and 'RTP header extension length'
	}

	//Check if the packet is not the 'dummy' packet for signaling the end
	if(iHeader == pPckt->size())
	{
	    pPckt->type = packet_t::end;
        return classify( pPckt, false );
	}

	//Check the FU Type
	int fu_indicator = oHeader.read(8) & 0x1f;
	debug(1, "RTP seq_nb(%d), header_size(%d), packet_size(%d), FU_indicator(%d)", iSeqNumber, iHeader, pPckt->size(), fu_indicator);
	if(fu_indicator >= 1 && fu_indicator <= 23)
	{
		//Single NAL Unit packet
		if(fu_indicator >= 1 && fu_indicator <= 5)
		{
			iCurrNalNumber++;
			debug(1, "iCurrNalNumber: %d", iCurrNalNumber);
		}

		//Check if the NALU must be dropped
		if(hasToDrop(iCurrNalNumber))
			return classify( pPckt, true );
	}
	else
	{
		if(fu_indicator == 28)
		{
			//FU-A Fragmentation unit
			int fu_header = oHeader.read(8);
			int nalu_type = fu_header & 0x1f;
			debug(1, "NALU_type: %d", nalu_type);

			if(nalu_type >= 1 && nalu_type <= 5 && ((fu_header & 0x80) == 0x80))
			{
				iCurrNalNumber++;
				debug(1, "iCurrNalNumber: %d", iCurrNalNumber);
			}

			//Check if the NALU must be dropped
			if(hasToDrop(iCurrNalNumber))
				return classify( pPckt, true );
		}
		else
		{
			//Currently unsupported
			SirannonWarning( this, "RTP seq_nb(%d), header_size(%d), packet_size(%d), FU_indicator(%d)", iSeqNumber, iHeader, pPckt->size(), fu_indicator);
			RuntimeError( this, "Unsupported NAL unit type (see Table 1 in RFC3984)");
		}
	}

	/* Normal */
	return classify( pPckt, false );
}

void nalu_drop_Classifier::receive_rtpmode( MediaPacketPtr& pPckt )
{
	/* Check if the packet must be dropped or not, depending on the frame number and frame type */
	if(FrameToChar(pPckt->frame) != 'H')
	{
		//Increment NALU counter if needed
		if(iSub != pPckt->subframenumber or iFrame != pPckt->framenumber)
		{
			iCurrNalNumber ++;
			iSub = pPckt->subframenumber;
			iFrame = pPckt->framenumber;
		}
		//Check if the NALU must be dropped
		if(hasToDrop(iCurrNalNumber))
			return classify( pPckt, true );
	}

	/* Normal */
	return classify( pPckt, false );
}

void nalu_drop_Classifier::receive_reset( MediaPacketPtr& pPckt )
{
	/* Have to route the pPckt at least */
	route( pPckt );
}

bool nalu_drop_Classifier::hasToDrop(int nalu)
{
	if(vDropNALUs.size() <= 0) return false;

	for( uint32_t i = 0; i < vDropNALUs.size(); i++)
	{
		if(vDropNALUs[i] == nalu)
		{
			debug(1, "Dropping NALU %d", nalu);
			return true;
		}
	}

	return false;
}

nalu_drop_Classifier::~nalu_drop_Classifier( )
{
	vDropNALUs.clear();
}

/**
 * FIXED CLASSIFIER
 * @component fixed-classifier
 * @type classifier
 * @param mode, string, frame, whether the indices inputted in values or values-file specify packets, slices or frames, accepted values: frame, slice, packet
 * @param values, string, , if defined, a comma seperated string of indices, indicating which packet, slice or frame to classify counted from 0
 * @param values-file, string, , if defined, a file containing a new line seperated string of indices, indicating which packet, slice or frame to classify counted from 0
 * @param xroute, int, 1, offset added to the xroute of the packet if the packet is classified
 * @info Classifies the packets, slice or frames based on fixed indices provided by the user.
 * CAVEAT: Indices must rise strict monotonously; Non slice NAL units such as PPS, SPS, SEI, E, are not
 * counted as slices while different SVC layers are also counted as slices. For example an
 * MVC stream with 4 slices and with 2 layers per frame will be counted as 8 slices.
 **/

REGISTER_CLASS( FixedClassifier, "fixed-classifier" );

FixedClassifier::FixedClassifier( const string& sName, ProcessorManager* pScope )
	: Classifier(sName, pScope), iMode(FRAME), iCount(-1), iLastFrame(-1), iLastSubframe(-1)
{
	mBool["mode"] = "frame";
	mString["values"] = "";
	mString["values-file"] = "";
}

void FixedClassifier::init( void )
{
	Classifier::init();

	if( mString["values"].length() )
	{
		vector<string> vSplit;
		split( mString["values"], vSplit, "," );

		for( int i = 0; i < vSplit.size(); ++i )
			values.push( atoi( vSplit[i].c_str() ) );
	}
	else if( mString["values-file"].length() )
	{
		FILE* pFile = fopen( mString["values-file"].c_str(), "r" );
		if( not pFile )
			IOError( "Could not open (%s): %s", mString["values-file"].c_str(), strError() );

		int iFixed;
		while( fscanf( pFile, "%d", &iFixed ) == 1 )
			values.push( iFixed );
	}
	/* Set modus */
	if( mString["mode"] == "packet" )
		iMode = PACKET;
	else if( mString["mode"] == "slice" )
		iMode = SLICE;
}

void FixedClassifier::receive( MediaPacketPtr& pPckt )
{
	/* When we completed the list, it is easy */
	if( not values.size() )
		return classify( pPckt, false );

	/* Count */
	if( iMode == PACKET )
	{
		iCount++;
	}
	else if( iMode == FRAME )
	{
		if( pPckt->framenumber != iLastFrame )
			iCount++;
		iLastFrame = pPckt->framenumber;
	}
	else if( iMode == SLICE )
	{
		if( pPckt->framenumber != iLastFrame or pPckt->subframenumber != iLastSubframe )
		{
			if( !(pPckt->codec & codec_t::H264) or IsSlice( pPckt.get() ) )
				iCount++;
		}
		iLastSubframe = pPckt->subframenumber;
		iLastFrame = pPckt->framenumber;
	}
	else
		RuntimeError( "Condition exception" );

	/* Count at value? */
	while( values.size() and values.front() < iCount )
		values.pop();

	if( values.size() and values.front() == iCount )
		classify( pPckt, true );
	else
		classify( pPckt, false );
}
