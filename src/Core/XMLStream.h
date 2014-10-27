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
#ifndef IXMLSTREAM_H
#define IXMLSTREAM_H
#define	XML_DEBUG false
#include "sirannon.h"
#define SIRANNON_USE_BOOST_TOKENIZER
#include "Boost.h"
#include "expat.h"

class XMLStream
{
public:
	XMLStream( const string& sFile, int argc, const char* argv [], ProcessorManager& );
	~XMLStream();

	static void startElementWrapper( void* userData, const XML_Char* name, const XML_Char** attr );
    static void endElementWrapper( void* userData, const XML_Char* name );

	int parse( void );
	uint32_t getErrors( void ) const;

protected:
	int valid;
	const int argc;
	const char** argv;
	string filename, sCurrent;
	ProcessorManager& oProcessorManager;
	XML_Parser oXML;
	tokenizer<> oTokenizer;

	void startElement( const XML_Char* name, const XML_Char** attr );
    void endElement( const XML_Char* name );
};

inline uint32_t XMLStream::getErrors( void ) const
{
	return valid;
}

#endif
