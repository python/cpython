##
## Author: Lasse Collin
##
## This file has been put into the public domain.
## You can do whatever you want with this file.
##

bin_PROGRAMS = xz

xz_SOURCES = \
	args.c \
	args.h \
	coder.c \
	coder.h \
	file_io.c \
	file_io.h \
	hardware.c \
	hardware.h \
	main.c \
	main.h \
	message.c \
	message.h \
	mytime.c \
	mytime.h \
	options.c \
	options.h \
	private.h \
	signals.c \
	signals.h \
	suffix.c \
	suffix.h \
	util.c \
	util.h \
	../common/tuklib_open_stdxxx.c \
	../common/tuklib_progname.c \
	../common/tuklib_exit.c \
	../common/tuklib_mbstr_width.c \
	../common/tuklib_mbstr_fw.c

if COND_MAIN_DECODER
xz_SOURCES += \
	list.c \
	list.h
endif

if COND_W32
xz_SOURCES += xz_w32res.rc
endif

xz_CPPFLAGS = \
	-DLOCALEDIR=\"$(localedir)\" \
	-I$(top_srcdir)/src/common \
	-I$(top_srcdir)/src/liblzma/api \
	-I$(top_builddir)/lib

xz_LDADD = $(top_builddir)/src/liblzma/liblzma.la $(CAPSICUM_LIB)

if COND_GNULIB
xz_LDADD += $(top_builddir)/lib/libgnu.a
endif

# libgnu.a may need these libs, so this must be after libgnu.a.
xz_LDADD += $(LTLIBINTL)


# Windows resource compiler support
.rc.o:
	$(RC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) \
		$(xz_CPPFLAGS) $(CPPFLAGS) $(RCFLAGS) -i $< -o $@


dist_man_MANS = xz.1


## Create symlinks for unxz and xzcat for convenience. Create symlinks also
## for lzma, unlzma, and lzcat for compatibility with LZMA Utils 4.32.x.
xzlinks = unxz xzcat

if COND_LZMALINKS
xzlinks += lzma unlzma lzcat
endif

install-exec-hook:
	cd "$(DESTDIR)$(bindir)" && \
	target=`echo xz | sed '$(transform)'`$(EXEEXT) && \
	for name in $(xzlinks); do \
		link=`echo $$name | sed '$(transform)'`$(LN_EXEEXT) && \
		rm -f "$$link" && \
		$(LN_S) "$$target" "$$link"; \
	done

# The installation of translated man pages abuses Automake internals
# by calling "install-man" with redefined dist_man_MANS and man_MANS.
# If this breaks some day, don't blame Automake developers.
install-data-hook:
	languages= ; \
	if test "$(USE_NLS)" = yes && test -d "$(top_srcdir)/po4a/man"; then \
		languages=`ls "$(top_srcdir)/po4a/man"`; \
	fi; \
	target=`echo xz | sed '$(transform)'` && \
	for lang in . $$languages; do \
		man="$(top_srcdir)/po4a/man/$$lang/xz.1" ; \
		if test -f "$$man"; then \
			$(MAKE) dist_man_MANS="$$man" man_MANS= \
				mandir="$(mandir)/$$lang" install-man; \
		fi; \
		man1dir="$(DESTDIR)$(mandir)/$$lang/man1" && \
		if test -f "$$man1dir/$$target.1"; then ( \
			cd "$$man1dir" && \
			for name in $(xzlinks); do \
				link=`echo $$name | sed '$(transform)'` && \
				rm -f "$$link.1" && \
				$(LN_S) "$$target.1" "$$link.1"; \
			done \
		); fi; \
	done

uninstall-hook:
	cd "$(DESTDIR)$(bindir)" && \
	for name in $(xzlinks); do \
		link=`echo $$name | sed '$(transform)'`$(LN_EXEEXT) && \
		rm -f "$$link"; \
	done
	languages= ; \
	if test "$(USE_NLS)" = yes && test -d "$(top_srcdir)/po4a/man"; then \
		languages=`ls "$(top_srcdir)/po4a/man"`; \
	fi; \
	for lang in . $$languages; do \
		for name in xz $(xzlinks); do \
			name=`echo $$name | sed '$(transform)'` && \
			rm -f "$(DESTDIR)$(mandir)/$$lang/man1/$$name.1"; \
		done; \
	done
