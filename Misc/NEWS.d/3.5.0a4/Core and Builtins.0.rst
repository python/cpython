- Issue #22980: Under Linux, GNU/KFreeBSD and the Hurd, C extensions now include
  the architecture triplet in the extension name, to make it easy to test builds
  for different ABIs in the same working tree.  Under OS X, the extension name
  now includes PEP 3149-style information.

- Issue #22631: Added Linux-specific socket constant CAN_RAW_FD_FRAMES.
  Patch courtesy of Joe Jevnik.

- Issue #23731: Implement PEP 488: removal of .pyo files.

- Issue #23726: Don't enable GC for user subclasses of non-GC types that
  don't add any new fields.  Patch by Eugene Toder.

- Issue #23309: Avoid a deadlock at shutdown if a daemon thread is aborted
  while it is holding a lock to a buffered I/O object, and the main thread
  tries to use the same I/O object (typically stdout or stderr).  A fatal
  error is emitted instead.

- Issue #22977: Fixed formatting Windows error messages on Wine.
  Patch by Martin Panter.

- Issue #23466: %c, %o, %x, and %X in bytes formatting now raise TypeError on
  non-integer input.

- Issue #24044: Fix possible null pointer dereference in list.sort in out of
  memory conditions.

- Issue #21354: PyCFunction_New function is exposed by python DLL again.

