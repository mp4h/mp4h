
thisdir  = javascript

#   Architecture-dependent modules
mp4mdir = $(pkglibdir)/@VERSION@/$(thisdir)

INCLUDES = -I$(top_srcdir)/lib  -I$(top_srcdir)/src @INTLINCL@
LDFLAGS  = -no-undefined -module -avoid-version -rpath $(mp4mdir) @DLLDFLAGS@

if DO_MODULE_JAVASCRIPT_MOZJS
MODULE_JAVASCRIPT_MOZJS = mozjs.la
endif
if DO_MODULE_JAVASCRIPT_NJS
MODULE_JAVASCRIPT_NJS   = njs.la
endif

mp4m_LTLIBRARIES = $(MODULE_JAVASCRIPT_MOZJS) $(MODULE_JAVASCRIPT_NJS)

mozjs_la_SOURCES =  mozjs.c
mozjs_la_LDFLAGS = -module

njs_la_SOURCES =  njs.c mp4h-njs.c
njs_la_LDFLAGS = -module

noinst_HEADERS = mp4h-njs.h

EXTRA_DIST = mozjs.mp4h njs.mp4h mp4h-njs.jsi

