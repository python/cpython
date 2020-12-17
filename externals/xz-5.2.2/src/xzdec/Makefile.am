##
## Author: Lasse Collin
##
## This file has been put into the public domain.
## You can do whatever you want with this file.
##

# Windows resource compiler support. It's fine to use xz_CPPFLAGS
# also for lzmadec.
.rc.o:
	$(RC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) \
		$(xzdec_CPPFLAGS) $(CPPFLAGS) $(RCFLAGS) -i $< -o $@


xzdec_SOURCES = \
	xzdec.c \
	../common/tuklib_progname.c \
	../common/tuklib_exit.c

if COND_W32
xzdec_SOURCES += xzdec_w32res.rc
endif

xzdec_CPPFLAGS = \
	-DTUKLIB_GETTEXT=0 \
	-I$(top_srcdir)/src/common \
	-I$(top_srcdir)/src/liblzma/api \
	-I$(top_builddir)/lib
xzdec_LDADD = $(top_builddir)/src/liblzma/liblzma.la

if COND_GNULIB
xzdec_LDADD += $(top_builddir)/lib/libgnu.a
endif

xzdec_LDADD += $(LTLIBINTL)


lzmadec_SOURCES = \
	xzdec.c \
	../common/tuklib_progname.c \
	../common/tuklib_exit.c

if COND_W32
lzmadec_SOURCES += lzmadec_w32res.rc
endif

lzmadec_CPPFLAGS = $(xzdec_CPPFLAGS) -DLZMADEC
lzmadec_LDFLAGS = $(xzdec_LDFLAGS)
lzmadec_LDADD = $(xzdec_LDADD)


bin_PROGRAMS =
lzmadecmanlink =

if COND_XZDEC
bin_PROGRAMS += xzdec
dist_man_MANS = xzdec.1
endif

if COND_LZMADEC
bin_PROGRAMS += lzmadec

# Create the symlink lzmadec.1->xzdec.1 only if xzdec.1 was installed.
# This is better than creating a dangling symlink. The correct solution
# would be to install xzdec.1 as lzmadec.1 but this code is already too
# complicated so I won't do it. Installing only lzmadec is a bit unusual
# situation anyway so it's not that important.
if COND_XZDEC
lzmadecmanlink += lzmadec
endif
endif

if COND_XZDEC
# The installation of translated man pages abuses Automake internals
# by calling "install-man" with redefined dist_man_MANS and man_MANS.
# If this breaks some day, don't blame Automake developers.
install-data-hook:
	languages= ; \
	if test "$(USE_NLS)" = yes && test -d "$(top_srcdir)/po4a/man"; then \
		languages=`ls "$(top_srcdir)/po4a/man"`; \
	fi; \
	target=`echo xzdec | sed '$(transform)'` && \
	link=`echo lzmadec | sed '$(transform)'` && \
	for lang in . $$languages; do \
		man="$(top_srcdir)/po4a/man/$$lang/xzdec.1" ; \
		if test -f "$$man"; then \
			$(MAKE) dist_man_MANS="$$man" man_MANS= \
				mandir="$(mandir)/$$lang" install-man; \
		fi; \
		man1dir="$(DESTDIR)$(mandir)/$$lang/man1" && \
		if test -f "$$man1dir/$$target.1"; then \
			if test -n "$(lzmadecmanlink)"; then ( \
				cd "$$man1dir" && \
				rm -f "$$link.1" && \
				$(LN_S) "$$target.1" "$$link.1" \
			); fi; \
		fi; \
	done

uninstall-hook:
	languages= ; \
	if test "$(USE_NLS)" = yes && test -d "$(top_srcdir)/po4a/man"; then \
		languages=`ls "$(top_srcdir)/po4a/man"`; \
	fi; \
	for lang in . $$languages; do \
		for name in xzdec $(lzmadecmanlink); do \
			name=`echo $$name | sed '$(transform)'` && \
			rm -f "$(DESTDIR)$(mandir)/$$lang/man1/$$name.1"; \
		done; \
	done
endif
