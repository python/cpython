

TEXINPUTS=.:../texinputs:

all:	all-ps

all-ps:
	$(MAKE) srcdir=.. VPATH=.. TEXINPUTS=$(TEXINPUTS) -f ../Makefile all-ps

all-pdf:
	$(MAKE) srcdir=.. VPATH=.. TEXINPUTS=$(TEXINPUTS) -f ../Makefile all-pdf

all-dvi:
	$(MAKE) srcdir=.. VPATH=.. TEXINPUTS=$(TEXINPUTS) -f ../Makefile all-dvi
