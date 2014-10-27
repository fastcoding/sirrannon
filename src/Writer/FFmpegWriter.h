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
#ifndef FFMEG_MUX_H_
#define FFMEG_MUX_H_
#include "Multiplexer/FFmpegMultiplexer.h"
#define SIRANNON_USE_BOOST_FILESYSTEM
#include "Boost.h"

class FFmpegWriter : public FFmpegMultiplexer
{
public:
	FFmpegWriter( const string& sName, ProcessorManager* pScope );

	void init( void );

protected:
	void setFormat( void );
	void openBuffer( void );

	int iVersion;
	filesystem::path oVersionedFilename;
};

#endif /*FFMEG_WRITER_H_*/
