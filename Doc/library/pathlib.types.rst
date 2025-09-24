:mod:`!pathlib.types` --- Types for virtual filesystem paths
============================================================

.. module:: pathlib
   :synopsis: pathlib types for virtual filesystem paths

.. versionadded:: 3.14

**Source code:** :source:`Lib/pathlib/types.py`

--------------

The :mod:`pathlib.types` module provides protocols for user-defined types that
resemble :class:`pathlib.Path` and its associated classes. This module
includes type annotations, so it's also useful for static type checking.

.. versionadded:: 3.14


.. class:: PathInfo()

   A :class:`typing.Protocol` that supports fetching and caching information
   about a path. :class:`pathlib.Info` is an implementation of this protocol
   with additional methods specific to local filesystems.

   .. method:: exists(*, follow_symlinks=True)

      Return ``True`` if the path is an existing file or directory, or any
      other kind of file; return ``False`` if the path doesn't exist.

      If *follow_symlinks* is ``False``, return ``True`` for symlinks without
      checking if their targets exist.

   .. method:: is_dir(*, follow_symlinks=True)

      Return ``True`` if the path is a directory, or a symbolic link pointing
      to a directory; return ``False`` if the path is (or points to) any other
      kind of file, or if it doesn't exist.

      If *follow_symlinks* is ``False``, return ``True`` only if the path
      is a directory (without following symlinks); return ``False`` if the
      path is any other kind of file, or if it doesn't exist.

   .. method:: is_file(*, follow_symlinks=True)

      Return ``True`` if the path is a file, or a symbolic link pointing to
      a file; return ``False`` if the path is (or points to) a directory or
      other non-file, or if it doesn't exist.

      If *follow_symlinks* is ``False``, return ``True`` only if the path
      is a file (without following symlinks); return ``False`` if the path
      is a directory or other non-file, or if it doesn't exist.

   .. method:: is_symlink()

      Return ``True`` if the path is a symbolic link (even if broken); return
      ``False`` if the path is a directory or any kind of file, or if it
      doesn't exist.
