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
# There's one local style file: myformat.sty.  This defines a number
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
BIBTEX=		bibtex
DVIPS=		dvips -f -N0
DISTILL=	distill
MAKEINDEX=	makeindex
L2H=		latex2html
L2HARGS=	-address $$LOGNAME@`domainname`

# Install destination -- not used now but might be useful some time...
DESTDIR=	/usr/local
LIBDESTDIR=	$DESTDIR/lib
LIBDEST=	$LIBDESTDIR/python
DOCDESTDIR=	$LIBDEST/doc

# This is only used for .info generation:
EMACS=		emacs
PYTHON=		python
MAKEINFO=	makeinfo
PARTPARSE=	$(PYTHON) ./partparse.pyc

# Ideally, you shouldn't need to edit beyond this point

VERSION=1.5

# Main target
all:	all-ps

all-dvi: tut.dvi lib.dvi ext.dvi api.dvi
all-ps:	 tut.ps lib.ps ext.ps api.ps
all-pdf: tut.pdf lib.pdf ext.pdf api.pdf

# Individual document fake targets
tut:	tut.ps
lib:	lib.ps
ext:	ext.ps
api:	api.ps

# Rules to build PostScript and PDF formats
.SUFFIXES: .dvi .ps .pdf

.dvi.ps:
	$(DVIPS) $< >$@

.ps.pdf:
	$(DISTILL) $<

# Dependencies
tut.dvi lib.dvi ext.dvi api.dvi: myformat.sty fix_hack

# Tutorial document
tut.dvi: tut.tex 
	$(LATEX) tut
	$(LATEX) tut

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
    liblocale.tex libbasehttp.tex libcopyreg.tex

# Library document
lib.dvi: modindex.py $(LIBFILES)
	touch lib.ind
	touch modules.ind
	$(LATEX) lib
	./modindex.py modules.idx
	./fix_hack lib.idx
	$(MAKEINDEX) lib.idx
	./modindex.py modules.idx
	$(LATEX) lib

# Extensions document
ext.dvi: ext.tex
	touch ext.ind
	$(LATEX) ext
	./fix_hack ext.idx
	$(MAKEINDEX) ext.idx
	$(LATEX) ext

# Python-C API document
api.dvi: api.tex 
	touch api.ind
	$(LATEX) api
	./fix_hack api.idx
	$(MAKEINDEX) api.idx
	$(LATEX) api


# The remaining part of the Makefile is concerned with various
# conversions, as described above.  See also the README file.

.SUFFIXES: .py .pyc

.py.pyc:
	$(PYTHON) -c "import $*"

.PRECIOUS:	lib.texi

# The sed script in this target fixes a really nasty little condition in
# libcgi.tex where \e has to be used in a group to get the right behavior,
# and makeinfo can't handle a group with a leading @command.  But at least
# the info file gets generated.

lib1.texi: lib*.tex texipre.dat texipost.dat partparse.pyc
	$(PARTPARSE) -o lib1.texi `./whichlibs`
	sed 's/"{\\}n{\\}n/"\\n\\n/' lib1.texi >lib2.texi
	mv lib2.texi lib1.texi

lib.texi: lib1.texi fix.el
	$(EMACS) -batch -l fix.el -f save-buffer -kill
	cp lib1.texi lib.texi

python-lib.info: lib.texi
	-$(MAKEINFO) --footnote-style end --fill-column 72 \
		     --paragraph-indent 0 lib.texi

# this is needed to prevent a second set of info files from being generated,
# at least when using GNU make
.PHONY: lib.info

lib.info: python-lib.info

# Targets to convert the manuals to HTML using Nikos Drakos' LaTeX to
# HTML converter.  For more info on this program, see
# <URL:http://cbl.leeds.ac.uk/nikos/tex2html/doc/latex2html/latex2html.html>.

# Note that LaTeX2HTML inserts references to an icons directory in
# each page that it generates.  I have placed a copy of this directory
# in the distribution to simplify the process of creating a
# self-contained HTML distribution; for this purpose I have also added
# a (trivial) index.html.  Change the definition of $ICONSERVER in
# .latex2html-init to use a different location for the icons directory.

# The sed hack rips out a superfluous comma which I haven't found the
# source of.  The prominent location makes it worth the extra step;
# this affects the title pages!

l2h: l2htut l2hext l2hlib l2hapi

l2htut: tut.dvi myformat.perl
	$(L2H) $(L2HARGS) tut.tex
	sed 's/^<P CLASS=ABSTRACT>,/<P CLASS=ABSTRACT>/' \
		<tut/tut.html >tut/xxx
	mv tut/xxx tut/tut.html
	ln -s tut.html tut/index.html || true

l2hext: ext.dvi myformat.perl
	$(L2H) $(L2HARGS) ext.tex
	sed 's/^<P CLASS=ABSTRACT>,/<P CLASS=ABSTRACT>/' \
		<ext/ext.html >ext/xxx
	mv ext/xxx ext/ext.html
	ln -s ext.html ext/index.html || true

l2hlib: lib.dvi myformat.perl
	./fix_libaux.sed <lib.aux >lib1.aux
	mv lib1.aux lib.aux
	$(L2H) $(L2HARGS) lib.tex
	sed 's/^<P CLASS=ABSTRACT>,/<P CLASS=ABSTRACT>/' \
		<lib/lib.html >lib/xxx
	mv lib/xxx lib/lib.html
	ln -s lib.html lib/index.html || true

l2hapi: api.dvi myformat.perl
	$(L2H) $(L2HARGS) api.tex
	sed 's/^<P CLASS=ABSTRACT>,/<P CLASS=ABSTRACT>/' \
		<api/api.html >api/xxx
	mv api/xxx api/api.html
	ln -s api.html api/index.html || true

tarhtml:
	@echo "Did you remember to run makeMIFs.py in the ref subdirectory...?"
	tar cf - index.html ???/???.css ???/*.html lib/*.gif icons/*.* \
		| gzip -9 >html-$(VERSION).tar.gz

tarps:	all-ps
	cp ref/ref.ps .
	tar cf - ???.ps | gzip -9 >postscript-$(VERSION).tar.gz
	rm ref.ps

tarballs:  tarps tarhtml


# Housekeeping targets

# Remove temporary files; all except the following:
# - sources: .tex, .bib, .sty
# - useful results: .dvi, .ps, .texi, .info
clean:	l2hclean
	rm -f @* *~ *.aux *.idx *.ilg *.ind *.log *.toc *.blg *.bbl *.pyc
	rm -f *.texi *.info*
	rm -f *.bak *.orig
	rm -f html-$(VERSION).tar.gz postscript-$(VERSION).tar.gz

l2hclean:
	rm -rf api ext lib tut

# Remove temporaries as well as final products
clobber: clean
	 rm -f *.dvi *.ps *.texi *.info *.info-[0-9]*
