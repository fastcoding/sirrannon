#ifdef WITH_LIBSDL
#include "YUVDisplay.h"
#include "ffmpeg.h"
#include "Frame.h"

REGISTER_CLASS(YUVDisplay, "YUV-display" );

/**
 * @component YUV-display
 * @type miscellaneous
 * @properties beta
 * @param width, int, -1, in pixels, if defined, the width of the video display, otherwise obtain the information from the MediaPackets containing the YUV frames (pPckt->desc->width)
 * @param height, int, -1, in pixels, if defined, the width of the video display, otherwise obtain the information from the MediaPackets containing the YUV frames (pPckt->desc->height)
 * @param full-screen, bool, false, if true, display the video full screen
 * @info Creates a rudimentary video player with no user controls save 'ESC' (which forcibly terminates the video). This component displays immediately any YUV frame it receives. Sirannon
 * must be compiled with the option --with-libSDL to use this component.
 */

YUVDisplay::YUVDisplay( const string& sName, ProcessorManager* pScope )
	: MediaProcessor(sName, pScope), pWindow(NULL), pVideo(NULL),
	  iWidth(-1), iHeight(-1), bInitSDL(false), iScreenWidth(-1), iScreenHeight(-1)
{ }

YUVDisplay::~YUVDisplay()
{
	if( pVideo )
		SDL_FreeYUVOverlay( pVideo );
	if( pWindow )
		SDL_FreeSurface( pWindow );
	SDL_Quit();
}

void YUVDisplay::init()
{
	MediaProcessor::init();
	if( mInt["width"] > 0 and mInt["height"] > 0 )
		initSDL( mInt["width"], mInt["height"] );
}

void YUVDisplay::initSDL( int iWidth, int iHeight )
{
	/* Once we know the dimensions we can create the display */
	debug( 1, "creating display for video (%dx%d)", iWidth, iHeight );
	this->iWidth = iWidth;
	this->iHeight = iHeight;
	if( iWidth <= 0 or iHeight <= 0 )
		RuntimeError( this, "Illegal picture dimensions" );

	if( SDL_Init( SDL_INIT_EVERYTHING ) < 0 )
		RuntimeError( this, "Could not initialize SDL" );

	if( mBool["full-screen"] )
		pWindow = SDL_SetVideoMode( 0, 0, 32, SDL_FULLSCREEN | SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_ANYFORMAT );
	else
		pWindow = SDL_SetVideoMode( iWidth, iHeight, 32, SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_ANYFORMAT );
	if( not pWindow )
		RuntimeError( this, "Could not create main window" );

	pVideo = SDL_CreateYUVOverlay( iWidth, iHeight, SDL_IYUV_OVERLAY, pWindow );
	if( not pVideo )
		RuntimeError( this, "Could not create video surface" );

	/* Stretch to screen dimensions */
	oClipping.x = 0;
	oClipping.y = 0;
	if( mBool["full-screen"] )
	{
		const SDL_VideoInfo* pScreenInfo = SDL_GetVideoInfo();
		oClipping.w = pScreenInfo->current_w;
		oClipping.h = pScreenInfo->current_h;
	}
	else
	{
		oClipping.w = iWidth;
		oClipping.h = iHeight;
	}
	debug( 1, "Spawning a window of size(%dx%d)", oClipping.w, oClipping.h );

	/* Activate the component */
	bInitSDL = true;
	bSchedule = true;
}

void YUVDisplay::receive( MediaPacketPtr& pPckt )
{
	if( not pPckt->codec == codec_t::yuv )
		ValueError( "Expected uncompressed YUV, received (%s)", CodecToString(pPckt->codec) );
	if( not bInitSDL )
		initSDL( pPckt->desc->width, pPckt->desc->height );

	/* Use the ffmpeg convenience function */
	AVPicture oPicture;
	int iSize = avpicture_fill( &oPicture, pPckt->data(), PIX_FMT_YUV420P, iWidth, iHeight );
	debug( 2, "displaying a new frame: %d %d (%dx%d)", pPckt->size(), iSize, iWidth, iHeight );

	/* Copy to the library */
	SDL_LockYUVOverlay( pVideo );
	memcpy( pVideo->pixels[0], oPicture.data[0], oPicture.linesize[0] * iHeight );
	memcpy( pVideo->pixels[1], oPicture.data[1], oPicture.linesize[1] * iHeight / 2 );
	memcpy( pVideo->pixels[2], oPicture.data[2], oPicture.linesize[2] * iHeight / 2 );
	SDL_UnlockYUVOverlay( pVideo );

	/* Normal */
	route( pPckt );
}

void YUVDisplay::process( void )
{
	/* Event handling */
	SDL_Event oEvent;
	while( SDL_PollEvent( &oEvent ) )
		onEvent( &oEvent );

	/* Display the video */
	SDL_DisplayYUVOverlay( pVideo, &oClipping );
}

void YUVDisplay::onEvent( SDL_Event* pEvent )
{
	switch( pEvent->type )
	{
		case SDL_QUIT:
			RuntimeError( "SDL window closed" );

		case SDL_KEYDOWN:
			switch( pEvent->key.keysym.sym )
			{
		        case SDLK_ESCAPE:
		        	RuntimeError( "ESC requested" );
		        default:
		        	return;
		     }
			break;
	}
}
#endif
