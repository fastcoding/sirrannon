#ifndef NETWORK_H_
#define NETWORK_H_

#ifdef WIN32
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <windows.h>
	typedef int socklen_t;
	typedef char raw_type;       // Type used for raw data on this platform
	#ifndef SHUT_RDWR
		#define SHUT_RDWR SD_BOTH
	#endif
#else
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netdb.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <errno.h>
	#include <netinet/tcp.h>
	#include <unistd.h>
	typedef void raw_type;       // Type used for raw data on this platform
#endif

#endif /* NETWORK_H_ */
