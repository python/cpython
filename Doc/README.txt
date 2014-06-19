Python Documentation README
~~~~~~~~~~~~~~~~~~~~~~~~~~~

This directory contains the reStructuredText (reST) sources to the Python
documentation.  You don't need to build them yourself, prebuilt versions are
available at https://docs.python.org/2/download.html

Documentation on the authoring Python documentation, including information about
both style and markup, is available in the "Documenting Python" chapter of the
documentation.


Building the docs
=================

You need to have Python 2 installed; the toolset used to build the
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

On Windows, we try to emulate the Makefile as closely as possible with a
``make.bat`` file.

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

 * "suspicious", which checks the parsed markup for text that looks like
   malformed and thus unconverted reST.

A "make update" updates the Subversion checkouts in `tools/`.


Without make
------------

Install the Sphinx package and its dependencies from PyPI.

Then, from the ``Docs`` directory, run ::

   sphinx-build -b<builder> . build/<builder>

where ``<builder>`` is one of html, text, latex, or htmlhelp (for explanations
see the make targets above).


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
Copyright (c) 2000-2014 Python Software Foundation.
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
