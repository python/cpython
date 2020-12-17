##
## Author: Lasse Collin
##
## This file has been put into the public domain.
## You can do whatever you want with this file.
##

SUBDIRS = liblzma xzdec

if COND_XZ
SUBDIRS += xz
endif

if COND_LZMAINFO
SUBDIRS += lzmainfo
endif

if COND_SCRIPTS
SUBDIRS += scripts
endif

EXTRA_DIST = \
	common/common_w32res.rc \
	common/mythread.h \
	common/sysdefs.h \
	common/tuklib_common.h \
	common/tuklib_config.h \
	common/tuklib_cpucores.c \
	common/tuklib_cpucores.h \
	common/tuklib_exit.c \
	common/tuklib_exit.h \
	common/tuklib_gettext.h \
	common/tuklib_integer.h \
	common/tuklib_mbstr_fw.c \
	common/tuklib_mbstr.h \
	common/tuklib_mbstr_width.c \
	common/tuklib_open_stdxxx.c \
	common/tuklib_open_stdxxx.h \
	common/tuklib_physmem.c \
	common/tuklib_physmem.h \
	common/tuklib_progname.c \
	common/tuklib_progname.h
