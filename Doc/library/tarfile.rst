.. _tarfile-mod:

:mod:`tarfile` --- Read and write tar archive files
===================================================

.. module:: tarfile
   :synopsis: Read and write tar-format archive files.


.. versionadded:: 2.3

.. moduleauthor:: Lars Gustäbel <lars@gustaebel.de>
.. sectionauthor:: Lars Gustäbel <lars@gustaebel.de>


The :mod:`tarfile` module makes it possible to read and create tar archives.
Some facts and figures:

* reads and writes :mod:`gzip` and :mod:`bzip2` compressed archives.

* read/write support for the POSIX.1-1988 (ustar) format.

* read/write support for the GNU tar format including *longname* and *longlink*
  extensions, read-only support for the *sparse* extension.

* read/write support for the POSIX.1-2001 (pax) format.

  .. versionadded:: 2.6

* handles directories, regular files, hardlinks, symbolic links, fifos,
  character devices and block devices and is able to acquire and restore file
  information like timestamp, access permissions and owner.

* can handle tape devices.


.. function:: open(name[, mode[, fileobj[, bufsize]]], **kwargs)

   Return a :class:`TarFile` object for the pathname *name*. For detailed
   information on :class:`TarFile` objects and the keyword arguments that are
   allowed, see :ref:`tarfile-objects`.

   *mode* has to be a string of the form ``'filemode[:compression]'``, it defaults
   to ``'r'``. Here is a full list of mode combinations:

   +------------------+---------------------------------------------+
   | mode             | action                                      |
   +==================+=============================================+
   | ``'r' or 'r:*'`` | Open for reading with transparent           |
   |                  | compression (recommended).                  |
   +------------------+---------------------------------------------+
   | ``'r:'``         | Open for reading exclusively without        |
   |                  | compression.                                |
   +------------------+---------------------------------------------+
   | ``'r:gz'``       | Open for reading with gzip compression.     |
   +------------------+---------------------------------------------+
   | ``'r:bz2'``      | Open for reading with bzip2 compression.    |
   +------------------+---------------------------------------------+
   | ``'a' or 'a:'``  | Open for appending with no compression. The |
   |                  | file is created if it does not exist.       |
   +------------------+---------------------------------------------+
   | ``'w' or 'w:'``  | Open for uncompressed writing.              |
   +------------------+---------------------------------------------+
   | ``'w:gz'``       | Open for gzip compressed writing.           |
   +------------------+---------------------------------------------+
   | ``'w:bz2'``      | Open for bzip2 compressed writing.          |
   +------------------+---------------------------------------------+

   Note that ``'a:gz'`` or ``'a:bz2'`` is not possible. If *mode* is not suitable
   to open a certain (compressed) file for reading, :exc:`ReadError` is raised. Use
   *mode* ``'r'`` to avoid this.  If a compression method is not supported,
   :exc:`CompressionError` is raised.

   If *fileobj* is specified, it is used as an alternative to a file object opened
   for *name*. It is supposed to be at position 0.

   For special purposes, there is a second format for *mode*:
   ``'filemode|[compression]'``.  :func:`open` will return a :class:`TarFile`
   object that processes its data as a stream of blocks.  No random seeking will
   be done on the file. If given, *fileobj* may be any object that has a
   :meth:`read` or :meth:`write` method (depending on the *mode*). *bufsize*
   specifies the blocksize and defaults to ``20 * 512`` bytes. Use this variant
   in combination with e.g. ``sys.stdin``, a socket file object or a tape
   device. However, such a :class:`TarFile` object is limited in that it does
   not allow to be accessed randomly, see :ref:`tar-examples`.  The currently
   possible modes:

   +-------------+--------------------------------------------+
   | Mode        | Action                                     |
   +=============+============================================+
   | ``'r|*'``   | Open a *stream* of tar blocks for reading  |
   |             | with transparent compression.              |
   +-------------+--------------------------------------------+
   | ``'r|'``    | Open a *stream* of uncompressed tar blocks |
   |             | for reading.                               |
   +-------------+--------------------------------------------+
   | ``'r|gz'``  | Open a gzip compressed *stream* for        |
   |             | reading.                                   |
   +-------------+--------------------------------------------+
   | ``'r|bz2'`` | Open a bzip2 compressed *stream* for       |
   |             | reading.                                   |
   +-------------+--------------------------------------------+
   | ``'w|'``    | Open an uncompressed *stream* for writing. |
   +-------------+--------------------------------------------+
   | ``'w|gz'``  | Open an gzip compressed *stream* for       |
   |             | writing.                                   |
   +-------------+--------------------------------------------+
   | ``'w|bz2'`` | Open an bzip2 compressed *stream* for      |
   |             | writing.                                   |
   +-------------+--------------------------------------------+


.. class:: TarFile

   Class for reading and writing tar archives. Do not use this class directly,
   better use :func:`open` instead. See :ref:`tarfile-objects`.


.. function:: is_tarfile(name)

   Return :const:`True` if *name* is a tar archive file, that the :mod:`tarfile`
   module can read.


.. class:: TarFileCompat(filename[, mode[, compression]])

   Class for limited access to tar archives with a :mod:`zipfile`\ -like interface.
   Please consult the documentation of the :mod:`zipfile` module for more details.
   *compression* must be one of the following constants:


   .. data:: TAR_PLAIN

      Constant for an uncompressed tar archive.


   .. data:: TAR_GZIPPED

      Constant for a :mod:`gzip` compressed tar archive.


.. exception:: TarError

   Base class for all :mod:`tarfile` exceptions.


.. exception:: ReadError

   Is raised when a tar archive is opened, that either cannot be handled by the
   :mod:`tarfile` module or is somehow invalid.


.. exception:: CompressionError

   Is raised when a compression method is not supported or when the data cannot be
   decoded properly.


.. exception:: StreamError

   Is raised for the limitations that are typical for stream-like :class:`TarFile`
   objects.


.. exception:: ExtractError

   Is raised for *non-fatal* errors when using :meth:`extract`, but only if
   :attr:`TarFile.errorlevel`\ ``== 2``.


.. exception:: HeaderError

   Is raised by :meth:`frombuf` if the buffer it gets is invalid.

   .. versionadded:: 2.6

Each of the following constants defines a tar archive format that the
:mod:`tarfile` module is able to create. See section :ref:`tar-formats` for
details.


.. data:: USTAR_FORMAT

   POSIX.1-1988 (ustar) format.


.. data:: GNU_FORMAT

   GNU tar format.


.. data:: PAX_FORMAT

   POSIX.1-2001 (pax) format.


.. data:: DEFAULT_FORMAT

   The default format for creating archives. This is currently :const:`GNU_FORMAT`.


.. seealso::

   Module :mod:`zipfile`
      Documentation of the :mod:`zipfile` standard module.

   `GNU tar manual, Basic Tar Format <http://www.gnu.org/software/tar/manual/html_node/tar_134.html#SEC134>`_
      Documentation for tar archive files, including GNU tar extensions.

.. % -----------------
.. % TarFile Objects
.. % -----------------


.. _tarfile-objects:

TarFile Objects
---------------

The :class:`TarFile` object provides an interface to a tar archive. A tar
archive is a sequence of blocks. An archive member (a stored file) is made up of
a header block followed by data blocks. It is possible to store a file in a tar
archive several times. Each archive member is represented by a :class:`TarInfo`
object, see :ref:`tarinfo-objects` for details.


.. class:: TarFile(name=None, mode='r', fileobj=None, format=DEFAULT_FORMAT, tarinfo=TarInfo, dereference=False, ignore_zeros=False, encoding=None, errors=None, pax_headers=None, debug=0, errorlevel=0)

   All following arguments are optional and can be accessed as instance attributes
   as well.

   *name* is the pathname of the archive. It can be omitted if *fileobj* is given.
   In this case, the file object's :attr:`name` attribute is used if it exists.

   *mode* is either ``'r'`` to read from an existing archive, ``'a'`` to append
   data to an existing file or ``'w'`` to create a new file overwriting an existing
   one.

   If *fileobj* is given, it is used for reading or writing data. If it can be
   determined, *mode* is overridden by *fileobj*'s mode. *fileobj* will be used
   from position 0.

   .. note::

      *fileobj* is not closed, when :class:`TarFile` is closed.

   *format* controls the archive format. It must be one of the constants
   :const:`USTAR_FORMAT`, :const:`GNU_FORMAT` or :const:`PAX_FORMAT` that are
   defined at module level.

   .. versionadded:: 2.6

   The *tarinfo* argument can be used to replace the default :class:`TarInfo` class
   with a different one.

   .. versionadded:: 2.6

   If *dereference* is ``False``, add symbolic and hard links to the archive. If it
   is ``True``, add the content of the target files to the archive. This has no
   effect on systems that do not support symbolic links.

   If *ignore_zeros* is ``False``, treat an empty block as the end of the archive.
   If it is *True*, skip empty (and invalid) blocks and try to get as many members
   as possible. This is only useful for reading concatenated or damaged archives.

   *debug* can be set from ``0`` (no debug messages) up to ``3`` (all debug
   messages). The messages are written to ``sys.stderr``.

   If *errorlevel* is ``0``, all errors are ignored when using :meth:`extract`.
   Nevertheless, they appear as error messages in the debug output, when debugging
   is enabled.  If ``1``, all *fatal* errors are raised as :exc:`OSError` or
   :exc:`IOError` exceptions. If ``2``, all *non-fatal* errors are raised as
   :exc:`TarError` exceptions as well.

   The *encoding* and *errors* arguments control the way strings are converted to
   unicode objects and vice versa. The default settings will work for most users.
   See section :ref:`tar-unicode` for in-depth information.

   .. versionadded:: 2.6

   The *pax_headers* argument is an optional dictionary of unicode strings which
   will be added as a pax global header if *format* is :const:`PAX_FORMAT`.

   .. versionadded:: 2.6


.. method:: TarFile.open(...)

   Alternative constructor. The :func:`open` function on module level is actually a
   shortcut to this classmethod. See section :ref:`tarfile-mod` for details.


.. method:: TarFile.getmember(name)

   Return a :class:`TarInfo` object for member *name*. If *name* can not be found
   in the archive, :exc:`KeyError` is raised.

   .. note::

      If a member occurs more than once in the archive, its last occurrence is assumed
      to be the most up-to-date version.


.. method:: TarFile.getmembers()

   Return the members of the archive as a list of :class:`TarInfo` objects. The
   list has the same order as the members in the archive.


.. method:: TarFile.getnames()

   Return the members as a list of their names. It has the same order as the list
   returned by :meth:`getmembers`.


.. method:: TarFile.list(verbose=True)

   Print a table of contents to ``sys.stdout``. If *verbose* is :const:`False`,
   only the names of the members are printed. If it is :const:`True`, output
   similar to that of :program:`ls -l` is produced.


.. method:: TarFile.next()

   Return the next member of the archive as a :class:`TarInfo` object, when
   :class:`TarFile` is opened for reading. Return ``None`` if there is no more
   available.


.. method:: TarFile.extractall([path[, members]])

   Extract all members from the archive to the current working directory or
   directory *path*. If optional *members* is given, it must be a subset of the
   list returned by :meth:`getmembers`. Directory information like owner,
   modification time and permissions are set after all members have been extracted.
   This is done to work around two problems: A directory's modification time is
   reset each time a file is created in it. And, if a directory's permissions do
   not allow writing, extracting files to it will fail.

   .. warning::

      Never extract archives from untrusted sources without prior inspection.
      It is possible that files are created outside of *path*, e.g. members
      that have absolute filenames starting with ``"/"`` or filenames with two
      dots ``".."``.

   .. versionadded:: 2.5


.. method:: TarFile.extract(member[, path])

   Extract a member from the archive to the current working directory, using its
   full name. Its file information is extracted as accurately as possible. *member*
   may be a filename or a :class:`TarInfo` object. You can specify a different
   directory using *path*.

   .. note::

      Because the :meth:`extract` method allows random access to a tar archive there
      are some issues you must take care of yourself. See the description for
      :meth:`extractall` above.

   .. warning::

      See the warning for :meth:`extractall`.


.. method:: TarFile.extractfile(member)

   Extract a member from the archive as a file object. *member* may be a filename
   or a :class:`TarInfo` object. If *member* is a regular file, a file-like object
   is returned. If *member* is a link, a file-like object is constructed from the
   link's target. If *member* is none of the above, ``None`` is returned.

   .. note::

      The file-like object is read-only and provides the following methods:
      :meth:`read`, :meth:`readline`, :meth:`readlines`, :meth:`seek`, :meth:`tell`.


.. method:: TarFile.add(name[, arcname[, recursive[, exclude]]])

   Add the file *name* to the archive. *name* may be any type of file (directory,
   fifo, symbolic link, etc.). If given, *arcname* specifies an alternative name
   for the file in the archive. Directories are added recursively by default. This
   can be avoided by setting *recursive* to :const:`False`. If *exclude* is given
   it must be a function that takes one filename argument and returns a boolean
   value. Depending on this value the respective file is either excluded
   (:const:`True`) or added (:const:`False`).

   .. versionchanged:: 2.6
      Added the *exclude* parameter.


.. method:: TarFile.addfile(tarinfo[, fileobj])

   Add the :class:`TarInfo` object *tarinfo* to the archive. If *fileobj* is given,
   ``tarinfo.size`` bytes are read from it and added to the archive.  You can
   create :class:`TarInfo` objects using :meth:`gettarinfo`.

   .. note::

      On Windows platforms, *fileobj* should always be opened with mode ``'rb'`` to
      avoid irritation about the file size.


.. method:: TarFile.gettarinfo([name[, arcname[, fileobj]]])

   Create a :class:`TarInfo` object for either the file *name* or the file object
   *fileobj* (using :func:`os.fstat` on its file descriptor).  You can modify some
   of the :class:`TarInfo`'s attributes before you add it using :meth:`addfile`.
   If given, *arcname* specifies an alternative name for the file in the archive.


.. method:: TarFile.close()

   Close the :class:`TarFile`. In write mode, two finishing zero blocks are
   appended to the archive.


.. attribute:: TarFile.posix

   Setting this to :const:`True` is equivalent to setting the :attr:`format`
   attribute to :const:`USTAR_FORMAT`, :const:`False` is equivalent to
   :const:`GNU_FORMAT`.

   .. versionchanged:: 2.4
      *posix* defaults to :const:`False`.

   .. deprecated:: 2.6
      Use the :attr:`format` attribute instead.


.. attribute:: TarFile.pax_headers

   A dictionary containing key-value pairs of pax global headers.

   .. versionadded:: 2.6

.. % -----------------
.. % TarInfo Objects
.. % -----------------


.. _tarinfo-objects:

TarInfo Objects
---------------

A :class:`TarInfo` object represents one member in a :class:`TarFile`. Aside
from storing all required attributes of a file (like file type, size, time,
permissions, owner etc.), it provides some useful methods to determine its type.
It does *not* contain the file's data itself.

:class:`TarInfo` objects are returned by :class:`TarFile`'s methods
:meth:`getmember`, :meth:`getmembers` and :meth:`gettarinfo`.


.. class:: TarInfo([name])

   Create a :class:`TarInfo` object.


.. method:: TarInfo.frombuf(buf)

   Create and return a :class:`TarInfo` object from string buffer *buf*.

   .. versionadded:: 2.6
      Raises :exc:`HeaderError` if the buffer is invalid..


.. method:: TarInfo.fromtarfile(tarfile)

   Read the next member from the :class:`TarFile` object *tarfile* and return it as
   a :class:`TarInfo` object.

   .. versionadded:: 2.6


.. method:: TarInfo.tobuf([format[, encoding [, errors]]])

   Create a string buffer from a :class:`TarInfo` object. For information on the
   arguments see the constructor of the :class:`TarFile` class.

   .. versionchanged:: 2.6
      The arguments were added.

A ``TarInfo`` object has the following public data attributes:


.. attribute:: TarInfo.name

   Name of the archive member.


.. attribute:: TarInfo.size

   Size in bytes.


.. attribute:: TarInfo.mtime

   Time of last modification.


.. attribute:: TarInfo.mode

   Permission bits.


.. attribute:: TarInfo.type

   File type.  *type* is usually one of these constants: :const:`REGTYPE`,
   :const:`AREGTYPE`, :const:`LNKTYPE`, :const:`SYMTYPE`, :const:`DIRTYPE`,
   :const:`FIFOTYPE`, :const:`CONTTYPE`, :const:`CHRTYPE`, :const:`BLKTYPE`,
   :const:`GNUTYPE_SPARSE`.  To determine the type of a :class:`TarInfo` object
   more conveniently, use the ``is_*()`` methods below.


.. attribute:: TarInfo.linkname

   Name of the target file name, which is only present in :class:`TarInfo` objects
   of type :const:`LNKTYPE` and :const:`SYMTYPE`.


.. attribute:: TarInfo.uid

   User ID of the user who originally stored this member.


.. attribute:: TarInfo.gid

   Group ID of the user who originally stored this member.


.. attribute:: TarInfo.uname

   User name.


.. attribute:: TarInfo.gname

   Group name.


.. attribute:: TarInfo.pax_headers

   A dictionary containing key-value pairs of an associated pax extended header.

   .. versionadded:: 2.6

A :class:`TarInfo` object also provides some convenient query methods:


.. method:: TarInfo.isfile()

   Return :const:`True` if the :class:`Tarinfo` object is a regular file.


.. method:: TarInfo.isreg()

   Same as :meth:`isfile`.


.. method:: TarInfo.isdir()

   Return :const:`True` if it is a directory.


.. method:: TarInfo.issym()

   Return :const:`True` if it is a symbolic link.


.. method:: TarInfo.islnk()

   Return :const:`True` if it is a hard link.


.. method:: TarInfo.ischr()

   Return :const:`True` if it is a character device.


.. method:: TarInfo.isblk()

   Return :const:`True` if it is a block device.


.. method:: TarInfo.isfifo()

   Return :const:`True` if it is a FIFO.


.. method:: TarInfo.isdev()

   Return :const:`True` if it is one of character device, block device or FIFO.

.. % ------------------------
.. % Examples
.. % ------------------------


.. _tar-examples:

Examples
--------

How to extract an entire tar archive to the current working directory::

   import tarfile
   tar = tarfile.open("sample.tar.gz")
   tar.extractall()
   tar.close()

How to create an uncompressed tar archive from a list of filenames::

   import tarfile
   tar = tarfile.open("sample.tar", "w")
   for name in ["foo", "bar", "quux"]:
       tar.add(name)
   tar.close()

How to read a gzip compressed tar archive and display some member information::

   import tarfile
   tar = tarfile.open("sample.tar.gz", "r:gz")
   for tarinfo in tar:
       print tarinfo.name, "is", tarinfo.size, "bytes in size and is",
       if tarinfo.isreg():
           print "a regular file."
       elif tarinfo.isdir():
           print "a directory."
       else:
           print "something else."
   tar.close()

How to create a tar archive with faked information::

   import tarfile
   tar = tarfile.open("sample.tar.gz", "w:gz")
   for name in namelist:
       tarinfo = tar.gettarinfo(name, "fakeproj-1.0/" + name)
       tarinfo.uid = 123
       tarinfo.gid = 456
       tarinfo.uname = "johndoe"
       tarinfo.gname = "fake"
       tar.addfile(tarinfo, file(name))
   tar.close()

The *only* way to extract an uncompressed tar stream from ``sys.stdin``::

   import sys
   import tarfile
   tar = tarfile.open(mode="r|", fileobj=sys.stdin)
   for tarinfo in tar:
       tar.extract(tarinfo)
   tar.close()

.. % ------------
.. % Tar format
.. % ------------


.. _tar-formats:

Supported tar formats
---------------------

There are three tar formats that can be created with the :mod:`tarfile` module:

* The POSIX.1-1988 ustar format (:const:`USTAR_FORMAT`). It supports filenames
  up to a length of at best 256 characters and linknames up to 100 characters. The
  maximum file size is 8 gigabytes. This is an old and limited but widely
  supported format.

* The GNU tar format (:const:`GNU_FORMAT`). It supports long filenames and
  linknames, files bigger than 8 gigabytes and sparse files. It is the de facto
  standard on GNU/Linux systems. :mod:`tarfile` fully supports the GNU tar
  extensions for long names, sparse file support is read-only.

* The POSIX.1-2001 pax format (:const:`PAX_FORMAT`). It is the most flexible
  format with virtually no limits. It supports long filenames and linknames, large
  files and stores pathnames in a portable way. However, not all tar
  implementations today are able to handle pax archives properly.

  The *pax* format is an extension to the existing *ustar* format. It uses extra
  headers for information that cannot be stored otherwise. There are two flavours
  of pax headers: Extended headers only affect the subsequent file header, global
  headers are valid for the complete archive and affect all following files. All
  the data in a pax header is encoded in *UTF-8* for portability reasons.

There are some more variants of the tar format which can be read, but not
created:

* The ancient V7 format. This is the first tar format from Unix Seventh Edition,
  storing only regular files and directories. Names must not be longer than 100
  characters, there is no user/group name information. Some archives have
  miscalculated header checksums in case of fields with non-ASCII characters.

* The SunOS tar extended format. This format is a variant of the POSIX.1-2001
  pax format, but is not compatible.

.. % ----------------
.. % Unicode issues
.. % ----------------


.. _tar-unicode:

Unicode issues
--------------

The tar format was originally conceived to make backups on tape drives with the
main focus on preserving file system information. Nowadays tar archives are
commonly used for file distribution and exchanging archives over networks. One
problem of the original format (that all other formats are merely variants of)
is that there is no concept of supporting different character encodings. For
example, an ordinary tar archive created on a *UTF-8* system cannot be read
correctly on a *Latin-1* system if it contains non-ASCII characters. Names (i.e.
filenames, linknames, user/group names) containing these characters will appear
damaged.  Unfortunately, there is no way to autodetect the encoding of an
archive.

The pax format was designed to solve this problem. It stores non-ASCII names
using the universal character encoding *UTF-8*. When a pax archive is read,
these *UTF-8* names are converted to the encoding of the local file system.

The details of unicode conversion are controlled by the *encoding* and *errors*
keyword arguments of the :class:`TarFile` class.

The default value for *encoding* is the local character encoding. It is deduced
from :func:`sys.getfilesystemencoding` and :func:`sys.getdefaultencoding`. In
read mode, *encoding* is used exclusively to convert unicode names from a pax
archive to strings in the local character encoding. In write mode, the use of
*encoding* depends on the chosen archive format. In case of :const:`PAX_FORMAT`,
input names that contain non-ASCII characters need to be decoded before being
stored as *UTF-8* strings. The other formats do not make use of *encoding*
unless unicode objects are used as input names. These are converted to 8-bit
character strings before they are added to the archive.

The *errors* argument defines how characters are treated that cannot be
converted to or from *encoding*. Possible values are listed in section
:ref:`codec-base-classes`. In read mode, there is an additional scheme
``'utf-8'`` which means that bad characters are replaced by their *UTF-8*
representation. This is the default scheme. In write mode the default value for
*errors* is ``'strict'`` to ensure that name information is not altered
unnoticed.

