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
#include "MediaProcessor.h"
#include "SirannonTime.h"
#include "signal.h"
#include "SirannonException.h"
#include "OSSupport.h"
#include "SirannonPrint.h"
#include "Scheduler/Scheduler.h"
#include "Utils.h"

/** CORE
 * @component core
 * @properties abstract
 * @param debug, bool, false, if true, print debug info for this component
 * @param thread, bool, false, if true, run the component in a seperate thread
 */


const SirannonTime MediaProcessor::oPollQuantum( 40 );

/* Here is the mapping defined */
inline bool string_ncase_cmp( const string& s1, const string& s2 )
{
	return strcasecmp( s1.c_str(), s2.c_str() ) < 0;
}

generator_t& getMediaProcessorGenerator( void )
{
	static  generator_t oMediaProcessorGenerators( &string_ncase_cmp );
	return oMediaProcessorGenerators;
}

MediaProcessorGenerator::MediaProcessorGenerator( const char* sName )
{
	getMediaProcessorGenerator()[sName] = this;
}

/* Base class of all components */
MediaProcessor::MediaProcessor( const string& sName, ProcessorManager* pScope )
	: sName(sName), pScope(pScope), bDebug(false),
	  bStop(false), iCachedRoute(-2), pCachedRoute(NULL),
	  bSchedule(false), bReceive(false), bThread(false)
{
	mBool["debug"] = false;
	mBool["thread"] = false;
	mInt["quantum"] = -1;
}

MediaProcessor::~MediaProcessor()
{
	if( mThreads.size() )
		RuntimeError( this, "Destructor called while threads still active" );
	while( not vPacketQueue.empty() )
	{
		delete vPacketQueue.front();
		vPacketQueue.pop();
	}
	print( 1, "destructor" );
}

void MediaProcessor::initProcessor( void )
{
	init();
	createPriviligedThread();
	bReceive = not bReceive; // Kludge to fix TCP-transmitter
}

void MediaProcessor::init( void )
{
	bDebug = mBool["debug"];
	bThread = mBool["thread"];

	/* Timing behaviour for threads */
	int iQuantum = mInt["quantum"];
	if( iQuantum >= 0 )
	{
		//iQuantum = max( (uint32_t)iQuantum, SIRANNON_MIN_QUANTUM );
		oQuantum = SirannonTime( 0, mInt["quantum"] );
	}
	else
		oQuantum = pScope->getQuantum();
}

void MediaProcessor::end( void )
{
	bStop = true;
}

int MediaProcessor::flush( void )
{
	if( bThread )
	{
		FlowLock_t oLock( oFlowMutex );
		while( not vPacketQueue.empty() )
		{
			delete vPacketQueue.front();
			vPacketQueue.pop();
		}
	}
	return 0;
}

thread* MediaProcessor::createThread( function0<void> oFunction )
{
	/* Do not block when the lock fails for the threadmutex. Blocking could lead to a deadlock! */
	LockUnique_t oLock( oThreadMutex, try_to_lock_t() );
	while( not oLock.owns_lock() )
	{
		oPollQuantum.sleep();
		oLock.try_lock();
	}
	/* Create a thread */
	thread* pThread = new thread( bind( &MediaProcessor::mainThread, this, oFunction ) );
	mThreads[pThread->get_id()] = pThread;
	//print( 1, "new thread(%s)", thread_get_string( pThread->get_id() ).c_str() );
	return pThread;
}

void MediaProcessor::endThread( thread::id iThread )
{
	/* Do not block when the lock fails for the threadmutex. Blocking could lead to a deadlock! */
	LockUnique_t oLock( oThreadMutex, try_to_lock_t() );
	while( not oLock.owns_lock() )
	{
		oPollQuantum.sleep();
		oLock.try_lock();
	}
	/* End the thread */
	if( mThreads.find( iThread ) != mThreads.end() )
	{
		delete mThreads[iThread];
		mThreads.erase( iThread );
	}
}

void MediaProcessor::cancelThread( thread::id iThread )
{
	/* Do not block when the lock fails for the threadmutex. Blocking could lead to a deadlock! */
	LockUnique_t oLock( oThreadMutex, try_to_lock_t() );
	while( not oLock.owns_lock() )
	{
		oPollQuantum.sleep();
		oLock.try_lock();
	}
	/* Cancel this one thread */
	if( mThreads.find( iThread ) != mThreads.end() )
	{
		thread* pThread = mThreads[iThread];
		pThread->interrupt();
		pThread->join();
		delete pThread;
		mThreads.erase( iThread );
	}
	//print( 1, "cancelling thread done, active(%d)", mThreads.size() );
}

void MediaProcessor::cancelThreads( void )
{
	/* Since the thread of the scope calls this, not try locking is required */
	Lock_t oLock( oThreadMutex );
	int iThreads = mThreads.size();
	if( not iThreads )
		return;
	//print( 1, "cancelling %d thread(s)", iThreads );

	/* Cancel this one thread */
	for( 	map<thread::id, thread*>::iterator ppThread = mThreads.begin();
			ppThread != mThreads.end();
			++ppThread )
	{
		thread* pThread = ppThread->second;
		pThread->interrupt();
		pThread->join();
		delete pThread;
	}

	//print( 1, "cancelling thread(s) done" );
	mThreads.clear();
}

void MediaProcessor::forcePriviligedThread( int64_t iNano )
{
	bThread = true;
	bSchedule = true;
	if( iNano >= 0 )
		oQuantum = SirannonTime::fromNSecs( iNano );
}

void MediaProcessor::createPriviligedThread( void )
{
	if( bThread )
		createThread( bind( &MediaProcessor::mainThreadPriviliged, this ) );
}

void MediaProcessor::mainThread( function0<void> oFunction )
{
	try
	{
		oFunction();
		//print( 1, "thread finished(%s)", thread_get_string().c_str() );
		endThread( this_thread::get_id() );
	}
	catch( thread_interrupted e )
	{
		//print( 1, "thread cancelled(%s)", thread_get_string().c_str() );
	}
	catch( SirannonException& pException )
	{
		pScope->handleError( &pException, pScope, this );
		//print( 1, "thread exception(%s)", thread_get_string().c_str() );
	}
}

void MediaProcessor::mainThreadPriviliged( void )
{
	/* Timing */
	SirannonTime oRef = SirannonTime::getCurrentTime();

	while( not bStop )
	{
		/* Read all packets from the queue */
		//fprintf( stderr, "PACKETS %s %lld\n", sName.c_str(), SirannonTime::getUpTime().convertMsecs() );
		while( bReceive and not bStop )
		{
			/* Obtain a packet from the queue */
			MediaPacketPtr pPckt;
			{
				Lock_t oLock( oQueueMutex );
				if( not vPacketQueue.empty() )
				{
					pPckt = MediaPacketPtr( vPacketQueue.front() );
					vPacketQueue.pop();
				}
			}
			/* Stop when depleted */
			if( not pPckt.get() )
				break;

			/* Receive the packet in a normal way now */
			{
				FlowLock_t oLock( oFlowMutex );
				switch( pPckt->type )
				{
					case packet_t::media:
						receive( pPckt );
						break;

					case packet_t::reset:
						receive_reset( pPckt );
						break;

					case packet_t::end:
						receive_end( pPckt );
						break;

					default:
						RuntimeError( this, "unknown packet type: %s", pPckt->c_str_full( 100 ) );
				}
			}
			this_thread::interruption_point();
		}
		/* Schedule and rest */
		if( not bStop )
		{
			/* Do a tick */
			//fprintf( stderr, "SCHEDULE %s %lld\n", sName.c_str(), SirannonTime::getUpTime().convertMsecs() );
			if( bSchedule  )
			{
				FlowLock_t oLock( oFlowMutex );
				process();
			}
			/* Synchronize in steps */
			//fprintf( stderr, "SLEEP %s %lld\n", sName.c_str(), SirannonTime::getUpTime().convertMsecs() );
			oRef.synchronizeInterval( oQuantum );
			//fprintf( stderr, "WAKE %s %lld\n", sName.c_str(), SirannonTime::getUpTime().convertMsecs() );
		}
		/* Resync */
		this_thread::interruption_point();
	}
}

void MediaProcessor::receive( MediaPacketPtr& pckt )
{
	route( pckt );
}

void MediaProcessor::receive_reset( MediaPacketPtr& pckt )
{
	route( pckt );
}

void MediaProcessor::receive_end( MediaPacketPtr& pckt )
{
	route( pckt );
}

void MediaProcessor::process( void )
{ }

/* Route receive packets according to the xroutes */
void MediaProcessor::route( MediaPacketPtr& pPckt ) synchronized
{
	/* Find the related route */
	vector<MediaProcessor*>* pRoute = NULL;

	/* Cached route? */
	if( pPckt->xroute == iCachedRoute )
	{
		/* Use the cached value */
		pRoute = pCachedRoute;
	}
	else
	{
		/* Only try looking if there are any routes at all */
		if( not vRouting.empty() )
		{
			/* Look it up */
			pRoute = &vRouting[pPckt->xroute];

			/* Fallback, Copy results from route 0 */
			if( pRoute->empty() )
			{
				/* Copy from route 0 and cache */
				vRouting[pPckt->xroute] = vRouting[0];
				pRoute = &vRouting[pPckt->xroute];

				/* Route 0 empty too */
				if( pRoute->empty() )
					ValueError( this, "No route(%s)", pPckt->c_str() );
			}
		}
		/* Cache value */
		iCachedRoute = pPckt->xroute;
		pCachedRoute = pRoute;
	}
	/* Found any route? */
	if( not pRoute or pRoute->empty() )
		return;

	/* Send the packet */
	const uint32_t iRoutes = pRoute->size();
	for( uint32_t k = 0; k < iRoutes; k++ )
	{
		/* Copy the packt if needed */
		MediaPacketPtr pNewPckt;
		if( k + 1 < iRoutes )
		{
			/* Make a deep copy of the packet */
			pNewPckt = MediaPacketPtr( new MediaPacket( *pPckt ) );
		}
		/* Route different kind of packets to different parts */
		MediaProcessor* pProcessor = (*pRoute)[k];

		/* Lock the component's mutex and let it receive the packet */
		pProcessor->receivePacket( pPckt );
		pPckt = pNewPckt;
	}
} end_synchronized

int MediaProcessor::getQueueSize( void ) const
{
	if( bThread )
	{
		Lock_t oLock( oQueueMutex );
		if( vPacketQueue.empty() )
			return 0;
		else
			return MAX( 0, vPacketQueue.back()->dts - vPacketQueue.front()->dts );
	}
	else
		return -1;
}

void MediaProcessor::setRoute( uint32_t xroute, MediaProcessor* proc ) synchronized
{
	vRouting[xroute].push_back( proc );

	/* Reset cache values */
	iCachedRoute = -2;
	pCachedRoute = NULL;
} end_synchronized

void MediaProcessor::clearRoute( MediaProcessor* pProc ) synchronized
{
	/* Reset cache values */
	iCachedRoute = -2;
	pCachedRoute = NULL;

	/* Ugly delete function, really who ever invented this template crap */
	for( routing_t::iterator i = vRouting.begin(); i != vRouting.end(); )
	{
		/* Remove the processor from the vector */
		pvector_t& oVector = i->second;
		oVector.erase( remove_if( oVector.begin(), oVector.end(), bind( equal_to<MediaProcessor*>(), pProc, _1) ), oVector.end() );

		/* Remove the entire map entry if the vector is empty */
		if( oVector.empty() )
			vRouting.erase( i++ );
		else
			++i;
	}
	debug( 1, "cleared route from(%s) to(%s) rem(%d)", this->c_str(), pProc->c_str(), vRouting.size() );
} end_synchronized

void MediaProcessor::setParams( const MediaProcessor* pProc )
{
	for( map<string,int>::const_iterator i = pProc->mInt.begin(); i != pProc->mInt.end(); ++i )
		mInt[i->first] = i->second;
	for( map<string,string>::const_iterator i = pProc->mString.begin(); i != pProc->mString.end(); ++i )
		mString[i->first] = i->second;
	for( map<string,bool>::const_iterator i = pProc->mBool.begin(); i != pProc->mBool.end(); ++i )
		mBool[i->first] = i->second;
	for( map<string,double>::const_iterator i = pProc->mDouble.begin(); i != pProc->mDouble.end(); ++i )
		mDouble[i->first] = i->second;
}

void MediaProcessor::getDownstream( vector<MediaProcessor*>& vResults ) synchronized
{
	/* Follow the vRouting table */
	for( routing_it k = vRouting.begin(); k != vRouting.end(); k++ )
	{
		for( pvector_it l = k->second.begin(); l != k->second.end(); l++ )
		{
			/* Ensure no double adds */
			if( find( vResults.begin(), vResults.end(), *l ) == vResults.end() )
			{
				/* Add to the vector */
				RateController* pScheduler = dynamic_cast<RateController*>(*l);
				if( pScheduler )
					vResults.push_back( (*l) );
				(*l)->getDownstream( vResults );
			}
		}
	}
} end_synchronized

int MediaProcessor::_debug( const char* fmt, ... ) const
{
	va_list ap;
	va_start( ap, fmt );
	Lock_t oLock( oPrintMutex );
	fprintf( stderr, "[%08"LL"u] %s: ", SirannonTime::getUpTime().convertMsecs(), sName.c_str() );
	vfprintf( stderr, fmt, ap );
	fprintf( stderr, "\n" );
	va_end( ap );
    return 0;
}

int MediaProcessor::print( int iLevel, const char* fmt, ... ) const
{
	if( pScope->getVerbose() >= iLevel )
	{
		va_list ap;
		va_start(ap, fmt);
		Lock_t oLock( oPrintMutex );
		fprintf( stderr, "info @ %s [%08"LL"u]: ", sName.c_str(), SirannonTime::getUpTime().convertMsecs() );
		vfprintf( stderr, fmt, ap );
		fprintf( stderr, "\n" );
		va_end( ap );
		fflush( stderr );
	}
	return 0;
}
