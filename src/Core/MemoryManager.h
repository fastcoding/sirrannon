#ifndef MEMORYMANAGER_H_
#define MEMORYMANAGER_H_
#include <stdint.h>

void mem_register( uint64_t iBytes );
void mem_unregister( uint64_t iBytes );
uint64_t mem_usage( void );
bool mem_full( uint64_t iLimit );

#endif /* MEMORYMANAGER_H_ */
