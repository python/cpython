# Convenience Makefile for building HTML documentation.  You probably need to
# set TEXINPUTS from the command line for this to be useful, unless you
# actually build the .dvi files in the top level directory.
#
# Note that the .dvi files must already be built and TEXINPUTS must include the
# directory where latex's working files (esp. *.aux) are kept.

TEXINPUTS=..:../texinputs

all:	icons l2h

l2h:	l2hapi l2hext l2hlib l2hmac l2href l2htut

l2hapi:
	$(MAKE) srcdir=.. VPATH=.. TEXINPUTS=$(TEXINPUTS) -f ../Makefile l2hapi

l2hext:
	$(MAKE) srcdir=.. VPATH=.. TEXINPUTS=$(TEXINPUTS) -f ../Makefile l2hext

l2hlib:
	$(MAKE) srcdir=.. VPATH=.. TEXINPUTS=$(TEXINPUTS) -f ../Makefile l2hlib

l2hmac:
	$(MAKE) srcdir=.. VPATH=.. TEXINPUTS=$(TEXINPUTS) -f ../Makefile l2hmac

l2href:
	$(MAKE) srcdir=.. VPATH=.. TEXINPUTS=$(TEXINPUTS) -f ../Makefile l2href

l2htut:
	$(MAKE) srcdir=.. VPATH=.. TEXINPUTS=$(TEXINPUTS) -f ../Makefile l2htut

icons:
	mkdir icons
	cp ../icons/*.gif icons/
