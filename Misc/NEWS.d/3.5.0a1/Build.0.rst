- Issue #15506: Use standard PKG_PROG_PKG_CONFIG autoconf macro in the configure
  script.

- Issue #22935: Allow the ssl module to be compiled if openssl doesn't support
  SSL 3.

- Issue #22592: Drop support of the Borland C compiler to build Python. The
  distutils module still supports it to build extensions.

- Issue #22591: Drop support of MS-DOS, especially of the DJGPP compiler
  (MS-DOS port of GCC).

- Issue #16537: Check whether self.extensions is empty in setup.py. Patch by
  Jonathan Hosmer.

- Issue #22359: Remove incorrect uses of recursive make.  Patch by Jonas
  Wagner.

- Issue #21958: Define HAVE_ROUND when building with Visual Studio 2013 and
  above.  Patch by Zachary Turner.

- Issue #18093: the programs that embed the CPython runtime are now in a
  separate "Programs" directory, rather than being kept in the Modules
  directory.

- Issue #15759: "make suspicious", "make linkcheck" and "make doctest" in Doc/
  now display special message when and only when there are failures.

- Issue #21141: The Windows build process no longer attempts to find Perl,
  instead relying on OpenSSL source being configured and ready to build.  The
  ``PCbuild\build_ssl.py`` script has been re-written and re-named to
  ``PCbuild\prepare_ssl.py``, and takes care of configuring OpenSSL source
  for both 32 and 64 bit platforms.  OpenSSL sources obtained from
  svn.python.org will always be pre-configured and ready to build.

- Issue #21037: Add a build option to enable AddressSanitizer support.

- Issue #19962: The Windows build process now creates "python.bat" in the
  root of the source tree, which passes all arguments through to the most
  recently built interpreter.

- Issue #21285: Refactor and fix curses configure check to always search
  in a ncursesw directory.

- Issue #15234: For BerkelyDB and Sqlite, only add the found library and
  include directories if they aren't already being searched. This avoids
  an explicit runtime library dependency.

- Issue #17861: Tools/scripts/generate_opcode_h.py automatically regenerates
  Include/opcode.h from Lib/opcode.py if the latter gets any change.

- Issue #20644: OS X installer build support for documentation build changes
  in 3.4.1: assume externally supplied sphinx-build is available in /usr/bin.

- Issue #20022: Eliminate use of deprecated bundlebuilder in OS X builds.

- Issue #15968: Incorporated Tcl, Tk, and Tix builds into the Windows build
  solution.

- Issue #17095: Fix Modules/Setup *shared* support.

- Issue #21811: Anticipated fixes to support OS X versions > 10.9.

- Issue #21166: Prevent possible segfaults and other random failures of
  python --generate-posix-vars in pybuilddir.txt build target.

- Issue #18096: Fix library order returned by python-config.

- Issue #17219: Add library build dir for Python extension cross-builds.

- Issue #22919: Windows build updated to support VC 14.0 (Visual Studio 2015),
  which will be used for the official release.

- Issue #21236: Build _msi.pyd with cabinet.lib instead of fci.lib

- Issue #17128: Use private version of OpenSSL for OS X 10.5+ installer.

