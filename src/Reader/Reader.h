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
#ifndef READER_H
#define READER_H
#include "sirannon.h"
#include "Interfaces.h"
#include "Scheduler/Scheduler.h"

class Reader : public MediaProcessor, public SourceInterface
{
public:
	virtual void findSchedulers( void );
	virtual void init( void );

protected:
	Reader( const string& sName, ProcessorManager* pScope );
	virtual ~Reader();

	/* File operations */
	virtual void openBuffer( void );
	virtual void closeBuffer( void );
	virtual void process( void );
	virtual bool doStep( void ) = 0;

	/* Buffer handling */
	bool checkBufferFull( void );

	/* Commands */
	virtual int seek( uint32_t iTimeIndex ) { return SirannonWarning( this,  "seek not supported"); }
	virtual bool ready( void ) const { return true; }

	FILE* oFile;
	vector<RateController*> vSchedulers;
	int iLoop;
	SirannonTime oStepStart;
};

#endif /* READER_H */
