# Makefile for 'pysvr' application embedding Python.
# Tailored for Python 1.5a3 or later.
# Some details are specific for Solaris or CNRI.
# Also see ## comments for tailoring.

# Which C compiler
CC=gcc
##PURIFY=/usr/local/pure/purify
LINKCC=$(PURIFY) $(CC)

# Optimization preferences
OPT=-g

# Which Python version we're using
VER=2.2

# Expressions using the above definitions
PYVER=python$(VER)

# Use these defs when compiling against installed Python
##INST=/usr/local
##PYC=$(INST)/lib/$(PYVER)/config
##PYINCL=-I$(INST)/include/$(PYVER) -I$(PYC)
##PYLIBS=$(PYC)/lib$(PYVER).a

# Use these defs when compiling against built Python
PLAT=linux
PYINCL=-I../../Include -I../../$(PLAT)
PYLIBS=../../$(PLAT)/lib$(PYVER).a

# Libraries to link with -- very installation dependent
# (See LIBS= in Modules/Makefile in build tree)
RLLIBS=-lreadline -ltermcap
OTHERLIBS=-lnsl -lpthread -ldl -lm -ldb -lutil

# Compilation and link flags -- no need to change normally
CFLAGS=$(OPT)
CPPFLAGS=$(PYINCL)
LIBS=$(PYLIBS) $(RLLIBS) $(OTHERLIBS)

# Default port for the pysvr application
PORT=4000

# Default target
all: pysvr

# Target to build pysvr
pysvr: pysvr.o $(PYOBJS) $(PYLIBS)
	$(LINKCC) pysvr.o $(LIBS) -o pysvr

# Target to build and run pysvr
run: pysvr
	pysvr $(PORT)

# Target to clean up the directory
clean:
	-rm -f pysvr *.o *~ core
