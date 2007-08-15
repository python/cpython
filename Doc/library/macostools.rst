
:mod:`macostools` --- Convenience routines for file manipulation
================================================================

.. module:: macostools
   :platform: Mac
   :synopsis: Convenience routines for file manipulation.


This module contains some convenience routines for file-manipulation on the
Macintosh. All file parameters can be specified as pathnames, :class:`FSRef` or
:class:`FSSpec` objects.  This module expects a filesystem which supports forked
files, so it should not be used on UFS partitions.

The :mod:`macostools` module defines the following functions:


.. function:: copy(src, dst[, createpath[, copytimes]])

   Copy file *src* to *dst*.  If *createpath* is non-zero the folders leading to
   *dst* are created if necessary. The method copies data and resource fork and
   some finder information (creator, type, flags) and optionally the creation,
   modification and backup times (default is to copy them). Custom icons, comments
   and icon position are not copied.


.. function:: copytree(src, dst)

   Recursively copy a file tree from *src* to *dst*, creating folders as needed.
   *src* and *dst* should be specified as pathnames.


.. function:: mkalias(src, dst)

   Create a finder alias *dst* pointing to *src*.


.. function:: touched(dst)

   Tell the finder that some bits of finder-information such as creator or type for
   file *dst* has changed. The file can be specified by pathname or fsspec. This
   call should tell the finder to redraw the files icon.

   .. deprecated:: 2.6
      The function is a no-op on OS X.


.. data:: BUFSIZ

   The buffer size for ``copy``, default 1 megabyte.

Note that the process of creating finder aliases is not specified in the Apple
documentation. Hence, aliases created with :func:`mkalias` could conceivably
have incompatible behaviour in some cases.


:mod:`findertools` --- The :program:`finder`'s Apple Events interface
=====================================================================

.. module:: findertools
   :platform: Mac
   :synopsis: Wrappers around the finder's Apple Events interface.


.. index:: single: AppleEvents

This module contains routines that give Python programs access to some
functionality provided by the finder. They are implemented as wrappers around
the AppleEvent interface to the finder.

All file and folder parameters can be specified either as full pathnames, or as
:class:`FSRef` or :class:`FSSpec` objects.

The :mod:`findertools` module defines the following functions:


.. function:: launch(file)

   Tell the finder to launch *file*. What launching means depends on the file:
   applications are started, folders are opened and documents are opened in the
   correct application.


.. function:: Print(file)

   Tell the finder to print a file. The behaviour is identical to selecting the
   file and using the print command in the finder's file menu.


.. function:: copy(file, destdir)

   Tell the finder to copy a file or folder *file* to folder *destdir*. The
   function returns an :class:`Alias` object pointing to the new file.


.. function:: move(file, destdir)

   Tell the finder to move a file or folder *file* to folder *destdir*. The
   function returns an :class:`Alias` object pointing to the new file.


.. function:: sleep()

   Tell the finder to put the Macintosh to sleep, if your machine supports it.


.. function:: restart()

   Tell the finder to perform an orderly restart of the machine.


.. function:: shutdown()

   Tell the finder to perform an orderly shutdown of the machine.

