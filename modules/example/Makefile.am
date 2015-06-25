
INCLUDES = -I$(top_srcdir)/lib  -I$(top_srcdir)/src @INTLINCL@
LDFLAGS  = -no-undefined -module -avoid-version -rpath $(pkglibdir) @DLLDFLAGS@

if DO_MODULE_TEST
noinst_LTLIBRARIES = test.la
endif

test_la_SOURCES =  test.c
test_la_LDFLAGS = -module

EXTRA_DIST = test.mp4h  test.ok
