Python Documentation README
~~~~~~~~~~~~~~~~~~~~~~~~~~~

This directory contains the reStructuredText (reST) sources to the Python
documentation.  You don't need to build them yourself, prebuilt versions are
available at <https://docs.python.org/dev/download.html>.

Documentation on authoring Python documentation, including information about
both style and markup, is available in the "Documenting Python" chapter of the
developers guide <https://docs.python.org/devguide/documenting.html>.


Building the docs
=================

You need to have Sphinx <http://sphinx-doc.org/> installed; it is the toolset
used to build the docs.  It is not included in this tree, but maintained
separately and available from PyPI <https://pypi.python.org/pypi/Sphinx>.


Using make
----------

A Makefile has been prepared so that on Unix, provided you have installed
Sphinx, you can just run ::

   make html

to build the HTML output files.

On Windows, we try to emulate the Makefile as closely as possible with a
``make.bat`` file.

To use a Python interpreter that's not called ``python``, use the standard
way to set Makefile variables, using e.g. ::

   make html PYTHON=python3

On Windows, set the PYTHON environment variable instead.

To use a specific sphinx-build (something other than ``sphinx-build``), set
the SPHINXBUILD variable.

Available make targets are:

* "clean", which removes all build files.

* "html", which builds standalone HTML files for offline viewing.

* "htmlview", which re-uses the "html" builder, but then opens the main page
  in your default web browser.

* "htmlhelp", which builds HTML files and a HTML Help project file usable to
  convert them into a single Compiled HTML (.chm) file -- these are popular
  under Microsoft Windows, but very handy on every platform.

  To create the CHM file, you need to run the Microsoft HTML Help Workshop
  over the generated project (.hhp) file.  The make.bat script does this for
  you on Windows.

* "latex", which builds LaTeX source files as input to "pdflatex" to produce
  PDF documents.

* "text", which builds a plain text file for each source file.

* "epub", which builds an EPUB document, suitable to be viewed on e-book
  readers.

* "linkcheck", which checks all external references to see whether they are
  broken, redirected or malformed, and outputs this information to stdout as
  well as a plain-text (.txt) file.

* "changes", which builds an overview over all versionadded/versionchanged/
  deprecated items in the current version. This is meant as a help for the
  writer of the "What's New" document.

* "coverage", which builds a coverage overview for standard library modules and
  C API.

* "pydoc-topics", which builds a Python module containing a dictionary with
  plain text documentation for the labels defined in
  `tools/pyspecific.py` -- pydoc needs these to show topic and keyword help.

* "suspicious", which checks the parsed markup for text that looks like
  malformed and thus unconverted reST.

* "check", which checks for frequent markup errors.

* "serve", which serves the build/html directory on port 8000.

* "dist", (Unix only) which creates distributable archives of HTML, text,
  PDF, and EPUB builds.


Without make
------------

Install the Sphinx package and its dependencies from PyPI.

Then, from the ``Doc`` directory, run ::

   sphinx-build -b<builder> . build/<builder>

where ``<builder>`` is one of html, text, latex, or htmlhelp (for explanations
see the make targets above).


Contributing
============

Bugs in the content should be reported to the Python bug tracker at
https://bugs.python.org.

Bugs in the toolset should be reported in the Sphinx bug tracker at
https://www.bitbucket.org/birkenfeld/sphinx/issues/.

You can also send a mail to the Python Documentation Team at docs@python.org,
and we will process your request as soon as possible.

If you want to help the Documentation Team, you are always welcome.  Just send
a mail to docs@python.org.
