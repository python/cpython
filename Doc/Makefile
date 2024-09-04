#
# Makefile for Python documentation
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#

# You can set these variables from the command line.
PYTHON       = python3
VENVDIR      = ./venv
SPHINXBUILD  = PATH=$(VENVDIR)/bin:$$PATH sphinx-build
BLURB        = PATH=$(VENVDIR)/bin:$$PATH blurb
JOBS         = auto
PAPER        =
SOURCES      =
DISTVERSION  = $(shell $(PYTHON) tools/extensions/patchlevel.py)
REQUIREMENTS = requirements.txt
SPHINXERRORHANDLING = -W

# Internal variables.
PAPEROPT_a4     = -D latex_elements.papersize=a4paper
PAPEROPT_letter = -D latex_elements.papersize=letterpaper

ALLSPHINXOPTS = -b $(BUILDER) \
                -d build/doctrees \
                -j $(JOBS) \
                $(PAPEROPT_$(PAPER)) \
                $(SPHINXOPTS) $(SPHINXERRORHANDLING) \
                . build/$(BUILDER) $(SOURCES)

.PHONY: help
help:
	@echo "Please use \`make <target>' where <target> is one of"
	@echo "  clean      to remove build files"
	@echo "  venv       to create a venv with necessary tools"
	@echo "  html       to make standalone HTML files"
	@echo "  htmlview   to open the index page built by the html target in your browser"
	@echo "  htmllive   to rebuild and reload HTML files in your browser"
	@echo "  htmlhelp   to make HTML files and a HTML help project"
	@echo "  latex      to make LaTeX files, you can set PAPER=a4 or PAPER=letter"
	@echo "  text       to make plain text files"
	@echo "  texinfo    to make Texinfo file"
	@echo "  epub       to make EPUB files"
	@echo "  changes    to make an overview over all changed/added/deprecated items"
	@echo "  linkcheck  to check all external links for integrity"
	@echo "  coverage   to check documentation coverage for library and C API"
	@echo "  doctest    to run doctests in the documentation"
	@echo "  pydoc-topics  to regenerate the pydoc topics file"
	@echo "  dist       to create a \"dist\" directory with archived docs for download"
	@echo "  suspicious to check for suspicious markup in output text"
	@echo "  check      to run a check for frequent markup errors"

.PHONY: build
build:
	-mkdir -p build
# Look first for a Misc/NEWS file (building from a source release tarball
# or old repo) and use that, otherwise look for a Misc/NEWS.d directory
# (building from a newer repo) and use blurb to generate the NEWS file.
	@if [ -f  ../Misc/NEWS ] ; then \
		echo "Using existing Misc/NEWS file"; \
		cp ../Misc/NEWS build/NEWS; \
	elif $(BLURB) help >/dev/null 2>&1 && $(SPHINXBUILD) --version >/dev/null 2>&1; then \
		if [ -d ../Misc/NEWS.d ]; then \
			echo "Building NEWS from Misc/NEWS.d with blurb"; \
			$(BLURB) merge -f build/NEWS; \
		else \
			echo "Neither Misc/NEWS.d nor Misc/NEWS found; cannot build docs"; \
			exit 1; \
		fi \
	else \
		echo ""; \
		echo "Missing the required blurb or sphinx-build tools."; \
		echo "Please run 'make venv' to install local copies."; \
		echo ""; \
		exit 1; \
	fi
	$(SPHINXBUILD) $(ALLSPHINXOPTS)
	@echo

.PHONY: html
html: BUILDER = html
html: build
	@echo "Build finished. The HTML pages are in build/html."

.PHONY: htmlhelp
htmlhelp: BUILDER = htmlhelp
htmlhelp: build
	@echo "Build finished; now you can run HTML Help Workshop with the" \
	      "build/htmlhelp/pydoc.hhp project file."

.PHONY: latex
latex: BUILDER = latex
latex: build
	@echo "Build finished; the LaTeX files are in build/latex."
	@echo "Run \`make all-pdf' or \`make all-ps' in that directory to" \
	      "run these through (pdf)latex."

.PHONY: text
text: BUILDER = text
text: build
	@echo "Build finished; the text files are in build/text."

.PHONY: texinfo
texinfo: BUILDER = texinfo
texinfo: build
	@echo "Build finished; the python.texi file is in build/texinfo."
	@echo "Run \`make info' in that directory to run it through makeinfo."

.PHONY: epub
epub: BUILDER = epub
epub: build
	@echo "Build finished; the epub files are in build/epub."

.PHONY: changes
changes: BUILDER = changes
changes: build
	@echo "The overview file is in build/changes."

.PHONY: linkcheck
linkcheck: BUILDER = linkcheck
linkcheck:
	@$(MAKE) build BUILDER=$(BUILDER) || { \
	echo "Link check complete; look for any errors in the above output" \
	     "or in build/$(BUILDER)/output.txt"; \
	false; }

.PHONY: suspicious
suspicious: BUILDER = suspicious
suspicious:
	@$(MAKE) build BUILDER=$(BUILDER) || { \
	echo "Suspicious check complete; look for any errors in the above output" \
	     "or in build/$(BUILDER)/suspicious.csv.  If all issues are false" \
	     "positives, append that file to tools/susp-ignored.csv."; \
	false; }
	@echo "⚠ make suspicious is deprecated and will be removed soon."
	@echo "⚠ Use:"
	@echo "⚠     make check"
	@echo "⚠ instead."

.PHONY: coverage
coverage: BUILDER = coverage
coverage: build
	@echo "Coverage finished; see c.txt and python.txt in build/coverage"

.PHONY: doctest
doctest: BUILDER = doctest
doctest:
	@$(MAKE) build BUILDER=$(BUILDER) || { \
	echo "Testing of doctests in the sources finished, look at the" \
	     "results in build/doctest/output.txt"; \
	false; }

.PHONY: pydoc-topics
pydoc-topics: BUILDER = pydoc-topics
pydoc-topics: build
	@echo "Building finished; now run this:" \
	      "cp build/pydoc-topics/topics.py ../Lib/pydoc_data/topics.py"

.PHONY: htmlview
htmlview: html
	$(PYTHON) -c "import os, webbrowser; webbrowser.open('file://' + os.path.realpath('build/html/index.html'))"

.PHONY: htmllive
htmllive: SPHINXBUILD = $(VENVDIR)/bin/sphinx-autobuild
htmllive: SPHINXOPTS = --re-ignore="/venv/" --open-browser --delay 0
htmllive: html

.PHONY: clean
clean: clean-venv
	-rm -rf build/*

.PHONY: clean-venv
clean-venv:
	rm -rf $(VENVDIR)

.PHONY: venv
venv:
	@if [ -d $(VENVDIR) ] ; then \
		echo "venv already exists."; \
		echo "To recreate it, remove it first with \`make clean-venv'."; \
	else \
		echo "Creating venv in $(VENVDIR)"; \
		$(PYTHON) -m venv $(VENVDIR); \
		$(VENVDIR)/bin/python3 -m pip install --upgrade pip; \
		$(VENVDIR)/bin/python3 -m pip install -r $(REQUIREMENTS); \
		echo "The venv has been created in the $(VENVDIR) directory"; \
	fi

.PHONY: dist
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

	# archive the texinfo build
	rm -rf build/texinfo
	make texinfo
	make info --directory=build/texinfo
	cp -pPR build/texinfo dist/python-$(DISTVERSION)-docs-texinfo
	tar -C dist -cf dist/python-$(DISTVERSION)-docs-texinfo.tar python-$(DISTVERSION)-docs-texinfo
	bzip2 -9 -k dist/python-$(DISTVERSION)-docs-texinfo.tar
	(cd dist; zip -q -r -9 python-$(DISTVERSION)-docs-texinfo.zip python-$(DISTVERSION)-docs-texinfo)
	rm -r dist/python-$(DISTVERSION)-docs-texinfo
	rm dist/python-$(DISTVERSION)-docs-texinfo.tar

.PHONY: check
check: venv
	$(VENVDIR)/bin/python3 -m pre_commit --version > /dev/null || $(VENVDIR)/bin/python3 -m pip install pre-commit
	$(VENVDIR)/bin/python3 -m pre_commit run --all-files

.PHONY: serve
serve:
	@echo "The serve target was removed, use htmlview instead (see bpo-36329)"

# Targets for daily automated doc build
# By default, Sphinx only rebuilds pages where the page content has changed.
# This means it doesn't always pick up changes to preferred link targets, etc
# To ensure such changes are picked up, we build the published docs with
# `-E` (to ignore the cached environment) and `-a` (to ignore already existing
# output files)

# for development releases: always build
.PHONY: autobuild-dev
autobuild-dev:
	make dist SPHINXOPTS='$(SPHINXOPTS) -Ea -A daily=1'

# for quick rebuilds (HTML only)
.PHONY: autobuild-dev-html
autobuild-dev-html:
	make html SPHINXOPTS='$(SPHINXOPTS) -Ea -A daily=1'

# for stable releases: only build if not in pre-release stage (alpha, beta)
# release candidate downloads are okay, since the stable tree can be in that stage
.PHONY: autobuild-stable
autobuild-stable:
	@case $(DISTVERSION) in *[ab]*) \
		echo "Not building; $(DISTVERSION) is not a release version."; \
		exit 1;; \
	esac
	@make autobuild-dev

.PHONY: autobuild-stable-html
autobuild-stable-html:
	@case $(DISTVERSION) in *[ab]*) \
		echo "Not building; $(DISTVERSION) is not a release version."; \
		exit 1;; \
	esac
	@make autobuild-dev-html
