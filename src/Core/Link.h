#ifndef LINK_H_
#define LINK_H_
#include "sirannon.h"
#include "Interfaces.h"
#define SIRANNON_USE_BOOST_THREAD
#include "Boost.h"
#include <map>

class InLink : public MediaProcessor
{
public:
	InLink( const string& sName, ProcessorManager* pScope );
	~InLink();

protected:
	virtual void init( void );
	void clearLink( void );
	bool bUrl;
};

class OutLink : public MediaProcessor
{
public:
	OutLink( const string& sName, ProcessorManager* pScope );
	~OutLink();

protected:
	virtual void init( void );
};

#endif /* LINK_H_ */
