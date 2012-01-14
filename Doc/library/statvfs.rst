:mod:`statvfs` --- Constants used with :func:`os.statvfs`
=========================================================

.. module:: statvfs
   :synopsis: Constants for interpreting the result of os.statvfs().
   :deprecated:

.. deprecated:: 2.6
   The :mod:`statvfs` module has been deprecated for removal in Python 3.0.


.. sectionauthor:: Moshe Zadka <moshez@zadka.site.co.il>


The :mod:`statvfs` module defines constants so interpreting the result if
:func:`os.statvfs`, which returns a tuple, can be made without remembering
"magic numbers."  Each of the constants defined in this module is the *index* of
the entry in the tuple returned by :func:`os.statvfs` that contains the
specified information.


.. data:: F_BSIZE

   Preferred file system block size.


.. data:: F_FRSIZE

   Fundamental file system block size.


.. data:: F_BLOCKS

   Total number of blocks in the filesystem.


.. data:: F_BFREE

   Total number of free blocks.


.. data:: F_BAVAIL

   Free blocks available to non-super user.


.. data:: F_FILES

   Total number of file nodes.


.. data:: F_FFREE

   Total number of free file nodes.


.. data:: F_FAVAIL

   Free nodes available to non-super user.


.. data:: F_FLAG

   Flags. System dependent: see :c:func:`statvfs` man page.


.. data:: F_NAMEMAX

   Maximum file name length.

