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
#include "SVCClassifier.h"
#include "Frame.h"
#include <cmath>
#include "h264_avc.h"

/**
 * SVC CLASSIFIER
 * @component svc-classifier
 * @type classifier
 * @param layer, int, 0, offset for layer 0
 * @param layer-1, string, , definintion and offset for layer 1
 * @param layer-2, string, , definintion and offset for layer 2
 * @param layer-3, string, , definintion and offset for layer 3
 * @param layer-4, string, , definintion and offset for layer 4
 * @param layer-5, string, , definintion and offset for layer 5
 * @param layer-6, string, , definintion and offset for layer 6
 * @param layer-7, string, , definintion and offset for layer 7
 * @info This component classifies an SVC stream. The partitioning is defined by a
 * series of $(T,D,Q)$ triplets. For each passing NAL unit this series is reverse
 * iterated. If the NAL unit depends on the $n^{th}$ triplet then it is classified as
 * $layer_{n}$. When it didn't depend on any layer it is classified as $layer_{0}$.
 * The triplets are entered in the form of $"T,D,Q:offset"$, for example $"4,0,0:2"$,
 * meaning \textit{"add offset 2 to packets of temporal layer (T) 4 or higher"}.
 **/
REGISTER_CLASS( SVCClassifier, "svc-classifier" );
REGISTER_CLASS( svc_analyser  , "svc-analyzer"   );

SVCClassifier::SVCClassifier( const string& sName, ProcessorManager* pScope )
	: Classifier(sName, pScope), layer(0)
{
	mInt["layer-0"]  = 0;
}

void SVCClassifier::init( void )
{
	/* Base class */
	MediaProcessor::init();

	/* Get the triple points */
	char s [32];

	/* Default base layer */
	layers.push_back( mInt[ "layer-0"] );
	triplets.push_back( triplet( 0, 0, 0 ) );

	/* Iterate over the steps */
	for( int i = 1; i < 1000; i++ )
	{
		/* Step i */
		snprintf( s, sizeof(s), "layer-%d", i );

		/* We accept formates of the form T,D,Q:layer */
		if( mString.count( s ) )
		{
			/* Stop if we meet an empty string */
			if( mString[s].size() == 0 )
				break;

			/* Analyse this string */
			vector<string> pieces1;
			split( mString[s], pieces1, ":" );

			/* Check if the correct format */
			if( pieces1.size() != 2 )
			{
				SirannonWarning( this, "%s: non well formed triplet", s );
				break;
			}
			/* Store the layer */
			layers.push_back( atoi( pieces1[1].c_str() ) );

			/* Analyse this string */
			vector<string> pieces2;
			split( pieces1[0], pieces2, "," );

			/* Check if it is well formed*/
			if( pieces2.size() != 3 )
			{
				SirannonWarning( this, "step %d: non well formed triplet", i+1 );
				break;
			}

			/* Add to our list of triplets */
			triplet t = triplet( 	atoi( pieces2[0].c_str() ),
									atoi( pieces2[1].c_str() ),
									atoi( pieces2[2].c_str() )  );
			/* Store the triplet */
			triplets.push_back( t );
		}
		else
			break;
	}
}

void SVCClassifier::receive( MediaPacketPtr& pPckt )
{
	/* SVC or AVC? */
	if( pPckt->codec == codec_t::svc or pPckt->codec == codec_t::mvc )
	{
		/* TDQ information */
		const triplet u = triplet( pPckt->T, pPckt->D, pPckt->Q );

		/* Iterate over the different layers and check if the triplet is include in it */
		int i;
		for( i = triplets.size()-1; i >= 0; i-- )
		{
			if( u >= triplets[i] )
			{
				/* Found the layer of the packet */
				layer = i;
				break;
			}
		}
	}
	else if( pPckt->codec == codec_t::avc )
	{
		/* For SEI, PPS, SPS we just assign it the base layer */
		switch( pPckt->frame )
		{
		case frame_t::SPS:
		case frame_t::PPS:
		case frame_t::SEI:
			layer = 0;
			break;
		default:
			/* Use the layer info from the previous packet */
			break;
		}
	}
	else
		RuntimeError( this, "Wrong codec(%s) for %s", CodecToString(pPckt->codec), pPckt->c_str() );

	/* Classify the packet */
	classify( pPckt, layer > 0, layers[layer] );
}

bool triplet::operator==( const triplet& other ) const
{
	return T == other.T and D == other.D and Q == other.Q;
}

bool triplet::operator>=( const triplet& other ) const
{
	if( T >= other.T && Q >= other.Q && D >= other.D )
		return true;
	else
		return false;
}

bool triplet::operator<( const triplet& other ) const
{
	if( T < other.T )
		return true;
	else if( T > other.T )
		return false;
	if( D < other.D )
		return true;
	else if( D > other.D )
		return false;
	if( Q < other.Q )
		return true;
	else if( Q > other.Q )
		return false;
	return false;
}

bool triplet::compareT( const triplet& bottom, const triplet& top )
{
	if( bottom.T < top.T )
		return false;
	return ( bottom.t >> ( bottom.T - top.T ) ) == top.t;
}

triplet& triplet::operator=( const triplet& other )
{
	T = other.T;
	Q = other.Q;
	D = other.D;
	t = other.t;
	return *this;
}

/**
 * SVC ANALYZER
 * @component svc-analyzer
 * @type miscellaneous
 * @param filename, string, , the path of the maple worksheet with extension .mws
 * @param dimension, int, 1, in how many dimensions to scale the cube (1, 2 or 3)
 * @info This component constructs an inter layer bandwidth comparison at the end
 * of a passing svc stream. A 3D graph is constructed with for each layer a cube
 * scaled with the relative weight of the layer in the total bandwidth. The graph
 * is generated as a maple worksheet (\url{www.maplesoft.com}).
 **/
svc_analyser::svc_analyser( const string& sName, ProcessorManager* pScope )
	: MediaProcessor(sName, pScope), lastKey(0,0,0), bSingleton(false), fTotal(0.0)
{
	mString["filename"] = "";
	mInt["dimension"] = 1;
}

void svc_analyser::init( void )
{
	/* Base class */
	MediaProcessor::init();

	bSingleton = mBool["steps"];
}

void svc_analyser::receive( MediaPacketPtr& pPckt )
{
	if( pPckt->codec == codec_t::svc )
	{
		/* Make a TDQ triplet */
		key_t key( pPckt->T, pPckt->D, pPckt->Q );
		lastKey = key;

		/* Mark start of new GOP */
		if( key.T == 0 and key.D == 0 and key.Q == 0 )
			newGOP();

		/* Append it to the layers */
		layers[key].push_back( val_t( pPckt->dts, pPckt->size() ) );
	}
	else if( pPckt->codec == codec_t::avc )
	{
		//if( IsSlice( pPckt ) )
		//{
			/* Append it to the layers */
			layers[lastKey].push_back( val_t( pPckt->dts, pPckt->size() ) );
		//}
	}
	else
		RuntimeError( this, "unsupported codec in %s", pPckt->c_str() );

	/* Just forward the packet */
	route( pPckt );
}

void svc_analyser::newGOP( void )
{
	if( bSingleton and layers.size() )
	{
		/* Analyze */
		analyze();

		/* Output data */
		chatter();

		/* Reset */
		layers.clear();
		bitrates.clear();
	}
}

void svc_analyser::analyze( void )
{
	/* Compute the bitrate for all layers */
	fTotal = 0.0;
	for( map_it i = layers.begin(); i != layers.end(); i++ )
	{
		/* Elements */
		vector<val_t>& stream = i->second;
		key_t key = i->first;

		/* Total number of bytes */
		int64_t iSize = 0;
		for( uint32_t j = 0; j < stream.size(); j++ )
			iSize += stream[j].second;

		/* Save the bitrate */
		bitrates[key] = iSize;
		fTotal += iSize;
	 }

	/* Get the maximum ratio */
	vector<float> values;
	get_values<key_t,float>( bitrates, values );
	if( values.size() )
		fMaxRatio = *max_element( values.begin(), values.end() );
}

void svc_analyser::chatter( void )
{
	timestamp_t dts = 0;
	if( layers[key_t(0,0,0)].size() )
		dts = layers[key_t(0,0,0)].front().first;
	for( map<key_t,float>::iterator i = bitrates.begin(); i != bitrates.end(); i++ )
  	{
 		/* Elements */
		float val = i->second;
		key_t key = i->first;

		/* Give division */
		debug( 1, "(%d,%d,%d): %f%", key.T, key.D, key.Q, val / fTotal * 100.0 );
  	}
}

void svc_analyser::maple( void )
{
	/* No need to construct when no file is given or it is stepped */
	if( mString["filename"].empty() or bSingleton )
		return;

	/* Analyze */
	analyze();

	/* Construct the maple string and save it to a file */
	/* Start of maple command */
	string s = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		 		"<Worksheet><Group><Input><Text-field layout=\"Normal\" linebreak=\"space\" prompt=\"&gt; \" style=\"Maple Input\">"
		 		"PLOT3D(";

	/* Now generate the cubes */
	for( map_it i = layers.begin(); i != layers.end(); i++ )
 	{
		/* Elements */
 		vector<val_t>& stream = i->second;
 		key_t key = i->first;

 		/* Construct a cube */
 		s.append( cube( key, bitrates[key], fMaxRatio ) );
 	}

	/* Collect the percentages for the different layers */
	map<int,float> aggr_T;
 	for( map<key_t,float>::iterator i = bitrates.begin(); i != bitrates.end(); i++ )
  	{
 		/* Elements */
		float val = i->second;
		key_t key = i->first;

		/* Sum */
		aggr_T[key.T] += val * 100.0/ fTotal;
  	}
 	map<int,float> aggr_D;
 	for( map<key_t,float>::iterator i = bitrates.begin(); i != bitrates.end(); i++ )
  	{
 		/* Elements */
		float val = i->second;
		key_t key = i->first;

		/* Sum */
		aggr_D[key.D] += val * 100.0/ fTotal;
  	}
 	map<int,float> aggr_Q;
	for( map<key_t,float>::iterator i = bitrates.begin(); i != bitrates.end(); i++ )
	{
		/* Elements */
		float val = i->second;
		key_t key = i->first;

		/* Sum */
		aggr_Q[key.Q] += val * 100.0/ fTotal;
	}
	/* Construct the axe strings */
 	static const char tmplt [] = "[0.35=`0-%.0f%%`,1.35=`1-%.0f%%`,2.35=`2-%.0f%%`, 3.35=`3-%.0f%%`,4.35=`4-%.0f%%`]";
 	char sAxeT [256];
 	char sAxeD [256];
 	char sAxeQ [256];
	snprintf( sAxeT, sizeof(sAxeT), tmplt, aggr_T[0], aggr_T[1], aggr_T[2], aggr_T[3], aggr_T[4] );
	snprintf( sAxeD, sizeof(sAxeD), tmplt, aggr_D[0], aggr_D[1], aggr_D[2], aggr_D[3], aggr_D[4] );
 	snprintf( sAxeQ, sizeof(sAxeQ), tmplt, aggr_Q[0], aggr_Q[1], aggr_Q[2], aggr_Q[3], aggr_Q[4] );

	/* End of maple command */
 	static const char tModifiers [] =
		"SCALING(CONSTRAINED),"
 		"AXESSTYLE(BOXED),"
 		"ORIENTATION(-59,66),"
        "AXESLABELS(\"Temporal (T)\",\"Quality (Q)\",\"Resolution (D)\",FONT(HELVETICA,BOLD,10)),"
 		"AXESTICKS(%s,%s,%s,FONT(HELVETICA,BOLD,10)));"
 		""
 		"</Text-field></Input></Group><View-Properties/><Page-Numbers enabled=\"false\" first-number=\"1\" horizontal-location=\"right\" style=\"Page Number\" vertical-location=\"bottom\"/></Worksheet>";
 	char sModifiers [ 256 + sizeof(tModifiers) ];
 	snprintf( sModifiers, sizeof(sModifiers), tModifiers, sAxeT, sAxeQ, sAxeD );
 	s = s + sModifiers;

	/* Open a file and add the string */
	FILE* oFile = fopen( mString["filename"].c_str(), "w" );
	if( oFile )
	{
		fwrite( s.c_str(), sizeof(char), s.size(), oFile );
		if( ferror( oFile) )
			SirannonWarning( this,  "could not write to file %s", mString["filename"].c_str() );
		fclose( oFile );
	}
	else
	{
		 SirannonWarning( this,  "could not write to file %s", mString["filename"].c_str() );
	}

	/* Chatter */
	chatter();
}

string svc_analyser::cube( triplet& key, float fRatio, float fMaxRatio )
{
	string s = 	"POLYGONS([[a,b,c],[a+u,b,c],[a+u,b+v,c],[a,b+v,c]],\n"
				"[[a,b,c],[a,b+v,c],[a,b+v,c+w],[a,b,c+w]],\n"
				"[[a+u,b,c],[a+u,b+v,c],[a+u,b+v,c+w],[a+u,b,c+w]],\n"
				"[[a,b,c],[a+u,b,c],[a+u,b,c+w],[a,b,c+w]],\n"
				"[[a,b+v,c],[a+u,b+v,c],[a+u,b+v,c+w],[a,b+v,c+w]],\n"
				"[[a,b,c+w],[a+u,b,c+w],[a+u,b+v,c+w],[a,b+v,c+w]],COLOR(HUE,e)),\n";

	replace( s, 'a', key.T );
	replace( s, 'b', key.Q );
	replace( s, 'c', key.D );

	int iDim           = mInt[ "dimension"];
	float fSqrRatio    = powf( fRatio, 1.0 / iDim );
	float fSqrMaxRatio = powf( fMaxRatio, 1.0 / iDim );

	if( iDim >= 3 )
		replace( s, 'v', fSqrRatio / fSqrMaxRatio * 0.70 );
	else
		replace( s, 'v', 0.70 );
	if( iDim >= 2 )
		replace( s, 'u', fSqrRatio / fSqrMaxRatio * 0.70 );
	else
		replace( s, 'u', 0.70 );
	if( iDim >= 1 )
		replace( s, 'w', fSqrRatio / fSqrMaxRatio * 0.70 );
	else
		RuntimeError( this, "invalid dimension" );

	replace( s, 'e', 0.1 + 0.9 * fRatio / fMaxRatio );
	return s;
}

void svc_analyser::replace( string& s, char key, float fval )
{
	char sval [256];
	snprintf( sval, sizeof(sval), "%.4f", fval );
	size_t i = s.find( key, 0 );
	while( i != string::npos )
	{
		s.replace( i, 1, sval );
		i = s.find( key, i );
	}
}
