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
#include "sirannon.h"
#include "SirannonTime.h"
#include "ProcessorManager.h"
#include "sirannon.h"
#include "SirannonException.h"
#include "Utils.h"
#include "OSSupport.h"
#include <signal.h>

const char* sUsage = "Usage: sirannon [-bchv] [-q=NUM[s|ms|us|ns]] [-s=NUM[s|ms|us|ns]] [-r=NUM] FILE [ARG-1] [ARG-2] ... [ARG-n]";

/* Catch SIGTERM and call a proper destructor */
static bool bTerminate = false;
void terminate( int iVal )
{
	bTerminate = true;
}

void kill( int iVal )
{
	exit( 0 );
}

int main( int iArgc, const char* argv [] )
{
	/* Catch some signals */
	#ifndef WIN32
	signal( SIGTSTP, &kill ); // C-z
	signal( SIGHUP, &terminate );
	signal( SIGQUIT, &terminate ); // C-backslash
	#endif
	signal( SIGINT, &terminate ); // C-c
	signal( SIGTERM, &kill );

	/* Ingore broken pipe signals */
	#ifdef SIGPIPE
	signal( SIGPIPE, SIG_IGN );
	#endif

	/* Create a manager for the processors */
	ProcessorManager oProcessorManager( "core", NULL, NULL );

	/* Parse options */
	int64_t iQuantum = -1, iSimulation = -1, iVerbose = 0;
	int iArg = 0;
	bool bWarn = false;
	while( ++iArg < iArgc )
	{
		/* Detected an option */
		char sOption [16] = "";
		char sExtra [16] = "";
		uint64_t iVal = 0;

		/* Try to parse it */
		if( sscanf( argv[iArg], "-%1[qsr]=%"LL"u%15s", sOption, &iVal, sExtra ) >= 2 )
		{
			/* Modifier */
			if( not strcmp( sExtra, "ns" ) )
				iVal *= 1;
			else if( not strcmp( sExtra, "us" ) )
				iVal *= 1000;
			else if( not strcmp( sExtra, "s" ) )
				iVal *= 1000000000;
			else if( not strcmp( sExtra, "ms" ) )
				iVal *= 1000000;
			else if( strlen( sExtra ) == 0 )
				iVal *= 1000000;
			else
			{
				SirannonWarning( "malformatted time unit '%s'", sExtra );
				bWarn = true;
			}
			/* Detected the valid option format */
			switch( sOption[0] )
			{
			case 'q':
				iQuantum = iVal;
				iSimulation = 0;
				break;

			case 's':
				iSimulation = iVal;
				iQuantum = 0;
				break;

			case 'r':
				iSeed = iVal;
				break;

			default:
				RuntimeError( "Core: condition exception" );
			}
		}
		else if( sscanf( argv[iArg], "-%15[cvhb]%15s", sOption, sExtra ) == 1 )
		{
			for( uint32_t i = 0; i < strlen(sOption); i++ )
			{
				switch( sOption[i] )
				{
				case 'v':
					iVerbose++;
					break;

				case 'b':
					if( strlen(SVN_VERSION) )
						fprintf( stderr, "Sirannon version-%s-%s, Copyright (c) 2006-2011 IBCN\n", VERSION, SVN_VERSION );
					else
						fprintf( stderr, "Sirannon version-%s, Copyright (c) 2006-2011 IBCN\n", VERSION );
					fprintf(stderr, "  built on " __DATE__ " " __TIME__);
					#ifdef __GNUC__
						fprintf(stderr, ", gcc: " __VERSION__ "\n");
					#else
						fprintf(stderr, ", using a non-gcc compiler\n");
					#endif
					break;

				case 'h':
					fprintf( stderr, 	"%s\n"
										"Run the program with FILE as configuration.\n"
										"\n"
										"Options:\n"
										"  -h       Help information\n"
										"  -b       Build information\n"
										"  -c       Overview of components\n"
										"  -v       Verbose, use up to 5 repeats to increase the level\n"
										"  -q=NUM   Quantum in milliseconds\n"
										"  -s=NUM   Simulation in milliseconds\n"
										"  -r=NUM   Seed for the random number MediaProcessorGenerator\n"
										"\n"
										"Arguments:\n"
										"  ARG-1    Replace any occurence of $1 in FILE with ARG-1\n"
										"  ARG-2    Replace any occurence of $2 in FILE with ARG-2\n"
										"  ARG-NUM  Replace any occurence of $NUM in FILE with ARG-NUM\n"
										"\n"
										"Report bugs to <alexis.rombaut@intec.ugent.be>.\n"	,
							sUsage );
					return 0;

				case 'c':
					fprintf( stderr, "Available components (%d total):\n", (int)getMediaProcessorGenerator().size() );
					for( map<string,MediaProcessorGenerator*>::iterator j = getMediaProcessorGenerator().begin(); j != getMediaProcessorGenerator().end(); j++ )
						fprintf( stderr, "%s\n", j->first.c_str() );
					break;

				default:
					RuntimeError( "Core: condition exception" );
				}
			}
		}
		else if( sscanf( argv[iArg], "-%15s", sOption ) == 1 )
		{
			SirannonWarning( "core: Unrecognized or malformatted option '-%s'.", sOption );
			bWarn = true;
		}
		else
		{
			/* Found the XML file */
			break;
		}
	}
	/* Warning */
	if( bWarn )
	{
		SirannonWarning( "core: %s", sUsage );
		SirannonWarning( "core: Try 'Sirannon -h' for more information." );
		return 1;
	}
	/* Correct arguments? */
	if( iArg >= iArgc )
	{
		SirannonWarning( "core: no configuration file" );
		return 0;
	}
	/* Read the configuration */
	const char* sXML = argv[iArg++];
	try
	{
		if( oProcessorManager.loadXML( sXML, iArgc - iArg, argv + iArg ) < 0 )
			RuntimeError( "core: While parsing('%s'), detected %d Error(s)", sXML, oProcessorManager.getXMLErrors() );
	}
	catch( SirannonException& e )
	{
		e.unhandled();
		return 1;
	}
	/* Overwrite values */
	if( iQuantum >= 0 )
	{
		if( iQuantum < 0 )
			iQuantum = 1000000;
		oProcessorManager.setQuantum( SirannonTime::fromNSecs( iQuantum ) );
	}
	if( iSimulation >= 0 )
		oProcessorManager.setSimulation(SirannonTime::fromNSecs( iSimulation ) );
	oProcessorManager.setVerbose( iVerbose );

	/* Simulation mode */
	SirannonTime oStart = SirannonTime::getCurrentTime();
	if( not oProcessorManager.getSimulation().zero() )
		SirannonTime::setSimulatedTime( oProcessorManager.getSimulation() );
	else
		SirannonTime::setRealtime();

	/* Random numbers */
	if( oProcessorManager.getSeed() )
		srand( oProcessorManager.getSeed() );
	else
		srand( time(NULL) );

	/* Any components? */
	if( not oProcessorManager.size() )
	{
		SirannonWarning( "core: No components defined",  sXML, oProcessorManager.getXMLErrors() );
		return 0;
	}
	/* Initialize */
	try
	{
		oProcessorManager.initProcessors();
	}
	catch( SirannonException& e )
	{
		e.unhandled();
		return 1;
	}
	/* Timing info */
	SirannonTime oRef = SirannonTime::getCurrentTime(), oTot, oMax;
	int32_t iTick = 0;
	const SirannonTime oStep = oProcessorManager.getQuantum();

	/* Main loop */
	while( not bTerminate )
	{
		/* Do a step */
		SirannonTime oBegin = SirannonTime::getCurrentTime();
		if( not oProcessorManager.scheduleProcessors() )
			break;
		SirannonTime oDelta = SirannonTime::getCurrentTime() - oBegin;

		/* Simulated clock */
		if( SirannonTime::isSimulatedTime() )
		{
			SirannonTime::tick();
		}
		/* Realtime clock */
		else
		{
			/* How much are we ahead of schedule? */
			oRef.synchronizeInterval( oStep );

			/* Aggregates */
			if( oDelta > oMax )
				oMax = oDelta;
			oTot += oDelta;
			iTick++;
		}
	}
	/* Simulated clock */
	if( SirannonTime::isSimulatedTime() )
	{
		SirannonTime oEnd1 = SirannonTime::getCurrentTime();
		SirannonTime::setRealtime();
		SirannonTime oEnd2 = SirannonTime::getCurrentTime();
		oEnd2 -= oStart;
		if( oProcessorManager.getVerbose() )
			fprintf( stderr, "timing information: simulated(%3f s) real(%3f s) step(%"LL"u us)\n",
					oEnd1.convertDouble(), oEnd2.convertDouble(), oProcessorManager.getSimulation().convertUsecs() );
	}
	else
	{
		SirannonTime oEnd = SirannonTime::getCurrentTime();
		oEnd -= oStart;
		if( iTick <= 0 )
			iTick = 1;
		if( oProcessorManager.getVerbose() )
			fprintf( stderr, "timing information: quantum(%"LL"d ns) actual(%"LL"d ns) load(%2.2f%%) max(%"LL"d ns) duration(%f s)\n",
				oProcessorManager.getQuantum().convertNsecs(),
				oTot.convertNsecs() / iTick,
				MIN( 100., oTot.convertNsecs() * 1. / iTick / oProcessorManager.getQuantum().convertNsecs() * 100.),
				oMax.convertNsecs(),
				oEnd.convertDouble()  );
	}
	/* End */
	return bUnhandled;
}

void Sirannon_launch( int argc, const char* argv [] )
{
	main( argc, argv );
}
