# Makefile for Python documentation
# ---------------------------------
#
# See also the README file.
#
# This is a bit of a mess.  The main documents are:
#   api -- Python/C API Reference Manual
#   ext -- Extending and Embedding the Python Interpreter
#   lib -- Library Reference Manual
#   mac -- Macintosh Library Modules
#   ref -- Python Reference Manual
#   tut -- Python Tutorial
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

# Customizations -- you *may* have to edit these

# you could set this to a4
PAPER=letter

# Ideally, you shouldn't need to edit beyond this point

HTMLDIR=	html
INFODIR=	info
TOOLSDIR=	tools

RELEASE=1.5.2b1
VERSION=1.5

# These must be declared phony since there
# are directories with matching names:
.PHONY: api ext lib mac ref tut
.PHONY: html info


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


api.dvi:
	(cd paper-$(PAPER); $(MAKE) api.dvi)

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


api.pdf:
	(cd paper-$(PAPER); $(MAKE) api.pdf)

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


# webchecker needs an extra flag to process the huge index from the libref
webcheck:
	(cd $(HTMLDIR); $(MAKE) -f ../html/Makefile webcheck)


# Release packaging targets:

info-$(RELEASE).tgz: info
	(cd $(INFODIR); tar cf - python-???.info*) | gzip -9 >$@

latex-$(RELEASE).tgz:
	$(TOOLSDIR)/mktarball.sh $(RELEASE)

pdf-$(PAPER)-$(RELEASE).tgz: pdf
	(cd paper-$(PAPER); tar cf - *.pdf) | gzip -9 >$@

postscript-$(PAPER)-$(RELEASE).tgz: ps
	(cd paper-$(PAPER); $(MAKE) README)
	(cd paper-$(PAPER); tar cf - *.ps README) | gzip -9 >$@

html-$(RELEASE).tgz:	html
	(cd $(HTMLDIR); tar cf - index.html ???/???.css ???/*.html */*.gif) \
		| gzip -9 >$@

# convenience targets:

tarhtml:	html-$(RELEASE).tgz
tarinfo:	info-$(RELEASE).tgz
tarps:		postscript-$(PAPER)-$(RELEASE).tgz
tarpdf:		pdf-$(PAPER)-$(RELEASE).tgz
tarlatex:	latex-$(RELEASE).tgz

tarballs:	tarpdf tarps tarhtml tarlatex


# Housekeeping targets

# Remove temporary files; all except the following:
# - sources: .tex, .bib, .sty, *.cls
# - useful results: .dvi, .pdf, .ps, .texi, .info
clean:
	(cd paper-$(PAPER); $(MAKE) clean)
	(cd $(HTMLDIR); $(MAKE) clean)
	(cd $(INFODIR); $(MAKE) clean)

l2hclean:
	(cd $(HTMLDIR); $(MAKE) clean)

# Remove temporaries as well as final products
clobber:
	(cd $(HTMLDIR); $(MAKE) clobber)
	rm -f html-$(RELEASE).tgz info-$(RELEASE).tgz
	rm -f pdf-$(RELEASE).tgz postscript-$(RELEASE).tgz
	rm -f latex-$(RELEASE).tgz
	(cd paper-$(PAPER); $(MAKE) clobber)
	(cd $(HTMLDIR); $(MAKE) clobber)
	(cd $(INFODIR); $(MAKE) clobber)

realclean:  clobber
distclean:  clobber
