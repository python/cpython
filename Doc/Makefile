#
# Makefile for Python documentation
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#

# You can set these variables from the command line.
PYTHON       = python
SVNROOT      = http://svn.python.org/projects
SPHINXOPTS   =
PAPER        =
SOURCES      =
DISTVERSION  = $(shell $(PYTHON) tools/sphinxext/patchlevel.py)

ALLSPHINXOPTS = -b $(BUILDER) -d build/doctrees -D latex_paper_size=$(PAPER) \
                $(SPHINXOPTS) . build/$(BUILDER) $(SOURCES)

.PHONY: help checkout update build html htmlhelp latex text changes linkcheck \
	suspicious coverage doctest pydoc-topics htmlview clean dist check serve \
	autobuild-dev autobuild-stable

help:
	@echo "Please use \`make <target>' where <target> is one of"
	@echo "  clean      to remove build files"
	@echo "  update     to update build tools"
	@echo "  html       to make standalone HTML files"
	@echo "  htmlhelp   to make HTML files and a HTML help project"
	@echo "  latex      to make LaTeX files, you can set PAPER=a4 or PAPER=letter"
	@echo "  text       to make plain text files"
	@echo "  changes    to make an overview over all changed/added/deprecated items"
	@echo "  linkcheck  to check all external links for integrity"
	@echo "  coverage   to check documentation coverage for library and C API"
	@echo "  doctest    to run doctests in the documentation"
	@echo "  pydoc-topics  to regenerate the pydoc topics file"
	@echo "  dist       to create a \"dist\" directory with archived docs for download"
	@echo "  suspicious to check for suspicious markup in output text"
	@echo "  check      to run a check for frequent markup errors"
	@echo "  serve      to serve the documentation on the localhost (8000)"

# Note: if you update versions here, do the same in make.bat and README.txt
checkout:
	@if [ ! -d tools/sphinx ]; then \
	  echo "Checking out Sphinx..."; \
	  svn checkout $(SVNROOT)/external/Sphinx-1.0.5/sphinx tools/sphinx; \
	fi
	@if [ ! -d tools/docutils ]; then \
	  echo "Checking out Docutils..."; \
	  svn checkout $(SVNROOT)/external/docutils-0.6/docutils tools/docutils; \
	fi
	@if [ ! -d tools/jinja2 ]; then \
	  echo "Checking out Jinja..."; \
	  svn checkout $(SVNROOT)/external/Jinja-2.3.1/jinja2 tools/jinja2; \
	fi
	@if [ ! -d tools/pygments ]; then \
	  echo "Checking out Pygments..."; \
	  svn checkout $(SVNROOT)/external/Pygments-1.3.1/pygments tools/pygments; \
	fi

update: clean checkout

build: checkout
	mkdir -p build/$(BUILDER) build/doctrees
	$(PYTHON) tools/sphinx-build.py $(ALLSPHINXOPTS)
	@echo

html: BUILDER = html
html: build
	@echo "Build finished. The HTML pages are in build/html."

htmlhelp: BUILDER = htmlhelp
htmlhelp: build
	@echo "Build finished; now you can run HTML Help Workshop with the" \
	      "build/htmlhelp/pydoc.hhp project file."

latex: BUILDER = latex
latex: build
	@echo "Build finished; the LaTeX files are in build/latex."
	@echo "Run \`make all-pdf' or \`make all-ps' in that directory to" \
	      "run these through (pdf)latex."

text: BUILDER = text
text: build
	@echo "Build finished; the text files are in build/text."

changes: BUILDER = changes
changes: build
	@echo "The overview file is in build/changes."

linkcheck: BUILDER = linkcheck
linkcheck: build
	@echo "Link check complete; look for any errors in the above output" \
	      "or in build/$(BUILDER)/output.txt"

suspicious: BUILDER = suspicious
suspicious: build
	@echo "Suspicious check complete; look for any errors in the above output" \
	      "or in build/$(BUILDER)/suspicious.csv.  If all issues are false" \
	      "positives, append that file to tools/sphinxext/susp-ignored.csv."

coverage: BUILDER = coverage
coverage: build
	@echo "Coverage finished; see c.txt and python.txt in build/coverage"

doctest: BUILDER = doctest
doctest: build
	@echo "Testing of doctests in the sources finished, look at the" \
	      "results in build/doctest/output.txt"

pydoc-topics: BUILDER = pydoc-topics
pydoc-topics: build
	@echo "Building finished; now copy build/pydoc-topics/topics.py" \
	      "to Lib/pydoc_data/topics.py"

htmlview: html
	 $(PYTHON) -c "import webbrowser; webbrowser.open('build/html/index.html')"

clean:
	-rm -rf build/*
	-rm -rf tools/sphinx
	-rm -rf tools/pygments
	-rm -rf tools/jinja2
	-rm -rf tools/docutils

dist:
	rm -rf dist
	mkdir -p dist

	# archive the HTML
	make html
	cp -pPR build/html dist/python-$(DISTVERSION)-docs-html
	tar -C dist -cf dist/python-$(DISTVERSION)-docs-html.tar python-$(DISTVERSION)-docs-html
	bzip2 -9 -k dist/python-$(DISTVERSION)-docs-html.tar
	(cd dist; zip -q -r -9 python-$(DISTVERSION)-docs-html.zip python-$(DISTVERSION)-docs-html)
	rm -r dist/python-$(DISTVERSION)-docs-html
	rm dist/python-$(DISTVERSION)-docs-html.tar

	# archive the text build
	make text
	cp -pPR build/text dist/python-$(DISTVERSION)-docs-text
	tar -C dist -cf dist/python-$(DISTVERSION)-docs-text.tar python-$(DISTVERSION)-docs-text
	bzip2 -9 -k dist/python-$(DISTVERSION)-docs-text.tar
	(cd dist; zip -q -r -9 python-$(DISTVERSION)-docs-text.zip python-$(DISTVERSION)-docs-text)
	rm -r dist/python-$(DISTVERSION)-docs-text
	rm dist/python-$(DISTVERSION)-docs-text.tar

	# archive the A4 latex
	rm -rf build/latex
	make latex PAPER=a4
	-sed -i 's/makeindex/makeindex -q/' build/latex/Makefile
	(cd build/latex; make clean && make all-pdf && make FMT=pdf zip bz2)
	cp build/latex/docs-pdf.zip dist/python-$(DISTVERSION)-docs-pdf-a4.zip
	cp build/latex/docs-pdf.tar.bz2 dist/python-$(DISTVERSION)-docs-pdf-a4.tar.bz2

	# archive the letter latex
	rm -rf build/latex
	make latex PAPER=letter
	-sed -i 's/makeindex/makeindex -q/' build/latex/Makefile
	(cd build/latex; make clean && make all-pdf && make FMT=pdf zip bz2)
	cp build/latex/docs-pdf.zip dist/python-$(DISTVERSION)-docs-pdf-letter.zip
	cp build/latex/docs-pdf.tar.bz2 dist/python-$(DISTVERSION)-docs-pdf-letter.tar.bz2

check:
	$(PYTHON) tools/rstlint.py -i tools

serve:
	../Tools/scripts/serve.py build/html

# Targets for daily automated doc build

# for development releases: always build
autobuild-dev:
	make update
	make dist SPHINXOPTS='-A daily=1'

# for stable releases: only build if not in pre-release stage (alpha, beta, rc)
autobuild-stable:
	@case $(DISTVERSION) in *[abc]*) \
		echo "Not building; $(DISTVERSION) is not a release version."; \
		exit 1;; \
	esac
	@make autobuild-dev
