#ifndef FILEMANAGER_H_
#define FILEMANAGER_H_
#include "sirannon.h"
#define SIRANNON_USE_BOOST_THREAD
#include "Boost.h"
#include <set>

class FileManager
{
public:
	FileManager() { };
	virtual ~FileManager() { };

	bool test_and_set_lock( const string& sFile );
	bool is_locked( const string& sFile );
	void unlock( const string& sFile );

	void touch( const string& sFile ) const;

private:
	mutable mutex oMutex;
	set<string> vFiles;
};

extern FileManager oFileManager;

#endif /* FILEMANAGER_H_ */
