INCLUDES = -I../crcanvas $(CR_CFLAGS)

noinst_PROGRAMS = viewdemo itemdemo customitems inverse blit

viewdemo_SOURCES = \
		   view-demo.c

viewdemo_LDADD = $(CR_LIBS) ../crcanvas/libcrcanvas.la

itemdemo_SOURCES = item-demo.c

itemdemo_LDADD = $(CR_LIBS) ../crcanvas/libcrcanvas.la

customitems_SOURCES = custom-items.c

customitems_LDADD = $(CR_LIBS) ../crcanvas/libcrcanvas.la

inverse_SOURCES = inverse.c
inverse_LDADD = $(CR_LIBS) ../crcanvas/libcrcanvas.la

blit_SOURCES = blit.c
blit_LDADD = $(CR_LIBS) ../crcanvas/libcrcanvas.la

EXTRA_DIST = bug.png
