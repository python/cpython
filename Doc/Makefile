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
DVIPS=		dvips -f
MAKEINDEX=	makeindex
L2H=		latex2html
L2HARGS=	-address $$USER@`domainname`

# Install destination -- not used now but might be useful some time...
DESTDIR=	/usr/local
LIBDESTDIR=	$DESTDIR/lib
LIBDEST=	$LIBDESTDIR/python
DOCDESTDIR=	$LIBDEST/doc

# Ideally, you shouldn't need to edit beyond this point

# Main target
all:	all-ps

all-dvi: tut.dvi lib.dvi ext.dvi api.dvi
all-ps:	tut.ps lib.ps ext.ps api.ps

# Individual document fake targets
tut:	tut.ps
lib:	lib.ps
ext:	ext.ps
api:	api.ps

# Dependencies
tut.dvi lib.dvi ext.dvi api.dvi: myformat.sty fix_hack

# Tutorial document
tut.dvi: tut.tex 
	$(LATEX) tut
	$(LATEX) tut

tut.ps:	tut.dvi
	$(DVIPS) tut >tut.ps

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
    libpprint.tex libcode.tex

# Library document
lib.dvi: $(LIBFILES)
	touch lib.ind
	$(LATEX) lib
	./fix_hack lib.idx
	$(MAKEINDEX) lib.idx
	$(LATEX) lib

lib.ps:	lib.dvi
	$(DVIPS) lib >lib.ps

# Extensions document
ext.dvi: ext.tex
	touch ext.ind
	$(LATEX) ext
	./fix_hack ext.idx
	$(MAKEINDEX) ext.idx
	$(LATEX) ext

ext.ps:	ext.dvi
	$(DVIPS) ext >ext.ps

# Python-C API document
api.dvi: api.tex 
	touch api.ind
	$(LATEX) api
	./fix_hack api.idx
	$(MAKEINDEX) api.idx
	$(LATEX) api

api.ps:	api.dvi
	$(DVIPS) api >api.ps


# The remaining part of the Makefile is concerned with various
# conversions, as described above.  See also the README file.

# Targets to convert the manuals to HTML using Nikos Drakos' LaTeX to
# HTML converter.  For more info on this program, see
# <URL:http://cbl.leeds.ac.uk/nikos/tex2html/doc/latex2html/latex2html.html>.

# Note that LaTeX2HTML inserts references to an "icons" directory in
# each page that it generates.  You can customize where these icons
# are to be found; I generally make it point to "../icons" and then
# create a symbolic link to the icons directory in the LaTeX2HTML
# source at the appropriate place.  Change the definition of
# $ICONSERVER in .latex2html-init to point to a different location.

# The sed hack rips out a superfluous comma which I haven't found the source
# of; the prominent location makes it worth the extra step.  This affects the
# title pages!

l2h: l2htut l2hext l2hlib l2hapi

l2htut: tut.dvi myformat.perl
	$(L2H) $(L2HARGS) tut.tex
	@rm -rf python-tut
	sed 's/^<P CLASS=ABSTRACT>,/<P CLASS=ABSTRACT>/' \
		<tut/tut.html >tut/xxx
	mv tut/xxx tut/tut.html
	ln -s tut.html tut/index.html
	mv tut python-tut

l2hext: ext.dvi myformat.perl
	$(L2H) $(L2HARGS) ext.tex
	@rm -rf python-ext
	sed 's/^<P CLASS=ABSTRACT>,/<P CLASS=ABSTRACT>/' \
		<ext/ext.html >ext/xxx
	ln -s ext.html ext/index.html
	mv ext/xxx ext/ext.html
	mv ext python-ext

l2hlib: lib.dvi myformat.perl
	./fix_libaux.sed <lib.aux >@lib.aux
	mv @lib.aux lib.aux
	$(L2H) $(L2HARGS) lib.tex
	sed 's/^<P CLASS=ABSTRACT>,/<P CLASS=ABSTRACT>/' \
		<lib/lib.html >lib/xxx
	mv lib/xxx lib/lib.html
	ln -s lib.html lib/index.html
	@rm -rf python-lib
	mv lib python-lib

l2hapi: api.dvi myformat.perl
	$(L2H) $(L2HARGS) api.tex
	@rm -rf python-api
	sed 's/^<P CLASS=ABSTRACT>,/<P CLASS=ABSTRACT>/' \
		<api/api.html >api/xxx
	ln -s api.html api/index.html
	mv api/xxx api/api.html
	mv api python-api


# Housekeeping targets

# Remove temporary files; all except the following:
# - sources: .tex, .bib, .sty
# - useful results: .dvi, .ps, .texi, .info
clean:
	rm -f @* *~ *.aux *.idx *.ilg *.ind *.log *.toc *.blg *.bbl *.pyc
	rm -f *.bak *.orig

# Remove temporaries as well as final products
clobber: clean
	 rm -f *.dvi *.ps *.texi *.info *.info-[0-9]*
