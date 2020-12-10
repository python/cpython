##
## Author: Lasse Collin
##
## This file has been put into the public domain.
## You can do whatever you want with this file.
##

bin_PROGRAMS = lzmainfo

lzmainfo_SOURCES = \
	lzmainfo.c \
	../common/tuklib_progname.c \
	../common/tuklib_exit.c

if COND_W32
lzmainfo_SOURCES += lzmainfo_w32res.rc
endif

lzmainfo_CPPFLAGS = \
	-DLOCALEDIR=\"$(localedir)\" \
	-I$(top_srcdir)/src/common \
	-I$(top_srcdir)/src/liblzma/api \
	-I$(top_builddir)/lib

lzmainfo_LDADD = $(top_builddir)/src/liblzma/liblzma.la

if COND_GNULIB
lzmainfo_LDADD += $(top_builddir)/lib/libgnu.a
endif

lzmainfo_LDADD += $(LTLIBINTL)


dist_man_MANS = lzmainfo.1

# Windows resource compiler support
.rc.o:
	$(RC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) \
		$(lzmainfo_CPPFLAGS) $(CPPFLAGS) $(RCFLAGS) -i $< -o $@
