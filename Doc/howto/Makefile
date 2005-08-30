
MKHOWTO=../tools/mkhowto
WEBDIR=.
RSTARGS = --input-encoding=utf-8
VPATH=.:dvi:pdf:ps:txt

# List of HOWTOs that aren't to be processed

REMOVE_HOWTO =

# Determine list of files to be built

HOWTO=$(filter-out $(REMOVE_HOWTO),$(wildcard *.tex))
RST_SOURCES =	$(shell echo *.rst)
DVI  =$(patsubst %.tex,%.dvi,$(HOWTO))
PDF  =$(patsubst %.tex,%.pdf,$(HOWTO))
PS   =$(patsubst %.tex,%.ps,$(HOWTO))
TXT  =$(patsubst %.tex,%.txt,$(HOWTO))
HTML =$(patsubst %.tex,%,$(HOWTO))

# Rules for building various formats
%.dvi : %.tex
	$(MKHOWTO) --dvi $<
	mv $@ dvi

%.pdf : %.tex
	$(MKHOWTO) --pdf $<
	mv $@ pdf

%.ps : %.tex
	$(MKHOWTO) --ps $<
	mv $@ ps

%.txt : %.tex
	$(MKHOWTO) --text $<
	mv $@ txt

% : %.tex
	$(MKHOWTO) --html --iconserver="." $<
	tar -zcvf html/$*.tgz $*
	#zip -r html/$*.zip $*

default:
	@echo "'all'    -- build all files"
	@echo "'dvi', 'pdf', 'ps', 'txt', 'html' -- build one format"

all: $(HTML)

.PHONY : dvi pdf ps txt html rst
dvi: $(DVI)

pdf: $(PDF)
ps:  $(PS)
txt: $(TXT)
html:$(HTML)

# Rule to build collected tar files
dist: #all
	for i in dvi pdf ps txt ; do \
	    cd $$i ; \
	    tar -zcf All.tgz *.$$i ;\
	    cd .. ;\
	done

# Rule to copy files to the Web tree on AMK's machine
web: dist
	cp dvi/* $(WEBDIR)/dvi
	cp ps/* $(WEBDIR)/ps
	cp pdf/* $(WEBDIR)/pdf
	cp txt/* $(WEBDIR)/txt
	for dir in $(HTML) ; do cp -rp $$dir $(WEBDIR) ; done
	for ltx in $(HOWTO) ; do cp -p $$ltx $(WEBDIR)/latex ; done

rst: unicode.html

%.html: %.rst
	rst2html $(RSTARGS) $< >$@

clean:
	rm -f *~ *.log *.ind *.l2h *.aux *.toc *.how
	rm -f *.dvi *.ps *.pdf *.bkm
	rm -f unicode.html

clobber:
	rm dvi/* ps/* pdf/* txt/* html/*



