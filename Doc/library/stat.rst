:mod:`stat` --- Interpreting :func:`~os.stat` results
=====================================================

.. module:: stat
   :synopsis: Utilities for interpreting the results of os.stat(),
              os.lstat() and os.fstat().

.. sectionauthor:: Skip Montanaro <skip@automatrix.com>

**Source code:** :source:`Lib/stat.py`

--------------

The :mod:`stat` module defines constants and functions for interpreting the
results of :func:`os.stat`, :func:`os.fstat` and :func:`os.lstat` (if they
exist).  For complete details about the :c:func:`stat`, :c:func:`fstat` and
:c:func:`lstat` calls, consult the documentation for your system.

.. versionchanged:: 3.4
   The stat module is backed by a C implementation.

The :mod:`stat` module defines the following functions to test for specific file
types:


.. function:: S_ISDIR(mode)

   Return non-zero if the mode is from a directory.


.. function:: S_ISCHR(mode)

   Return non-zero if the mode is from a character special device file.


.. function:: S_ISBLK(mode)

   Return non-zero if the mode is from a block special device file.


.. function:: S_ISREG(mode)

   Return non-zero if the mode is from a regular file.


.. function:: S_ISFIFO(mode)

   Return non-zero if the mode is from a FIFO (named pipe).


.. function:: S_ISLNK(mode)

   Return non-zero if the mode is from a symbolic link.


.. function:: S_ISSOCK(mode)

   Return non-zero if the mode is from a socket.

.. function:: S_ISDOOR(mode)

   Return non-zero if the mode is from a door.

   .. versionadded:: 3.4

.. function:: S_ISPORT(mode)

   Return non-zero if the mode is from an event port.

   .. versionadded:: 3.4

.. function:: S_ISWHT(mode)

   Return non-zero if the mode is from a whiteout.

   .. versionadded:: 3.4

Two additional functions are defined for more general manipulation of the file's
mode:


.. function:: S_IMODE(mode)

   Return the portion of the file's mode that can be set by
   :func:`os.chmod`\ ---that is, the file's permission bits, plus the sticky
   bit, set-group-id, and set-user-id bits (on systems that support them).


.. function:: S_IFMT(mode)

   Return the portion of the file's mode that describes the file type (used by the
   :func:`S_IS\*` functions above).

Normally, you would use the :func:`os.path.is\*` functions for testing the type
of a file; the functions here are useful when you are doing multiple tests of
the same file and wish to avoid the overhead of the :c:func:`stat` system call
for each test.  These are also useful when checking for information about a file
that isn't handled by :mod:`os.path`, like the tests for block and character
devices.

Example::

   import os, sys
   from stat import *

   def walktree(top, callback):
       '''recursively descend the directory tree rooted at top,
          calling the callback function for each regular file'''

       for f in os.listdir(top):
           pathname = os.path.join(top, f)
           mode = os.stat(pathname).st_mode
           if S_ISDIR(mode):
               # It's a directory, recurse into it
               walktree(pathname, callback)
           elif S_ISREG(mode):
               # It's a file, call the callback function
               callback(pathname)
           else:
               # Unknown file type, print a message
               print('Skipping %s' % pathname)

   def visitfile(file):
       print('visiting', file)

   if __name__ == '__main__':
       walktree(sys.argv[1], visitfile)

An additional utility function is provided to convert a file's mode in a human
readable string:

.. function:: filemode(mode)

   Convert a file's mode to a string of the form '-rwxrwxrwx'.

   .. versionadded:: 3.3

   .. versionchanged:: 3.4
      The function supports :data:`S_IFDOOR`, :data:`S_IFPORT` and
      :data:`S_IFWHT`.


All the variables below are simply symbolic indexes into the 10-tuple returned
by :func:`os.stat`, :func:`os.fstat` or :func:`os.lstat`.


.. data:: ST_MODE

   Inode protection mode.


.. data:: ST_INO

   Inode number.


.. data:: ST_DEV

   Device inode resides on.


.. data:: ST_NLINK

   Number of links to the inode.


.. data:: ST_UID

   User id of the owner.


.. data:: ST_GID

   Group id of the owner.


.. data:: ST_SIZE

   Size in bytes of a plain file; amount of data waiting on some special files.


.. data:: ST_ATIME

   Time of last access.


.. data:: ST_MTIME

   Time of last modification.


.. data:: ST_CTIME

   The "ctime" as reported by the operating system.  On some systems (like Unix) is
   the time of the last metadata change, and, on others (like Windows), is the
   creation time (see platform documentation for details).

The interpretation of "file size" changes according to the file type.  For plain
files this is the size of the file in bytes.  For FIFOs and sockets under most
flavors of Unix (including Linux in particular), the "size" is the number of
bytes waiting to be read at the time of the call to :func:`os.stat`,
:func:`os.fstat`, or :func:`os.lstat`; this can sometimes be useful, especially
for polling one of these special files after a non-blocking open.  The meaning
of the size field for other character and block devices varies more, depending
on the implementation of the underlying system call.

The variables below define the flags used in the :data:`ST_MODE` field.

Use of the functions above is more portable than use of the first set of flags:

.. data:: S_IFSOCK

   Socket.

.. data:: S_IFLNK

   Symbolic link.

.. data:: S_IFREG

   Regular file.

.. data:: S_IFBLK

   Block device.

.. data:: S_IFDIR

   Directory.

.. data:: S_IFCHR

   Character device.

.. data:: S_IFIFO

   FIFO.

.. data:: S_IFDOOR

   Door.

   .. versionadded:: 3.4

.. data:: S_IFPORT

   Event port.

   .. versionadded:: 3.4

.. data:: S_IFWHT

   Whiteout.

   .. versionadded:: 3.4

.. note::

   :data:`S_IFDOOR`, :data:`S_IFPORT` or :data:`S_IFWHT` are defined as
   0 when the platform does not have support for the file types.

The following flags can also be used in the *mode* argument of :func:`os.chmod`:

.. data:: S_ISUID

   Set UID bit.

.. data:: S_ISGID

   Set-group-ID bit.  This bit has several special uses.  For a directory
   it indicates that BSD semantics is to be used for that directory:
   files created there inherit their group ID from the directory, not
   from the effective group ID of the creating process, and directories
   created there will also get the :data:`S_ISGID` bit set.  For a
   file that does not have the group execution bit (:data:`S_IXGRP`)
   set, the set-group-ID bit indicates mandatory file/record locking
   (see also :data:`S_ENFMT`).

.. data:: S_ISVTX

   Sticky bit.  When this bit is set on a directory it means that a file
   in that directory can be renamed or deleted only by the owner of the
   file, by the owner of the directory, or by a privileged process.

.. data:: S_IRWXU

   Mask for file owner permissions.

.. data:: S_IRUSR

   Owner has read permission.

.. data:: S_IWUSR

   Owner has write permission.

.. data:: S_IXUSR

   Owner has execute permission.

.. data:: S_IRWXG

   Mask for group permissions.

.. data:: S_IRGRP

   Group has read permission.

.. data:: S_IWGRP

   Group has write permission.

.. data:: S_IXGRP

   Group has execute permission.

.. data:: S_IRWXO

   Mask for permissions for others (not in group).

.. data:: S_IROTH

   Others have read permission.

.. data:: S_IWOTH

   Others have write permission.

.. data:: S_IXOTH

   Others have execute permission.

.. data:: S_ENFMT

   System V file locking enforcement.  This flag is shared with :data:`S_ISGID`:
   file/record locking is enforced on files that do not have the group
   execution bit (:data:`S_IXGRP`) set.

.. data:: S_IREAD

   Unix V7 synonym for :data:`S_IRUSR`.

.. data:: S_IWRITE

   Unix V7 synonym for :data:`S_IWUSR`.

.. data:: S_IEXEC

   Unix V7 synonym for :data:`S_IXUSR`.

The following flags can be used in the *flags* argument of :func:`os.chflags`:

.. data:: UF_NODUMP

   Do not dump the file.

.. data:: UF_IMMUTABLE

   The file may not be changed.

.. data:: UF_APPEND

   The file may only be appended to.

.. data:: UF_OPAQUE

   The directory is opaque when viewed through a union stack.

.. data:: UF_NOUNLINK

   The file may not be renamed or deleted.

.. data:: UF_COMPRESSED

   The file is stored compressed (Mac OS X 10.6+).

.. data:: UF_HIDDEN

   The file should not be displayed in a GUI (Mac OS X 10.5+).

.. data:: SF_ARCHIVED

   The file may be archived.

.. data:: SF_IMMUTABLE

   The file may not be changed.

.. data:: SF_APPEND

   The file may only be appended to.

.. data:: SF_NOUNLINK

   The file may not be renamed or deleted.

.. data:: SF_SNAPSHOT

   The file is a snapshot file.

See the \*BSD or Mac OS systems man page :manpage:`chflags(2)` for more information.

On Windows, the following file attribute constants are available for use when
testing bits in the ``st_file_attributes`` member returned by :func:`os.stat`.
See the `Windows API documentation
<https://msdn.microsoft.com/en-us/library/windows/desktop/gg258117.aspx>`_
for more detail on the meaning of these constants.

.. data:: FILE_ATTRIBUTE_ARCHIVE
          FILE_ATTRIBUTE_COMPRESSED
          FILE_ATTRIBUTE_DEVICE
          FILE_ATTRIBUTE_DIRECTORY
          FILE_ATTRIBUTE_ENCRYPTED
          FILE_ATTRIBUTE_HIDDEN
          FILE_ATTRIBUTE_INTEGRITY_STREAM
          FILE_ATTRIBUTE_NORMAL
          FILE_ATTRIBUTE_NOT_CONTENT_INDEXED
          FILE_ATTRIBUTE_NO_SCRUB_DATA
          FILE_ATTRIBUTE_OFFLINE
          FILE_ATTRIBUTE_READONLY
          FILE_ATTRIBUTE_REPARSE_POINT
          FILE_ATTRIBUTE_SPARSE_FILE
          FILE_ATTRIBUTE_SYSTEM
          FILE_ATTRIBUTE_TEMPORARY
          FILE_ATTRIBUTE_VIRTUAL

   .. versionadded:: 3.5
