PYTHONBUILDDIR=../..

OPT=-g -O3 -Wall -Wstrict-prototypes -no-cpp-precomp -fno-common -dynamic
INCLUDES=-I$(PYTHONBUILDDIR) -I$(PYTHONBUILDDIR)/Include \
	-I$(PYTHONBUILDDIR)/Mac/Include
DEFINES=-DHAVE_CONFIG_H

CFLAGS=$(OPT) $(DEFINES) $(INCLUDES)
LDFLAGS=-framework System -framework Python -framework Carbon \
	-framework Foundation
CC=cc
LD=cc

OBJECTS=$(PYTHONBUILDDIR)/Mac/Python/macmain.o \
	$(PYTHONBUILDDIR)/Mac/Python/macgetargv.o

pythonforbundle: $(OBJECTS)
	$(LD) $(LDFLAGS) $(OBJECTS) -o pythonforbundle