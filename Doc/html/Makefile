# Convenience Makefile for building HTML documentation.  You probably need to
# set TEXINPUTS from the command line for this to be useful, unless you
# actually build the .dvi files in the top level directory.
#
# Note that the .dvi files must already be built and TEXINPUTS must include the
# directory where latex's working files (esp. *.aux) are kept.

TOPDIR=..

TEXINPUTS=$(TOPDIR):$(TOPDIR)/texinputs:


SUBMAKE=$(MAKE) srcdir=$(TOPDIR) VPATH=$(TOPDIR) TEXINPUTS=$(TEXINPUTS) \
	-f ../Makefile


all:	icons l2h

l2h:	l2hapi l2hext l2hlib l2hmac l2href l2htut

l2hapi:	icons
	$(SUBMAKE) l2hapi

l2hext:	icons
	$(SUBMAKE) l2hext

l2hlib:	icons
	$(SUBMAKE) l2hlib

l2hmac:	icons
	$(SUBMAKE) l2hmac

l2href:	icons
	$(SUBMAKE) l2href

l2htut:	icons
	$(SUBMAKE) l2htut

icons:
	mkdir icons
	cp $(TOPDIR)/icons/*.gif icons/
