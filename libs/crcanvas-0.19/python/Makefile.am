INCLUDES = -I../$(PACKAGE) -I.. $(CR_CFLAGS) $(PYTHON_INCLUDES) $(PYGTK_CFLAGS)

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

PYGTK_DEFSDIR = @PYGTK_DEFSDIR@

defsdir = @PYGTK_DEFSDIR@
defs_DATA = \
	crcanvas.defs

EXTRA_DIST = $(defs_DATA) \
	     __init__.py \
	     mvc-demo.py \
	     custom-demo.py


pycrcanvasdir = $(pyexecdir)/crcanvas
pycrcanvasexecdir = $(pyexecdir)/crcanvas
pycrcanvas_PYTHON = __init__.py

common_ldflags = $(no_undefined) -module -avoid-version

pycrcanvasexec_LTLIBRARIES = \
			      crcanvas.la

crcanvas_la_LDFLAGS = $(common_ldflags) -export-symbols-regex initcrcanvas
crcanvas_la_LIBADD = ../crcanvas/libcrcanvas.la $(CR_LIBS)
crcanvas_la_SOURCES = crcanvasmodule.c
nodist_crcanvas_la_SOURCES = crcanvas.c
CLEANFILES = crcanvas.c
EXTRA_DIST += crcanvas.override  

crcanvas.c: crcanvas.override

.defs.c:
	(cd $(srcdir) \
	 && $(PYGTK_CODEGEN) \
	    --register $(PYGTK_DEFSDIR)/pango-types.defs \
	    --register $(PYGTK_DEFSDIR)/gdk-types.defs \
	    --register $(PYGTK_DEFSDIR)/gtk-types.defs \
	    --override $*.override \
	    --prefix py$* $*.defs) > gen-$*.c \
	&& cp gen-$*.c $*.c \
	&& rm -f gen-$*.c

all-local:
	test -L ./crcanvas.so || $(LN_S) .libs/crcanvas.so . 
