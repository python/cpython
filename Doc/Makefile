#
# Makefile for Python documentation
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#

# You can set these variables from the command line.
PYTHON       = python3
VENVDIR      = ./venv
UV           = uv
SPHINXBUILD  = PATH=$(VENVDIR)/bin:$$PATH sphinx-build
BLURB        = PATH=$(VENVDIR)/bin:$$PATH blurb
JOBS         = auto
PAPER        =
SOURCES      =
DISTVERSION  = $(shell $(PYTHON) tools/extensions/patchlevel.py)
REQUIREMENTS = requirements.txt
SPHINXERRORHANDLING = --fail-on-warning

# Internal variables.
PAPEROPT_a4     = --define latex_elements.papersize=a4paper
PAPEROPT_letter = --define latex_elements.papersize=letterpaper

ALLSPHINXOPTS = --builder $(BUILDER) \
                --doctree-dir build/doctrees \
                --jobs $(JOBS) \
                $(PAPEROPT_$(PAPER)) \
                $(SPHINXOPTS) $(SPHINXERRORHANDLING) \
                . build/$(BUILDER) $(SOURCES)

.PHONY: help
help:
	@echo "Please use \`make <target>' where <target> is one of"
	@echo "  clean      to remove build files"
	@echo "  venv       to create a venv with necessary tools"
	@echo "  html       to make standalone HTML files"
	@echo "  gettext    to generate POT files"
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

.PHONY: gettext
gettext: BUILDER = gettext
gettext: override SPHINXOPTS := --doctree-dir build/doctrees-gettext $(SPHINXOPTS)
gettext: build

.PHONY: htmlview
htmlview: html
	$(PYTHON) -c "import os, webbrowser; webbrowser.open('file://' + os.path.realpath('build/html/index.html'))"

.PHONY: htmllive
htmllive: SPHINXBUILD = PATH=$(VENVDIR)/bin:$$PATH sphinx-autobuild
htmllive: SPHINXOPTS = --re-ignore="/venv/" --open-browser --delay 0
htmllive: _ensure-sphinx-autobuild html

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
		if $(UV) --version >/dev/null 2>&1; then \
			$(UV) venv --python=$(PYTHON) $(VENVDIR); \
			VIRTUAL_ENV=$(VENVDIR) $(UV) pip install -r $(REQUIREMENTS); \
		else \
			$(PYTHON) -m venv $(VENVDIR); \
			$(VENVDIR)/bin/python3 -m pip install --upgrade pip; \
			$(VENVDIR)/bin/python3 -m pip install -r $(REQUIREMENTS); \
		fi; \
		echo "The venv has been created in the $(VENVDIR) directory"; \
	fi

.PHONY: dist-no-html
dist-no-html: dist-text dist-pdf dist-epub dist-texinfo

.PHONY: dist
dist:
	rm -rf dist
	mkdir -p dist
	$(MAKE) dist-html
	$(MAKE) dist-text
	$(MAKE) dist-pdf
	$(MAKE) dist-epub
	$(MAKE) dist-texinfo

.PHONY: dist-html
dist-html:
	# archive the HTML
	@echo "Building HTML..."
	mkdir -p dist
	rm -rf build/html
	find dist -name 'python-$(DISTVERSION)-docs-html*' -exec rm -rf {} \;
	$(MAKE) html
	cp -pPR build/html dist/python-$(DISTVERSION)-docs-html
	tar -C dist -cf dist/python-$(DISTVERSION)-docs-html.tar python-$(DISTVERSION)-docs-html
	bzip2 -9 -k dist/python-$(DISTVERSION)-docs-html.tar
	(cd dist; zip -q -r -9 python-$(DISTVERSION)-docs-html.zip python-$(DISTVERSION)-docs-html)
	rm -r dist/python-$(DISTVERSION)-docs-html
	rm dist/python-$(DISTVERSION)-docs-html.tar
	@echo "Build finished and archived!"

.PHONY: dist-text
dist-text:
	# archive the text build
	@echo "Building text..."
	mkdir -p dist
	rm -rf build/text
	find dist -name 'python-$(DISTVERSION)-docs-text*' -exec rm -rf {} \;
	$(MAKE) text
	cp -pPR build/text dist/python-$(DISTVERSION)-docs-text
	tar -C dist -cf dist/python-$(DISTVERSION)-docs-text.tar python-$(DISTVERSION)-docs-text
	bzip2 -9 -k dist/python-$(DISTVERSION)-docs-text.tar
	(cd dist; zip -q -r -9 python-$(DISTVERSION)-docs-text.zip python-$(DISTVERSION)-docs-text)
	rm -r dist/python-$(DISTVERSION)-docs-text
	rm dist/python-$(DISTVERSION)-docs-text.tar
	@echo "Build finished and archived!"

.PHONY: dist-pdf
dist-pdf:
	# archive the A4 latex
	@echo "Building LaTeX (A4 paper)..."
	mkdir -p dist
	rm -rf build/latex
	find dist -name 'python-$(DISTVERSION)-docs-pdf*' -exec rm -rf {} \;
	$(MAKE) latex PAPER=a4
	# remove zip & bz2 dependency on all-pdf,
	# as otherwise the full latexmk process is run twice.
	# ($$ is needed to escape the $; https://www.gnu.org/software/make/manual/make.html#Basics-of-Variable-References)
	-sed -i 's/: all-$$(FMT)/:/' build/latex/Makefile
	(cd build/latex; $(MAKE) clean && $(MAKE) --jobs=$$((`nproc`+1)) --output-sync LATEXMKOPTS='-quiet' all-pdf && $(MAKE) FMT=pdf zip bz2)
	cp build/latex/docs-pdf.zip dist/python-$(DISTVERSION)-docs-pdf-a4.zip
	cp build/latex/docs-pdf.tar.bz2 dist/python-$(DISTVERSION)-docs-pdf-a4.tar.bz2
	@echo "Build finished and archived!"

.PHONY: dist-epub
dist-epub:
	# copy the epub build
	@echo "Building EPUB..."
	mkdir -p dist
	rm -rf build/epub
	rm -f dist/python-$(DISTVERSION)-docs.epub
	$(MAKE) epub
	cp -pPR build/epub/Python.epub dist/python-$(DISTVERSION)-docs.epub
	@echo "Build finished and archived!"

.PHONY: dist-texinfo
dist-texinfo:
	# archive the texinfo build
	@echo "Building Texinfo..."
	mkdir -p dist
	rm -rf build/texinfo
	find dist -name 'python-$(DISTVERSION)-docs-texinfo*' -exec rm -rf {} \;
	$(MAKE) texinfo
	$(MAKE) info --directory=build/texinfo
	cp -pPR build/texinfo dist/python-$(DISTVERSION)-docs-texinfo
	tar -C dist -cf dist/python-$(DISTVERSION)-docs-texinfo.tar python-$(DISTVERSION)-docs-texinfo
	bzip2 -9 -k dist/python-$(DISTVERSION)-docs-texinfo.tar
	(cd dist; zip -q -r -9 python-$(DISTVERSION)-docs-texinfo.zip python-$(DISTVERSION)-docs-texinfo)
	rm -r dist/python-$(DISTVERSION)-docs-texinfo
	rm dist/python-$(DISTVERSION)-docs-texinfo.tar
	@echo "Build finished and archived!"

.PHONY: _ensure-package
_ensure-package: venv
	if $(UV) --version >/dev/null 2>&1; then \
		VIRTUAL_ENV=$(VENVDIR) $(UV) pip install $(PACKAGE); \
	else \
		$(VENVDIR)/bin/python3 -m pip install $(PACKAGE); \
	fi

.PHONY: _ensure-pre-commit
_ensure-pre-commit:
	$(MAKE) _ensure-package PACKAGE=pre-commit

.PHONY: _ensure-sphinx-autobuild
_ensure-sphinx-autobuild:
	$(MAKE) _ensure-package PACKAGE=sphinx-autobuild

.PHONY: check
check: _ensure-pre-commit
	$(VENVDIR)/bin/python3 -m pre_commit run --all-files

.PHONY: serve
serve:
	@echo "The serve target was removed, use htmllive instead (see gh-80510)"

# Targets for daily automated doc build
# By default, Sphinx only rebuilds pages where the page content has changed.
# This means it doesn't always pick up changes to preferred link targets, etc
# To ensure such changes are picked up, we build the published docs with
# ``--fresh-env`` (to ignore the cached environment) and ``--write-all``
# (to ignore already existing output files)

# for development releases: always build
.PHONY: autobuild-dev
autobuild-dev: DISTVERSION = $(shell $(PYTHON) tools/extensions/patchlevel.py --short)
autobuild-dev:
	$(MAKE) dist-no-html SPHINXOPTS='$(SPHINXOPTS) --fresh-env --write-all --html-define daily=1' DISTVERSION=$(DISTVERSION)

# for HTML-only rebuilds
.PHONY: autobuild-dev-html
autobuild-dev-html: DISTVERSION = $(shell $(PYTHON) tools/extensions/patchlevel.py --short)
autobuild-dev-html:
	$(MAKE) dist-html SPHINXOPTS='$(SPHINXOPTS) --fresh-env --write-all --html-define daily=1' DISTVERSION=$(DISTVERSION)

# for stable releases: only build if not in pre-release stage (alpha, beta)
# release candidate downloads are okay, since the stable tree can be in that stage
.PHONY: autobuild-stable
autobuild-stable:
	@case $(DISTVERSION) in *[ab]*) \
		echo "Not building; $(DISTVERSION) is not a release version."; \
		exit 1;; \
	esac
	@$(MAKE) autobuild-dev

.PHONY: autobuild-stable-html
autobuild-stable-html:
	@case $(DISTVERSION) in *[ab]*) \
		echo "Not building; $(DISTVERSION) is not a release version."; \
		exit 1;; \
	esac
	@$(MAKE) autobuild-dev-html
