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
LATEX=		latex
PDFLATEX=	pdflatex
DVIPS=		dvips -N0
DISTILL=	distill
KPSEWHICH=	TEXINPUTS=$(TEXINPUTS) kpsewhich
MAKEINDEX=	makeindex -s $(srcdir)/texinputs/myindex.ist
L2HARGS=
WEBCHECKER=	$(PYTHON) $(srcdir)/../Tools/webchecker/webchecker.py

# Install destination -- not used now but might be useful some time...
DESTDIR=	/usr/local
LIBDESTDIR=	$DESTDIR/lib
LIBDEST=	$LIBDESTDIR/python$(VERSION)
DOCDESTDIR=	$LIBDEST/doc

# These is only used for .info generation:
EMACS=		emacs
PYTHON=		python
MAKEINFO=	makeinfo
PARTPARSE=	$(PYTHON) $(srcdir)/tools/partparse.py

srcdir=.
VPATH=.

# Ideally, you shouldn't need to edit beyond this point

RELEASE=1.5.1
VERSION=1.5

DVIFILES=	api.dvi ext.dvi lib.dvi ref.dvi tut.dvi
INFOFILES=	python-lib.info
PDFFILES=	api.pdf ext.pdf lib.pdf ref.pdf tut.pdf
PSFILES=	api.ps ext.ps lib.ps ref.ps tut.ps

MANSTYLES=texinputs/fncychap.sty texinputs/manual.cls \
	  texinputs/python.sty texinputs/myindex.ist

# Be careful when messing with this one!
TEXINPUTS=	.:$(srcdir)/texinputs:

MKDVI=		TEXINPUTS=$(TEXINPUTS) $(srcdir)/tools/mkdvi.sh

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
	$(DVIPS) -o $@ $<

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
$(PDFFILES): $(srcdir)/tools/toc2bkm.py
.dvi.pdf:
	TEXINPUTS=$(srcdir)/$*:$(TEXINPUTS) $(PDFLATEX) $*
	$(PYTHON) $(srcdir)/tools/toc2bkm.py $*
	TEXINPUTS=$(srcdir)/$*:$(TEXINPUTS) $(PDFLATEX) $*

# Dependencies
COMMONTEX=$(MANSTYLES) texinputs/copyright.tex texinputs/boilerplate.tex
COMMONPERL=perl/manual.perl perl/python.perl

$(DVIFILES): tools/fix_hack tools/mkdvi.sh $(COMMONTEX)

ref.dvi:  ref/ref1.tex ref/ref2.tex ref/ref3.tex ref/ref4.tex \
	  ref/ref5.tex ref/ref6.tex ref/ref7.tex ref/ref8.tex

# LaTeX source files for the Python Library Reference
LIBFILES = lib/lib.tex \
    lib/libintro.tex lib/libobjs.tex lib/libtypes.tex lib/libexcs.tex \
    lib/libfuncs.tex lib/libpython.tex lib/libsys.tex lib/libtypes2.tex \
    lib/libtraceback.tex lib/libpickle.tex lib/libshelve.tex \
    lib/libcopy.tex lib/libmarshal.tex lib/libimp.tex lib/libparser.tex \
    lib/libbltin.tex lib/libmain.tex lib/libstrings.tex lib/libstring.tex \
    lib/libregex.tex lib/libregsub.tex lib/libstruct.tex lib/libmisc.tex \
    lib/libmath.tex lib/librand.tex lib/libwhrandom.tex lib/libarray.tex \
    lib/liballos.tex lib/libos.tex lib/libtime.tex lib/libgetopt.tex \
    lib/libtempfile.tex lib/liberrno.tex lib/libsomeos.tex lib/libsignal.tex \
    lib/libsocket.tex lib/libselect.tex lib/libthread.tex lib/libunix.tex \
    lib/libposix.tex lib/libppath.tex lib/libpwd.tex lib/libgrp.tex \
    lib/libcrypt.tex lib/libdbm.tex lib/libgdbm.tex lib/libtermios.tex \
    lib/libfcntl.tex lib/libposixfile.tex lib/libsyslog.tex lib/libpdb.tex \
    lib/libprofile.tex lib/libwww.tex lib/libcgi.tex lib/liburllib.tex \
    lib/libhttplib.tex lib/libftplib.tex lib/libgopherlib.tex \
    lib/libnntplib.tex lib/liburlparse.tex lib/libhtmllib.tex \
    lib/libsgmllib.tex lib/librfc822.tex lib/libmimetools.tex \
    lib/libbinascii.tex lib/libmm.tex lib/libaudioop.tex lib/libimageop.tex \
    lib/libaifc.tex lib/libjpeg.tex lib/librgbimg.tex lib/libcrypto.tex \
    lib/libmd5.tex lib/libmpz.tex lib/librotor.tex lib/libstdwin.tex \
    lib/libsgi.tex lib/libal.tex lib/libcd.tex lib/libfl.tex lib/libfm.tex \
    lib/libgl.tex lib/libimgfile.tex lib/libsun.tex lib/libxdrlib.tex \
    lib/libimghdr.tex lib/librestricted.tex lib/librexec.tex \
    lib/libbastion.tex lib/libformatter.tex lib/liboperator.tex \
    lib/libsoundex.tex lib/libresource.tex lib/libstat.tex lib/libstrio.tex \
    lib/libundoc.tex lib/libmailcap.tex lib/libglob.tex lib/libuser.tex \
    lib/libanydbm.tex lib/librandom.tex lib/libsite.tex lib/libwhichdb.tex \
    lib/libbase64.tex lib/libfnmatch.tex lib/libquopri.tex lib/libzlib.tex \
    lib/libsocksvr.tex lib/libmailbox.tex lib/libcommands.tex \
    lib/libcmath.tex lib/libgzip.tex lib/libpprint.tex \
    lib/libcode.tex lib/libmimify.tex lib/libre.tex lib/libuserdict.tex \
    lib/libdis.tex lib/libxmllib.tex lib/libqueue.tex lib/liblocale.tex \
    lib/libbasehttp.tex lib/libcopyreg.tex lib/libsymbol.tex lib/libtoken.tex \
    lib/libbinhex.tex lib/libuu.tex \
    lib/libsunaudio.tex lib/libfileinput.tex lib/libimaplib.tex \
    lib/libpoplib.tex lib/libcalendar.tex lib/libpopen2.tex lib/libbisect.tex

# LaTeX source files for Macintosh Library Modules.
MACLIBFILES = mac/mac.tex mac/libmac.tex mac/libctb.tex mac/libmacconsole.tex \
    mac/libmacdnr.tex mac/libmacfs.tex mac/libmacos.tex mac/libmacostools.tex \
    mac/libmactcp.tex mac/libmacspeech.tex mac/libmacui.tex mac/libmacic.tex \
    mac/libframework.tex mac/libminiae.tex

# Python Library Reference
lib.dvi: tools/indfix.py $(LIBFILES)
	$(srcdir)/tools/newind.py >$*.ind
	$(srcdir)/tools/newind.py modindex >mod$*.ind
	TEXINPUTS=$(srcdir)/lib:$(TEXINPUTS) $(LATEX) $*
	$(MAKEINDEX) mod$*.idx
	$(srcdir)/tools/fix_hack $*.idx
	$(MAKEINDEX) $*.idx
	$(srcdir)/tools/indfix.py $*.ind
	TEXINPUTS=$(srcdir)/lib:$(TEXINPUTS) $(LATEX) $*

# Python/C API Reference Manual
api.dvi: api/api.tex
	$(MKDVI) api

# Extending and Embedding the Python Interpreter
ext.dvi: ext/ext.tex
	$(MKDVI) ext

# Python Reference Manual
ref.dvi: ref/ref.tex
	$(MKDVI) ref

# Python Tutorial
tut.dvi: tut/tut.tex
	$(MKDVI) tut


# The remaining part of the Makefile is concerned with various
# conversions, as described above.  See also the README file.

.PRECIOUS:	python-lib.texi

# The sed script in this target fixes a really nasty little condition in
# libcgi.tex where \e has to be used in a group to get the right behavior,
# and makeinfo can't handle a group without a leading @command.  But at
# least the info file gets generated.

lib1.texi: $(LIBFILES) texipre.dat texipost.dat tools/partparse.py
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
# perl/l2hinit.perl to use a different location for the icons directory.


l2h:
	(cd html; $(MAKE))

l2hapi: $(COMMONPERL)
	TEXINPUTS=$(TEXINPUTS) $(srcdir)/tools/mkhtml.sh api $(L2HARGS)

l2hext: $(COMMONPERL)
	TEXINPUTS=$(TEXINPUTS) $(srcdir)/tools/mkhtml.sh ext $(L2HARGS)

l2hlib: $(COMMONPERL)
	$(srcdir)/tools/fix_libaux.sed <`$(KPSEWHICH) lib.aux` >lib1.aux
	mv lib1.aux `$(KPSEWHICH) lib.aux`
	TEXINPUTS=$(TEXINPUTS) $(srcdir)/tools/mkhtml.sh lib $(L2HARGS)

l2href: $(COMMONPERL)
	TEXINPUTS=$(TEXINPUTS) $(srcdir)/tools/mkhtml.sh ref $(L2HARGS)

l2htut: $(COMMONPERL)
	TEXINPUTS=$(TEXINPUTS) $(srcdir)/tools/mkhtml.sh tut $(L2HARGS)

# webchecker needs an extra flag to process the huge index from the libref
webcheck:
	$(WEBCHECKER) file:`pwd`/api/
	$(WEBCHECKER) file:`pwd`/ext/
	$(WEBCHECKER) -m290000 file:`pwd`/lib/
	$(WEBCHECKER) file:`pwd`/ref/
	$(WEBCHECKER) file:`pwd`/tut/

lib-info-$(RELEASE).tar.gz: $(INFOFILES)
	tar cf - python-???.info* | gzip -9 >$@

latex-$(RELEASE).tar.gz:
	$(srcdir)/tools/mktarball.sh

# This snags a PDF version if available, but doesn't fail if not.
pdf-$(RELEASE).tar.gz: $(PDFFILES)
	tar cf - ???.pdf | gzip -9 >$@

postscript-$(RELEASE).tar.gz: $(PSFILES)
	tar cf - ???.ps | gzip -9 >$@

html-$(RELEASE).tar.gz:
	tar cf - index.html ???/???.css ???/*.html */*.gif \
		| gzip -9 >html-$(RELEASE).tar.gz

# convenience targets:

tarhtml:  html-$(RELEASE).tar.gz

tarinfo:  lib-info-$(RELEASE).tar.gz

tarps:	postscript-$(RELEASE).tar.gz

tarpdf:	pdf-$(RELEASE).tar.gz

tarlatex: latex-$(RELEASE).tar.gz

tarballs:  tarpdf tarps tarhtml tarlatex


# Housekeeping targets

# Remove temporary files; all except the following:
# - sources: .tex, .bib, .sty, *.cls
# - useful results: .dvi, .pdf, .ps, .texi, .info
clean:
	rm -f @* *~ *.aux *.idx *.ilg *.ind *.log *.toc *.blg *.bbl *.py[co]
	rm -f *.bak *.orig lib1.texi *.out @webchecker.pickle *.bkm
	rm -f html-$(RELEASE).tar.gz info-$(RELEASE).tar.gz
	rm -f pdf-$(RELEASE).tar.gz postscript-$(RELEASE).tar.gz
	rm -f latex-$(RELEASE).tar.gz

l2hclean:
	rm -rf api ext lib tut

# Remove temporaries as well as final products
clobber: clean l2hclean
	rm -f $(DVIFILES) $(PSFILES) $(PDFFILES)
	rm -f *.texi python-???.info python-???.info-[0-9]*

realclean:  clobber
distclean:  clobber
