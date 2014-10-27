#ifndef YUVDISPLAY_H_
#define YUVDISPLAY_H_
#ifdef WITH_LIBSDL

#include "MediaProcessor.h"
#include <SDL/SDL.h>

class YUVDisplay: public MediaProcessor
{
public:
	YUVDisplay( const string& sName, ProcessorManager* pScope );
	virtual ~YUVDisplay();
	void init( void );

protected:
	void initSDL( int iWidth, int iHeight );
	void receive( MediaPacketPtr& pPckt );
	void process( void );
	void onEvent( SDL_Event* pEvent );

	SDL_Surface* pWindow;
	SDL_Overlay* pVideo;
	SDL_Rect oClipping;
	int iWidth, iHeight, iScreenWidth, iScreenHeight;
	bool bInitSDL;
};

#endif
#endif /* YUVDISPLAY_H_ */
