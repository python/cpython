# Makefile for embedded Python use demo

# Top of the build tree and source tree
blddir=		../..
srcdir=		../..

# Compiler flags
OPT=		-g
INCLUDES=	-I$(srcdir)/Include -I$(blddir)
DEFINES=	-DHAVE_CONFIG_H
CFLAGS=		$(OPT) $(DEFINES) $(INCLUDES)

# Libraries
# XXX edit MODLIBS, LIBS and SYSLIBS to match $(blddir)/Modules/Makefile
MYLIBS=		$(blddir)/Modules/libModules.a \
		$(blddir)/Python/libPython.a \
		$(blddir)/Objects/libObjects.a \
		$(blddir)/Parser/libParser.a
MODLIBS=	
LIBS=		
SYSLIBS=	-lm
ALLLIBS=	$(MYLIBS) $(MODLIBS) $(LIBS) $(SYSLIBS)

# Build the demo application
all:		demo
demo:		demo.o config.o
		$(CC) demo.o config.o $(ALLLIBS) -o demo

# Build config.o, suppressing the main() function
config.o:	$(blddir)/Modules/config.c
		$(CC) $(CFLAGS) -DNO_MAIN -c $(blddir)/Modules/config.c

# Administrative targets

test:		demo
		./demo

clean:
		-rm -f *.o core

clobber:	clean
		-rm -f *~ @* '#'* demo
