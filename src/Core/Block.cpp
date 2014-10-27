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
#include "Block.h"
#include "SirannonTime.h"

/** BLOCK
 * @component block
 * @type system
 * @param config, string, , path of the sirannon configuration file
 * @param param1, string, , if defined, command line parameter 1 for the configuration file
 * @param param2, string, , if defined, command line parameter 2 for the configuration file
 * @param param3, string, , if defined, command line parameter 3 for the configuration file
 * @param param4, string, , if defined, command line parameter 4 for the configuration file
 * @param param5, string, , if defined, command line parameter 5 for the configuration file
 * @param param6, string, , if defined, command line parameter 6 for the configuration file
 * @param param7, string, , if defined, command line parameter 7 for the configuration file
 * @param param8, string, , if defined, command line parameter 8 for the configuration file
 * @param param9, string, , if defined, command line parameter 9 for the configuration file
 * @param param10, string, , if defined, command line parameter 10 for the configuration file
 * @param param11, string, , if defined, command line parameter 11 for the configuration file
 * @param param12, string, , if defined, command line parameter 12 for the configuration file
 * @param param13, string, , if defined, command line parameter 13 for the configuration file
 * @param param14, string, , if defined, command line parameter 14 for the configuration file
 * @param param15, string, , if defined, command line parameter 15 for the configuration file
 * @info This component takes Sirannon configuration and loads this configuration within
 * itself. It allows grouping of components into one component that can be used in many
 * configurations. Packets are sent to or received from the surrounding scope of the
 * block using the \textit{in} component for input and \textit{out} component for output.
 **/
REGISTER_CLASS ( Block, "block" );


Block::Block( const string& sName, ProcessorManager* pScope )
	: MediaProcessor(sName, pScope), NestedInterface(sName, pScope),
	  in(NULL), out(NULL)

{
	mString["config"] = "";
}

void Block::handleError( SirannonException* pException, ProcessorManager* pManager, MediaProcessor* pProcessor )
{
	/* We cannot handle, let our scope handle it */
	pScope->handleError( pException, pScope, this );
}

void Block::init( void )
{
	/* Base class */
	MediaProcessor::init();

	/* Construct argv (with maximum 9 args) */
	int iArg;
	char sArg [8];
	const char* oArg [64];
	for( iArg = 0; iArg < 64; iArg++ )
	{
		sprintf( sArg, "param%d", iArg+1 );
		if(  mString.count( sArg ) )
			oArg[iArg] = mString[sArg].c_str();
		else
			break;
	}
	/* XML parser */
	if( oProcessorManager.loadXML( mString["config"], iArg, (const char**) oArg ) < 0 )
		RuntimeError( this, "while processing xml %s, detected %d errors",  mString["config"].c_str(), oProcessorManager.getXMLErrors() );

	/* Save in & out module */
	in = oProcessorManager.getProcessor( "in" );
	out = oProcessorManager.getProcessor( "out" );

	/* Init */
	oProcessorManager.initProcessors();

	/* We always schedule */
	bSchedule = true;

	/* Out */
	initOut();
}

void Block::initOut( void )
{
	/* Copy the vRouting of this Block to the out Block */
	if( out )
		out->setRouting( vRouting );
}

void Block::setRoute( uint32_t iRoute, MediaProcessor* pProc )
{
	MediaProcessor::setRoute( iRoute, pProc );
	if( out )
		out->setRouting( vRouting );
}

void Block::process( void )
{
	/* Schedule the processors */
	if( not oProcessorManager.scheduleProcessors() )
	{
		/* Terminate */
		pScope->stopProcessors();
	}
}

int Block::flush( void ) synchronized
{
	/* Super */
	MediaProcessor::flush();

	/* Flush all subcomp0nents */
	vector<MediaProcessor*> vProcs;
	oProcessorManager.getProcessors( vProcs );
	for( uint32_t i = 0; i < vProcs.size(); i++ )
		vProcs[i]->flush();
	return 0;
} end_synchronized

void Block::receive( MediaPacketPtr& pckt )
{
	if( in )
		in->receive( pckt );
}

void Block::receive_end( MediaPacketPtr& pckt )
{
	if( in )
		in->receive_end( pckt );
}

void Block::receive_reset( MediaPacketPtr& pckt )
{
	if( in )
		in->receive_reset( pckt );
}
