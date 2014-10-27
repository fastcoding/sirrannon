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
#ifndef SVC_CLASSIFIER_H_
#define SVC_CLASSIFIER_H_
#include "Classifier.h"

class triplet
{
public:
	int T, D, Q, t;
	triplet( int T=0, int D=0, int Q=0, int t=0) : T(T), Q(Q), D(D), t(t) { }	
	bool operator>=( const triplet& other ) const;
	bool operator< ( const triplet& other ) const;
	bool operator==( const triplet& other ) const;
	triplet& operator=( const triplet& other );
	static bool compareT( const triplet& self, const triplet& other );
};

class SVCClassifier : public Classifier
{
public:
	SVCClassifier( const string& sName, ProcessorManager* pScope );
	void init( void );	
	
protected:
	void receive( MediaPacketPtr& pckt );	
	
	/* Attrs */
	vector<triplet> triplets;
	vector<int> layers;
	int layer;
};

class svc_analyser : public MediaProcessor
{
public:
	svc_analyser( const string& sName, ProcessorManager* pScope );
	void init( void );
	
protected:
	void receive( MediaPacketPtr& pckt );
	void receive_reset( MediaPacketPtr& pckt ) { maple(); route(pckt); }
	void receive_end( MediaPacketPtr& pckt )   { maple(); route(pckt); }
	void maple( void );
	void chatter( void );
	void newGOP( void );
	void analyze( void );
	string cube( triplet& key, float fRatio, float fMaxRatio );
	void replace( string& s, char key, float fRatio );
	
	/* Attrs */
	typedef pair<timestamp_t,int> val_t;
	typedef triplet key_t;
	typedef map<key_t,vector<val_t> > map_t;
	typedef map_t::iterator map_it;
	map_t layers;
	key_t lastKey;
	map<key_t,float> bitrates;
	float fTotal, fMaxRatio;
	bool bSingleton;
};

#endif /*SVC_CLASSIFIER_H_*/
