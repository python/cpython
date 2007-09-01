
:mod:`stat` --- Interpreting :func:`stat` results
=================================================

.. module:: stat
   :synopsis: Utilities for interpreting the results of os.stat(), os.lstat() and os.fstat().
.. sectionauthor:: Skip Montanaro <skip@automatrix.com>


The :mod:`stat` module defines constants and functions for interpreting the
results of :func:`os.stat`, :func:`os.fstat` and :func:`os.lstat` (if they
exist).  For complete details about the :cfunc:`stat`, :cfunc:`fstat` and
:cfunc:`lstat` calls, consult the documentation for your system.

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

Two additional functions are defined for more general manipulation of the file's
mode:


.. function:: S_IMODE(mode)

   Return the portion of the file's mode that can be set by :func:`os.chmod`\
   ---that is, the file's permission bits, plus the sticky bit, set-group-id, and
   set-user-id bits (on systems that support them).


.. function:: S_IFMT(mode)

   Return the portion of the file's mode that describes the file type (used by the
   :func:`S_IS\*` functions above).

Normally, you would use the :func:`os.path.is\*` functions for testing the type
of a file; the functions here are useful when you are doing multiple tests of
the same file and wish to avoid the overhead of the :cfunc:`stat` system call
for each test.  These are also useful when checking for information about a file
that isn't handled by :mod:`os.path`, like the tests for block and character
devices.

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

Example::

   import os, sys
   from stat import *

   def walktree(top, callback):
       '''recursively descend the directory tree rooted at top,
          calling the callback function for each regular file'''

       for f in os.listdir(top):
           pathname = os.path.join(top, f)
           mode = os.stat(pathname)[ST_MODE]
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

