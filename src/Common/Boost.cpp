#define SIRANNON_USE_BOOST_THREAD
#define SIRANNON_USE_BOOST_FILESYSTEM
#include "Boost.h"
#include <string.h>

#ifdef WIN32
void tss_cleanup_implemented( void )
{ }
#endif

string thread_get_string( void )
{
	stringstream s;
	s << this_thread::get_id();
	return s.str();
}

string thread_get_string( thread::id i )
{
	stringstream s;
	s << i;
	return s.str();
}

filesystem::path createVersionTemplate( const char* sFileName, const char* sExtension )
{
	const filesystem::path oOriginal( sFileName );
	filesystem::path oNew( oOriginal.branch_path() );
	string sLeaf( oOriginal.stem().c_str() );
	if( sExtension and strlen(sExtension) )
	{
		sLeaf.append( "-" );
		sLeaf.append( sExtension );
	}
	sLeaf.append( "-%d" );
	sLeaf.append( oOriginal.extension().c_str() );
	oNew /= sLeaf;
	return oNew;
}
