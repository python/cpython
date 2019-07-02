This is Python version 3.6.9
============================

.. image:: https://travis-ci.org/python/cpython.svg?branch=3.6
   :alt: CPython build status on Travis CI
   :target: https://travis-ci.org/python/cpython

.. image:: https://ci.appveyor.com/api/projects/status/4mew1a93xdkbf5ua/branch/3.6?svg=true
   :alt: CPython build status on Appveyor
   :target: https://ci.appveyor.com/project/python/cpython/branch/3.6

.. image:: https://dev.azure.com/python/cpython/_apis/build/status/Azure%20Pipelines%20CI?branchName=3.6
   :alt: CPython build status on Azure Pipelines
   :target: https://dev.azure.com/python/cpython/_build/latest?definitionId=4&branchName=3.6

.. image:: https://codecov.io/gh/python/cpython/branch/3.6/graph/badge.svg
   :alt: CPython code coverage on Codecov
   :target: https://codecov.io/gh/python/cpython

Copyright (c) 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011,
2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019 Python Software Foundation.  All
rights reserved.

See the end of this file for further copyright and license information.

General Information
-------------------

- Website: https://www.python.org
- Source code: https://github.com/python/cpython
- Issue tracker: https://bugs.python.org
- Documentation: https://docs.python.org
- Developer's Guide: https://devguide.python.org/

Contributing to CPython
-----------------------

For more complete instructions on contributing to CPython development,
see the `Developer Guide`_.

.. _Developer Guide: https://devguide.python.org/

Using Python
------------

Installable Python kits, and information about using Python, are available at
`python.org`_.

.. _python.org: https://www.python.org/


Build Instructions
------------------

On Unix, Linux, BSD, macOS, and Cygwin::

    ./configure
    make
    make test
    sudo make install

This will install Python as ``python3``.

You can pass many options to the configure script; run ``./configure --help``
to find out more.  On macOS and Cygwin, the executable is called ``python.exe``;
elsewhere it's just ``python``.

If you are running on macOS with the latest updates installed, make sure to install
openSSL or some other SSL software along with Homebrew or another package manager.
If issues persist, see https://devguide.python.org/setup/#macos-and-os-x for more 
information. 

On macOS, if you have configured Python with ``--enable-framework``, you
should use ``make frameworkinstall`` to do the installation.  Note that this
installs the Python executable in a place that is not normally on your PATH,
you may want to set up a symlink in ``/usr/local/bin``.

On Windows, see `PCbuild/readme.txt
<https://github.com/python/cpython/blob/3.6/PCbuild/readme.txt>`_.

If you wish, you can create a subdirectory and invoke configure from there.
For example::

    mkdir debug
    cd debug
    ../configure --with-pydebug
    make
    make test

(This will fail if you *also* built at the top-level directory.  You should do
a ``make clean`` at the toplevel first.)

To get an optimized build of Python, ``configure --enable-optimizations``
before you run ``make``.  This sets the default make targets up to enable
Profile Guided Optimization (PGO) and may be used to auto-enable Link Time
Optimization (LTO) on some platforms.  For more details, see the sections
below.


Profile Guided Optimization
---------------------------

PGO takes advantage of recent versions of the GCC or Clang compilers.  If used,
either via ``configure --enable-optimizations`` above or by manually running
``make profile-opt`` regardless of configure flags it will do several steps.

First, the entire Python directory is cleaned of temporary files that may have
resulted in a previous compilation.

Then, an instrumented version of the interpreter is built, using suitable
compiler flags for each flavour. Note that this is just an intermediary step.
The binary resulting from this step is not good for real life workloads as
it has profiling instructions embedded inside.

After this instrumented version of the interpreter is built, the Makefile will
automatically run a training workload. This is necessary in order to profile
the interpreter execution. Note also that any output, both stdout and stderr,
that may appear at this step is suppressed.

Finally, the last step is to rebuild the interpreter, using the information
collected in the previous one. The end result will be a Python binary that is
optimized and suitable for distribution or production installation.


Link Time Optimization
----------------------

Enabled via configure's ``--with-lto`` flag.  LTO takes advantage of the
ability of recent compiler toolchains to optimize across the otherwise
arbitrary ``.o`` file boundary when building final executables or shared
libraries for additional performance gains.


What's New
----------

We have a comprehensive overview of the changes in the `What's New in Python
3.6 <https://docs.python.org/3.6/whatsnew/3.6.html>`_ document.  For a more
detailed change log, read `Misc/NEWS
<https://github.com/python/cpython/blob/3.6/Misc/NEWS.d>`_, but a full
accounting of changes can only be gleaned from the `commit history
<https://github.com/python/cpython/commits/3.6>`_.

If you want to install multiple versions of Python see the section below
entitled "Installing multiple versions".


Documentation
-------------

`Documentation for Python 3.6 <https://docs.python.org/3.6/>`_ is online,
updated daily.

It can also be downloaded in many formats for faster access.  The documentation
is downloadable in HTML, PDF, and reStructuredText formats; the latter version
is primarily for documentation authors, translators, and people with special
formatting requirements.

For information about building Python's documentation, refer to `Doc/README.rst
<https://github.com/python/cpython/blob/3.6/Doc/README.rst>`_.


Converting From Python 2.x to 3.x
---------------------------------

Significant backward incompatible changes were made for the release of Python
3.0, which may cause programs written for Python 2 to fail when run with Python
3.  For more information about porting your code from Python 2 to Python 3, see
the `Porting HOWTO <https://docs.python.org/3/howto/pyporting.html>`_.


Testing
-------

To test the interpreter, type ``make test`` in the top-level directory.  The
test set produces some output.  You can generally ignore the messages about
skipped tests due to optional features which can't be imported.  If a message
is printed about a failed test or a traceback or core dump is produced,
something is wrong.

By default, tests are prevented from overusing resources like disk space and
memory.  To enable these tests, run ``make testall``.

If any tests fail, you can re-run the failing test(s) in verbose mode.  For
example, if ``test_os`` and ``test_gdb`` failed, you can run::

    make test TESTOPTS="-v test_os test_gdb"

If the failure persists and appears to be a problem with Python rather than
your environment, you can `file a bug report <https://bugs.python.org>`_ and
include relevant output from that command to show the issue.

See `Running & Writing Tests <https://devguide.python.org/runtests/>`_
for more on running tests.

Installing multiple versions
----------------------------

On Unix and Mac systems if you intend to install multiple versions of Python
using the same installation prefix (``--prefix`` argument to the configure
script) you must take care that your primary python executable is not
overwritten by the installation of a different version.  All files and
directories installed using ``make altinstall`` contain the major and minor
version and can thus live side-by-side.  ``make install`` also creates
``${prefix}/bin/python3`` which refers to ``${prefix}/bin/pythonX.Y``.  If you
intend to install multiple versions using the same prefix you must decide which
version (if any) is your "primary" version.  Install that version using ``make
install``.  Install all other versions using ``make altinstall``.

For example, if you want to install Python 2.7, 3.5, and 3.6 with 3.6 being the
primary version, you would execute ``make install`` in your 3.6 build directory
and ``make altinstall`` in the others.


Issue Tracker and Mailing List
------------------------------

Bug reports are welcome!  You can use the `issue tracker
<https://bugs.python.org>`_ to report bugs, and/or submit pull requests `on
GitHub <https://github.com/python/cpython>`_.

You can also follow development discussion on the `python-dev mailing list
<https://mail.python.org/mailman/listinfo/python-dev/>`_.


Proposals for enhancement
-------------------------

If you have a proposal to change Python, you may want to send an email to the
comp.lang.python or `python-ideas`_ mailing lists for initial feedback.  A
Python Enhancement Proposal (PEP) may be submitted if your idea gains ground.
All current PEPs, as well as guidelines for submitting a new PEP, are listed at
`python.org/dev/peps/ <https://www.python.org/dev/peps/>`_.

.. _python-ideas: https://mail.python.org/mailman/listinfo/python-ideas/


Release Schedule
----------------

See :pep:`494` for Python 3.6 release details.


Copyright and License Information
---------------------------------

Copyright (c) 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011,
2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019 Python Software Foundation.  All
rights reserved.

Copyright (c) 2000 BeOpen.com.  All rights reserved.

Copyright (c) 1995-2001 Corporation for National Research Initiatives.  All
rights reserved.

Copyright (c) 1991-1995 Stichting Mathematisch Centrum.  All rights reserved.

See the file "LICENSE" for information on the history of this software, terms &
conditions for usage, and a DISCLAIMER OF ALL WARRANTIES.

This Python distribution contains *no* GNU General Public License (GPL) code,
so it may be used in proprietary projects.  There are interfaces to some GNU
code but these are entirely optional.

All trademarks referenced herein are property of their respective holders.
