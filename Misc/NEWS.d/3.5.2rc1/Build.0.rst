- Issue #26884: Fix linking extension modules for cross builds.
  Patch by Xavier de Gaye.

- Issue #22359: Disable the rules for running _freeze_importlib and pgen when
  cross-compiling.  The output of these programs is normally saved with the
  source code anyway, and is still regenerated when doing a native build.
  Patch by Xavier de Gaye.

- Issue #27229: Fix the cross-compiling pgen rule for in-tree builds.  Patch
  by Xavier de Gaye.

- Issue #21668: Link audioop, _datetime, _ctypes_test modules to libm,
  except on Mac OS X. Patch written by Xavier de Gaye.

- Issue #25702: A --with-lto configure option has been added that will
  enable link time optimizations at build time during a make profile-opt.
  Some compilers and toolchains are known to not produce stable code when
  using LTO, be sure to test things thoroughly before relying on it.
  It can provide a few % speed up over profile-opt alone.

- Issue #26624: Adds validation of ucrtbase[d].dll version with warning
  for old versions.

- Issue #17603: Avoid error about nonexistant fileblocks.o file by using a
  lower-level check for st_blocks in struct stat.

- Issue #26079: Fixing the build output folder for tix-8.4.3.6. Patch by
  Bjoern Thiel.

- Issue #26465: Update Windows builds to use OpenSSL 1.0.2g.

- Issue #24421: Compile Modules/_math.c once, before building extensions.
  Previously it could fail to compile properly if the math and cmath builds
  were concurrent.

- Issue #25348: Added ``--pgo`` and ``--pgo-job`` arguments to
  ``PCbuild\build.bat`` for building with Profile-Guided Optimization.  The
  old ``PCbuild\build_pgo.bat`` script is now deprecated, and simply calls
  ``PCbuild\build.bat --pgo %*``.

- Issue #25827: Add support for building with ICC to ``configure``, including
  a new ``--with-icc`` flag.

- Issue #25696: Fix installation of Python on UNIX with make -j9.

- Issue #26930: Update OS X 10.5+ 32-bit-only installer to build
  and link with OpenSSL 1.0.2h.

- Issue #26268: Update Windows builds to use OpenSSL 1.0.2f.

- Issue #25136: Support Apple Xcode 7's new textual SDK stub libraries.

- Issue #24324: Do not enable unreachable code warnings when using
  gcc as the option does not work correctly in older versions of gcc
  and has been silently removed as of gcc-4.5.

