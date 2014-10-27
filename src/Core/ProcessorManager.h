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
#ifndef PROCESSORMANAGER_H_
#define PROCESSORMANAGER_H_
#include "sirannon.h"
#include "MediaProcessor.h"
#include "SirannonTime.h"
#define SIRANNON_USE_BOOST_FUNCTION
#define SIRANNON_USE_BOOST_THREAD
#include "Boost.h"
#include <vector>
#include <map>

class NestedInterface;
class ProcessorManager
{
public:
	ProcessorManager( const string& sParent, ProcessorManager* pParent, NestedInterface* pOwner );
	~ProcessorManager();

	int loadXML( const string& sFile, int argc, const char* argv [] );
	int initProcessors( void );
	int initProcessor( const string& sProcessor );
	int stopProcessors( void );
	bool scheduleProcessors( void );

	static int checkProcessor( const string& sType );
	MediaProcessor* createProcessorGeneric( function2<MediaProcessor*, string, ProcessorManager*> oMediaProcessorFactory, const string& sName );
	MediaProcessor* createProcessor( const string& sType, const string& sName );
	MediaProcessor* createProcessorDynamic(
			const string& sType, const string& sName,
			vector<pair<MediaProcessor*,int> >* vIn,
			vector<pair<MediaProcessor*,int> >* vOut,
			map<string,int>* mInt,
			map<string,double>* mDouble,
			map<string,string>* mString,
			map<string,bool>* mBool,
			map<string,void*>* mPrivate );
	MediaProcessor* getProcessor( const string& sName ) const;
	MediaProcessor* getProcessor( const string& sName, const string& sParent ) const;
	int stopProcessor( const string sName ); /* Only this to components inside your own and not yourself */
	void handleError( SirannonException* pException, ProcessorManager* pManager, MediaProcessor* pProcessor );

	int setParent( ProcessorManager* pScope );
	int setInt( const string& sProcessor, const string& sVar, int iVal );
	int setDouble( const string& sProcessor, const string& sVar, double fVal );
	int setString( const string& sProcessor, const string& sVar, const string& sVal );
	int setBool( const string& sProcessor, const string& sVar, bool bVal );
	int setPrivate( const string& sIn, const string& sVar, void* pVal );
	int setRoute( const string& sFrom, const string& sTo, int iRoute );

	int size( void ) const;
	const string& str( void ) const;
	const char* c_str( void ) const;

	int getSeed( void ) const;
	const SirannonTime& getQuantum( void ) const;
	const SirannonTime& getSimulation( void ) const;
	int getVerbose( void ) const;
	void setSeed( int iSeed );
	void setQuantum( const SirannonTime& oQuantum );
	void setSimulation( const SirannonTime& oSimulation );
	void setVerbose( int iVerbose );
	int getXMLErrors( void ) const;
	void getProcessorNames( vector<string>& vProcessors ) const;
	int getProcessors( vector<MediaProcessor*>& vProcessors ) const;
	ProcessorManager* getParent( void ) const;
	ProcessorManager* getCore( void ) const;

private:
	int iErrors;
	typedef map<string,MediaProcessor*> map_t;
	map_t mProcessors;
	bool bStop, bError;
	string sParent;
	ProcessorManager* pParent;
	NestedInterface* pOwner;
	mutable mutex oMutex;
};

const static SirannonTime oDefaultQuantum( 0, 40000000 );

/* Inline functions */

#include "ProcessorManager_priv.h"

#endif /*PROCESSORMANAGER_H_*/
