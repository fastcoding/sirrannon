## Preamble
AC_INIT(configure.ac)
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

## Automake
AM_INIT_AUTOMAKE(sirannon, 1.0.0)

## PKG config
# PKG_PROG_PKG_CONFIG()

## Libtool
AM_PROG_LIBTOOL

## Program type
AC_PROG_CXX
AC_PROG_CC

## Temporary libs and flags
SIRA_LIBS=
SIRA_FLAGS=
CXXFLAGS=

## Error functions
AC_DEFUN([SIRA_ERROR_LIB], [AC_MSG_ERROR(["could not find or build '$1' on your system and remove --enable-external-$1 to use the version included in Sirannon distribution"])] )
AC_DEFUN([SIRA_ERROR], [AC_MSG_ERROR(["could not find or build '$1' on your system"])] )

## Detect cygwin
AC_CANONICAL_HOST
case $host_os in
	*cygwin* ) SIRA_OS=cygwin;;
	*mingw* ) SIRA_OS=mingw;;
           * ) SIRA_OS=unix;;          
esac

## OS Specific libs
if test $SIRA_OS = mingw ; then
AC_CHECK_LIB( ws2_32, main, [], [SIRA_ERROR(ws2_32)] )
AC_CHECK_LIB( wsock32, main, [], [SIRA_ERROR(wsock32)] )
AC_CHECK_LIB( winmm, main, [], [SIRA_ERROR(winmm)] )
CXXFLAGS+=" -mthreads"
else
AC_CHECK_LIB( pthread, main, [], [SIRA_ERROR_LIB(pthread)] )
fi

## Basic libs
AC_CHECK_LIB( expat, XML_Parse, [], [SIRA_ERROR(expat)] )
					
## SVNVERSION
AC_CHECK_PROG( svnversionlin, svnversion, yes, no )
if test $svnversionlin = yes ;  then
	SVN_VERSION="\$(shell svnversion -n \$(top_srcdir) | sed 's/ /_/' )"
else
	AC_CHECK_PROG( svnversionwin, SubWCRev.exe, yes, no )
	if test $svnversionwin = yes ;  then
		## Dirty version for windows
		SVN_VERSION="\$(shell SubWCRev.exe \$(top_srcdir) | grep -E \"Last committed at revision\" | cut -c 28- | sed 's/ /_/')"
	else
		SVN_VERSION="???"
	fi
fi

## @<:@0-9@:>@+ quadrigraphs
			
## FFMPEG
if test $SIRA_OS = unix ; then
	AC_CHECK_LIB( pthread, pthread_create, [], [SIRA_ERROR(pthread)] )
	AC_CHECK_LIB( m, main, [], [SIRA_ERROR(m)] )
	AC_CHECK_LIB( z, main, [], [SIRA_ERROR(z)] )
	AC_CHECK_LIB( bz2, main, [], [SIRA_ERROR(bz2)] )
	AC_CHECK_LIB( x264, main, [], [SIRA_ERROR(x264)] )
	AC_CHECK_LIB( vpx, main, [], [SIRA_ERROR(vpx)] )
	AC_CHECK_LIB( vorbisenc, main, [], [SIRA_ERROR(vorbisenc)] )
	AC_CHECK_LIB( vorbis, main, [], [SIRA_ERROR(vorbis)] )
	AC_CHECK_LIB( theoradec, main, [], [SIRA_ERROR(theoradec)] )
	AC_CHECK_LIB( theoraenc, main, [], [SIRA_ERROR(theoraenc)] ) 
	AC_CHECK_LIB( opencore-amrwb, main, [], [SIRA_ERROR(opencore-amrwb)] )
	AC_CHECK_LIB( opencore-amrnb, main, [], [SIRA_ERROR(opencore-amrnb)] )		
	AC_CHECK_LIB( faac, main, [], [] )
	AC_CHECK_LIB( mp3lame, main, [], [] )
	FF_DIR=ffmpeg-0.9
	SIRA_LIBS="$SIRA_LIBS \$(top_srcdir)/libs/$FF_DIR/libavformat/libavformat.a \$(top_srcdir)/libs/$FF_DIR/libavcodec/libavcodec.a \$(top_srcdir)/libs/$FF_DIR/libavutil/libavutil.a \$(top_srcdir)/libs/$FF_DIR/libswscale/libswscale.a"
	SIRA_FLAGS="$SIRA_FLAGS -I\$(top_srcdir)/libs/$FF_DIR"
elif test $SIRA_OS = mingw ; then
	AC_CHECK_LIB( m, main, [], [SIRA_ERROR(m)] )
#	AC_CHECK_LIB( z, main, [], [SIRA_ERROR(z)] )
#	AC_CHECK_LIB( bz2, main, [], [SIRA_ERROR(bz2)] )
	AC_CHECK_LIB( x264, main, [], [] )
#	AC_CHECK_LIB( avi, main, [], [SIRA_ERROR(avi)] )
	AC_CHECK_LIB( avutil, main, [], [SIRA_ERROR(avutil)] )		
	AC_CHECK_LIB( swscale, main, [], [SIRA_ERROR(swscale)] )
	AC_CHECK_LIB( avcodec, main, [], [SIRA_ERROR(avcodec)] )
	AC_CHECK_LIB( avformat, main, [], [SIRA_ERROR(avformat)] )
else
	FF_DIR=ffmpeg-webM
	SIRA_LIBS="$SIRA_LIBS \$(top_srcdir)/libs/$FF_DIR/libavformat/libavformat.a \$(top_srcdir)/libs/$FF_DIR/libavcodec/libavcodec.a \$(top_srcdir)/libs/$FF_DIR/libavutil/libavutil.a \$(top_srcdir)/libs/$FF_DIR/libswscale/libswscale.a"
	SIRA_FLAGS="$SIRA_FLAGS -I\$(top_srcdir)/libs/$FF_DIR"
fi

## JRTPLIB
JRTP_DIR=jrtplib-3.9.1
if test $SIRA_OS = unix ; then	
	SIRA_LIBS="$SIRA_LIBS \$(top_srcdir)/libs/$JRTP_DIR/src/libjrtp.a"
	SIRA_FLAGS="$SIRA_FLAGS -I\$(top_srcdir)/libs/$JRTP_DIR/src"
elif test $SIRA_OS = mingw ; then
	AC_CHECK_LIB( jrtp, main, [], [SIRA_ERROR_LIB(jrtp)] )
else
	SIRA_LIBS="$SIRA_LIBS \$(top_srcdir)/libs/$JRTP_DIR/src/.libs/libjrtp.a"
	SIRA_FLAGS="$SIRA_FLAGS -I\$(top_srcdir)/libs/$JRTP_DIR/src"
fi

## BOOST
if test $SIRA_OS = mingw ; then
	AX_BOOST_BASE([1.40])
	AC_CHECK_LIB( boost_regex-mt, main, [], [SIRA_ERROR_LIB(boost_regex)] )
	AC_CHECK_LIB( boost_system-mt, main, [], [SIRA_ERROR_LIB(boost_system)] )
	AC_CHECK_LIB( boost_filesystem-mt, main, [], [SIRA_ERROR_LIB(boost_filesystem)] )
	AC_CHECK_LIB( boost_thread-mt, main, [], [SIRA_ERROR_LIB(boost_thread)] )
else
	AC_CHECK_LIB( rt, clock_gettime, [], [SIRA_ERROR(rt)] )
	AX_BOOST_BASE([1.40])
	AX_BOOST_SYSTEM
	AX_BOOST_FILESYSTEM
	AX_BOOST_REGEX
	AX_BOOST_THREAD
	LIBS="$BOOST_BASE_LIB $BOOST_REGEX_LIB $BOOST_THREAD_LIB $BOOST_FILESYSTEM_LIB $BOOST_SYSTEM_LIB $LIBS"
fi

## PCAP
AC_ARG_WITH(	libpcap,
				[AS_HELP_STRING([--with-libpcap], [Enable the component PCAP-writer])],
				[	AC_CHECK_LIB(pcap, pcap_open_live, SIRA_LIBS="-lpcap $SIRA_LIBS", [SIRA_ERROR(pcap)])
					AC_DEFINE(WITH_LIBPCAP) ]
			)

## SDL
AC_ARG_WITH(	libsdl,
				[AS_HELP_STRING([--with-libsdl], [Enable the component YUV-display])],
				[	AC_CHECK_LIB([asound], main, SIRA_LIBS="-lasound $SIRA_LIBS", [SIRA_ERROR(asound)])
					AC_CHECK_LIB([pulse], main, SIRA_LIBS="-lpulse-simple $SIRA_LIBS", [SIRA_ERROR(pulse)])
					AC_CHECK_LIB([aa], main, SIRA_LIBS="-laa $SIRA_LIBS", [SIRA_ERROR(aa)])
					AC_CHECK_LIB([caca], main, SIRA_LIBS="-lcaca $SIRA_LIBS", [SIRA_ERROR(caca)])
					AC_CHECK_LIB(SDLmain, main, SIRA_LIBS="-lSDLmain $SIRA_LIBS", [SIRA_ERROR(SDLmain)])
					AC_CHECK_LIB(SDL, main, SIRA_LIBS="-lSDL $SIRA_LIBS", [SIRA_ERROR(SDL)])
					AC_DEFINE(WITH_LIBSDL) ]
			)

## Local files
AC_ARG_ENABLE(	private,
				[  --enable-private Check for private files under src/Private, add those to src/Automake.am and call autoreconf],
				[	if test "$enableval" = yes ; then
						private="yes"
						$srcdir/python/private.py $srcdir
						autoreconf $srcdir
					else
						private="no"						
					fi ],
				[ private="no"  ] )				
AM_CONDITIONAL( [PRIVATE], [test "$private" = "yes" ] )

AC_ARG_ENABLE(	local,
				[  --enable-local Check for local files under src/Local, add those to src/Automake.am and call autoreconf],
				[	if test "$enableval" = yes ; then
						local="yes"
						$srcdir/python/local.py $srcdir
						autoreconf $srcdir
					else
						local="no"						
					fi ],
				[ local="no"  ] )				
AM_CONDITIONAL( [LOCAL], [test "$local" = "yes" ] )

AC_ARG_ENABLE(	library,
				[  --enable-library Create a library version of sirannon],
				[	if test "$enableval" = yes ; then
						library="yes"
					else
						library="no"						
					fi ],
				[ library="no"  ] )				
AM_CONDITIONAL( [BUILD_LIBRARY], [test "$library" = "yes" ] )

## Profiling
AC_ARG_ENABLE(	profiling,
				[  --enable-profiling        Enable profiling],
				[	if test "$enableval" = yes ; then
						CXXFLAGS+=" -pg"
					fi])
			
## Valgrind
AC_ARG_ENABLE(	debug,
				[  --enable-debug       Sets compiler appropriate for debugging],
				[	if test "$enableval" = yes ; then
						CXXFLAGS+=" -g -ggdb -O0 -fno-inline"
					else
						CXXFLAGS+=" -O3 -Os"						
					fi ],
				[ CXXFLAGS+=" -O3 -Os"  ] )
				
## Static
AC_ARG_ENABLE(	static-build,
				[  --enable-static-build          Enable static build],
				[	if test "$enableval" = yes ; then
						LDFLAGS+=" -all-static"
					fi],
				[])

XDLL_FLG="// No DLL loading"
LIBS="$SIRA_LIBS $LIBS"
CPPFLAGS+=" $SIRA_FLAGS"
	
dnl Output 
dnl ------------------------------------------------
AC_SUBST(SVN_VERSION)
AC_SUBST(XDLL_FLG)
AC_OUTPUT(Makefile src/Makefile)
