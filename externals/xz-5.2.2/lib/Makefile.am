##
## Copyright (C) 2004-2007 Free Software Foundation, Inc.
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##

## Not using gnulib-tool, at least for now. It is likely that we won't
## need anything else from Gnulib than getopt_long().

noinst_LIBRARIES = libgnu.a

libgnu_a_SOURCES =
libgnu_a_DEPENDENCIES = $(LIBOBJS)
libgnu_a_LIBADD = $(LIBOBJS)

EXTRA_DIST = getopt.in.h getopt.c getopt1.c getopt_int.h
BUILT_SOURCES = $(GETOPT_H)
MOSTLYCLEANFILES = getopt.h getopt.h-t

getopt.h: getopt.in.h
	{ echo '/* DO NOT EDIT! GENERATED AUTOMATICALLY! */'; \
	  cat $(srcdir)/getopt.in.h; \
	} > $@-t
	mv -f $@-t $@
