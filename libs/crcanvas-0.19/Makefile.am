DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc

if ENABLE_PYTHON
SUBDIRS = crcanvas unit-test examples doc cross-build python
else
SUBDIRS = crcanvas unit-test examples doc cross-build
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = crcanvas.pc

EXTRA_DIST = crcanvas.spec

# For creating RPM automatically
rpm: dist
	mkdir -p build/BUILD build/RPMS build/SRPMS build/SOURCES build/SPECS && \
	cp $(PACKAGE).spec build/SPECS/$(PACKAGE)-$(VERSION).spec
	cp $(PACKAGE)-$(VERSION).tar.gz build/SOURCES/. && \
	rpmbuild -ba build/SPECS/$(PACKAGE)-$(VERSION).spec && \
	mv build/RPMS/*/* . && \
	mv build/SRPMS/* . && \
	rm -rf build

