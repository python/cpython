#
# Makefile for Python documentation
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#

# You can set these variables from the command line.
PYTHON       = python
SPHINXBUILD  = sphinx-build
PAPER        =
SOURCES      =
DISTVERSION  = $(shell $(PYTHON) tools/extensions/patchlevel.py)

ALLSPHINXOPTS = -b $(BUILDER) -d build/doctrees -D latex_paper_size=$(PAPER) \
                $(SPHINXOPTS) . build/$(BUILDER) $(SOURCES)

.PHONY: help build html htmlhelp latex text changes linkcheck \
	suspicious coverage doctest pydoc-topics htmlview clean dist check serve \
	autobuild-dev autobuild-stable venv

help:
	@echo "Please use \`make <target>' where <target> is one of"
	@echo "  clean      to remove build files"
	@echo "  venv       to create a venv with necessary tools"
	@echo "  html       to make standalone HTML files"
	@echo "  htmlview   to open the index page built by the html target in your browser"
	@echo "  htmlhelp   to make HTML files and a HTML help project"
	@echo "  latex      to make LaTeX files, you can set PAPER=a4 or PAPER=letter"
	@echo "  text       to make plain text files"
	@echo "  epub       to make EPUB files"
	@echo "  changes    to make an overview over all changed/added/deprecated items"
	@echo "  linkcheck  to check all external links for integrity"
	@echo "  coverage   to check documentation coverage for library and C API"
	@echo "  doctest    to run doctests in the documentation"
	@echo "  pydoc-topics  to regenerate the pydoc topics file"
	@echo "  dist       to create a \"dist\" directory with archived docs for download"
	@echo "  suspicious to check for suspicious markup in output text"
	@echo "  check      to run a check for frequent markup errors"
	@echo "  serve      to serve the documentation on the localhost (8000)"

build:
	$(SPHINXBUILD) $(ALLSPHINXOPTS)
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

epub: BUILDER = epub
epub: build
	@echo "Build finished; the epub files are in build/epub."

changes: BUILDER = changes
changes: build
	@echo "The overview file is in build/changes."

linkcheck: BUILDER = linkcheck
linkcheck:
	@$(MAKE) build BUILDER=$(BUILDER) || { \
	echo "Link check complete; look for any errors in the above output" \
	     "or in build/$(BUILDER)/output.txt"; \
	false; }

suspicious: BUILDER = suspicious
suspicious:
	@$(MAKE) build BUILDER=$(BUILDER) || { \
	echo "Suspicious check complete; look for any errors in the above output" \
	     "or in build/$(BUILDER)/suspicious.csv.  If all issues are false" \
	     "positives, append that file to tools/susp-ignored.csv."; \
	false; }

coverage: BUILDER = coverage
coverage: build
	@echo "Coverage finished; see c.txt and python.txt in build/coverage"

doctest: BUILDER = doctest
doctest:
	@$(MAKE) build BUILDER=$(BUILDER) || { \
	echo "Testing of doctests in the sources finished, look at the" \
	     "results in build/doctest/output.txt"; \
	false; }

pydoc-topics: BUILDER = pydoc-topics
pydoc-topics: build
	@echo "Building finished; now run this:" \
	      "cp build/pydoc-topics/topics.py ../Lib/pydoc_data/topics.py"

htmlview: html
	 $(PYTHON) -c "import webbrowser; webbrowser.open('build/html/index.html')"

clean:
	-rm -rf build/* venv/*

venv:
	$(PYTHON) -m venv venv
	./venv/bin/python3 -m pip install -U Sphinx

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

	# copy the epub build
	rm -rf build/epub
	make epub
	cp -pPR build/epub/Python.epub dist/python-$(DISTVERSION)-docs.epub

check:
	$(PYTHON) tools/rstlint.py -i tools

serve:
	../Tools/scripts/serve.py build/html

# Targets for daily automated doc build

# for development releases: always build
autobuild-dev:
	make dist SPHINXOPTS='-A daily=1 -A versionswitcher=1'
	-make suspicious

# for quick rebuilds (HTML only)
autobuild-html:
	make html SPHINXOPTS='-A daily=1 -A versionswitcher=1'

# for stable releases: only build if not in pre-release stage (alpha, beta)
# release candidate downloads are okay, since the stable tree can be in that stage
autobuild-stable:
	@case $(DISTVERSION) in *[ab]*) \
		echo "Not building; $(DISTVERSION) is not a release version."; \
		exit 1;; \
	esac
	@make autobuild-dev
