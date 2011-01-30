Building a Python Mac OS X distribution
=======================================

The ``build-install.py`` script creates Python distributions, including
certain third-party libraries as necessary.  It builds a complete 
framework-based Python out-of-tree, installs it in a funny place with 
$DESTROOT, massages that installation to remove .pyc files and such, creates 
an Installer package from the installation plus other files in ``resources`` 
and ``scripts`` and placed that on a ``.dmg`` disk image.

As of Python 2.7.x and 3.2, PSF practice is to build two installer variants
for each release:

1.  32-bit-only, i386 and PPC universal, capable on running on all machines
    supported by Mac OS X 10.3.9 through (at least) 10.6::

        python build-installer.py \
            --sdk-path=/Developer/SDKs/MacOSX10.4u.sdk \
            --universal-archs=32-bit \
            --dep-target=10.3 
            # These are the current default options

    - builds the following third-party libraries

        * Bzip2
        * Zlib 1.2.3
        * GNU Readline (GPL)
        * SQLite 3
        * NCurses
        * Oracle Sleepycat DB 4.8 (Python 2.x only)

    - requires ActiveState ``Tcl/Tk 8.4`` (currently 8.4.19) to be installed for building

    - current target build environment:
        
        * Mac OS X 10.5.8 PPC or Intel
        * Xcode 3.1.4 (or later)
        * ``MacOSX10.4u`` SDK (later SDKs do not support PPC G3 processors)
        * ``MACOSX_DEPLOYMENT_TARGET=10.3``
        * Apple ``gcc-4.0``
        * Python 2.n (n >= 4) for documentation build with Sphinx

    - alternate build environments:

        * Mac OS X 10.4.11 with Xcode 2.5
        * Mac OS X 10.6.6 with Xcode 3.2.5
            - need to change ``/System/Library/Frameworks/{Tcl,Tk}.framework/Version/Current`` to ``8.4``

2.  64-bit / 32-bit, x86_64 and i386 universal, for OS X 10.6 (and later)::

        python build-installer.py \
            --sdk-path=/Developer/SDKs/MacOSX10.6.sdk \
            --universal-archs=intel \
            --dep-target=10.6

    - uses system-supplied versions of third-party libraries
    
        * readline module links with Apple BSD editline (libedit)
        * builds Oracle Sleepycat DB 4.8 (Python 2.x only)

    - requires ActiveState Tcl/Tk 8.5.9 (or later) to be installed for building

    - current target build environment:
        
        * Mac OS X 10.6.6 (or later)
        * Xcode 3.2.5 (or later)
        * ``MacOSX10.6`` SDK
        * ``MACOSX_DEPLOYMENT_TARGET=10.6``
        * Apple ``gcc-4.2``
        * Python 2.n (n >= 4) for documentation build with Sphinx

    - alternate build environments:

        * none


General Prerequisites
---------------------

* No Fink (in ``/sw``) or MacPorts (in ``/opt/local``) or other local
  libraries or utilities (in ``/usr/local``) as they could
  interfere with the build.

* The documentation for the release is built using Sphinx
  because it is included in the installer.

* It is safest to start each variant build with an empty source directory
  populated with a fresh copy of the untarred source.


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
OS X Leopard or later::

  python 2.6 /build-installer.py \
        --dep-target=10.5
        --universal-archs=all
        --sdk-path=/Developer/SDKs/MacOSX10.5.sdk

This requires that the deployment target is 10.5, and hence
also that you are building on at least OS X 10.5.  4-way includes
``i386``, ``x86_64``, ``ppc``, and ``ppc64`` (G5).  ``ppc64`` executable
variants can only be run on G5 machines running 10.5.  Note that,
while OS X 10.6 is only supported on Intel-based machines, it is possible
to run ``ppc`` (32-bit) executables unmodified thanks to the Rosetta ppc
emulation in OS X 10.5 and 10.6.

Other ``--universal-archs`` options are ``64-bit`` (``x86_64``, ``ppc64``),
and ``3-way`` (``ppc``, ``i386``, ``x86_64``).  None of these options
are regularly exercised; use at your own risk.


Testing
-------

Ideally, the resulting binaries should be installed and the test suite run
on all supported OS X releases and architectures.  As a practical matter,
that is generally not possible.  At a minimum, variant 1 should be run on
at least one Intel, one PPC G4, and one PPC G3 system and one each of 
OS X 10.6, 10.5, 10.4, and 10.3.9.  Not all tests run on 10.3.9.
Variant 2 should be run on 10.6 in both 32-bit and 64-bit modes.::

    arch -i386 /usr/local/bin/pythonn.n -m test.regrtest -w -u all 
    arch -X86_64 /usr/local/bin/pythonn.n -m test.regrtest -w -u all
    
Certain tests will be skipped and some cause the interpreter to fail
which will likely generate ``Python quit unexpectedly`` alert messages
to be generated at several points during a test run.  These can
be ignored.

