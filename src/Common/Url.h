#ifndef URL_H_
#define URL_H_
#include "sirannon.h"
#include "MediaPacket.h"

void URL_canonize( string& sUrl, const char* sProtocol );

void URL_parse( const string& sUrl, string& sProtocol, string& sServer, int& iPort, string& sApp, string& sMedia );

void URL_parse( const string& sUrl, string& sProtocol, string& sServer, int& iPort, string& sMedia );

void URL_parse( const string& sUrl,
		string& sProtocol, string& sServer, int& iPort, string& sApp, container_t& oFormat, target_t::type& iTarget, string& sMedia );

void URL_parse( const string& sUrl, string& sApp, container_t& oFormat, target_t::type& iTarget, string& sMedia );

void URL_construct( string& sUrl,
		const char* sProtocol, const char* sServer, int iPort, const char* sApp, const char* sMedia );

void URL_construct( string& sUrl,
		const char* sProtocol, const char* sServer, int iPort, const char* sMedia );

void URL_walk( const string& sUrl, container_t& oFormat, target_t::type& iTarget );

void URL_options( const string& sUrl, string& sMedia, map<string,string>& mOptions );

#endif /* URL_H_ */
