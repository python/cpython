#
# Makefile for Python documentation
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#

# You can set these variables from the command line.
PYTHON       = python
SVNROOT      = http://svn.python.org/projects
SPHINXOPTS   =

ALLSPHINXOPTS = -b$(BUILDER) -dbuild/doctrees $(SPHINXOPTS) . build/$(BUILDER)

.PHONY: help checkout update build html web htmlhelp clean

help:
	@echo "Please use \`make <target>' where <target> is one of"
	@echo "  html  to make standalone HTML files"
	@echo "  web   to make file usable by Sphinx.web"
	@echo "  htmlhelp to make HTML files and a HTML help project"

checkout:
	@if [ ! -d tools/sphinx ]; then \
	  echo "Checking out Sphinx..."; \
	  svn checkout $(SVNROOT)/doctools/trunk/sphinx tools/sphinx; \
	fi
	@if [ ! -d tools/docutils ]; then \
	  echo "Checking out Docutils..."; \
	  svn checkout $(SVNROOT)/external/docutils-0.4/docutils tools/docutils; \
	fi
	@if [ ! -d tools/pygments ]; then \
	  echo "Checking out Pygments..."; \
	  svn checkout $(SVNROOT)/external/Pygments-0.9/pygments tools/pygments; \
	fi

update: checkout
	svn update tools/sphinx
	svn update tools/docutils
	svn update tools/pygments

build: checkout
	mkdir -p build/$(BUILDER) build/doctrees
	$(PYTHON) tools/sphinx-build.py $(ALLSPHINXOPTS)
	@echo

html: BUILDER = html
html: build
	@echo "Build finished. The HTML pages are in build/html."

web: BUILDER = web
web: build
	@echo "Build finished; now you can run"
	@echo "  PYTHONPATH=tools $(PYTHON) -m sphinx.web build/web"
	@echo "to start the server."

htmlhelp: BUILDER = htmlhelp
htmlhelp: build
	@echo "Build finished; now you can run HTML Help Workshop with the" \
	      "build/htmlhelp/pydoc.hhp project file."

clean:
	-rm -rf build/*
	-rm -rf tools/sphinx
