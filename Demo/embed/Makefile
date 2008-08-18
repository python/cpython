# Makefile for embedded Python use demo.
# (This version tailored for my Red Hat Linux 6.1 setup;
# edit lines marked with XXX.)

# XXX The compiler you are using
CC=	 	gcc

# XXX Top of the build tree and source tree
blddir=		../..
srcdir=		../..

# Python version
VERSION=	2.6

# Compiler flags
OPT=		-g
INCLUDES=	-I$(srcdir)/Include -I$(blddir)
CFLAGS=		$(OPT)
CPPFLAGS=	$(INCLUDES)

# The Python library
LIBPYTHON=	$(blddir)/libpython$(VERSION).a

# XXX edit LIBS (in particular) to match $(blddir)/Modules/Makefile
LIBS=		-lnsl -ldl -lreadline -ltermcap -lieee -lpthread -lutil
LDFLAGS=	-Xlinker -export-dynamic
SYSLIBS=	-lm
MODLIBS=	
ALLLIBS=	$(LIBPYTHON) $(MODLIBS) $(LIBS) $(SYSLIBS)

# Build the demo applications
all:		demo loop importexc
demo:		demo.o
		$(CC) $(LDFLAGS) demo.o $(ALLLIBS) -o demo

loop:		loop.o
		$(CC) $(LDFLAGS) loop.o $(ALLLIBS) -o loop

importexc:	importexc.o
		$(CC) $(LDFLAGS) importexc.o $(ALLLIBS) -o importexc

# Administrative targets

test:		demo
		./demo

COMMAND="print 'hello world'"
looptest:	loop
		./loop $(COMMAND)

clean:
		-rm -f *.o core

clobber:	clean
		-rm -f *~ @* '#'* demo loop importexc

realclean:	clobber
