INCLUDES = $(CR_CFLAGS)

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

lib_LTLIBRARIES = libcrcanvas.la

libcrcanvasincludedir = $(includedir)/crcanvas
crcanvasdatadir = $(datadir)/crcanvas

libcrcanvas_la_SOURCES = \
			 cr-arrow.c \
			 cr-blit.c \
			 cr-canvas.c \
			 cr-ellipse.c \
			 cr-inverse.c \
			 cr-item.c \
			 cr-line.c \
			 cr-marshal.c \
			 cr-panner.c \
			 cr-path.c \
			 cr-pixbuf.c \
			 cr-rectangle.c \
			 cr-rotator.c \
			 cr-text.c \
			 cr-types.c \
			 cr-vector.c \
			 cr-zoomer.c


libcrcanvas_h_SOURCES = \
			 cr-arrow.h \
			 cr-blit.h \
			 cr-canvas.h \
			 cr-ellipse.h \
			 cr-inverse.h \
			 cr-item.h \
			 cr-line.h \
			 cr-panner.h \
			 cr-path.h \
			 cr-pixbuf.h \
			 cr-rectangle.h \
			 cr-rotator.h \
			 cr-text.h \
			 cr-types.h \
			 cr-vector.h \
			 cr-version.h \
			 cr-zoomer.h \
			 libcrcanvas.h

libcrcanvas_la_LDFLAGS = $(no_undefined)

libcrcanvasinclude_HEADERS = $(libcrcanvas_h_SOURCES) cr-marshal.h
libcrcanvas_la_LIBADD = $(CR_LIBS)

BUILT_SOURCES = cr-marshal.h cr-marshal.c

crcanvasdata_DATA = cr-marshal.list

EXTRA_DIST = $(crcanvasdata_DATA) $(BUILT_SOURCES) cr-version.h.in

cr-marshal.h: cr-marshal.list
	glib-genmarshal --prefix=cr_marshal cr-marshal.list --header >> xgen-gmh \
	&& (cmp -s xgen-gmh crmarshal.h || cp xgen-gmh cr-marshal.h) \
	&& rm -f xgen-gmh xgen-gmh~

cr-marshal.c: cr-marshal.list
	glib-genmarshal --prefix=cr_marshal cr-marshal.list --body >> xgen-gmc \
	&& cp xgen-gmc cr-marshal.c \
	&& rm -f xgen-gmc xgen-gmc~



