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

/************************************************************************************
 * MediaProcessor - ProcessorManger contract:
 * Art 1: MediaProcessor MUST NOT call destruct(), ~MediaProcessor(), cancelThreads() but MUST use end() for termination
 * Art 2: MediaProcessor MUST use createThread() to create threads
 * Art 3: MediaProcessor MUST NOT block inside the calls of receive, receive_reset, receive_end, process
 */
#include "ProcessorManager.h"
#include "XMLStream.h"
#include "SirannonException.h"
#include "Interfaces.h"

int iSeed=0, iVerbose=0;
SirannonTime oQuantum(40), oSimulation(0);

ProcessorManager::ProcessorManager( const string& sParent, ProcessorManager* pParent, NestedInterface* pOwner )
	: iErrors(0), bStop(false), sParent(sParent), pParent(pParent), pOwner(pOwner), bError(false)
{ }

ProcessorManager::~ProcessorManager()
{
	Lock_t oLock( oMutex );
	/* Cancel all threads */
	for( map_t::iterator i = mProcessors.begin(); i != mProcessors.end(); i++ )
	{
		MediaProcessor* pProc = i->second;
		pProc->cancelThreads();
	}
	/* Destruct all components */
	for( map_t::iterator i = mProcessors.begin(); i != mProcessors.end(); i++ )
	{
		MediaProcessor* pProc = i->second;
		delete pProc;
	}
}

int ProcessorManager::size( void ) const
{
	int iSize;
	Lock_t oLock( oMutex );
	iSize = mProcessors.size();
	return iSize;
}

int ProcessorManager::initProcessors( void )
{
	for( map_t::iterator i = mProcessors.begin(); i != mProcessors.end(); i++ )
		i->second->initProcessor();
	return 0;
}

int ProcessorManager::initProcessor( const string& sProcessor )
{
	/* Obtain the pointer */
	MediaProcessor* pProcessor = getProcessor( sProcessor );

	/* Initialize */
	pProcessor->initProcessor();
	return 0;
}

int ProcessorManager::loadXML( const string& sFile, int argc, const char* argv [] )
{
	/* Create an XML parser */
	XMLStream xml_stream = XMLStream( sFile, argc, argv, *this );

	/* Parse the file */
	xml_stream.parse();
	if( ( iErrors = xml_stream.getErrors() ) )
		return -1;
	else
		return 0;
}

void ProcessorManager::getProcessorNames( vector<string>& vProcessors ) const
{
	Lock_t oLock( oMutex );
	for( map_t::const_iterator i = mProcessors.begin(); i != mProcessors.end(); i++ )
		vProcessors.push_back( i->second->str() );
}

int ProcessorManager::stopProcessors( void )
{
	bStop = true;
	return 0;
}

MediaProcessor* ProcessorManager::createProcessorGeneric(
		function2<MediaProcessor*, string, ProcessorManager*> oMediaProcessorFactory, const string& sName )
{
	/* Actual name */
	string sFullName = sParent + "." + sName;

	/* Valid name */
	if( sName.find( '.' ) != string::npos )
		SyntaxError( "name(%s) contains the illegal character '.'", sName.c_str() );

	/* Unique name? */
	Lock_t oLock( oMutex );
	if( mProcessors.count(sFullName) > 0 )
		ValueError( "component name(%s) not unique", sName.c_str() );

	/* Create it */
	MediaProcessor* pProcessor = oMediaProcessorFactory( sFullName, this );
	mProcessors[sName] = pProcessor;
	return pProcessor;
}

MediaProcessor* ProcessorManager::createProcessor( const string& sType , const string& sName )
{
	/* Valid name */
	if( sName.find( '.' ) != string::npos )
		SyntaxError( "Name(%s) contains the illegal character '.'", sName.c_str() );

	/* Type exists? */
	if( getMediaProcessorGenerator().count(sType) == 0 )
		TypeError( "Type(%s) unkown for component(%s)", sType.c_str(), sName.c_str() );

	/* Unique name? */
	Lock_t oLock( oMutex );
	if( mProcessors.count(sName) > 0 )
		ValueError( "Component name(%s) not unique", sName.c_str() );

	/* Map the typename to the matching component */
	MediaProcessorGenerator* pFac = getMediaProcessorGenerator()[sType];

	/* Create it */
	string sFullName = sParent + "." + sName;
	MediaProcessor* pProcessor = pFac->create( sFullName.c_str(), this );
	mProcessors[sName] = pProcessor;
	return pProcessor;
}

MediaProcessor* ProcessorManager::createProcessorDynamic(
		const string& sType, const string& sName,
		vector<pair<MediaProcessor*,int> >* vIn,
		vector<pair<MediaProcessor*,int> >* vOut,
		map<string,int>* mInt,
		map<string,double>* mDouble,
		map<string,string>* mString,
		map<string,bool>* mBool,
		map<string,void*>* mPrivate )
{
	/* Make the component */
	MediaProcessor* oProcessor = createProcessor( sType, sName );
	if( oProcessor == NULL )
		return NULL;

	/* Set parameters */
	Lock_t oLock( oMutex );
	if( mInt )
		for( map<string,int>::iterator i = mInt->begin(); i != mInt->end(); i++ )
			oProcessor->setInt( i->first, i->second );

	if( mDouble )
		for( map<string,double>::iterator i = mDouble->begin(); i != mDouble->end(); i++ )
			oProcessor->setDouble( i->first, i->second );

	if( mString )
		for( map<string,string>::iterator i = mString->begin(); i != mString->end(); i++ )
			oProcessor->setString( i->first, i->second );

	if( mBool )
		for( map<string,bool>::iterator i = mBool->begin(); i != mBool->end(); i++ )
			oProcessor->setBool( i->first, i->second );

	if( mPrivate )
		for( map<string,void*>::iterator i = mPrivate->begin(); i != mPrivate->end(); i++ )
			oProcessor->setPrivate( i->first, i->second );

	/* Connections */
	if( vIn )
		for( uint32_t i = 0; i < vIn->size(); i++ )
			(*vIn)[i].first->setRoute( (*vIn)[i].second, oProcessor );

	if( vOut )
		for( uint32_t i = 0; i < vOut->size(); i++ )
			oProcessor->setRoute( (*vOut)[i].second, (*vOut)[i].first );

	/* Initialize it */
	oProcessor->initProcessor();
	return oProcessor;
}

/* CAVEAT: the Sirannon will crash if another component routed to this component
 * tries to send it a message FIXME */
int ProcessorManager::stopProcessor( string sIn )
{
	/*  Does it exist? */
	int iCount;
	Lock_t oLock( oMutex );
	if( (iCount = mProcessors.count(sIn)) > 0 )
	{
		/* Free the component and remove it from the list */
		MediaProcessor* pProc = mProcessors[sIn];
		pProc->end();
	}
	/* Succes? */
	if( iCount > 0 )
		return 0;
	else
		return -1;
}

void ProcessorManager::handleError( SirannonException* pException, ProcessorManager* pManager, MediaProcessor* pProcessor )
{
	bError = true;
	if( pOwner )
		pOwner->handleError( pException, pManager, pProcessor );
	else
		pException->unhandled();
}

int ProcessorManager::setInt( const string& sIn, const string& sVar, int iVal )
{
	Lock_t oLock( oMutex );
	if( not mProcessors.count(sIn) )
		ValueError( "Component(%s) does not exist", sIn.c_str() );
	mProcessors[sIn]->setInt( sVar, iVal );
	return 0;
}

int ProcessorManager::setString( const string& sIn, const string& sVar, const string& sVal )
{
	Lock_t oLock( oMutex );
	if( not mProcessors.count(sIn) )
		ValueError( "Component(%s) does not exist", sIn.c_str() );
	mProcessors[sIn]->setString( sVar, sVal );
	return 0;
}

int ProcessorManager::setBool( const string& sIn, const string& sVar, bool bVal )
{
	Lock_t oLock( oMutex );
	if( not mProcessors.count(sIn) )
		ValueError( "Component(%s) does not exist", sIn.c_str() );
	mProcessors[sIn]->setBool( sVar, bVal );
	return 0;
}

int ProcessorManager::setDouble( const string& sIn, const string& sVar, double fVal )
{
	Lock_t oLock( oMutex );
	if( not mProcessors.count(sIn) )
		ValueError( "Component(%s) does not exist", sIn.c_str() );
	mProcessors[sIn]->setDouble( sVar, fVal );
	return 0;
}

int ProcessorManager::setPrivate( const string& sIn, const string& sVar, void* pVal )
{
	Lock_t oLock( oMutex );
	if( not mProcessors.count(sIn) )
		ValueError( "Component(%s) does not exist", sIn.c_str() );
	mProcessors[sIn]->setPrivate( sVar, pVal );
	return 0;
}

int ProcessorManager::setRoute( const string& sInFrom, const string& sInTo, int iRoute )
{
	Lock_t oLock( oMutex );

	/* Do the processors exists? */
	if( not mProcessors.count( sInFrom )  )
		ValueError( "Component(%s) does not exist", sInFrom.c_str() );
	if( not mProcessors.count( sInTo ) )
		ValueError( "Component(%s) does not exist", sInTo.c_str() );

	/* Connect them */
	mProcessors[sInFrom]->setRoute( iRoute, mProcessors[sInTo] );
	return 0;
}

bool ProcessorManager::scheduleProcessors( void )
{
	/* Iterate over the STL */
	Lock_t oLock( oMutex );
	map_t::iterator i;
	try
	{
		//fprintf( stderr, "BEGIN %s %llu\n", sParent.c_str(), SirannonTime::getUpTime().convertMsecs() );
		for( i = mProcessors.begin(); i != mProcessors.end(); )
		{
			MediaProcessor* pProc = i->second;

			/* Check for stop */
			//if( not pProc->is_threaded() )
			//	fprintf( stderr, "SCHEDULE %s %s %llu\n", sParent.c_str(), pProc->c_str(), SirannonTime::getUpTime().convertMsecs() );
			if( pProc->is_stopped() )
			{
				pProc->cancelThreads();
				delete pProc;
				mProcessors.erase( i++ );
			}
			/* Check for schedule */
			else
			{
				pProc->schedule();
				i++;
			}
		}
		//fprintf( stderr, "END %s %llu\n", sParent.c_str(), SirannonTime::getUpTime().convertMsecs() );
	}
	catch( SirannonException& oException )
	{
		handleError( &oException, this, i->second );
	}
	return not bStop;
}

MediaProcessor* ProcessorManager::getProcessor( const string& sIn ) const
{
	/* Find */
	Lock_t oLock( oMutex );
	if( not mProcessors.count(sIn) )
		return NULL;
	return mProcessors.find( sIn )->second;
}

int ProcessorManager::getProcessors( vector<MediaProcessor*>& vProcessors ) const
{
	Lock_t oLock( oMutex );
	for( map_t::const_iterator i = mProcessors.begin(); i != mProcessors.end(); i++ )
		vProcessors.push_back( i->second );
	return 0;
}

int ProcessorManager::checkProcessor( const string& sType )
{
	/* Type exists? */
	if( getMediaProcessorGenerator().count(sType) == 0 )
		return -1;
	return 0;
}

ProcessorManager* ProcessorManager::getCore( void ) const
{
	if( pParent )
		return pParent->getCore();
	else
		return (ProcessorManager*) this;
}
