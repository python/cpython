TOP=		..
PGENDIR=	$(TOP)/Parser
PGEN=		$(PGENDIR)/pgen
DESTH=		$(TOP)/Include/graminit.h
DESTC=		$(TOP)/Python/graminit.c

all:		install

install:	$(DESTH) $(DESTC)

depend:

$(DESTH):	graminit.h
		cp graminit.h $(DESTH)

$(DESTC):	graminit.c
		cp graminit.c $(DESTC)

graminit.c graminit.h: $(PGEN) Grammar
		$(PGEN) Grammar

$(PGEN):
		cd $(PGENDIR); make pgen

clean:
		-rm -f *.o core *~ [@,#]* *.old *.orig *.rej
		-rm -f graminit.[ch]

clobber:	clean
		-rm -f tags TAGS
