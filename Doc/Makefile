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
# The latex sources for each of these documents are in subdirectories
# with the three-letter designations above as the directory names.
#
# The main target creates DVI and PostScript for the main each of the
# documents.  You can also do "make lib" (etc.) to create the DVI and
# PostScript versions of individual documents.
#
# The document classes and styles are in the texinputs/ directory.
# These define a number of macros that are similar in name and intent
# as macros in Texinfo (e.g. \code{...} and \emph{...}), as well as a
# number of environments for formatting function and data definitions.
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
#   html -- convert all documents from LaTeX to HTML
#   pdf  -- convert all documents from LaTeX to the
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

# Customizations -- you *may* have to edit these

# you could set this to a4
PAPER=letter

# Ideally, you shouldn't need to edit beyond this point

HTMLDIR=	html
INFODIR=	info
TOOLSDIR=	tools

# This is the *documentation* release, and is used to construct the file
# names of the downloadable tarballs.
RELEASE=2.0


# These must be declared phony since there
# are directories with matching names:
.PHONY: api doc ext lib mac ref tut inst dist
.PHONY: html info longhtml


# Main target
all:	ps

dvi:
	(cd paper-$(PAPER); $(MAKE) dvi)

pdf:
	(cd paper-$(PAPER); $(MAKE) pdf)

ps:
	(cd paper-$(PAPER); $(MAKE) ps)

world:	ps pdf html tarballs


# Targets for each document:
api api.ps:
	(cd paper-$(PAPER); $(MAKE) api.ps)

doc doc.ps:
	(cd paper-$(PAPER); $(MAKE) doc.ps)

ext ext.ps:
	(cd paper-$(PAPER); $(MAKE) ext.ps)

lib lib.ps:
	(cd paper-$(PAPER); $(MAKE) lib.ps)

mac mac.ps:
	(cd paper-$(PAPER); $(MAKE) mac.ps)

ref ref.ps:
	(cd paper-$(PAPER); $(MAKE) ref.ps)

tut tut.ps:
	(cd paper-$(PAPER); $(MAKE) tut.ps)

inst inst.ps:
	(cd paper-$(PAPER); $(MAKE) inst.ps)

dist dist.ps:
	(cd paper-$(PAPER); $(MAKE) dist.ps)


api.dvi:
	(cd paper-$(PAPER); $(MAKE) api.dvi)

doc.dvi:
	(cd paper-$(PAPER); $(MAKE) doc.dvi)

ext.dvi:
	(cd paper-$(PAPER); $(MAKE) ext.dvi)

lib.dvi:
	(cd paper-$(PAPER); $(MAKE) lib.dvi)

mac.dvi:
	(cd paper-$(PAPER); $(MAKE) mac.dvi)

ref.dvi:
	(cd paper-$(PAPER); $(MAKE) ref.dvi)

tut.dvi:
	(cd paper-$(PAPER); $(MAKE) tut.dvi)

inst.dvi:
	(cd paper-$(PAPER); $(MAKE) inst.dvi)

dist.dvi:
	(cd paper-$(PAPER); $(MAKE) dist.dvi)


api.pdf:
	(cd paper-$(PAPER); $(MAKE) api.pdf)

doc.pdf:
	(cd paper-$(PAPER); $(MAKE) doc.pdf)

ext.pdf:
	(cd paper-$(PAPER); $(MAKE) ext.pdf)

lib.pdf:
	(cd paper-$(PAPER); $(MAKE) lib.pdf)

mac.pdf:
	(cd paper-$(PAPER); $(MAKE) mac.pdf)

ref.pdf:
	(cd paper-$(PAPER); $(MAKE) ref.pdf)

tut.pdf:
	(cd paper-$(PAPER); $(MAKE) tut.pdf)

inst.pdf:
	(cd paper-$(PAPER); $(MAKE) inst.pdf)

dist.pdf:
	(cd paper-$(PAPER); $(MAKE) dist.pdf)


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

html:
	(cd $(HTMLDIR); $(MAKE) PAPER=$(PAPER) -f ../html/Makefile)

htmlapi:
	(cd $(HTMLDIR); $(MAKE) PAPER=$(PAPER) -f ../html/Makefile api)

htmldoc:
	(cd $(HTMLDIR); $(MAKE) PAPER=$(PAPER) -f ../html/Makefile doc)

htmlext:
	(cd $(HTMLDIR); $(MAKE) PAPER=$(PAPER) -f ../html/Makefile ext)

htmllib:
	(cd $(HTMLDIR); $(MAKE) PAPER=$(PAPER) -f ../html/Makefile lib)

htmlmac:
	(cd $(HTMLDIR); $(MAKE) PAPER=$(PAPER) -f ../html/Makefile mac)

htmlref:
	(cd $(HTMLDIR); $(MAKE) PAPER=$(PAPER) -f ../html/Makefile ref)

htmltut:
	(cd $(HTMLDIR); $(MAKE) PAPER=$(PAPER) -f ../html/Makefile tut)

htmlinst:
	(cd $(HTMLDIR); $(MAKE) PAPER=$(PAPER) -f ../html/Makefile inst)

htmldist:
	(cd $(HTMLDIR); $(MAKE) PAPER=$(PAPER) -f ../html/Makefile dist)

longhtml:
	(cd longhtml; $(MAKE) PAPER=$(PAPER))


# webchecker needs an extra flag to process the huge index from the libref
webcheck:
	(cd $(HTMLDIR); $(MAKE) -f ../html/Makefile webcheck)


# Release packaging targets:

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

postscript-$(PAPER)-$(RELEASE).tar.bz2: ps
	(cd paper-$(PAPER); $(MAKE) README)
	(cd paper-$(PAPER); tar cf - *.ps README) | bzip2 -9 >$@

postscript-$(PAPER)-$(RELEASE).tgz: ps
	(cd paper-$(PAPER); $(MAKE) README)
	(cd paper-$(PAPER); tar cf - *.ps README) | gzip -9 >$@

postscript-$(PAPER)-$(RELEASE).zip: ps
	(cd paper-$(PAPER); $(MAKE) README)
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

longhtml-$(RELEASE).zip:	longhtml
	rm -f $@
	(cd longhtml; \
		zip -q -9 ../$@ */*.css */*.html */*.gif)

# convenience targets:

tarhtml:	html-$(RELEASE).tgz
tarinfo:	info-$(RELEASE).tgz
tarps:		postscript-$(PAPER)-$(RELEASE).tgz
tarpdf:		pdf-$(PAPER)-$(RELEASE).tgz
tarlatex:	latex-$(RELEASE).tgz

tarballs:	tarpdf tarps tarhtml

ziphtml:	html-$(RELEASE).zip
ziplonghtml:	longhtml-$(RELEASE).zip
zipps:		postscript-$(PAPER)-$(RELEASE).zip
zippdf:		pdf-$(PAPER)-$(RELEASE).zip
ziplatex:	latex-$(RELEASE).zip

# Only build the longhtml version for final releases; use the second
# version of this for pre-release versions.
#
#zips:		zippdf zipps ziphtml ziplonghtml
zips:		zippdf zipps ziphtml

bziphtml:	html-$(RELEASE).tar.bz2
bzipinfo:	info-$(RELEASE).tar.bz2
bzipps:		postscript-$(PAPER)-$(RELEASE).tar.bz2
bzippdf:	pdf-$(PAPER)-$(RELEASE).tar.bz2
bziplatex:	latex-$(RELEASE).tar.bz2

bzips:		bzippdf bzipps bziphtml

distfiles:	tarballs zips bzips
	$(TOOLSDIR)/mksourcepkg --all $(RELEASE)


# Housekeeping targets

# Remove temporary files; all except the following:
# - sources: .tex, .bib, .sty, *.cls
# - useful results: .dvi, .pdf, .ps, .texi, .info
clean:
	(cd paper-$(PAPER); $(MAKE) clean)
	(cd longhtml; $(MAKE) clean)
	(cd $(HTMLDIR); $(MAKE) clean)
	(cd $(INFODIR); $(MAKE) clean)

l2hclean:
	(cd $(HTMLDIR); $(MAKE) clean)

# Remove temporaries as well as final products
clobber:
	(cd $(HTMLDIR); $(MAKE) clobber)
	rm -f html-$(RELEASE).tgz info-$(RELEASE).tgz
	rm -f pdf-$(RELEASE).tgz postscript-$(RELEASE).tgz
	rm -f latex-$(RELEASE).tgz html-$(RELEASE).zip
	rm -f pdf-$(RELEASE).zip postscript-$(RELEASE).zip
	(cd paper-$(PAPER); $(MAKE) clobber)
	(cd longhtml; $(MAKE) clobber)
	(cd $(HTMLDIR); $(MAKE) clobber)
	(cd $(INFODIR); $(MAKE) clobber)

realclean:  clobber
distclean:  clobber
