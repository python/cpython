# Makefile for 'pysvr' application embedding Python.
# Tailored for Python 1.5a3 or later.
# Some details are specific for Solaris or CNRI.

# Which C compiler
CC=gcc

# Optimization preferences
OPT=-g

# Where Python is installed, and which version
INST=/usr/local
VER=1.5

# Expressions using the above definitions -- no need to change
PYVER=python$(VER)
PYC=$(INST)/lib/$(PYVER)/config
PYINCL=-I$(INST)/include/$(PYVER) -I$(PYC)
PYLIBS=$(PYC)/libpython1.5.a

# Libraries to link with -- very installation dependent
# (See LIBS= in Modules/Makefile in build tree)
RLLIBS=-lreadline -ltermcap
OTHERLIBS=-lsocket -lnsl -lpthread -ldl -lm

# Compilation and link flags -- no need to change normally
CFLAGS=$(PYINCL) $(OPT)
LIBS=$(PYLIBS) $(RLLIBS) $(OTHERLIBS)

# Default port for the pysvr application
PORT=4000

# Default target
all: pysvr

# Target to build pysvr
pysvr: pysvr.o $(PYOBJS)
	$(CC) pysvr.o $(LIBS) -o pysvr

# Target to build and run pysvr
run: pysvr
	pysvr $(PORT)

# Target to clean up the directory
clean:
	-rm -f pysvr *.o *~ core
