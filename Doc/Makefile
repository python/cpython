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

# Install destination -- not used now but might be useful some time...
DESTDIR=	/usr/local
LIBDESTDIR=	$DESTDIR/lib
LIBDEST=	$LIBDESTDIR/python
DOCDESTDIR=	$LIBDEST/doc

# Ideally, you shouldn't need to edit beyond this point

# Main target
all:	tut.dvi lib.dvi ref.dvi ext.dvi

# Individual document fake targets
tut:	tut.dvi
lib:	lib.dvi
ref:	ref.dvi
ext:	ext.dvi

# CWI Quarterly document fake target
qua:	qua.dvi

# Dependencies
tut.dvi lib.dvi ref.dvi ext.dvi: myformat.sty fix_hack

# Tutorial document
tut.dvi: tut.tex 
	$(LATEX) tut
	$(LATEX) tut
	$(DVIPS) tut >tut.ps

# Reference document
ref.dvi: ref.tex ref1.tex ref2.tex ref3.tex ref4.tex ref5.tex ref6.tex \
						     ref7.tex ref8.tex 
	touch ref.ind
	$(LATEX) ref
	./fix_hack ref.idx
	$(MAKEINDEX) ref
	$(LATEX) ref
	$(DVIPS) ref >ref.ps

# LaTeX source files for the Python Library Reference
LIBFILES = lib.tex \
libal.tex libaifc.tex libamoeba.tex libarray.tex libaudio.tex libaudioop.tex \
libbltin.tex \
libcgi.tex libcopy.tex libctb.tex libcrypto.tex \
libdbm.tex \
libexcs.tex \
libfcntl.tex libfl.tex libfm.tex libftplib.tex libfuncs.tex \
libgdbm.tex libgetopt.tex libgl.tex libgopherlib.tex libgrp.tex \
libhtmllib.tex libhttplib.tex \
libimageop.tex libimgfile.tex libintro.tex \
libjpeg.tex \
libmac.tex libmacconsole.tex libmacdnr.tex \
	libmacfs.tex libmactcp.tex libmacspeech.tex \
	libmain.tex libmarshal.tex libmath.tex \
	libmd5.tex libmimetools.tex libmm.tex libmods.tex libmpz.tex \
libnntplib.tex \
libobjs.tex libos.tex \
libpanel.tex libpickle.tex libposix.tex libposixfile.tex \
	libppath.tex libprofile.tex libpwd.tex \
librand.tex libregex.tex libregsub.tex \
	librfc822.tex librgbimg.tex librotor.tex \
libselect.tex libsgi.tex libsgmllib.tex \
	libshelve.tex libsocket.tex libstd.tex libstdwin.tex \
	libstring.tex libstruct.tex libsun.tex libsys.tex \
libtempfile.tex libthread.tex libtime.tex \
	libtraceback.tex libtypes.tex libtypes2.tex \
libunix.tex liburllib.tex liburlparse.tex \
libwhrandom.tex libwww.tex

# Library document
lib.dvi: $(LIBFILES)
	touch lib.ind
	$(LATEX) lib
	./fix_hack lib.idx
	$(MAKEINDEX) lib
	$(LATEX) lib
	$(DVIPS) lib >lib.ps

# Extensions document
ext.dvi: ext.tex 
	touch ext.ind
	$(LATEX) ext
	./fix_hack ext.idx
	$(MAKEINDEX) ext
	$(LATEX) ext
	$(DVIPS) ext >ext.ps

# Quarterly document
qua.dvi: qua.tex quabib.bib
	$(LATEX) qua
	$(BIBTEX) qua
	$(LATEX) qua
	$(BIBTEX) qua
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

# This target is very local to CWI...
libwww: lib.texi
	$(PYTHON) texi2html.py -d lib.texi /ufs/guido/public_html/python-lib

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

l2htut: tut
	$(L2H) $(L2HARGS) tut.tex
	@rm -rf python-tut
	mv tut python-tut

l2href: ref
	$(L2H) $(L2HARGS) ref.tex
	@rm -rf python-ref
	mv ref python-ref

l2hext: ext
	$(L2H) $(L2HARGS) ext.tex
	@rm -rf python-ext
	mv ext python-ext

# This target doesn't quite work, since l2h doesn't understand the
# funcdesc and similar environments, and turns them into GIF images.
# Use the "libwww" target above instead.
l2hlib: lib
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
