

TEXINPUTS=.:../texinputs:

all:	all-ps

all-ps:
	$(MAKE) srcdir=.. VPATH=.. TEXINPUTS=$(TEXINPUTS) \
		-f ../Makefile do-ps

all-pdf:
	$(MAKE) srcdir=.. VPATH=.. TEXINPUTS=$(TEXINPUTS) \
		-f ../Makefile do-pdf

all-dvi:
	$(MAKE) srcdir=.. VPATH=.. TEXINPUTS=$(TEXINPUTS) \
		-f ../Makefile do-dvi

clean:
	$(MAKE) -f ../Makefile clean

clobber:
	$(MAKE) -f ../Makefile clobber
