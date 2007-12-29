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

You need to install Python 2.5 or higher; the toolset used to build the docs are
written in Python.  The toolset used to build the documentation is called
*Sphinx*, it is not included in this tree, but maintained separately in the
Python Subversion repository.  Also needed are Jinja, a templating engine
(included in Sphinx as a Subversion external), and optionally Pygments, a code
highlighter.


Using make
----------

Luckily, a Makefile has been prepared so that on Unix, provided you have
installed Python and Subversion, you can just run ::

   make html

to check out the necessary toolset in the `tools/` subdirectory and build the
HTML output files.  To view the generated HTML, point your favorite browser at
the top-level index `build/html/index.html` after running "make".

Available make targets are:

 * "html", which builds standalone HTML files for offline viewing.

 * "web", which builds files usable with the Sphinx.web application (used to
   serve the docs online at http://docs.python.org/).

 * "htmlhelp", which builds HTML files and a HTML Help project file usable to
   convert them into a single Compiled HTML (.chm) file -- these are popular
   under Microsoft Windows, but very handy on every platform.

   To create the CHM file, you need to run the Microsoft HTML Help Workshop
   over the generated project (.hhp) file.

 * "latex", which builds LaTeX source files that can be run with "pdflatex"
   to produce PDF documents.

 * "changes", which builds an overview over all versionadded/versionchanged/
   deprecated items in the current version. This is meant as a help for the
   writer of the "What's New" document.

A "make update" updates the Subversion checkouts in `tools/`.


Without make
------------

You'll need to checkout the Sphinx package to the `tools/` directory::

   svn co http://svn.python.org/projects/doctools/trunk/sphinx tools/sphinx

Then, you need to install Docutils 0.4 (the SVN snapshot won't work), either
by checking it out via ::

   svn co http://svn.python.org/projects/external/docutils-0.4/docutils tools/docutils

or by installing it from http://docutils.sf.net/.

You can optionally also install Pygments, either as a checkout via :: 

   svn co http://svn.python.org/projects/external/Pygments-0.9/pygments tools/pygments

or from PyPI at http://pypi.python.org/pypi/Pygments.


Then, make an output directory, e.g. under `build/`, and run ::

   python tools/sphinx-build.py -b<builder> . build/<outputdirectory>

where `<builder>` is one of html, web or htmlhelp (for explanations see the make
targets above).


Contributing
============

For bugs in the content, the online version at http://docs.python.org/ has a
"suggest change" facility that can be used to correct errors in the source text
and submit them as a patch to the maintainers.

Bugs in the toolset should be reported in the Python bug tracker at
http://bugs.python.org/.

You can also send a mail to the Python Documentation Team at docs@python.org,
and we will process your request as soon as possible.

If you want to help the Documentation Team, you are always welcome.  Just send
a mail to docs@python.org.


Copyright notice
================

The Python source is copyrighted, but you can freely use and copy it
as long as you don't change or remove the copyright notice:

----------------------------------------------------------------------
Copyright (c) 2000-2007 Python Software Foundation.
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
