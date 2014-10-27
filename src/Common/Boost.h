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
#ifdef SIRANNON_USE_BOOST_THREAD
#ifndef SIRANNON_USING_BOOST_THREAD
#define SIRANNON_USING_BOOST_THREAD

#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#endif

#ifdef WIN32
#define BOOST_USE_WINDOWS_H
#define BOOST_THREAD_USE_LIB
#define BOOST_THREAD_PLATFORM_WIN32
extern "C" void tss_cleanup_implemented();
#else
#define BOOST_THREAD_POSIX
#endif
#include <string>
#include <sstream>
using namespace std;
#include <boost/thread.hpp>
using namespace boost;
typedef lock_guard<mutex> Lock_t;
typedef unique_lock<mutex> LockUnique_t;
typedef lock_guard<recursive_mutex> FlowLock_t;
string thread_get_string( void );
string thread_get_string( thread::id i );
#endif
#endif

#ifdef SIRANNON_USE_BOOST_TOKENIZER
#ifndef SIRANNON_USING_BOOST_TOKENIZER
#define SIRANNON_USING_BOOST_TOKENIZER
#include <boost/tokenizer.hpp>
using namespace boost;
typedef tokenizer<char_separator<char> > Tokenizer_t;
typedef char_separator<char> Delim_t;
#endif
#endif

#ifdef SIRANNON_USE_BOOST_REGEX
#ifndef SIRANNON_USING_BOOST_REGEX
#define SIRANNON_USING_BOOST_REGEX
#include <boost/regex.hpp>
using namespace boost;
#endif
#endif

#ifdef SIRANNON_USE_BOOST_FILESYSTEM
#ifndef SIRANNON_USING_BOOST_FILESYSTEM
#define SIRANNON_USING_BOOST_FILESYSTEM
#define BOOST_FILESYSTEM_VERSION 2
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <string>
using namespace boost;

filesystem::path createVersionTemplate( const char* sFileName, const char* sExtension=NULL );
#endif
#endif

#ifdef SIRANNON_USE_BOOST_FUNCTION
#ifndef SIRANNON_USING_BOOST_FUNCTION
#define SIRANNON_USING_BOOST_FUNCTION
#include <boost/function.hpp>
using namespace boost;
#endif
#endif

#ifdef SIRANNON_USE_BOOST_BIND
#ifndef SIRANNON_USING_BOOST_BIND
#define SIRANNON_USING_BOOST_BIND
#include <boost/bind.hpp>
using namespace boost;
#endif
#endif

#ifdef SIRANNON_USE_BOOST_TIME
#ifndef SIRANNON_USING_BOOST_TIME
#define SIRANNON_USING_BOOST_TIME
#include <boost/date_time.hpp>
using namespace boost;
#endif
#endif
