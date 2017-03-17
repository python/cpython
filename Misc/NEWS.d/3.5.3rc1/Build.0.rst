- Issue #29080: Removes hard dependency on hg.exe from PCBuild/build.bat

- Issue #23903: Added missed names to PC/python3.def.

- Issue #10656: Fix out-of-tree building on AIX.  Patch by Tristan Carel and
  Michael Haubenwallner.

- Issue #26359: Rename --with-optimiations to --enable-optimizations.

- Issue #28444: Fix missing extensions modules when cross compiling.

- Issue #28248: Update Windows build and OS X installers to use OpenSSL 1.0.2j.

- Issue #28258: Fixed build with Estonian locale (python-config and distclean
  targets in Makefile).  Patch by Arfrever Frehtes Taifersar Arahesis.

- Issue #26661: setup.py now detects system libffi with multiarch wrapper.

- Issue #28066: Fix the logic that searches build directories for generated
  include files when building outside the source tree.

- Issue #15819: Remove redundant include search directory option for building
  outside the source tree.

- Issue #27566: Fix clean target in freeze makefile (patch by Lisa Roach)

- Issue #27705: Update message in validate_ucrtbase.py

- Issue #27983: Cause lack of llvm-profdata tool when using clang as
  required for PGO linking to be a configure time error rather than
  make time when --with-optimizations is enabled.  Also improve our
  ability to find the llvm-profdata tool on MacOS and some Linuxes.

- Issue #26307: The profile-opt build now applies PGO to the built-in modules.

- Issue #26359: Add the --with-optimizations configure flag.

- Issue #27713: Suppress spurious build warnings when updating importlib's
  bootstrap files.  Patch by Xiang Zhang

- Issue #25825: Correct the references to Modules/python.exp and ld_so_aix,
  which are required on AIX.  This updates references to an installation path
  that was changed in 3.2a4, and undoes changed references to the build tree
  that were made in 3.5.0a1.

- Issue #27453: CPP invocation in configure must use CPPFLAGS. Patch by
  Chi Hsuan Yen.

- Issue #27641: The configure script now inserts comments into the makefile
  to prevent the pgen and _freeze_importlib executables from being cross-
  compiled.

- Issue #26662: Set PYTHON_FOR_GEN in configure as the Python program to be
  used for file generation during the build.

- Issue #10910: Avoid C++ compilation errors on FreeBSD and OS X.
  Also update FreedBSD version checks for the original ctype UTF-8 workaround.

- Issue #28676: Prevent missing 'getentropy' declaration warning on macOS.
  Patch by Gareth Rees.

