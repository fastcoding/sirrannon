#include "FileManager.h"

FileManager oFileManager;

bool FileManager::test_and_set_lock( const string& sFile )
{
	Lock_t oLock( oMutex );
	if( vFiles.find( sFile ) != vFiles.end() )
	{
		return false;
	}
	else
	{
		vFiles.insert( sFile );
		return true;
	}
}

bool FileManager::is_locked( const string& sFile )
{
	Lock_t oLock( oMutex );
	return vFiles.find( sFile ) != vFiles.end();
}

void FileManager::unlock( const string& sFile )
{
	Lock_t oLock( oMutex );
	vFiles.erase( sFile );
}

void FileManager::touch( const string& sFile ) const
{
	char sCmd [sFile.length() + 32];
	snprintf( sCmd, sizeof(sCmd), "touch %s", sFile.c_str() );
	system( sCmd );
}
