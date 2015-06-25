
thisdir         = WML

#   Architecture-independent modules
mp4mshareddir = $(pkgdatadir)/$(thisdir)

#   Architecture-dependent modules
mp4mdir = $(pkglibdir)/@VERSION@/$(thisdir)

manext          = 3
SRCFILES        = \
   href.src isolatin.src javascript.src \
   preload.src rollover.src url.src

if DO_MODULE_WML
mp4mshared_DATA = $(SRCFILES:.src=.mp4hp)
man3_MANS       = \
   $(thisdir)href.$(manext) $(thisdir)isolatin.$(manext) \
   $(thisdir)javascript.$(manext) $(thisdir)preload.$(manext) \
   $(thisdir)rollover.$(manext) $(thisdir)url.$(manext)
endif

EXTRA_DIST      = $(SRCFILES)
CLEANFILES = $(mp4mshared_DATA) $(man3_MANS)

SUFFIXES = .src .mp4hp
.src.mp4hp:
	sed -e '/^=head1/,/^=cut/d' -e '/^##EOF##/,$$d' $< > $@
	cat $< > $(thisdir):$*.pod && pod2man --section=$(manext) \
   --center="mp4h modules" \
   --release="mp4h modules" \
   $(thisdir):$*.pod > $(thisdir)$*.$(manext) && rm -f $(thisdir):$*.pod

$(man3_MANS): $(mp4mshared_DATA)

install-data-hook:
	list='$(man3_MANS)'; for f in $${list}; do \
      mv $(DESTDIR)$(man3dir)/$$f $(DESTDIR)$(man3dir)/`echo $$f | sed -e 's/^$(thisdir)/$(thisdir):/'`; \
    done

