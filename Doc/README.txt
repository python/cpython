Python Documentation README
~~~~~~~~~~~~~~~~~~~~~~~~~~~

This directory contains the reStructuredText (reST) sources to the Python
documentation.  You don't need to build them yourself, prebuilt versions are
available at http://docs.python.org/download/.

Documentation on the authoring Python documentation, including information about
both style and markup, is available in the "Documenting Python" chapter of the
documentation.  There's also a chapter intended to point out differences to
those familiar with the previous docs written in LaTeX.


Building the docs
=================

You need to have Python 2.4 or higher installed; the toolset used to build the
docs is written in Python.  It is called *Sphinx*, it is not included in this
tree, but maintained separately.  Also needed are the docutils, supplying the
base markup that Sphinx uses, Jinja, a templating engine, and optionally
Pygments, a code highlighter.


Using make
----------

Luckily, a Makefile has been prepared so that on Unix, provided you have
installed Python and Subversion, you can just run ::

   make html

to check out the necessary toolset in the `tools/` subdirectory and build the
HTML output files.  To view the generated HTML, point your favorite browser at
the top-level index `build/html/index.html` after running "make".

To use a Python interpreter that's not called ``python``, use the standard
way to set Makefile variables, using e.g. ::

   make html PYTHON=/usr/bin/python2.5

Available make targets are:

 * "html", which builds standalone HTML files for offline viewing.

 * "htmlhelp", which builds HTML files and a HTML Help project file usable to
   convert them into a single Compiled HTML (.chm) file -- these are popular
   under Microsoft Windows, but very handy on every platform.

   To create the CHM file, you need to run the Microsoft HTML Help Workshop over
   the generated project (.hhp) file.

 * "latex", which builds LaTeX source files as input to "pdflatex" to produce
   PDF documents.

 * "text", which builds a plain text file for each source file.

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
   `tools/sphinxext/pyspecific.py` -- pydoc needs these to show topic and
   keyword help.

A "make update" updates the Subversion checkouts in `tools/`.


Without make
------------

You'll need to install the Sphinx package, either by checking it out via ::

   svn co http://svn.python.org/projects/external/Sphinx-1.0.5/sphinx tools/sphinx

or by installing it from PyPI.

Then, you need to install Docutils, either by checking it out via ::

   svn co http://svn.python.org/projects/external/docutils-0.6/docutils tools/docutils

or by installing it from http://docutils.sf.net/.

You also need Jinja2, either by checking it out via ::

   svn co http://svn.python.org/projects/external/Jinja-2.3.1/jinja2 tools/jinja2

or by installing it from PyPI.

You can optionally also install Pygments, either as a checkout via ::

   svn co http://svn.python.org/projects/external/Pygments-1.3.1/pygments tools/pygments

or from PyPI at http://pypi.python.org/pypi/Pygments.


Then, make an output directory, e.g. under `build/`, and run ::

   python tools/sphinx-build.py -b<builder> . build/<outputdirectory>

where `<builder>` is one of html, text, latex, or htmlhelp (for explanations see
the make targets above).


Contributing
============

Bugs in the content should be reported to the Python bug tracker at
http://bugs.python.org.

Bugs in the toolset should be reported in the Sphinx bug tracker at
http://www.bitbucket.org/birkenfeld/sphinx/issues/.

You can also send a mail to the Python Documentation Team at docs@python.org,
and we will process your request as soon as possible.

If you want to help the Documentation Team, you are always welcome.  Just send
a mail to docs@python.org.


Copyright notice
================

The Python source is copyrighted, but you can freely use and copy it
as long as you don't change or remove the copyright notice:

----------------------------------------------------------------------
Copyright (c) 2000-2011 Python Software Foundation.
All rights reserved.

Copyright (c) 2000 BeOpen.com.
All rights reserved.

Copyright (c) 1995-2000 Corporation for National Research Initiatives.
All rights reserved.

Copyright (c) 1991-1995 Stichting Mathematisch Centrum.
All rights reserved.

See the file "license.rst" for information on usage and redistribution
of this file, and for a DISCLAIMER OF ALL WARRANTIES.
----------------------------------------------------------------------
