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
#include "OSSupport.h"
#include "XMLStream.h"
#include "SirannonTime.h"
#include "sirannon.h"

/* Constructor and parser */
XMLStream::XMLStream( const string& xml_filename, int _argc, const char* _argv [],
		ProcessorManager& oProcessorManager )
	: valid(0), argc(_argc), argv(_argv), filename(xml_filename),
	 oProcessorManager(oProcessorManager), oXML(NULL), oTokenizer(string())
{
}

int XMLStream::parse( void )
{
	/* Open the xml file */
	FILE* oFile = fopen( filename.c_str(), "r" );

	/* Check for error while reading the stream */
	if ( not oFile or ferror( oFile ) )
		IOError( "XML: Couldn't open file(%s)", filename.c_str() );

	/* Create our parser */
	oXML = XML_ParserCreate(0);

	/* Link the eventhandlers (because this a class structure) */
	XML_SetUserData( oXML, this );

	/* Setup our eventhandlers */
	XML_SetElementHandler( oXML, this->startElementWrapper, this->endElementWrapper );

	/* Pass our file through the parser */
	char sLine [1024];

	while( not feof(oFile) )
	{
		/* Get the line with special care for eof robustness */
		if( not fgets( sLine, sizeof(sLine), oFile ) )
			continue;

		/* Parse it */
		if( XML_Parse( oXML, sLine, strlen(sLine), feof(oFile) ) == XML_STATUS_ERROR )
			SyntaxError( "XML: Invalid syntax on line %d. Maybe you forgot to close your previous element with an </...>?",  XML_GetCurrentLineNumber(oXML) );
		XML_Parse( oXML, "\n", 1, 0);
	}
	/* Close inputfile */
	fclose( oFile );

	return valid;
}

XMLStream::~XMLStream( )
{
	/* Free the parser */
	if( oXML )
		XML_ParserFree(oXML);
}

/* Handle the opening tag of an element */
void XMLStream::startElement( const XML_Char* _c_name, const XML_Char** attr )
{
   	const string c_name (_c_name);
	int i;

    if( c_name == "config" )
    {
    	/* Default value */
    	for( i = 0; attr[i]; i += 2 )
		{
			if( string( attr[i] ) == "quantum" )
				oProcessorManager.setQuantum( SirannonTime( 0, 1000 * atoi( attr[i+1] ) ) );
			if( string( attr[i] ) == "simulation" )
				oProcessorManager.setSimulation( SirannonTime( 0, 1000 * atoi( attr[i+1] ) ) );
			if( string( attr[i] ) == "seed" )
				oProcessorManager.setSeed( atoi( attr[i+1] ) );
		}
    }
    else if( c_name == "component" )
	{
		/* Default data */
	    string sType = "";
	    string sName = "";

	    /* Cycle over the atributes */
		for( i = 0; attr[i]; i += 2 )
		{
			if( string(attr[i]) == "type" )
				sType = attr[i+1];
			else if( string(attr[i]) == "name" )
				sName = attr[i+1];
		}
		/* Was the component valid? */
	    if( not sType.length() or not sName.length() )
	    	SyntaxError( "XML: Component declaration without name or type on line %d", XML_GetCurrentLineNumber(oXML) );

		/* Create the component */
	    MediaProcessor* oProcessor = oProcessorManager.createProcessor( sType, sName );

		/* Remember the current component */
		sCurrent = sName;
	}
	else if( c_name == "param" )
	{
		/* Check if the element param is nested inside a component element */
		if( sCurrent == "" )
			SyntaxError( "XML: Param not nested inside a component on line %d", XML_GetCurrentLineNumber(oXML) );

		/* Extract the attributes from the element */
		string name	= "";
		string type	= "";
		string val	= "";
		string cmdl	= "";

	    for( i = 0; attr[i]; i += 2 )
		{
			if( string(attr[i]) == "name" )
				name = attr[i+1];
			else if( string(attr[i]) == "type" )
				type = attr[i+1];
			else if( string(attr[i])=="val" )
				val = attr[i+1];
	    }

	    /* Check if the attribute name is valid */
		if( name == "" )
			ValueError( "XML: Attribute name without string or missing on line %d", XML_GetCurrentLineNumber(oXML) );
	    if( name.find( '.' ) != string::npos )
	    	ValueError( "XML: Name(%s) contains a '.'", name.c_str() );

		/* Commandline paramters type 2 */
		size_t iPos = 0;
		while( iPos != string::npos )
		{
			iPos = val.find( '$', iPos );
			if( iPos != string::npos )
			{
				/* Perform check */
				if( iPos + 1 >= val.length() )
					ValueError( "XML: '$' without number following" );

				/* Catch $$ */
				if( *(val.c_str() + iPos + 1) == '$' )
				{
					iPos += 2;
					continue;
				}
				/* Acquire index */
				int iIndex = atoi( val.c_str() + iPos + 1 );
				if( iIndex < 1 or iIndex > argc )
				{
					if( iIndex > argc )
						ValueError( "XML: no matching command line argument for $%d", iIndex );
					else
						ValueError( "XML: '%s' pos(%d/%d) index(%d), illegal command line argument specification on line %d",
								val.c_str(), iPos, iIndex, (int)val.size(), XML_GetCurrentLineNumber(oXML) );
				}
				/* Find iDiff */
				int iDiff = iIndex / 10 + 1;

				/* Substitute */
				val.replace( iPos, 1+iDiff, argv[iIndex-1], 0, strlen(argv[iIndex-1]) );

				/* Shift */
				iPos += strlen(argv[iIndex-1]);
			}
		}
		/* Store the data in the correspoding components */
		if( type == "int" )
			oProcessorManager.setInt( sCurrent, name, atoi(val.c_str()) );
	    else if( type == "double" )
			oProcessorManager.setDouble( sCurrent, name, atof(val.c_str()) );
	    else if( type == "string" )
	    	oProcessorManager.setString( sCurrent, name, val );
	    else if( type == "bool" )
	    {
			if ( ( val == "true" ) || ( val == "True" ) || ( val == "1" ) )
				oProcessorManager.setBool( sCurrent, name, true );
			else
				oProcessorManager.setBool( sCurrent, name, false );
	    }
		else
		{
			TypeError( "XML: unknown parameter type(%s) on line(%d)", type.c_str(), XML_GetCurrentLineNumber(oXML) );
	    }
	}
	else if( c_name == "route" )
	{
	  	string sFrom, sTo, sRoute;
	    const string delim = ",";
		bool found_route = false;

	    /* Extract the attributes from the element */
		for( i = 0; attr[i]; i += 2 )
		{
			if( string(attr[i]) == "xroute" )
			{
				sRoute = attr[i+1];
				oTokenizer.assign( sRoute );
				found_route = true;
			}
			else if( string(attr[i]) == "from" )
			{
				sFrom = attr[i+1];
			}
			else if( string(attr[i]) == "to" )
			{
				sTo = attr[i+1];
			}
	    }
		/* Sanity check */
		if( not found_route )
			SyntaxError( "XML: Attribute xroute missing on line %d",  XML_GetCurrentLineNumber(oXML) );

		/* Store the route into the component */
		for( tokenizer<>::iterator i = oTokenizer.begin(); i != oTokenizer.end(); i++ )
		{
			if( oProcessorManager.setRoute( sFrom, sTo, atoi(i->c_str()) ) < 0 )
				ValueError( "XML: invalid route on line %d",  XML_GetCurrentLineNumber(oXML) );
		}
	}
}

/* Handle the close tag of an element */
void XMLStream::endElement( const XML_Char* c_name )
{
	if( string(c_name) == "component" )
	{
		/* Set our current element to none */
		sCurrent = "";
	}
}

/* Wrapper for class calls */
void XMLStream::startElementWrapper( void* userData, const XML_Char* name, const XML_Char** attr )
{
	((XMLStream*) userData) -> startElement( name, attr );
}

/* Wrapper for class calls */
void XMLStream::endElementWrapper( void* userData, const XML_Char* name )
{
	((XMLStream*) userData) -> endElement( name );
}
