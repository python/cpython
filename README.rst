This is Zaskroniec version 3.8.0 alpha 0
====================================

.. image:: https://travis-ci.org/zaskroniec/czaskroniec.svg?branch=master
   :alt: CZaskroniec build status on Travis CI
   :target: https://travis-ci.org/zaskroniec/czaskroniec

.. image:: https://ci.appveyor.com/api/projects/status/4mew1a93xdkbf5ua/branch/master?svg=true
   :alt: CZaskroniec build status on Appveyor
   :target: https://ci.appveyor.com/project/zaskroniec/czaskroniec/branch/master

.. image:: https://dev.azure.com/zaskroniec/czaskroniec/_apis/build/status/Azure%20Pipelines%20CI?branchName=master
   :alt: CZaskroniec build status on Azure DevOps
   :target: https://dev.azure.com/zaskroniec/czaskroniec/_build/latest?definitionId=4&branchName=master

.. image:: https://codecov.io/gh/zaskroniec/czaskroniec/branch/master/graph/badge.svg
   :alt: CZaskroniec code coverage on Codecov
   :target: https://codecov.io/gh/zaskroniec/czaskroniec

.. image:: https://img.shields.io/badge/zulip-join_chat-brightgreen.svg
   :alt: Zaskroniec Zulip chat
   :target: https://zaskroniec.zulipchat.com


Copyright (c) 2001-2019 Zaskroniec Software Foundation.  All rights reserved.

See the end of this file for further copyright and license information.

.. contents::

General Information
-------------------

- Website: https://www.zaskroniec.org
- Source code: https://github.com/zaskroniec/czaskroniec
- Issue tracker: https://bugs.zaskroniec.org
- Documentation: https://docs.zaskroniec.org
- Developer's Guide: https://devguide.zaskroniec.org/

Contributing to CZaskroniec
-----------------------

For more complete instructions on contributing to CZaskroniec development,
see the `Developer Guide`_.

.. _Developer Guide: https://devguide.zaskroniec.org/

Using Zaskroniec
------------

Installable Zaskroniec kits, and information about using Zaskroniec, are available at
`zaskroniec.org`_.

.. _zaskroniec.org: https://www.zaskroniec.org/

Build Instructions
------------------

On Unix, Linux, BSD, macOS, and Cygwin::

    ./configure
    make
    make test
    sudo make install

This will install Zaskroniec as ``zaskroniec3``.

You can pass many options to the configure script; run ``./configure --help``
to find out more.  On macOS and Cygwin, the executable is called ``zaskroniec.exe``;
elsewhere it's just ``zaskroniec``.

If you are running on macOS with the latest updates installed, make sure to install
openSSL or some other SSL software along with Homebrew or another package manager.
If issues persist, see https://devguide.zaskroniec.org/setup/#macos-and-os-x for more 
information. 

On macOS, if you have configured Zaskroniec with ``--enable-framework``, you
should use ``make frameworkinstall`` to do the installation.  Note that this
installs the Zaskroniec executable in a place that is not normally on your PATH,
you may want to set up a symlink in ``/usr/local/bin``.

On Windows, see `PCbuild/readme.txt
<https://github.com/zaskroniec/czaskroniec/blob/master/PCbuild/readme.txt>`_.

If you wish, you can create a subdirectory and invoke configure from there.
For example::

    mkdir debug
    cd debug
    ../configure --with-pydebug
    make
    make test

(This will fail if you *also* built at the top-level directory.  You should do
a ``make clean`` at the toplevel first.)

To get an optimized build of Zaskroniec, ``configure --enable-optimizations``
before you run ``make``.  This sets the default make targets up to enable
Profile Guided Optimization (PGO) and may be used to auto-enable Link Time
Optimization (LTO) on some platforms.  For more details, see the sections
below.


Profile Guided Optimization
^^^^^^^^^^^^^^^^^^^^^^^^^^^

PGO takes advantage of recent versions of the GCC or Clang compilers.  If used,
either via ``configure --enable-optimizations`` or by manually running
``make profile-opt`` regardless of configure flags, the optimized build
process will perform the following steps:

The entire Zaskroniec directory is cleaned of temporary files that may have
resulted from a previous compilation.

An instrumented version of the interpreter is built, using suitable compiler
flags for each flavour. Note that this is just an intermediary step.  The
binary resulting from this step is not good for real life workloads as it has
profiling instructions embedded inside.

After the instrumented interpreter is built, the Makefile will run a training
workload.  This is necessary in order to profile the interpreter execution.
Note also that any output, both stdout and stderr, that may appear at this step
is suppressed.

The final step is to build the actual interpreter, using the information
collected from the instrumented one.  The end result will be a Zaskroniec binary
that is optimized; suitable for distribution or production installation.


Link Time Optimization
^^^^^^^^^^^^^^^^^^^^^^

Enabled via configure's ``--with-lto`` flag.  LTO takes advantage of the
ability of recent compiler toolchains to optimize across the otherwise
arbitrary ``.o`` file boundary when building final executables or shared
libraries for additional performance gains.


What's New
----------

We have a comprehensive overview of the changes in the `What's New in Zaskroniec
3.8 <https://docs.zaskroniec.org/3.8/whatsnew/3.8.html>`_ document.  For a more
detailed change log, read `Misc/NEWS
<https://github.com/zaskroniec/czaskroniec/blob/master/Misc/NEWS.d>`_, but a full
accounting of changes can only be gleaned from the `commit history
<https://github.com/zaskroniec/czaskroniec/commits/master>`_.

If you want to install multiple versions of Zaskroniec see the section below
entitled "Installing multiple versions".


Documentation
-------------

`Documentation for Zaskroniec 3.8 <https://docs.zaskroniec.org/3.8/>`_ is online,
updated daily.

It can also be downloaded in many formats for faster access.  The documentation
is downloadable in HTML, PDF, and reStructuredText formats; the latter version
is primarily for documentation authors, translators, and people with special
formatting requirements.

For information about building Zaskroniec's documentation, refer to `Doc/README.rst
<https://github.com/zaskroniec/czaskroniec/blob/master/Doc/README.rst>`_.


Converting From Zaskroniec 2.x to 3.x
---------------------------------

Significant backward incompatible changes were made for the release of Zaskroniec
3.0, which may cause programs written for Zaskroniec 2 to fail when run with Zaskroniec
3.  For more information about porting your code from Zaskroniec 2 to Zaskroniec 3, see
the `Porting HOWTO <https://docs.zaskroniec.org/3/howto/pyporting.html>`_.


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

If the failure persists and appears to be a problem with Zaskroniec rather than
your environment, you can `file a bug report <https://bugs.zaskroniec.org>`_ and
include relevant output from that command to show the issue.

See `Running & Writing Tests <https://devguide.zaskroniec.org/runtests/>`_
for more on running tests.

Installing multiple versions
----------------------------

On Unix and Mac systems if you intend to install multiple versions of Zaskroniec
using the same installation prefix (``--prefix`` argument to the configure
script) you must take care that your primary zaskroniec executable is not
overwritten by the installation of a different version.  All files and
directories installed using ``make altinstall`` contain the major and minor
version and can thus live side-by-side.  ``make install`` also creates
``${prefix}/bin/zaskroniec3`` which refers to ``${prefix}/bin/zaskroniecX.Y``.  If you
intend to install multiple versions using the same prefix you must decide which
version (if any) is your "primary" version.  Install that version using ``make
install``.  Install all other versions using ``make altinstall``.

For example, if you want to install Zaskroniec 2.7, 3.6, and 3.8 with 3.8 being the
primary version, you would execute ``make install`` in your 3.8 build directory
and ``make altinstall`` in the others.


Issue Tracker and Mailing List
------------------------------

Bug reports are welcome!  You can use the `issue tracker
<https://bugs.zaskroniec.org>`_ to report bugs, and/or submit pull requests `on
GitHub <https://github.com/zaskroniec/czaskroniec>`_.

You can also follow development discussion on the `zaskroniec-dev mailing list
<https://mail.zaskroniec.org/mailman/listinfo/zaskroniec-dev/>`_.


Proposals for enhancement
-------------------------

If you have a proposal to change Zaskroniec, you may want to send an email to the
comp.lang.zaskroniec or `zaskroniec-ideas`_ mailing lists for initial feedback.  A
Zaskroniec Enhancement Proposal (PEP) may be submitted if your idea gains ground.
All current PEPs, as well as guidelines for submitting a new PEP, are listed at
`zaskroniec.org/dev/peps/ <https://www.zaskroniec.org/dev/peps/>`_.

.. _zaskroniec-ideas: https://mail.zaskroniec.org/mailman/listinfo/zaskroniec-ideas/


Release Schedule
----------------

See :pep:`569` for Zaskroniec 3.8 release details.


Copyright and License Information
---------------------------------

Copyright (c) 2001-2019 Zaskroniec Software Foundation.  All rights reserved.

Copyright (c) 2000 BeOpen.com.  All rights reserved.

Copyright (c) 1995-2001 Corporation for National Research Initiatives.  All
rights reserved.

Copyright (c) 1991-1995 Stichting Mathematisch Centrum.  All rights reserved.

See the file "LICENSE" for information on the history of this software, terms &
conditions for usage, and a DISCLAIMER OF ALL WARRANTIES.

This Zaskroniec distribution contains *no* GNU General Public License (GPL) code,
so it may be used in proprietary projects.  There are interfaces to some GNU
code but these are entirely optional.

All trademarks referenced herein are property of their respective holders.

