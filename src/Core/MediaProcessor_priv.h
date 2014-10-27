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
#ifndef MEDIA_PROCESSOR_PRIV_H_
#define MEDIA_PROCESSOR_PRIV_H_

/* MediaProcessorGenerator class for dynamic class loading */


template<class T>
class MediaProcessorFactory : public MediaProcessorGenerator
{
	public:
	MediaProcessorFactory( const char* sName )
		: MediaProcessorGenerator(sName)
	{ }

    MediaProcessor* create( const char* sName, ProcessorManager* pScope )
    {
    	return new T( sName, pScope );
    }
};

/* Our holy grale of the dynamic system, the extern map containing the MediaProcessorGenerators */

#define debug(x,...)\
	do{\
		if( pScope->getVerbose() >= 5 or ( bDebug and pScope->getVerbose() >= (x) ) )\
		{\
			_debug(__VA_ARGS__);\
		}\
	} while(0)

#define REGISTER_CLASS(X,S) static const MediaProcessorFactory<X> X##_1 = S
#define REGISTER_CLASSES(X,S,N) static const MediaProcessorFactory<X> X##_##N = S

#define synchronized { FlowLock_t _oLock( oFlowMutex );
#define end_synchronized }

/* Inline */
inline const string& MediaProcessor::str( void ) const
{
	return sName;
}

inline const char* MediaProcessor::c_str( void ) const
{
	return sName.c_str();
}

inline void MediaProcessor::setString( const string& prop, string value )
{
	mString[prop] = value;
}

inline void MediaProcessor::setInt( const string& prop, int value )
{
	mInt[prop]= value;
}

inline void MediaProcessor::setDouble( const string& prop, double value )
{
	mDouble[prop] = value;
}

inline void MediaProcessor::setBool( const string& prop, bool value )
{
	mBool[prop]	= value;
}

inline void MediaProcessor::setPrivate( const string& sProp, void* pVal )
{
	mPrivate[sProp] = pVal;
}

inline bool MediaProcessor::is_scheduled( void ) const
{
	return bSchedule;
}

inline bool MediaProcessor::is_threaded( void ) const
{
	return bThread;
}

inline bool MediaProcessor::is_stopped( void ) const
{
	return bStop;
}

inline void MediaProcessor::setRouting( const MediaProcessor::routing_t& oRouting )
{
	FlowLock_t oLock( oFlowMutex );
	vRouting = oRouting;
}

inline MediaProcessor::routing_t MediaProcessor::getRouting( void ) const
{
	return vRouting;
}

inline void MediaProcessor::schedule( void )
{
	if( bSchedule and not bThread )
	{
		FlowLock_t oLock( oFlowMutex );
		process();
	}
}

inline void MediaProcessor::receivePacket( MediaPacketPtr& pPckt )
{
	if( not bThread )
	{
		while( not bReceive )
		{
			oPollQuantum.sleep();
			this_thread::interruption_point();
		}
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
			RuntimeError( this, "Wrong packet type(%d)", (int)pPckt->type );
		}
	}
	else
	{
		Lock_t oLock( oQueueMutex );
		vPacketQueue.push( pPckt.release() );
	}
}


#endif /*MEDIA_PROCESSOR_PRIV_H_*/
