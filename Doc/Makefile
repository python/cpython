DESTDIR=/usr/local
LIBDESTDIR=$DESTDIR/lib
LIBDEST=$LIBDESTDIR/python
DOCDESTDIR=$LIBDEST/doc
DVIPS=	dvips -f

all:	tut.dvi lib.dvi ref.dvi ext.dvi

ext:	ext.dvi
tut:	tut.dvi
qua:	qua.dvi
ref:	ref.dvi
lib:	lib.dvi

tut.dvi lib.dvi ref.dvi ext.dvi: myformat.sty fix_hack

tut.dvi: tut.tex 
	latex tut
	latex tut
	$(DVIPS) tut >tut.ps

ref.dvi: ref.tex ref1.tex ref2.tex ref3.tex ref4.tex ref5.tex ref6.tex \
						     ref7.tex ref8.tex 
	touch ref.ind
	latex ref
	./fix_hack ref.idx
	makeindex ref
	latex ref
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
libmac.tex libmacconsole.tex libmacfs.tex libmactcp.tex libmacspeech.tex \
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

lib.dvi: $(LIBFILES)
	touch lib.ind
	latex lib
	./fix_hack lib.idx
	makeindex lib
	latex lib
	$(DVIPS) lib >lib.ps

ext.dvi: ext.tex 
	touch ext.ind
	latex ext
	./fix_hack ext.idx
	makeindex ext
	latex ext
	$(DVIPS) ext >ext.ps

qua.dvi: qua.tex quabib.bib
	latex qua
	bibtex qua
	latex qua
	latex qua
	$(DVIPS) qua >qua.ps

lib.texi: lib*.tex texipre.dat texipost.dat partparse.py fix.el
	python partparse.py -o @lib.texi `./whichlibs`
	emacs -batch -l fix.el -f save-buffer -kill
	mv @lib.texi lib.texi

.PRECIOUS:	lib.texi

python-lib.info: lib.texi
	makeinfo --footnote-style end --fill-column 72 --paragraph-indent 0 \
	         lib.texi

lib.info: python-lib.info

# This target is very local to CWI...
libwww: lib.texi
	texi2html -d lib.texi /ufs/guido/www/texinfo/python

# This one too...
L2H=	/usr/local/LaTeX2html/latex2html
L2HARGS=-address $$USER@`domainname` -dont_include myformat -nolatex
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

clean:
	rm -f @* *~ *.aux *.idx *.ilg *.ind *.log *.toc *.blg *.bbl *.pyc
	# Sources: .tex, .bib, .sty
	# Useful results: .dvi, .ps, .texi, .info

clobber: clean
	 rm -f *.dvi *.ps *.texi *.info *.info-[0-9]*
