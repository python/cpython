# Makefile for embedded Python use demo.
# (This version tailored for CNRI and Solaris; edit lines marked with XXX.)

# XXX The compiler you are using
CC=	 	gcc

# XXX Top of the build tree and source tree
blddir=		../..
srcdir=		../..

# Python version
VERSION=	1.5

# Compiler flags
OPT=		-g
INCLUDES=	-I$(srcdir)/Include -I$(blddir)
CFLAGS=		$(OPT) $(INCLUDES)

# The Python library
LIBPYTHON=		$(blddir)/libpython$(VERSION).a

# XXX edit LIBS (in particular) to match $(blddir)/Modules/Makefile
LIBS=		-lsocket -lnsl -ldl  -lreadline -ltermcap
SYSLIBS=	-lm
MODLIBS=	
ALLLIBS=	$(LIBPYTHON) $(MODLIBS) $(LIBS) $(SYSLIBS)

# Build the demo application
all:		demo
demo:		demo.o
		$(CC) demo.o $(ALLLIBS) -o demo

# Administrative targets

test:		demo
		./demo

clean:
		-rm -f *.o core

clobber:	clean
		-rm -f *~ @* '#'* demo
