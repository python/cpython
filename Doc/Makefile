# Makefile for Python documentation
# ---------------------------------
#
# See also the README file.
#
# This is a bit of a mess.  The main documents are:
#   tut -- Tutorial (file tut.tex)
#   lib -- Library Reference (file lib.tex, inputs lib*.tex)
#   ref -- Language Reference (file ref.tex, inputs ref*.tex)
#   ext -- Extending and Embedding (file ext.tex)
#
# The main target "make all" creates DVI and PostScript for these
# four.  You can also do "make lib" (etc.) to process individual
# documents.
#
# There's also:
#   qua -- Paper published in the CWI Quarterly (file qua.tex)
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
#   bibtex (only for formatting qua.tex)
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
#   l2h -- convert tut, ref, ext (but not lib!) from LaTeX to HTML
#   lib.texi -- convert lib from LaTeX to Texinfo
#   lib.info -- convert lib from Texinfo to Emacs INFO
#   libwww -- convert lib from Texinfo to HTML
# See the README file for more info on these targets.

# Customizations -- you *may* have to edit these

# Where are the various programs?
LATEX=		latex
BIBTEX=		bibtex
EMACS=		emacs
DVIPS=		dvips -f
MAKEINDEX=	makeindex
PYTHON=		python
MAKEINFO=	makeinfo
L2H=		/ufs/guido/l2h/latex2html
L2HARGS=	-address $$USER@`domainname` -dont_include myformat

# Destination directory for output of libwww target.
LIBHTMLDIR=	./python-lib

# Install destination -- not used now but might be useful some time...
DESTDIR=	/usr/local
LIBDESTDIR=	$DESTDIR/lib
LIBDEST=	$LIBDESTDIR/python
DOCDESTDIR=	$LIBDEST/doc

# Ideally, you shouldn't need to edit beyond this point

# Main target
all:	all-ps

all-dvi: tut.dvi lib.dvi ref.dvi ext.dvi
all-ps:	tut.ps lib.ps ref.ps ext.ps

# Individual document fake targets
tut:	tut.ps
lib:	lib.ps
ref:	ref.ps
ext:	ext.ps

# CWI Quarterly document fake target
qua:	qua.ps

# Dependencies
tut.dvi lib.dvi ref.dvi ext.dvi: myformat.sty fix_hack

# Tutorial document
tut.dvi: tut.tex 
	$(LATEX) tut
	$(LATEX) tut

tut.ps:	tut.dvi
	$(DVIPS) tut >tut.ps

# Reference document
ref.dvi: ref.tex ref1.tex ref2.tex ref3.tex ref4.tex ref5.tex ref6.tex \
						     ref7.tex ref8.tex 
	touch ref.ind
	$(LATEX) ref
	./fix_hack ref.idx
	$(MAKEINDEX) ref
	$(LATEX) ref

ref.ps:	ref.dvi
	$(DVIPS) ref >ref.ps

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
    libxdrlib.tex libimghdr.tex

# Library document
lib.dvi: $(LIBFILES)
	touch lib.ind
	$(LATEX) lib
	./fix_hack lib.idx
	$(MAKEINDEX) lib
	$(LATEX) lib

lib.ps:	lib.dvi
	$(DVIPS) lib >lib.ps

# Extensions document
ext.dvi: ext.tex extref.tex
	touch ext.ind
	$(LATEX) ext
	./fix_hack ext.idx
	$(MAKEINDEX) ext
	$(LATEX) ext

ext.ps:	ext.dvi
	$(DVIPS) ext >ext.ps

# Quarterly document
qua.dvi: qua.tex quabib.bib
	$(LATEX) qua
	$(BIBTEX) qua
	$(LATEX) qua
	$(BIBTEX) qua

qua.ps:	qua.dvi
	$(DVIPS) qua >qua.ps


# The remaining part of the Makefile is concerned with various
# conversions, as described above.  See also the README file.

lib.texi: lib*.tex texipre.dat texipost.dat partparse.py fix.el
	$(PYTHON) partparse.py -o @lib.texi `./whichlibs`
	$(EMACS) -batch -l fix.el -f save-buffer -kill
	mv @lib.texi lib.texi

.PRECIOUS:	lib.texi

python-lib.info: lib.texi
	-$(MAKEINFO) --footnote-style end --fill-column 72 \
		     --paragraph-indent 0 lib.texi

lib.info: python-lib.info

libwww: lib.texi
	if test ! -d $(LIBHTMLDIR); then mkdir $(LIBHTMLDIR); else true; fi
	$(PYTHON) texi2html.py -d lib.texi $(LIBHTMLDIR)

# Targets to convert the manuals to HTML using Nikos Drakos' LaTeX to
# HTML converter.  For more info on this program, see
# <URL:http://cbl.leeds.ac.uk/nikos/tex2html/doc/latex2html/latex2html.html>.
# (I've had some trouble getting this to work with the netpbm version
# of the pbmplus library; ppmtogif dumped core because giftopnm
# outputs bitmap (pbm) files.  I've fixed this by changing the source
# of LaTeX2HTML to insert a call to pnmtoppm, which I wrote myself.
# You can probably also use "pbmtopgm | pgmtoppm"...

# In order to use these targets, you must edit the definition of L2H
# earlier in the Makefile to point to the latex2html program.  Note
# that LaTeX2HTML inserts references to an "icons" directory in each
# page that it generates.  You can customize where these icons are to
# be found; I generally make it point to "../icons" and then create a
# symbolic link to the icons directory in the LaTeX2HTML source at the
# appropriate place.

l2h: l2htut l2href l2hext

l2htut: tut.dvi
	$(L2H) $(L2HARGS) tut.tex
	@rm -rf python-tut
	mv tut python-tut

l2href: ref.dvi
	$(L2H) $(L2HARGS) ref.tex
	@rm -rf python-ref
	mv ref python-ref

l2hext: ext.dvi
	$(L2H) $(L2HARGS) ext.tex
	@rm -rf python-ext
	mv ext python-ext

# This target doesn't quite work, since l2h doesn't understand the
# funcdesc and similar environments, and turns them into GIF images.
# Use the "libwww" target above instead.
l2hlib: lib.dvi
	$(L2H) $(L2HARGS) lib.tex
	@rm -rf python-lib
	mv lib python-lib


# Housekeeping targets

# Remove temporary files
clean:
	rm -f @* *~ *.aux *.idx *.ilg *.ind *.log *.toc *.blg *.bbl *.pyc
	rm -f *.bak *.orig
	# Sources: .tex, .bib, .sty
	# Useful results: .dvi, .ps, .texi, .info

# Remove temporaries as well as final products
clobber: clean
	 rm -f *.dvi *.ps *.texi *.info *.info-[0-9]*
