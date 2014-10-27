INCLUDES = $(CR_CFLAGS) -I$(top_builddir)/crcanvas

LDADD = $(CR_LIBS) ../crcanvas/libcrcanvas.la

UNIT_TESTS = \
	test-item \
	test-group \
	test-canvas

EXTRA_DIST = unit-test.h

# For use with 'make check'
TESTS = $(UNIT_TESTS)

noinst_PROGRAMS = $(UNIT_TESTS)

TESTS_ENVIRONMENT = function RUNTST { $$1; RC="$$?"; if test $$RC -ne 0; then echo "test: Test failed: $$1"; exit; fi; }; RUNTST

