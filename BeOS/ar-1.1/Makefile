######################################################################
# Makefile for ar
#
# Dec. 14, 1997 Chris Herborth (chrish@kagi.com)
#
# $Id$
######################################################################

AR_VERSION=1.1

# Make variables
CC=mwcc
LD=mwcc

CFLAGS=-w9 -rostr -O3 -g
CFLAGS_O=-w9 -rostr -O7 -opt schedule604
LDFLAGS=-g -map ar.xMAP
LDFLAGS_O=

INSTALL=install -m 755

DESTINATION=/boot/home/config/bin

PARTS=main.o mwlib.o commands.o copy_attrs.o

all: ar

nodebug:
	-rm -f ar $(PARTS) ar.dbg ar.xSYM
	$(MAKE) CFLAGS="$(CFLAGS_O) -DNO_DEBUG" LDFLAGS="$(LDFLAGS_O)" ar

ar: $(PARTS)
	$(LD) $(LDFLAGS) -o $@ $(PARTS)

install: ar
	$(INSTALL) ar $(DESTINATION)
	ln -sf $(DESTINATION)/ar $(DESTINATION)/ar-posix

clean:
	-rm -f $(PARTS) ar ar.dbg ar.xSYM

zip:
	(cd .. ; zip -9ry ar-$(AR_VERSION).zip ar-$(AR_VERSION) \
		-x ar-$(AR_VERSION)/RCS -x ar-$(AR_VERSION)/docs/RCS \
		-x ar-$(AR_VERSION)/RCS/\* -x ar-$(AR_VERSION)/docs/RCS/\*)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
