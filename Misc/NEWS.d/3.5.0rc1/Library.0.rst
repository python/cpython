- Issue #24824: Signatures of codecs.encode() and codecs.decode() now are
  compatible with pydoc.

- Issue #24634: Importing uuid should not try to load libc on Windows

- Issue #24798: _msvccompiler.py doesn't properly support manifests

- Issue #4395: Better testing and documentation of binary operators.
  Patch by Martin Panter.

- Issue #23973: Update typing.py from GitHub repo.

- Issue #23004: mock_open() now reads binary data correctly when the type of
  read_data is bytes.  Initial patch by Aaron Hill.

- Issue #23888: Handle fractional time in cookie expiry. Patch by ssh.

- Issue #23652: Make it possible to compile the select module against the
  libc headers from the Linux Standard Base, which do not include some
  EPOLL macros.  Patch by Matt Frank.

- Issue #22932: Fix timezones in email.utils.formatdate.
  Patch from Dmitry Shachnev.

- Issue #23779: imaplib raises TypeError if authenticator tries to abort.
  Patch from Craig Holmquist.

- Issue #23319: Fix ctypes.BigEndianStructure, swap correctly bytes. Patch
  written by Matthieu Gautier.

- Issue #23254: Document how to close the TCPServer listening socket.
  Patch from Martin Panter.

- Issue #19450: Update Windows and OS X installer builds to use SQLite 3.8.11.

- Issue #17527: Add PATCH to wsgiref.validator. Patch from Luca Sbardella.

- Issue #24791: Fix grammar regression for call syntax: 'g(\*a or b)'.

