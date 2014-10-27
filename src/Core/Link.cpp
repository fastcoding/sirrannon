#include "Link.h"

/** DUMMY
 * @component dummy
 * @type system
 * @info This component does absolutely nothing!
 **/

/** IN
 * @component in
 * @type system
 * @param url, string, , if defined, create a route from the component "out" which uses this url
 * @info This component finds a component a runtime and creates a route from that component to this.
 * Using this technique a live captured stream can be tapped into and routed to this component.
 **/

/** OUT
 * @component out
 * @type system
 * @param url, string, , if defined, links this component with this url, when a new component "in" is made
 * with this url, this component will route to that new component, this process can be repeated multiple times.
 * @info This component declares a url to be associated with this component and routes to components subscribing
 * to this url
 **/
REGISTER_CLASSES( InLink, "in", 1 );
REGISTER_CLASSES( OutLink, "out", 1 );
REGISTER_CLASSES( OutLink, "dummy", 2 );

static mutex oMutex;
typedef map<string, MediaProcessor*> dict_t;
static dict_t mDict;

InLink::InLink( const string& sName, ProcessorManager* pScope )
	: MediaProcessor(sName, pScope), bUrl(false)
{
	mString["url"] = "";
}

InLink::~InLink()
{
	clearLink();
}

void InLink::clearLink( void )
{
	if( bUrl )
	{
		/* Find the linked component */
		Lock_t oLock( oMutex );
		const string& sUrl = mString["url"];
		bUrl = false;
		if( mDict.count( sUrl ) == 0 )
			return;
		MediaProcessor* pOut = mDict[sUrl];

		/* Clear the link */
		debug( 1, "clearing route from(%s) to(%s)", pOut->c_str(), c_str() );
		pOut->clearRoute( this );
	}
}

void InLink::init( void )
{
	MediaProcessor::init();

	const string& sUrl = mString["url"];
	bUrl = sUrl.length();
	if( bUrl )
	{
		/* Find the linked component */
		Lock_t oLock( oMutex );
		if( mDict.count( sUrl ) == 0 )
			RuntimeError( "No component linked with url(%s)", sUrl.c_str() );
		MediaProcessor* pOut = mDict[sUrl];

		/* Set the link */
		pOut->setRoute( 0, this );
	}
}

OutLink::OutLink( const string& sName, ProcessorManager* pScope )
	: MediaProcessor(sName, pScope)
{
	mString["url"] = "";
}

OutLink::~OutLink()
{
	/* Delete the mapping from the translation */
	const string& sUrl = mString["url"];
	if( sUrl.length() )
	{
		Lock_t oLock( oMutex );
		mDict.erase( sUrl );
	}
}

void OutLink::init( void )
{
	MediaProcessor::init();

	/* Log this component in the translation map */
	const string& sUrl = mString["url"];
	if( sUrl.length() )
	{
		Lock_t oLock( oMutex );
		mDict[sUrl] = this;
	}
}
