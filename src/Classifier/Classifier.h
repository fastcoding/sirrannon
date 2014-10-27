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
#ifndef CLASSIFIER_H_
#define CLASSIFIER_H_
#include "sirannon.h"
#include "Utils.h"

class Classifier : public MediaProcessor
{
protected:
	Classifier( const string& sName, ProcessorManager* pScope );

	virtual void init( void );
	virtual void classify( MediaPacketPtr& pckt, bool bClassify, int iRoad=-1 );

	bool bDiscard;
	int iRoute, iID;
	fileLog oSendLog, oRcvLog;
};

class frame_Classifier : public Classifier
{
public:
	frame_Classifier( const string& sName, ProcessorManager* pScope );

protected:
	void receive( MediaPacketPtr& pckt );
};


class avc_Classifier : public Classifier
{
public:
	avc_Classifier( const string& sName, ProcessorManager* pScope );

protected:
	void receive( MediaPacketPtr& pckt );
};


class timestamp_Classifier : public Classifier
{
public:
	timestamp_Classifier( const string& sName, ProcessorManager* pScope );

protected:
	void init( void );
	void receive( MediaPacketPtr& pckt );
	timestamp_t iDTS, iStart, iStop;
};


class time_Classifier : public Classifier
{
public:
	time_Classifier( const string& sName, ProcessorManager* pScope );

protected:
	void init( void );
	void receive( MediaPacketPtr& pckt );
	int32_t iBegin, iStep, iDelta, iStart, iStop;
};

class CountClassifier : public Classifier
{
public:
	CountClassifier( const string& sName, ProcessorManager* pScope );

protected:
	virtual void init( void );
	virtual void receive( MediaPacketPtr& pckt );
	int iCycle, iFrame;
};


class random_Classifier : public Classifier
{
public:
	random_Classifier( const string& sName, ProcessorManager* pScope );

protected:
	void receive( MediaPacketPtr& pckt );
};

class FixedClassifier : public Classifier
{
public:
	FixedClassifier( const string& sName, ProcessorManager* pScope );

protected:
	void init( void );
	void receive( MediaPacketPtr& pPckt );
	queue<int> values;
	int iCount, iLastFrame, iLastSubframe;
	typedef enum { FRAME, SLICE, PACKET } mode_t;
	mode_t iMode;
};

class gilbert_Classifier : public Classifier
{
public:
	gilbert_Classifier( const string& sName, ProcessorManager* pScope );

protected:
	void receive( MediaPacketPtr& pckt );
	void init( void );
	double fAlpha, fBeta, fGamma, fDelta;
	enum state_t { GOOD, BAD };
	state_t oState;
};

class avc_framedrop_Classifier : public Classifier
{
public:
	avc_framedrop_Classifier( const string& sName, ProcessorManager* pScope );
	~avc_framedrop_Classifier();

protected:
	void receive( MediaPacketPtr& pckt );
	void init( void );
	FILE* dropFile;

private:
	int getFramenumber( MediaPacketPtr& pckt);
	bool hasToDrop(int framenumber);

	bool bDecodIdx;
	double dFrameRate;
	double dPtsBase;
	vector<int> vDropFrames;
};

class nalu_drop_Classifier : public Classifier
{
public:
	nalu_drop_Classifier( const string& sName, ProcessorManager* pScope );
	~nalu_drop_Classifier();

protected:
	void receive( MediaPacketPtr& pckt );
	virtual void receive_reset( MediaPacketPtr& pckt );
	void init( void );
	FILE* dropFile;

private:
	void receive_udpmode( MediaPacketPtr& pckt );
	void receive_rtpmode( MediaPacketPtr& pckt );
	bool hasToDrop(int nalu);

	bool udpmode;
	int iCurrNalNumber;
	int iSub, iFrame;
	vector<int> vDropNALUs;
};

#endif /*CLASSIFIER_H_*/
