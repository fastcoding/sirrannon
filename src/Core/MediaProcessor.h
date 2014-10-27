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
#ifndef MEDIA_PROCESSOR_H
#define MEDIA_PROCESSOR_H
#include "sirannon.h"
#include "MediaPacket.h"
#include "ProcessorManager.h"
#include <map>
#include <vector>

/* Threading */
#define SIRANNON_USE_BOOST_THREAD
#include "Boost.h"

class MediaProcessor
{
public:
	/* Typedefs */
	typedef vector<MediaProcessor*> pvector_t;
	typedef pvector_t::iterator pvector_it;
	typedef map<int,pvector_t> routing_t;
	typedef routing_t::iterator routing_it;

public:
	virtual ~MediaProcessor();
	void receivePacket( MediaPacketPtr& pPckt );

	/* String */
    const string& str( void ) const;
	const char* c_str( void ) const;

	/* Activators */
	void schedule( void );
	virtual int flush( void );
	virtual void end();
	virtual void cancelThreads( void );
	void initProcessor( void );

	/* Data getters */
	routing_t getRouting( void ) const; // Copy
	void getDownstream( pvector_t& vResults );

	/* Data setters */
	void setParams( const MediaProcessor* pProc );
	void setString( const string& prop, string value );
	void setInt( const string& prop, int value );
	void setDouble( const string& prop, double value );
	void setBool( const string& prop, bool value );
	void setPrivate( const string& sProp, void* pVal );
	virtual void setRoute( uint32_t xroute, MediaProcessor* proc );
	virtual void clearRoute( MediaProcessor* proc );
	void setRouting( const routing_t& oRouting );

	/* Status */
	bool is_threaded( void ) const;
	bool is_scheduled( void ) const;
	bool is_stopped( void ) const;

protected:
	/* Protected members */
	string sName;
	routing_t vRouting;
    map<string, string>	 mString;
    map<string, int> mInt;
    map<string, double> mDouble;
	map<string, bool> mBool;
	map<string, void*> mPrivate;
	ProcessorManager* const pScope;

	/* Quick parameters */
	bool bDebug;

	/* Abstract class, constructor protected */
	MediaProcessor( const string& sName, ProcessorManager* pScope );

	/* Init */
	virtual void init();
	void createPriviligedThread( void );

	/* Receive & route */
	virtual void receive( MediaPacketPtr& pckt );
	virtual void receive_reset( MediaPacketPtr& pckt );
	virtual void receive_end( MediaPacketPtr& pckt );
	virtual void route( MediaPacketPtr& pckt );

	/* Activators */
	bool bSchedule, bReceive;
	SirannonTime oQuantum;
	virtual void process();

	/* Messages */
	virtual int _debug( const char* args, ... ) const;
	virtual int print( int iLevel, const char* args, ... ) const;

	/* Threading */
	const static SirannonTime oPollQuantum;
	mutable recursive_mutex oFlowMutex;
	thread* createThread( function0<void> oFunction );
	void cancelThread( thread::id iThread );
	void forcePriviligedThread( int64_t iNanoSeconds );
	void mainThread( function0<void> oFunction );
	int getQueueSize( void ) const;

private:
	/* Threading */
	bool bThread, bStop;
	queue_t vPacketQueue;
	mutable mutex oQueueMutex, oThreadMutex;
	map<thread::id, thread*> mThreads;
	void mainThreadPriviliged( void );
	void endThread( thread::id iThread );

	/* Routing efficiency */
	int32_t iCachedRoute;
	vector<MediaProcessor*>* pCachedRoute;

	/* Forbidden to copy */
	MediaProcessor( const MediaProcessor& other );
	MediaProcessor operator=( const MediaProcessor& oOther );

	/* Friends */
	friend class Block;
};

/* Generator for creating a component based on a name */
class MediaProcessorGenerator
{
	public:
	virtual MediaProcessor* create( const char* sName, ProcessorManager* pScope ) = 0;

	protected:
	MediaProcessorGenerator( const char* sName );
};

/* Map containing the names of the components */
typedef bool (*string_ncase_cmp_t) ( const string& s1, const string& s2 );
typedef map<string,MediaProcessorGenerator*,string_ncase_cmp_t> generator_t;
generator_t& getMediaProcessorGenerator( void );


/* Inline functions and other private definitions */
#include "MediaProcessor_priv.h"

#endif
