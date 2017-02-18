This is Python version 3.7.0 alpha 1
====================================

.. image:: https://travis-ci.org/python/cpython.svg?branch=master
   :alt: CPython build status on Travis CI
   :target: https://travis-ci.org/python/cpython

.. image:: https://codecov.io/gh/python/cpython/branch/master/graph/badge.svg
   :alt: CPython code coverage on Codecov
   :target: https://codecov.io/gh/python/cpython

Copyright (c) 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011,
2012, 2013, 2014, 2015, 2016, 2017 Python Software Foundation.  All rights
reserved.

See the end of this file for further copyright and license information.

Contributing to CPython
-----------------------

For more complete instructions on contributing to CPython development,
see the `Developer Guide`_.

.. _Developer Guide: https://docs.python.org/devguide/

Using Python
------------

Installable Python kits, and information about using Python, are available at
`python.org`_.

.. _python.org: https://www.python.org/


Build Instructions
------------------

On Unix, Linux, BSD, OSX, and Cygwin::

    ./configure
    make
    make test
    sudo make install

This will install Python as python3.

You can pass many options to the configure script; run ``./configure --help``
to find out more.  On OSX and Cygwin, the executable is called ``python.exe``;
elsewhere it's just ``python``.

On Mac OS X, if you have configured Python with ``--enable-framework``, you
should use ``make frameworkinstall`` to do the installation.  Note that this
installs the Python executable in a place that is not normally on your PATH,
you may want to set up a symlink in ``/usr/local/bin``.

On Windows, see `PCbuild/readme.txt
<https://github.com/python/cpython/blob/master/PCbuild/readme.txt>`_.

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

PGO takes advantage of recent versions of the GCC or Clang compilers.  If ran,