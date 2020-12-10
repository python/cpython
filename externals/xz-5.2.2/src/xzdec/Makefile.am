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

if COND_XZDEC
bin_PROGRAMS += xzdec
dist_man_MANS = xzdec.1
endif

if COND_LZMADEC
bin_PROGRAMS += lzmadec

# Create the symlink lzmadec.1->xzdec.1 only if xzdec.1 was installed.
# This is better than creating a dangling symlink, especially
# because creating the link may fail due to the directory being missing.
#
# FIXME: The correct solution would be to install xzdec.1 as lzmadec.1
# but I don't know what is the sane way to do it and since this is a bit
# unusual situation anyway, it's not that important.
if COND_XZDEC
install-data-hook:
	cd $(DESTDIR)$(mandir)/man1 && \
	target=`echo xzdec | sed '$(transform)'` && \
	link=`echo lzmadec | sed '$(transform)'` && \
	rm -f $$link.1 && \
	$(LN_S) $$target.1 $$link.1

uninstall-hook:
	cd $(DESTDIR)$(mandir)/man1 && \
	link=`echo lzmadec | sed '$(transform)'` && \
	rm -f $$link.1
endif
endif
