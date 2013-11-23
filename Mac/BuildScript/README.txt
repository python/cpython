Building a Python Mac OS X distribution
=======================================

The ``build-install.py`` script creates Python distributions, including
certain third-party libraries as necessary.  It builds a complete 
framework-based Python out-of-tree, installs it in a funny place with 
$DESTROOT, massages that installation to remove .pyc files and such, creates 
an Installer package from the installation plus other files in ``resources`` 
and ``scripts`` and placed that on a ``.dmg`` disk image.

For Python 3.4.0, PSF practice is to build two installer variants
for each release.

1.  32-bit-only, i386 and PPC universal, capable on running on all machines
    supported by Mac OS X 10.5 through (at least) 10.8::

        /usr/bin/python build-installer.py \
            --sdk-path=/Developer/SDKs/MacOSX10.5.sdk \
            --universal-archs=32-bit \
            --dep-target=10.5

    - builds the following third-party libraries

        * NCurses 5.9 (http://bugs.python.org/issue15037)
        * SQLite 3.8.1
        * XZ 5.0.5

    - uses system-supplied versions of third-party libraries

        * readline module links with Apple BSD editline (libedit)

    - requires ActiveState ``Tcl/Tk 8.4`` (currently 8.4.19) to be installed for building

    - recommended build environment:

        * Mac OS X 10.5.8 Intel or PPC
        * Xcode 3.1.4
        * ``MacOSX10.5`` SDK
        * ``MACOSX_DEPLOYMENT_TARGET=10.5``
        * Apple ``gcc-4.2``
        * system Python 2.5 for documentation build with Sphinx

    - alternate build environments:

        * Mac OS X 10.6.8 with Xcode 3.2.6
            - need to change ``/System/Library/Frameworks/{Tcl,Tk}.framework/Version/Current`` to ``8.4``
        * Note Xcode 4.* does not support building for PPC so cannot be used for this build

2.  64-bit / 32-bit, x86_64 and i386 universal, for OS X 10.6 (and later)::

        /usr/bin/python build-installer.py \
            --sdk-path=/Developer/SDKs/MacOSX10.6.sdk \
            --universal-archs=intel \
            --dep-target=10.6

    - builds the following third-party libraries

        * NCurses 5.9 (http://bugs.python.org/issue15037)
        * SQLite 3.8.1
        * XZ 5.0.5

    - uses system-supplied versions of third-party libraries

        * readline module links with Apple BSD editline (libedit)

    - requires ActiveState Tcl/Tk 8.5.15 (or later) to be installed for building

    - recommended build environment:

        * Mac OS X 10.6.8 (or later)
        * Xcode 3.2.6
        * ``MacOSX10.6`` SDK
        * ``MACOSX_DEPLOYMENT_TARGET=10.6``
        * Apple ``gcc-4.2``
        * system Python 2.6 for documentation build with Sphinx

    - alternate build environments:

        * none.  Xcode 4.x currently supplies two C compilers.
          ``llvm-gcc-4.2.1`` has been found to miscompile Python 3.3.x and
          produce a non-functional Python executable.  As it appears to be
          considered a migration aid by Apple and is not likely to be fixed,
          its use should be avoided.  The other compiler, ``clang``, has been
          undergoing rapid development.  While it appears to have become
          production-ready in the most recent Xcode 4 releases (Xcode 4.6.3
          as of this writing), there are still some open issues when
          building Python and there has not yet been the level of exposure in
          production environments that the Xcode 3 gcc-4.2 compiler has had.


*   For Python 2.7.x and 3.2.x, the 32-bit-only installer was configured to
    support Mac OS X 10.3.9 through (at least) 10.6.  Because it is
    believed that there are few systems still running OS X 10.3 or 10.4
    and because it has become increasingly difficult to test and
    support the differences in these earlier systems, as of Python 3.3.0 the PSF
    32-bit installer no longer supports them.  For reference in building such
    an installer yourself, the details are::

        /usr/bin/python build-installer.py \
            --sdk-path=/Developer/SDKs/MacOSX10.4u.sdk \
            --universal-archs=32-bit \
            --dep-target=10.3 

    - builds the following third-party libraries

        * Bzip2
        * NCurses
        * GNU Readline (GPL)
        * SQLite 3
        * XZ
        * Zlib 1.2.3
        * Oracle Sleepycat DB 4.8 (Python 2.x only)

    - requires ActiveState ``Tcl/Tk 8.4`` (currently 8.4.19) to be installed for building

    - recommended build environment:
        
        * Mac OS X 10.5.8 PPC or Intel
        * Xcode 3.1.4 (or later)
        * ``MacOSX10.4u`` SDK (later SDKs do not support PPC G3 processors)
        * ``MACOSX_DEPLOYMENT_TARGET=10.3``
        * Apple ``gcc-4.0``
        * system Python 2.5 for documentation build with Sphinx

    - alternate build environments:

        * Mac OS X 10.6.8 with Xcode 3.2.6
            - need to change ``/System/Library/Frameworks/{Tcl,Tk}.framework/Version/Current`` to ``8.4``



General Prerequisites
---------------------

* No Fink (in ``/sw``) or MacPorts (in ``/opt/local``) or other local
  libraries or utilities (in ``/usr/local``) as they could
  interfere with the build.

* The documentation for the release is built using Sphinx
  because it is included in the installer.

* It is safest to start each variant build with an empty source directory
  populated with a fresh copy of the untarred source.

* It is recommended that you remove any existing installed version of the
  Python being built::

      sudo rm -rf /Library/Frameworks/Python.framework/Versions/n.n


The Recipe
----------

Here are the steps you need to follow to build a Python installer:

* Run ``build-installer.py``. Optionally you can pass a number of arguments
  to specify locations of various files. Please see the top of
  ``build-installer.py`` for its usage.

  Running this script takes some time, it will not only build Python itself
  but also some 3th-party libraries that are needed for extensions.

* When done the script will tell you where the DMG image is (by default
  somewhere in ``/tmp/_py``).

Building other universal installers
...................................

It is also possible to build a 4-way universal installer that runs on 
OS X 10.5 Leopard or later::

    /usr/bin/python /build-installer.py \
        --dep-target=10.5
        --universal-archs=all
        --sdk-path=/Developer/SDKs/MacOSX10.5.sdk

This requires that the deployment target is 10.5, and hence
also that you are building on at least OS X 10.5.  4-way includes
``i386``, ``x86_64``, ``ppc``, and ``ppc64`` (G5).  ``ppc64`` executable
variants can only be run on G5 machines running 10.5.  Note that,
while OS X 10.6 is only supported on Intel-based machines, it is possible
to run ``ppc`` (32-bit) executables unmodified thanks to the Rosetta ppc
emulation in OS X 10.5 and 10.6.  The 4-way installer variant must be
built with Xcode 3.  It is not regularly built or tested.

Other ``--universal-archs`` options are ``64-bit`` (``x86_64``, ``ppc64``),
and ``3-way`` (``ppc``, ``i386``, ``x86_64``).  None of these options
are regularly exercised; use at your own risk.


Testing
-------

Ideally, the resulting binaries should be installed and the test suite run
on all supported OS X releases and architectures.  As a practical matter,
that is generally not possible.  At a minimum, variant 1 should be run on
a PPC G4 system with OS X 10.5 and at least one Intel system running OS X
10.8, 10.7, 10.6, or 10.5.  Variant 2 should be run on 10.8, 10.7, and 10.6
systems in both 32-bit and 64-bit modes.::

    /usr/local/bin/pythonn.n -m test -w -u all,-largefile
    /usr/local/bin/pythonn.n-32 -m test -w -u all
    
Certain tests will be skipped and some cause the interpreter to fail
which will likely generate ``Python quit unexpectedly`` alert messages
to be generated at several points during a test run.  These are normal
during testing and can be ignored.

It is also recommend to launch IDLE and verify that it is at least
functional.  Double-click on the IDLE app icon in ``/Applications/Pythonn.n``.
It should also be tested from the command line::

    /usr/local/bin/idlen.n

