# Makefile for Python documentation
# ---------------------------------
#
# See also the README file.
#
# This is a bit of a mess.  The main documents are:
#   tut -- Tutorial (file tut.tex)
#   lib -- Library Reference (file lib.tex, inputs lib*.tex)
#   ext -- Extending and Embedding (file ext.tex)
#   api -- Python-C API Reference
#
# The Reference Manual is now maintained as a FrameMaker document.
# See the subdirectory ref; PostScript is included as ref/ref.ps.
# (In the future, the Tutorial will also be converted to FrameMaker;
# the other documents will be maintained in a text format such
# as LaTeX or perhaps TIM.)
#
# The main target "make all" creates DVI and PostScript for these
# four.  You can also do "make lib" (etc.) to process individual
# documents.
#
# There's one local style file: python.sty.  This defines a number
# of macros that are similar in name and intent as macros in Texinfo
# (e.g. \code{...} and \emph{...}), as well as a number of
# environments for formatting function and data definitions, also in
# the style of Texinfo.
#
# Everything is processed by LaTeX.  The following tools are used:
#   latex
#   makeindex
#   dvips
#
# There's a problem with generating the index which has been solved by
# a sed command applied to the index file.  The shell script fix_hack
# does this (the Makefile takes care of calling it).
#
# To preview the dvi files produced by LaTeX it would be useful to
# have xdvi as well.
#
# Additional targets attempt to convert selected LaTeX sources to
# various other formats.  These are generally site specific because
# the tools used are all but universal.  These targets are:
#   l2h -- convert tut, lib, ext, api from LaTeX to HTML
# See the README file for more info on these targets.

# Customizations -- you *may* have to edit these

# Where are the various programs?
LATEX=		TEXINPUTS=$(TEXINPUTS) latex
PDFLATEX=	TEXINPUTS=$(TEXINPUTS) pdflatex
TEXINPUTS=	texinputs:
DVIPS=		dvips -f -N0
DISTILL=	distill
MAKEINDEX=	makeindex
L2H=		TEXINPUTS=$(TEXINPUTS) latex2html -init_file perl/l2hinit.perl
L2HARGS=	-address $$LOGNAME@`domainname`
WEBCHECKER=	$(PYTHON) ../Tools/webchecker/webchecker.py

# Install destination -- not used now but might be useful some time...
DESTDIR=	/usr/local
LIBDESTDIR=	$DESTDIR/lib
LIBDEST=	$LIBDESTDIR/python$(VERSION)
DOCDESTDIR=	$LIBDEST/doc

# This is only used for .info generation:
EMACS=		emacs
PYTHON=		python
MAKEINFO=	makeinfo
PARTPARSE=	$(PYTHON) tools/partparse.py

# Ideally, you shouldn't need to edit beyond this point

VERSION=1.5

DVIFILES=	api.dvi ext.dvi lib.dvi tut.dvi
INFOFILES=	python-lib.info
PDFFILES=	api.pdf ext.pdf lib.pdf tut.pdf
PSFILES=	api.ps ext.ps lib.ps tut.ps

MANSTYLES=texinputs/fncychap.sty texinputs/manual.cls texinputs/python.sty

# Main target
all:	all-ps

world:	all-ps all-pdf l2h tarballs

all-dvi: $(DVIFILES)
all-pdf: $(PDFFILES)
all-ps:	 $(PSFILES)

# This target gets both the PDF and PS files updated; the all-pdf target
# above doesn't ensure that both are done if the "alternate" rule (using
# pdflatex) for PDF generation is used.
#
all-formats:  $(PSFILES) $(PDFFILES)

# Individual document fake targets
tut:	tut.ps
lib:	lib.ps
ext:	ext.ps
api:	api.ps

# All formats for a single document
api-all:  api.dvi api.pdf api.ps l2hapi
ext-all:  ext.dvi ext.pdf ext.ps l2hext
lib-all:  lib.dvi lib.pdf lib.ps l2hlib
tut-all:  tut.dvi tut.pdf tut.ps l2htut


# Rules to build PostScript and PDF formats
.SUFFIXES: .dvi .ps .pdf .tex

.dvi.ps:
	$(DVIPS) $< >$@

#.ps.pdf:
#	$(DISTILL) $<

# An alternate formulation of PDF creation; requires latex format with
# pdftex.  To use this instead of the Acrobat distiller solution, comment
# out the above .ps.pdf rule and uncomment this rule.  This was tested
# using a pre-release of the teTeX distribution.  See
# http://www.tug.org/tetex/ for more information on getting & using teTeX.
# This rule avoids creation of the intermediate PostScript files and uses
# only free software.
#
.dvi.pdf: $*.bkm
	$(PYTHON) tools/toc2bkm.py $*
	$(PDFLATEX) $*

# Dependencies
COMMONTEX=$(MANSTYLES) copyright.tex boilerplate.tex
COMMONPERL=perl/manual.perl perl/python.perl

$(DVIFILES): tools/fix_hack $(COMMONTEX)

# LaTeX source files for the Python Library Reference
LIBFILES = lib.tex \
    libintro.tex libobjs.tex libtypes.tex libexcs.tex libfuncs.tex \
    libpython.tex libsys.tex libtypes2.tex libtraceback.tex libpickle.tex \
    libshelve.tex libcopy.tex libmarshal.tex libimp.tex libparser.tex \
    libbltin.tex libmain.tex libstrings.tex libstring.tex libregex.tex \
    libregsub.tex libstruct.tex libmisc.tex libmath.tex librand.tex \
    libwhrandom.tex libarray.tex liballos.tex libos.tex libtime.tex \
    libgetopt.tex libtempfile.tex liberrno.tex libsomeos.tex libsignal.tex \
    libsocket.tex libselect.tex libthread.tex libunix.tex libposix.tex \
    libppath.tex libpwd.tex libgrp.tex libcrypt.tex libdbm.tex libgdbm.tex \
    libtermios.tex libfcntl.tex libposixfile.tex libsyslog.tex libpdb.tex \
    libprofile.tex libwww.tex libcgi.tex liburllib.tex libhttplib.tex \
    libftplib.tex libgopherlib.tex libnntplib.tex liburlparse.tex \
    libhtmllib.tex libsgmllib.tex librfc822.tex libmimetools.tex \
    libbinascii.tex libmm.tex libaudioop.tex libimageop.tex libaifc.tex \
    libjpeg.tex librgbimg.tex libcrypto.tex libmd5.tex libmpz.tex \
    librotor.tex libmac.tex libctb.tex libmacconsole.tex libmacdnr.tex \
    libmacfs.tex libmacos.tex libmacostools.tex libmactcp.tex \
    libmacspeech.tex libmacui.tex libstdwin.tex libsgi.tex libal.tex \
    libcd.tex libfl.tex libfm.tex libgl.tex libimgfile.tex libsun.tex \
    libxdrlib.tex libimghdr.tex \
    librestricted.tex librexec.tex libbastion.tex \
    libformatter.tex liboperator.tex libsoundex.tex libresource.tex \
    libstat.tex libstrio.tex libundoc.tex libmailcap.tex libglob.tex \
    libuser.tex libanydbm.tex librandom.tex libsite.tex libwhichdb.tex \
    libbase64.tex libfnmatch.tex libquopri.tex libzlib.tex libsocksvr.tex \
    libmailbox.tex libcommands.tex libcmath.tex libni.tex libgzip.tex \
    libpprint.tex libcode.tex libmimify.tex libre.tex libmacic.tex \
    libuserdict.tex libdis.tex libxmllib.tex libqueue.tex \
    liblocale.tex libbasehttp.tex libcopyreg.tex libsymbol.tex libtoken.tex \
    libframework.tex libminiae.tex libbinhex.tex libuu.tex libsunaudio.tex

# Library document
lib.dvi: tools/modindex.py tools/indfix.py $(LIBFILES)
	tools/newind.py >$*.ind
	tools/newind.py modindex >mod$*.ind
	$(LATEX) $*
	tools/modindex.py mod$*.idx
	tools/fix_hack $*.idx
	$(MAKEINDEX) $*.idx
	tools/indfix.py $*.ind
	$(LATEX) $*

# Tutorial document
tut.dvi: tut.tex 
	$(LATEX) $*
	$(LATEX) $*

# Extending & Embedding, Python/C API documents.
# Done this way to avoid repeated command sets.
.tex.dvi:
	tools/newind.py >$*.ind
	$(LATEX) $*
	tools/fix_hack $*.idx
	$(MAKEINDEX) $*.idx
	$(LATEX) $*


# The remaining part of the Makefile is concerned with various
# conversions, as described above.  See also the README file.

.PRECIOUS:	python-lib.texi

# The sed script in this target fixes a really nasty little condition in
# libcgi.tex where \e has to be used in a group to get the right behavior,
# and makeinfo can't handle a group without a leading @command.  But at
# least the info file gets generated.

lib1.texi: $(LIBFILES) texipre.dat texipost.dat $(PARTPARSEOBJ)
	$(PARTPARSE) -o $@ `tools/whichlibs`
	sed 's/"{\\}n{\\}n/"\\n\\n/' $@ >temp.texi
	mv temp.texi $@

python-lib.texi: lib1.texi tools/fix.el
	cp lib1.texi temp.texi
	$(EMACS) -batch -l tools/fix.el -f save-buffer -kill
	mv temp.texi $@

python-lib.info: python-lib.texi
	$(MAKEINFO) --footnote-style end --fill-column 72 \
		     --paragraph-indent 0 $<

# this is needed to prevent a second set of info files from being generated,
# at least when using GNU make
.PHONY: lib.info lib.texi

lib.info: python-lib.info

lib.texi: python-lib.texi

# Targets to convert the manuals to HTML using Nikos Drakos' LaTeX to
# HTML converter.  For more info on this program, see
# <URL:http://cbl.leeds.ac.uk/nikos/tex2html/doc/latex2html/latex2html.html>.

# Note that LaTeX2HTML inserts references to an icons directory in
# each page that it generates.  I have placed a copy of this directory
# in the distribution to simplify the process of creating a
# self-contained HTML distribution; for this purpose I have also added
# a (trivial) index.html.  Change the definition of $ICONSERVER in
# .latex2html-init to use a different location for the icons directory.

l2h: l2hapi l2hext l2hlib l2htut

l2htut: tut.dvi $(COMMONPERL)
	$(L2H) $(L2HARGS) tut.tex
	(cd tut; ../tools/node2label.pl *.html)

l2hext: ext.dvi $(COMMONPERL)
	$(L2H) $(L2HARGS) ext.tex
	(cd ext; ../tools/node2label.pl *.html)

l2hlib: lib.dvi $(COMMONPERL)
	tools/fix_libaux.sed <lib.aux >lib1.aux
	mv lib1.aux lib.aux
	if test -d lib ; then rm -f lib/*.html ; fi
	$(L2H) $(L2HARGS) lib.tex
	(cd lib; ../tools/node2label.pl *.html)

l2hapi: api.dvi $(COMMONPERL)
	$(L2H) $(L2HARGS) api.tex
	(cd api; ../tools/node2label.pl *.html)

# webchecker needs an extra flag to process the huge index from the libref
webcheck:
	$(WEBCHECKER) file:`pwd`/api/
	$(WEBCHECKER) file:`pwd`/ext/
	$(WEBCHECKER) -m290000 file:`pwd`/lib/
	$(WEBCHECKER) file:`pwd`/tut/

lib-info-$(VERSION).tar.gz: $(INFOFILES)
	tar cf - python-???.info* | gzip -9 >$@

latex-$(VERSION).tar.gz:
	tools/mktarball.sh

# This snags a PDF version if available, but doesn't fail if not.
pdf-$(VERSION).tar.gz: $(PDFFILES)
	if test -f ref/ref.pdf ; then cp ref/ref.pdf . ; else true ; fi
	tar cf - ???.pdf | gzip -9 >$@
	if test -f ref.pdf ; then rm ref.pdf ; else true ; fi

postscript-$(VERSION).tar.gz: $(PSFILES) ref/ref.ps
	cp ref/ref.ps .
	tar cf - ???.ps | gzip -9 >$@
	rm ref.ps

tarhtml:
	@echo "Did you remember to run makeMIFs.py in the ref subdirectory...?"
	tar cf - index.html ???/???.css ???/*.html */*.gif \
		| gzip -9 >html-$(VERSION).tar.gz

# convenience targets:

tarinfo:  lib-info-$(VERSION).tar.gz

tarps:	postscript-$(VERSION).tar.gz

tarpdf:	pdf-$(VERSION).tar.gz

tarlatex: latex-$(VERSION).tar.gz

tarballs:  tarpdf tarps tarhtml tarlatex


# Housekeeping targets

# Remove temporary files; all except the following:
# - sources: .tex, .bib, .sty, *.cls
# - useful results: .dvi, .pdf, .ps, .texi, .info
clean:
	rm -f @* *~ *.aux *.idx *.ilg *.ind *.log *.toc *.blg *.bbl *.py[co]
	rm -f *.bak *.orig lib1.texi *.out
	rm -f html-$(VERSION).tar.gz info-$(VERSION).tar.gz
	rm -f pdf-$(VERSION).tar.gz postscript-$(VERSION).tar.gz
	rm -f latex-$(VERSION).tar.gz

l2hclean:
	rm -rf api ext lib tut

# Remove temporaries as well as final products
clobber: clean l2hclean
	 rm -f *.dvi *.pdf *.ps *.texi python-*.info python-*.info-[0-9]*

realclean:  clobber
distclean:  clobber
