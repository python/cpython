# Makefile for Python documentation
# ---------------------------------
#
# See also the README file.
#
# This is a bit of a mess.  The documents are identified by short names:
#   api -- Python/C API Reference Manual
#   doc -- Documenting Python
#   ext -- Extending and Embedding the Python Interpreter
#   lib -- Library Reference Manual
#   mac -- Macintosh Library Modules
#   ref -- Python Reference Manual
#   tut -- Python Tutorial
#   inst -- Installing Python Modules
#   dist -- Distributing Python Modules
#
# The LaTeX sources for each of these documents are in subdirectories
# with the three-letter designations above as the directory names.
#
# The main target creates HTML for each of the documents.  You can
# also do "make lib" (etc.) to create the HTML versions of individual
# documents.
#
# The document classes and styles are in the texinputs/ directory.
# These define a number of macros that are similar in name and intent
# as macros in Texinfo (e.g. \code{...} and \emph{...}), as well as a
# number of environments for formatting function and data definitions.
# Documentation for the macros is included in "Documenting Python"; see
# http://www.python.org/doc/current/doc/doc.html, or the sources for
# this document in the doc/ directory.
#
# Everything is processed by LaTeX.  See the file `README' for more
# information on the tools needed for processing.
#
# There's a problem with generating the index which has been solved by
# a sed command applied to the index file.  The shell script fix_hack
# does this (the Makefile takes care of calling it).
#
# Additional targets attempt to convert selected LaTeX sources to
# various other formats.  These are generally site specific because
# the tools used are all but universal.  These targets are:
#
#   ps  -- convert all documents from LaTeX to PostScript
#   pdf -- convert all documents from LaTeX to the
#		Portable Document Format
#
# See the README file for more information on these targets.
#
# The formatted output is located in subdirectories.  For PDF and
# PostScript, look in the paper-$(PAPER)/ directory.  For HTML, look in
# the html/ directory.  If you want to fix the GNU info process, look
# in the info/ directory; please send patches to python-docs@python.org.

# This Makefile only includes information on how to perform builds; for
# dependency information, see Makefile.deps.

# Customization -- you *may* have to edit this

# You could set this to a4:
PAPER=letter

# Ideally, you shouldn't need to edit beyond this point

HTMLDIR=	html
INFODIR=	info
TOOLSDIR=	tools

# This is the *documentation* release, and is used to construct the file
# names of the downloadable tarballs.
RELEASE=2.1.1c1

PYTHON=	   python
DVIPS=	   dvips -N0 -t $(PAPER)

MKDVI=	   ../tools/mkhowto --paper=$(PAPER) --dvi
MKHTML=	   tools/mkhowto --html --about html/stdabout.dat \
		--address $(PYTHONDOCS) --up-link ../index.html \
		--up-title "Python Documentation Index" \
		--global-module-index "../modindex.html"
MKPDF=	   ../tools/mkhowto --paper=$(PAPER) --pdf
MKPS=	   ../tools/mkhowto --paper=$(PAPER) --ps

BUILDINDEX=$(TOOLSDIR)/buildindex.py

PYTHONDOCS='See <i><a href="about.html">About this document...</a></i> for information on suggesting changes.'
HTMLBASE=  file:`pwd`

# what's what
MANDVIFILES=	paper-$(PAPER)/api.dvi paper-$(PAPER)/ext.dvi \
		paper-$(PAPER)/lib.dvi paper-$(PAPER)/mac.dvi \
		paper-$(PAPER)/ref.dvi paper-$(PAPER)/tut.dvi
HOWTODVIFILES=	paper-$(PAPER)/doc.dvi paper-$(PAPER)/inst.dvi \
		paper-$(PAPER)/dist.dvi

MANPDFFILES=	paper-$(PAPER)/api.pdf paper-$(PAPER)/ext.pdf \
		paper-$(PAPER)/lib.pdf paper-$(PAPER)/mac.pdf \
		paper-$(PAPER)/ref.pdf paper-$(PAPER)/tut.pdf
HOWTOPDFFILES=	paper-$(PAPER)/doc.pdf paper-$(PAPER)/inst.pdf \
		paper-$(PAPER)/dist.pdf

MANPSFILES=	paper-$(PAPER)/api.ps paper-$(PAPER)/ext.ps \
		paper-$(PAPER)/lib.ps paper-$(PAPER)/mac.ps \
		paper-$(PAPER)/ref.ps paper-$(PAPER)/tut.ps
HOWTOPSFILES=	paper-$(PAPER)/doc.ps paper-$(PAPER)/inst.ps \
		paper-$(PAPER)/dist.ps

DVIFILES=	$(MANDVIFILES) $(HOWTODVIFILES)
PDFFILES=	$(MANPDFFILES) $(HOWTOPDFFILES)
PSFILES=	$(MANPSFILES) $(HOWTOPSFILES)

INDEXFILES=html/api/api.html \
	html/doc/doc.html \
	html/ext/ext.html \
	html/lib/lib.html \
	html/mac/mac.html \
	html/ref/ref.html \
	html/tut/tut.html \
	html/inst/inst.html \
	html/dist/dist.html

COMMONPERL= perl/manual.perl perl/python.perl perl/l2hinit.perl

include Makefile.deps

# These must be declared phony since there
# are directories with matching names:
.PHONY: api doc ext lib mac ref tut inst dist
.PHONY: html info


# Main target
all:	html

dvi:	$(DVIFILES)
pdf:	$(PDFFILES)
ps:	$(PSFILES)

world:	ps pdf html distfiles


# Rules to build PostScript and PDF formats
.SUFFIXES: .dvi .ps

.dvi.ps:
	$(DVIPS) -o $@ $<


# Targets for each document:
# Python/C API Reference Manual
paper-$(PAPER)/api.dvi: paper-$(PAPER)/api.tex $(APIFILES)
	(cd paper-$(PAPER); $(MKDVI) api.tex)

paper-$(PAPER)/api.pdf: paper-$(PAPER)/api.tex $(APIFILES)
	(cd paper-$(PAPER); $(MKPDF) api.tex)

paper-$(PAPER)/api.tex: api/api.tex api/refcounts.dat tools/anno-api.py
	$(PYTHON) $(TOOLSDIR)/anno-api.py -o $@ api/api.tex

# Distributing Python Modules
paper-$(PAPER)/dist.dvi: $(DISTFILES)
	(cd paper-$(PAPER); $(MKDVI) ../dist/dist.tex)

paper-$(PAPER)/dist.pdf: $(DISTFILES)
	(cd paper-$(PAPER); $(MKPDF) ../dist/dist.tex)

# Documenting Python
paper-$(PAPER)/doc.dvi: $(DOCFILES)
	(cd paper-$(PAPER); $(MKDVI) ../doc/doc.tex)

paper-$(PAPER)/doc.pdf: $(DOCFILES)
	(cd paper-$(PAPER); $(MKPDF) ../doc/doc.tex)

# Extending and Embedding the Python Interpreter
paper-$(PAPER)/ext.dvi: $(EXTFILES)
	(cd paper-$(PAPER); $(MKDVI) ../ext/ext.tex)

paper-$(PAPER)/ext.pdf: $(EXTFILES)
	(cd paper-$(PAPER); $(MKPDF) ../ext/ext.tex)

# Installing Python Modules
paper-$(PAPER)/inst.dvi: $(INSTFILES)
	(cd paper-$(PAPER); $(MKDVI) ../inst/inst.tex)

paper-$(PAPER)/inst.pdf: $(INSTFILES)
	(cd paper-$(PAPER); $(MKPDF) ../inst/inst.tex)

# Python Library Reference
paper-$(PAPER)/lib.dvi: $(LIBFILES)
	(cd paper-$(PAPER); $(MKDVI) ../lib/lib.tex)

paper-$(PAPER)/lib.pdf: $(LIBFILES)
	(cd paper-$(PAPER); $(MKPDF) ../lib/lib.tex)

# Macintosh Library Modules
paper-$(PAPER)/mac.dvi: $(MACFILES)
	(cd paper-$(PAPER); $(MKDVI) ../mac/mac.tex)

paper-$(PAPER)/mac.pdf: $(MACFILES)
	(cd paper-$(PAPER); $(MKPDF) ../mac/mac.tex)

# Python Reference Manual
paper-$(PAPER)/ref.dvi: $(REFFILES)
	(cd paper-$(PAPER); $(MKDVI) ../ref/ref.tex)

paper-$(PAPER)/ref.pdf: $(REFFILES)
	(cd paper-$(PAPER); $(MKPDF) ../ref/ref.tex)

# Python Tutorial
paper-$(PAPER)/tut.dvi: $(TUTFILES)
	(cd paper-$(PAPER); $(MKDVI) ../tut/tut.tex)

paper-$(PAPER)/tut.pdf: $(TUTFILES)
	(cd paper-$(PAPER); $(MKPDF) ../tut/tut.tex)

# The remaining part of the Makefile is concerned with various
# conversions, as described above.  See also the README file.

info:
	(cd $(INFODIR); $(MAKE))

# Targets to convert the manuals to HTML using Nikos Drakos' LaTeX to
# HTML converter.  For more info on this program, see
# <URL:http://cbl.leeds.ac.uk/nikos/tex2html/doc/latex2html/latex2html.html>.

# Note that LaTeX2HTML inserts references to an icons directory in
# each page that it generates.  I have placed a copy of this directory
# in the distribution to simplify the process of creating a
# self-contained HTML distribution; for this purpose I have also added
# a (trivial) index.html.  Change the definition of $ICONSERVER in
# perl/l2hinit.perl to use a different location for the icons directory.

# If you have the standard LaTeX2HTML icons installed, the versions shipped
# with this documentation should be stored in a separate directory and used
# instead.  The standard set does *not* include all the icons used in the
# Python documentation.

$(INDEXFILES): $(COMMONPERL) html/about.dat tools/node2label.pl

html/acks.html: ACKS $(TOOLSDIR)/support.py $(TOOLSDIR)/mkackshtml
	$(TOOLSDIR)/mkackshtml --address $(PYTHONDOCS) \
		--output html/acks.html <ACKS

html/modindex.html: $(TOOLSDIR)/support.py $(TOOLSDIR)/mkmodindex
html/modindex.html: html/lib/lib.html html/mac/mac.html
	(cd html; \
	 ../$(TOOLSDIR)/mkmodindex --columns 4 --output modindex.html \
		--address $(PYTHONDOCS) \
		lib/modindex.html mac/modindex.html)

html:	$(INDEXFILES) html/index.html html/modindex.html html/acks.html

api html/api/api.html: $(APIFILES)
	$(MKHTML) --dir html/api api/api.tex

doc html/doc/doc.html: $(DOCFILES)
	$(MKHTML) --dir html/doc doc/doc.tex

ext html/ext/ext.html: $(EXTFILES)
	$(MKHTML) --dir html/ext ext/ext.tex

lib html/lib/lib.html: $(LIBFILES)
	$(MKHTML) --dir html/lib lib/lib.tex

mac html/mac/mac.html: $(MACFILES)
	$(MKHTML) --dir html/mac mac/mac.tex

ref html/ref/ref.html: $(REFFILES)
	$(MKHTML) --dir html/ref ref/ref.tex

tut html/tut/tut.html: $(TUTFILES)
	$(MKHTML) --dir html/tut --numeric --split 3 tut/tut.tex

inst html/inst/inst.html: $(INSTFILES) perl/distutils.perl
	$(MKHTML) --dir html/inst --split 4 inst/inst.tex

dist html/dist/dist.html: $(DISTFILES) perl/distutils.perl
	$(MKHTML) --dir html/dist --split 4 dist/dist.tex


# webchecker needs an extra flag to process the huge index from the libref
WEBCHECKER=$(PYTHON) ../Tools/webchecker/webchecker.py
HTMLBASE=  file:`pwd`/html

webcheck: html
	$(WEBCHECKER) $(HTMLBASE)/api/
	$(WEBCHECKER) $(HTMLBASE)/doc/
	$(WEBCHECKER) $(HTMLBASE)/ext/
	$(WEBCHECKER) -m290000 $(HTMLBASE)/lib/
	$(WEBCHECKER) $(HTMLBASE)/mac/
	$(WEBCHECKER) $(HTMLBASE)/ref/
	$(WEBCHECKER) $(HTMLBASE)/tut/
	$(WEBCHECKER) $(HTMLBASE)/dist/
	$(WEBCHECKER) $(HTMLBASE)/inst/

fastwebcheck: html
	$(WEBCHECKER) -x $(HTMLBASE)/api/
	$(WEBCHECKER) -x $(HTMLBASE)/doc/
	$(WEBCHECKER) -x $(HTMLBASE)/ext/
	$(WEBCHECKER) -x -m290000 $(HTMLBASE)/lib/
	$(WEBCHECKER) -x $(HTMLBASE)/mac/
	$(WEBCHECKER) -x $(HTMLBASE)/ref/
	$(WEBCHECKER) -x $(HTMLBASE)/tut/
	$(WEBCHECKER) -x $(HTMLBASE)/dist/
	$(WEBCHECKER) -x $(HTMLBASE)/inst/


# Release packaging targets:

paper-$(PAPER)/README: ps $(TOOLSDIR)/getpagecounts
	(cd paper-$(PAPER); ../$(TOOLSDIR)/getpagecounts >../$@)

info-$(RELEASE).tgz: info
	(cd $(INFODIR); tar cf - README python.dir python-???.info*) \
		| gzip -9 >$@

info-$(RELEASE).tar.bz2: info
	(cd $(INFODIR); tar cf - README python.dir python-???.info*) \
		| bzip2 -9 >$@

latex-$(RELEASE).tgz:
	$(TOOLSDIR)/mksourcepkg --gzip $(RELEASE)

latex-$(RELEASE).tar.bz2:
	$(TOOLSDIR)/mksourcepkg --bzip2 $(RELEASE)

latex-$(RELEASE).zip:
	rm -f $@
	$(TOOLSDIR)/mksourcepkg --zip $(RELEASE)

pdf-$(PAPER)-$(RELEASE).tgz: pdf
	(cd paper-$(PAPER); tar cf - *.pdf) | gzip -9 >$@

pdf-$(PAPER)-$(RELEASE).tar.bz2: pdf
	(cd paper-$(PAPER); tar cf - *.pdf) | bzip2 -9 >$@

pdf-$(PAPER)-$(RELEASE).zip: pdf
	rm -f $@
	(cd paper-$(PAPER); zip -q -9 ../$@ *.pdf)

postscript-$(PAPER)-$(RELEASE).tar.bz2: ps paper-$(PAPER)/README
	(cd paper-$(PAPER); tar cf - *.ps README) | bzip2 -9 >$@

postscript-$(PAPER)-$(RELEASE).tgz: ps paper-$(PAPER)/README
	(cd paper-$(PAPER); tar cf - *.ps README) | gzip -9 >$@

postscript-$(PAPER)-$(RELEASE).zip: ps paper-$(PAPER)/README
	rm -f $@
	(cd paper-$(PAPER); zip -q -9 ../$@ *.ps README)

html-$(RELEASE).tgz:	html
	(cd $(HTMLDIR); \
		tar cf - *.html */*.css */*.html */*.gif) \
		| gzip -9 >$@

html-$(RELEASE).tar.bz2:	html
	(cd $(HTMLDIR); \
		tar cf - *.html */*.css */*.html */*.gif) \
		| bzip2 -9 >$@

html-$(RELEASE).zip:	html
	rm -f $@
	(cd $(HTMLDIR); \
		zip -q -9 ../$@ *.html */*.css */*.html */*.gif)

# convenience targets:

tarhtml:	html-$(RELEASE).tgz
tarinfo:	info-$(RELEASE).tgz
tarps:		postscript-$(PAPER)-$(RELEASE).tgz
tarpdf:		pdf-$(PAPER)-$(RELEASE).tgz
tarlatex:	latex-$(RELEASE).tgz

tarballs:	tarpdf tarps tarhtml

ziphtml:	html-$(RELEASE).zip
zipps:		postscript-$(PAPER)-$(RELEASE).zip
zippdf:		pdf-$(PAPER)-$(RELEASE).zip
ziplatex:	latex-$(RELEASE).zip

zips:		zippdf zipps ziphtml

bziphtml:	html-$(RELEASE).tar.bz2
bzipinfo:	info-$(RELEASE).tar.bz2
bzipps:		postscript-$(PAPER)-$(RELEASE).tar.bz2
bzippdf:	pdf-$(PAPER)-$(RELEASE).tar.bz2
bziplatex:	latex-$(RELEASE).tar.bz2

bzips:		bzippdf bzipps bziphtml

disthtml:	tarhtml bziphtml ziphtml
distinfo:	tarinfo bzipinfo
distps:		tarps bzipps zipps
distpdf:	tarpdf bzippdf zippdf
distlatex:	tarlatex bziplatex ziplatex

distfiles:	tarballs zips bzips
	$(TOOLSDIR)/mksourcepkg --all $(RELEASE)


# Housekeeping targets

# Remove temporary files; all except the following:
# - sources: .tex, .bib, .sty, *.cls
# - useful results: .dvi, .pdf, .ps, .texi, .info
clean:
	(cd paper-$(PAPER); $(MAKE) clean)
	(cd $(INFODIR); $(MAKE) clean)

# Remove temporaries as well as final products
clobber:
	rm -f html-$(RELEASE).tgz info-$(RELEASE).tgz
	rm -f pdf-$(RELEASE).tgz postscript-$(RELEASE).tgz
	rm -f latex-$(RELEASE).tgz html-$(RELEASE).zip
	rm -f pdf-$(RELEASE).zip postscript-$(RELEASE).zip
	rm -f $(DVIFILES) $(PSFILES) $(PDFFILES)
	(cd $(INFODIR); $(MAKE) clobber)
	rm -rf html/index.html html/modindex.html html/acks.html
	rm -rf html/api/ html/doc/ html/ext/ html/lib/ html/mac/
	rm -rf html/ref/ html/tut/ html/inst/ html/dist/

realclean distclean:  clobber


# html/index.html is dependent on $(INDEXFILES) since we want the date
# on the front index to be updated whenever any of the child documents
# are updated and boilerplate.tex uses \today as the date.

# It's at the end of the file since it wedges font-lock in XEmacs.

BOILERPLATE=texinputs/boilerplate.tex
html/index.html: html/index.html.in $(BOILERPLATE) $(INDEXFILES)
	DATE=`grep '\\date{' $(BOILERPLATE) | sed 's/.*\\date{\(.*\)}.*$$/\1/'` ; \
	if [ "$$DATE" = '\today' ] ; then DATE=`date '+%B %e, %Y'`;fi;\
	sed -e "s/@DATE@/$$DATE/g" -e "s/@RELEASE@/$(RELEASE)/g" \
		$< >$@
