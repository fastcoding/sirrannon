#include "MemoryManager.h"
#define SIRANNON_USE_BOOST_THREAD
#include "Boost.h"

mutex oMutex;

uint64_t mem;

void mem_register( uint64_t iBytes )
{
	Lock_t oLock( oMutex );
	mem += iBytes;
}

void mem_unregister( uint64_t iBytes )
{
	Lock_t oLock( oMutex );
	mem -= iBytes;
}

uint64_t mem_usage( void )
{
	Lock_t oLock( oMutex );
	return mem;
}

bool mem_full( uint64_t iLimit )
{
	Lock_t oLock( oMutex );
	return mem >= iLimit;
}
