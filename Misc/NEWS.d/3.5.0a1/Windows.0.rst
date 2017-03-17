- Issue #23260: Update Windows installer

- The bundled version of Tcl/Tk has been updated to 8.6.3.  The most visible
  result of this change is the addition of new native file dialogs when
  running on Windows Vista or newer.  See Tcl/Tk's TIP 432 for more
  information.  Also, this version of Tcl/Tk includes support for Windows 10.

- Issue #17896: The Windows build scripts now expect external library sources
  to be in ``PCbuild\..\externals`` rather than ``PCbuild\..\..``.

- Issue #17717: The Windows build scripts now use a copy of NASM pulled from
  svn.python.org to build OpenSSL.

- Issue #21907: Improved the batch scripts provided for building Python.

- Issue #22644: The bundled version of OpenSSL has been updated to 1.0.1j.

- Issue #10747: Use versioned labels in the Windows start menu.
  Patch by Olive Kilburn.

