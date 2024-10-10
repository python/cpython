:mod:`!tarfile` --- Read and write tar archive files
====================================================

.. module:: tarfile
   :synopsis: Read and write tar-format archive files.

.. moduleauthor:: Lars Gustäbel <lars@gustaebel.de>
.. sectionauthor:: Lars Gustäbel <lars@gustaebel.de>

**Source code:** :source:`Lib/tarfile.py`

--------------

The :mod:`tarfile` module makes it possible to read and write tar
archives, including those using gzip, bz2 and lzma compression.
Use the :mod:`zipfile` module to read or write :file:`.zip` files, or the
higher-level functions in :ref:`shutil <archiving-operations>`.

Some facts and figures:

* reads and writes :mod:`gzip`, :mod:`bz2` and :mod:`lzma` compressed archives
  if the respective modules are available.

* read/write support for the POSIX.1-1988 (ustar) format.

* read/write support for the GNU tar format including *longname* and *longlink*
  extensions, read-only support for all variants of the *sparse* extension
  including restoration of sparse files.

* read/write support for the POSIX.1-2001 (pax) format.

* handles directories, regular files, hardlinks, symbolic links, fifos,
  character devices and block devices and is able to acquire and restore file
  information like timestamp, access permissions and owner.

.. versionchanged:: 3.3
   Added support for :mod:`lzma` compression.

.. versionchanged:: 3.12
   Archives are extracted using a :ref:`filter <tarfile-extraction-filter>`,
   which makes it possible to either limit surprising/dangerous features,
   or to acknowledge that they are expected and the archive is fully trusted.
   By default, archives are fully trusted, but this default is deprecated
   and slated to change in Python 3.14.


.. function:: open(name=None, mode='r', fileobj=None, bufsize=10240, **kwargs)

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
   | ``'r:xz'``       | Open for reading with lzma compression.     |
   +------------------+---------------------------------------------+
   | ``'x'`` or       | Create a tarfile exclusively without        |
   | ``'x:'``         | compression.                                |
   |                  | Raise a :exc:`FileExistsError` exception    |
   |                  | if it already exists.                       |
   +------------------+---------------------------------------------+
   | ``'x:gz'``       | Create a tarfile with gzip compression.     |
   |                  | Raise a :exc:`FileExistsError` exception    |
   |                  | if it already exists.                       |
   +------------------+---------------------------------------------+
   | ``'x:bz2'``      | Create a tarfile with bzip2 compression.    |
   |                  | Raise a :exc:`FileExistsError` exception    |
   |                  | if it already exists.                       |
   +------------------+---------------------------------------------+
   | ``'x:xz'``       | Create a tarfile with lzma compression.     |
   |                  | Raise a :exc:`FileExistsError` exception    |
   |                  | if it already exists.                       |
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
   | ``'w:xz'``       | Open for lzma compressed writing.           |
   +------------------+---------------------------------------------+

   Note that ``'a:gz'``, ``'a:bz2'`` or ``'a:xz'`` is not possible. If *mode*
   is not suitable to open a certain (compressed) file for reading,
   :exc:`ReadError` is raised. Use *mode* ``'r'`` to avoid this.  If a
   compression method is not supported, :exc:`CompressionError` is raised.

   If *fileobj* is specified, it is used as an alternative to a :term:`file object`
   opened in binary mode for *name*. It is supposed to be at position 0.

   For modes ``'w:gz'``, ``'x:gz'``, ``'w|gz'``, ``'w:bz2'``, ``'x:bz2'``,
   ``'w|bz2'``, :func:`tarfile.open` accepts the keyword argument
   *compresslevel* (default ``9``) to specify the compression level of the file.

   For modes ``'w:xz'`` and ``'x:xz'``, :func:`tarfile.open` accepts the
   keyword argument *preset* to specify the compression level of the file.

   For special purposes, there is a second format for *mode*:
   ``'filemode|[compression]'``.  :func:`tarfile.open` will return a :class:`TarFile`
   object that processes its data as a stream of blocks.  No random seeking will
   be done on the file. If given, *fileobj* may be any object that has a
   :meth:`~io.RawIOBase.read` or :meth:`~io.RawIOBase.write` method
   (depending on the *mode*) that works with bytes.
   *bufsize* specifies the blocksize and defaults to ``20 * 512`` bytes.
   Use this variant in combination with e.g. ``sys.stdin.buffer``, a socket
   :term:`file object` or a tape device.
   However, such a :class:`TarFile` object is limited in that it does
   not allow random access, see :ref:`tar-examples`.  The currently
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
   | ``'r|xz'``  | Open an lzma compressed *stream* for       |
   |             | reading.                                   |
   +-------------+--------------------------------------------+
   | ``'w|'``    | Open an uncompressed *stream* for writing. |
   +-------------+--------------------------------------------+
   | ``'w|gz'``  | Open a gzip compressed *stream* for        |
   |             | writing.                                   |
   +-------------+--------------------------------------------+
   | ``'w|bz2'`` | Open a bzip2 compressed *stream* for       |
   |             | writing.                                   |
   +-------------+--------------------------------------------+
   | ``'w|xz'``  | Open an lzma compressed *stream* for       |
   |             | writing.                                   |
   +-------------+--------------------------------------------+

   .. versionchanged:: 3.5
      The ``'x'`` (exclusive creation) mode was added.

   .. versionchanged:: 3.6
      The *name* parameter accepts a :term:`path-like object`.

   .. versionchanged:: 3.12
      The *compresslevel* keyword argument also works for streams.


.. class:: TarFile
   :noindex:

   Class for reading and writing tar archives. Do not use this class directly:
   use :func:`tarfile.open` instead. See :ref:`tarfile-objects`.


.. function:: is_tarfile(name)

   Return :const:`True` if *name* is a tar archive file, that the :mod:`tarfile`
   module can read. *name* may be a :class:`str`, file, or file-like object.

   .. versionchanged:: 3.9
      Support for file and file-like objects.


The :mod:`tarfile` module defines the following exceptions:


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

   Is raised for *non-fatal* errors when using :meth:`TarFile.extract`, but only if
   :attr:`TarFile.errorlevel`\ ``== 2``.


.. exception:: HeaderError

   Is raised by :meth:`TarInfo.frombuf` if the buffer it gets is invalid.


.. exception:: FilterError

   Base class for members :ref:`refused <tarfile-extraction-refuse>` by
   filters.

   .. attribute:: tarinfo

      Information about the member that the filter refused to extract,
      as :ref:`TarInfo <tarinfo-objects>`.

.. exception:: AbsolutePathError

   Raised to refuse extracting a member with an absolute path.

.. exception:: OutsideDestinationError

   Raised to refuse extracting a member outside the destination directory.

.. exception:: SpecialFileError

   Raised to refuse extracting a special file (e.g. a device or pipe).

.. exception:: AbsoluteLinkError

   Raised to refuse extracting a symbolic link with an absolute path.

.. exception:: LinkOutsideDestinationError

   Raised to refuse extracting a symbolic link pointing outside the destination
   directory.


The following constants are available at the module level:

.. data:: ENCODING

   The default character encoding: ``'utf-8'`` on Windows, the value returned by
   :func:`sys.getfilesystemencoding` otherwise.

.. data:: REGTYPE
          AREGTYPE

   A regular file :attr:`~TarInfo.type`.

.. data:: LNKTYPE

   A link (inside tarfile) :attr:`~TarInfo.type`.

.. data:: SYMTYPE

   A symbolic link :attr:`~TarInfo.type`.

.. data:: CHRTYPE

   A character special device :attr:`~TarInfo.type`.

.. data:: BLKTYPE

   A block special device :attr:`~TarInfo.type`.

.. data:: DIRTYPE

   A directory :attr:`~TarInfo.type`.

.. data:: FIFOTYPE

   A FIFO special device :attr:`~TarInfo.type`.

.. data:: CONTTYPE

   A contiguous file :attr:`~TarInfo.type`.

.. data:: GNUTYPE_LONGNAME

   A GNU tar longname :attr:`~TarInfo.type`.

.. data:: GNUTYPE_LONGLINK

   A GNU tar longlink :attr:`~TarInfo.type`.

.. data:: GNUTYPE_SPARSE

   A GNU tar sparse file :attr:`~TarInfo.type`.


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

   The default format for creating archives. This is currently :const:`PAX_FORMAT`.

   .. versionchanged:: 3.8
      The default format for new archives was changed to
      :const:`PAX_FORMAT` from :const:`GNU_FORMAT`.


.. seealso::

   Module :mod:`zipfile`
      Documentation of the :mod:`zipfile` standard module.

   :ref:`archiving-operations`
      Documentation of the higher-level archiving facilities provided by the
      standard :mod:`shutil` module.

   `GNU tar manual, Basic Tar Format <https://www.gnu.org/software/tar/manual/html_node/Standard.html>`_
      Documentation for tar archive files, including GNU tar extensions.


.. _tarfile-objects:

TarFile Objects
---------------

The :class:`TarFile` object provides an interface to a tar archive. A tar
archive is a sequence of blocks. An archive member (a stored file) is made up of
a header block followed by data blocks. It is possible to store a file in a tar
archive several times. Each archive member is represented by a :class:`TarInfo`
object, see :ref:`tarinfo-objects` for details.

A :class:`TarFile` object can be used as a context manager in a :keyword:`with`
statement. It will automatically be closed when the block is completed. Please
note that in the event of an exception an archive opened for writing will not
be finalized; only the internally used file object will be closed. See the
:ref:`tar-examples` section for a use case.

.. versionadded:: 3.2
   Added support for the context management protocol.

.. class:: TarFile(name=None, mode='r', fileobj=None, format=DEFAULT_FORMAT, tarinfo=TarInfo, dereference=False, ignore_zeros=False, encoding=ENCODING, errors='surrogateescape', pax_headers=None, debug=0, errorlevel=1, stream=False)

   All following arguments are optional and can be accessed as instance attributes
   as well.

   *name* is the pathname of the archive. *name* may be a :term:`path-like object`.
   It can be omitted if *fileobj* is given.
   In this case, the file object's :attr:`!name` attribute is used if it exists.

   *mode* is either ``'r'`` to read from an existing archive, ``'a'`` to append
   data to an existing file, ``'w'`` to create a new file overwriting an existing
   one, or ``'x'`` to create a new file only if it does not already exist.

   If *fileobj* is given, it is used for reading or writing data. If it can be
   determined, *mode* is overridden by *fileobj*'s mode. *fileobj* will be used
   from position 0.

   .. note::

      *fileobj* is not closed, when :class:`TarFile` is closed.

   *format* controls the archive format for writing. It must be one of the constants
   :const:`USTAR_FORMAT`, :const:`GNU_FORMAT` or :const:`PAX_FORMAT` that are
   defined at module level. When reading, format will be automatically detected, even
   if different formats are present in a single archive.

   The *tarinfo* argument can be used to replace the default :class:`TarInfo` class
   with a different one.

   If *dereference* is :const:`False`, add symbolic and hard links to the archive. If it
   is :const:`True`, add the content of the target files to the archive. This has no
   effect on systems that do not support symbolic links.

   If *ignore_zeros* is :const:`False`, treat an empty block as the end of the archive.
   If it is :const:`True`, skip empty (and invalid) blocks and try to get as many members
   as possible. This is only useful for reading concatenated or damaged archives.

   *debug* can be set from ``0`` (no debug messages) up to ``3`` (all debug
   messages). The messages are written to ``sys.stderr``.

   *errorlevel* controls how extraction errors are handled,
   see :attr:`the corresponding attribute <TarFile.errorlevel>`.

   The *encoding* and *errors* arguments define the character encoding to be
   used for reading or writing the archive and how conversion errors are going
   to be handled. The default settings will work for most users.
   See section :ref:`tar-unicode` for in-depth information.

   The *pax_headers* argument is an optional dictionary of strings which
   will be added as a pax global header if *format* is :const:`PAX_FORMAT`.

   If *stream* is set to :const:`True` then while reading the archive info about files
   in the archive are not cached, saving memory.

   .. versionchanged:: 3.2
      Use ``'surrogateescape'`` as the default for the *errors* argument.

   .. versionchanged:: 3.5
      The ``'x'`` (exclusive creation) mode was added.

   .. versionchanged:: 3.6
      The *name* parameter accepts a :term:`path-like object`.

   .. versionchanged:: 3.13
      Add the *stream* parameter.

.. classmethod:: TarFile.open(...)

   Alternative constructor. The :func:`tarfile.open` function is actually a
   shortcut to this classmethod.


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


.. method:: TarFile.list(verbose=True, *, members=None)

   Print a table of contents to ``sys.stdout``. If *verbose* is :const:`False`,
   only the names of the members are printed. If it is :const:`True`, output
   similar to that of :program:`ls -l` is produced. If optional *members* is
   given, it must be a subset of the list returned by :meth:`getmembers`.

   .. versionchanged:: 3.5
      Added the *members* parameter.


.. method:: TarFile.next()

   Return the next member of the archive as a :class:`TarInfo` object, when
   :class:`TarFile` is opened for reading. Return :const:`None` if there is no more
   available.


.. method:: TarFile.extractall(path=".", members=None, *, numeric_owner=False, filter=None)

   Extract all members from the archive to the current working directory or
   directory *path*. If optional *members* is given, it must be a subset of the
   list returned by :meth:`getmembers`. Directory information like owner,
   modification time and permissions are set after all members have been extracted.
   This is done to work around two problems: A directory's modification time is
   reset each time a file is created in it. And, if a directory's permissions do
   not allow writing, extracting files to it will fail.

   If *numeric_owner* is :const:`True`, the uid and gid numbers from the tarfile
   are used to set the owner/group for the extracted files. Otherwise, the named
   values from the tarfile are used.

   The *filter* argument specifies how ``members`` are modified or rejected
   before extraction.
   See :ref:`tarfile-extraction-filter` for details.
   It is recommended to set this explicitly depending on which *tar* features
   you need to support.

   .. warning::

      Never extract archives from untrusted sources without prior inspection.
      It is possible that files are created outside of *path*, e.g. members
      that have absolute filenames starting with ``"/"`` or filenames with two
      dots ``".."``.

      Set ``filter='data'`` to prevent the most dangerous security issues,
      and read the :ref:`tarfile-extraction-filter` section for details.

   .. versionchanged:: 3.5
      Added the *numeric_owner* parameter.

   .. versionchanged:: 3.6
      The *path* parameter accepts a :term:`path-like object`.

   .. versionchanged:: 3.12
      Added the *filter* parameter.


.. method:: TarFile.extract(member, path="", set_attrs=True, *, numeric_owner=False, filter=None)

   Extract a member from the archive to the current working directory, using its
   full name. Its file information is extracted as accurately as possible. *member*
   may be a filename or a :class:`TarInfo` object. You can specify a different
   directory using *path*. *path* may be a :term:`path-like object`.
   File attributes (owner, mtime, mode) are set unless *set_attrs* is false.

   The *numeric_owner* and *filter* arguments are the same as
   for :meth:`extractall`.

   .. note::

      The :meth:`extract` method does not take care of several extraction issues.
      In most cases you should consider using the :meth:`extractall` method.

   .. warning::

      See the warning for :meth:`extractall`.

      Set ``filter='data'`` to prevent the most dangerous security issues,
      and read the :ref:`tarfile-extraction-filter` section for details.

   .. versionchanged:: 3.2
      Added the *set_attrs* parameter.

   .. versionchanged:: 3.5
      Added the *numeric_owner* parameter.

   .. versionchanged:: 3.6
      The *path* parameter accepts a :term:`path-like object`.

   .. versionchanged:: 3.12
      Added the *filter* parameter.


.. method:: TarFile.extractfile(member)

   Extract a member from the archive as a file object. *member* may be
   a filename or a :class:`TarInfo` object. If *member* is a regular file or
   a link, an :class:`io.BufferedReader` object is returned. For all other
   existing members, :const:`None` is returned. If *member* does not appear
   in the archive, :exc:`KeyError` is raised.

   .. versionchanged:: 3.3
      Return an :class:`io.BufferedReader` object.

   .. versionchanged:: 3.13
      The returned :class:`io.BufferedReader` object has the :attr:`!mode`
      attribute which is always equal to ``'rb'``.

.. attribute:: TarFile.errorlevel
   :type: int

   If *errorlevel* is ``0``, errors are ignored when using :meth:`TarFile.extract`
   and :meth:`TarFile.extractall`.
   Nevertheless, they appear as error messages in the debug output when
   *debug* is greater than 0.
   If ``1`` (the default), all *fatal* errors are raised as :exc:`OSError` or
   :exc:`FilterError` exceptions. If ``2``, all *non-fatal* errors are raised
   as :exc:`TarError` exceptions as well.

   Some exceptions, e.g. ones caused by wrong argument types or data
   corruption, are always raised.

   Custom :ref:`extraction filters <tarfile-extraction-filter>`
   should raise :exc:`FilterError` for *fatal* errors
   and :exc:`ExtractError` for *non-fatal* ones.

   Note that when an exception is raised, the archive may be partially
   extracted. It is the user’s responsibility to clean up.

.. attribute:: TarFile.extraction_filter

   .. versionadded:: 3.12

   The :ref:`extraction filter <tarfile-extraction-filter>` used
   as a default for the *filter* argument of :meth:`~TarFile.extract`
   and :meth:`~TarFile.extractall`.

   The attribute may be ``None`` or a callable.
   String names are not allowed for this attribute, unlike the *filter*
   argument to :meth:`~TarFile.extract`.

   If ``extraction_filter`` is ``None`` (the default),
   calling an extraction method without a *filter* argument will raise a
   ``DeprecationWarning``,
   and fall back to the :func:`fully_trusted <fully_trusted_filter>` filter,
   whose dangerous behavior matches previous versions of Python.

   In Python 3.14+, leaving ``extraction_filter=None`` will cause
   extraction methods to use the :func:`data <data_filter>` filter by default.

   The attribute may be set on instances or overridden in subclasses.
   It also is possible to set it on the ``TarFile`` class itself to set a
   global default, although, since it affects all uses of *tarfile*,
   it is best practice to only do so in top-level applications or
   :mod:`site configuration <site>`.
   To set a global default this way, a filter function needs to be wrapped in
   :func:`staticmethod` to prevent injection of a ``self`` argument.

.. method:: TarFile.add(name, arcname=None, recursive=True, *, filter=None)

   Add the file *name* to the archive. *name* may be any type of file
   (directory, fifo, symbolic link, etc.). If given, *arcname* specifies an
   alternative name for the file in the archive. Directories are added
   recursively by default. This can be avoided by setting *recursive* to
   :const:`False`. Recursion adds entries in sorted order.
   If *filter* is given, it
   should be a function that takes a :class:`TarInfo` object argument and
   returns the changed :class:`TarInfo` object. If it instead returns
   :const:`None` the :class:`TarInfo` object will be excluded from the
   archive. See :ref:`tar-examples` for an example.

   .. versionchanged:: 3.2
      Added the *filter* parameter.

   .. versionchanged:: 3.7
      Recursion adds entries in sorted order.


.. method:: TarFile.addfile(tarinfo, fileobj=None)

   Add the :class:`TarInfo` object *tarinfo* to the archive. If *tarinfo* represents
   a non zero-size regular file, the *fileobj* argument should be a :term:`binary file`,
   and ``tarinfo.size`` bytes are read from it and added to the archive.  You can
   create :class:`TarInfo` objects directly, or by using :meth:`gettarinfo`.

   .. versionchanged:: 3.13

      *fileobj* must be given for non-zero-sized regular files.


.. method:: TarFile.gettarinfo(name=None, arcname=None, fileobj=None)

   Create a :class:`TarInfo` object from the result of :func:`os.stat` or
   equivalent on an existing file.  The file is either named by *name*, or
   specified as a :term:`file object` *fileobj* with a file descriptor.
   *name* may be a :term:`path-like object`.  If
   given, *arcname* specifies an alternative name for the file in the
   archive, otherwise, the name is taken from *fileobj*’s
   :attr:`~io.FileIO.name` attribute, or the *name* argument.  The name
   should be a text string.

   You can modify
   some of the :class:`TarInfo`’s attributes before you add it using :meth:`addfile`.
   If the file object is not an ordinary file object positioned at the
   beginning of the file, attributes such as :attr:`~TarInfo.size` may need
   modifying.  This is the case for objects such as :class:`~gzip.GzipFile`.
   The :attr:`~TarInfo.name` may also be modified, in which case *arcname*
   could be a dummy string.

   .. versionchanged:: 3.6
      The *name* parameter accepts a :term:`path-like object`.


.. method:: TarFile.close()

   Close the :class:`TarFile`. In write mode, two finishing zero blocks are
   appended to the archive.


.. attribute:: TarFile.pax_headers
   :type: dict

   A dictionary containing key-value pairs of pax global headers.



.. _tarinfo-objects:

TarInfo Objects
---------------

A :class:`TarInfo` object represents one member in a :class:`TarFile`. Aside
from storing all required attributes of a file (like file type, size, time,
permissions, owner etc.), it provides some useful methods to determine its type.
It does *not* contain the file's data itself.

:class:`TarInfo` objects are returned by :class:`TarFile`'s methods
:meth:`~TarFile.getmember`, :meth:`~TarFile.getmembers` and
:meth:`~TarFile.gettarinfo`.

Modifying the objects returned by :meth:`~TarFile.getmember` or
:meth:`~TarFile.getmembers` will affect all subsequent
operations on the archive.
For cases where this is unwanted, you can use :mod:`copy.copy() <copy>` or
call the :meth:`~TarInfo.replace` method to create a modified copy in one step.

Several attributes can be set to ``None`` to indicate that a piece of metadata
is unused or unknown.
Different :class:`TarInfo` methods handle ``None`` differently:

- The :meth:`~TarFile.extract` or :meth:`~TarFile.extractall` methods will
  ignore the corresponding metadata, leaving it set to a default.
- :meth:`~TarFile.addfile` will fail.
- :meth:`~TarFile.list` will print a placeholder string.

.. class:: TarInfo(name="")

   Create a :class:`TarInfo` object.


.. classmethod:: TarInfo.frombuf(buf, encoding, errors)

   Create and return a :class:`TarInfo` object from string buffer *buf*.

   Raises :exc:`HeaderError` if the buffer is invalid.


.. classmethod:: TarInfo.fromtarfile(tarfile)

   Read the next member from the :class:`TarFile` object *tarfile* and return it as
   a :class:`TarInfo` object.


.. method:: TarInfo.tobuf(format=DEFAULT_FORMAT, encoding=ENCODING, errors='surrogateescape')

   Create a string buffer from a :class:`TarInfo` object. For information on the
   arguments see the constructor of the :class:`TarFile` class.

   .. versionchanged:: 3.2
      Use ``'surrogateescape'`` as the default for the *errors* argument.


A ``TarInfo`` object has the following public data attributes:


.. attribute:: TarInfo.name
   :type: str

   Name of the archive member.


.. attribute:: TarInfo.size
   :type: int

   Size in bytes.


.. attribute:: TarInfo.mtime
   :type: int | float

   Time of last modification in seconds since the :ref:`epoch <epoch>`,
   as in :attr:`os.stat_result.st_mtime`.

   .. versionchanged:: 3.12

      Can be set to ``None`` for :meth:`~TarFile.extract` and
      :meth:`~TarFile.extractall`, causing extraction to skip applying this
      attribute.

.. attribute:: TarInfo.mode
   :type: int

   Permission bits, as for :func:`os.chmod`.

   .. versionchanged:: 3.12

      Can be set to ``None`` for :meth:`~TarFile.extract` and
      :meth:`~TarFile.extractall`, causing extraction to skip applying this
      attribute.

.. attribute:: TarInfo.type

   File type.  *type* is usually one of these constants: :const:`REGTYPE`,
   :const:`AREGTYPE`, :const:`LNKTYPE`, :const:`SYMTYPE`, :const:`DIRTYPE`,
   :const:`FIFOTYPE`, :const:`CONTTYPE`, :const:`CHRTYPE`, :const:`BLKTYPE`,
   :const:`GNUTYPE_SPARSE`.  To determine the type of a :class:`TarInfo` object
   more conveniently, use the ``is*()`` methods below.


.. attribute:: TarInfo.linkname
   :type: str

   Name of the target file name, which is only present in :class:`TarInfo` objects
   of type :const:`LNKTYPE` and :const:`SYMTYPE`.

   For symbolic links (``SYMTYPE``), the *linkname* is relative to the directory
   that contains the link.
   For hard links (``LNKTYPE``), the *linkname* is relative to the root of
   the archive.


.. attribute:: TarInfo.uid
   :type: int

   User ID of the user who originally stored this member.

   .. versionchanged:: 3.12

      Can be set to ``None`` for :meth:`~TarFile.extract` and
      :meth:`~TarFile.extractall`, causing extraction to skip applying this
      attribute.

.. attribute:: TarInfo.gid
   :type: int

   Group ID of the user who originally stored this member.

   .. versionchanged:: 3.12

      Can be set to ``None`` for :meth:`~TarFile.extract` and
      :meth:`~TarFile.extractall`, causing extraction to skip applying this
      attribute.

.. attribute:: TarInfo.uname
   :type: str

   User name.

   .. versionchanged:: 3.12

      Can be set to ``None`` for :meth:`~TarFile.extract` and
      :meth:`~TarFile.extractall`, causing extraction to skip applying this
      attribute.

.. attribute:: TarInfo.gname
   :type: str

   Group name.

   .. versionchanged:: 3.12

      Can be set to ``None`` for :meth:`~TarFile.extract` and
      :meth:`~TarFile.extractall`, causing extraction to skip applying this
      attribute.

.. attribute:: TarInfo.chksum
   :type: int

   Header checksum.


.. attribute:: TarInfo.devmajor
   :type: int

   Device major number.


.. attribute:: TarInfo.devminor
   :type: int

   Device minor number.


.. attribute:: TarInfo.offset
   :type: int

   The tar header starts here.


.. attribute:: TarInfo.offset_data
   :type: int

   The file's data starts here.


.. attribute:: TarInfo.sparse

   Sparse member information.


.. attribute:: TarInfo.pax_headers
   :type: dict

   A dictionary containing key-value pairs of an associated pax extended header.

.. method:: TarInfo.replace(name=..., mtime=..., mode=..., linkname=..., \
                            uid=..., gid=..., uname=..., gname=..., \
                            deep=True)

   .. versionadded:: 3.12

   Return a *new* copy of the :class:`!TarInfo` object with the given attributes
   changed. For example, to return a ``TarInfo`` with the group name set to
   ``'staff'``, use::

       new_tarinfo = old_tarinfo.replace(gname='staff')

   By default, a deep copy is made.
   If *deep* is false, the copy is shallow, i.e. ``pax_headers``
   and any custom attributes are shared with the original ``TarInfo`` object.

A :class:`TarInfo` object also provides some convenient query methods:


.. method:: TarInfo.isfile()

   Return :const:`True` if the :class:`TarInfo` object is a regular file.


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


.. _tarfile-extraction-filter:

Extraction filters
------------------

.. versionadded:: 3.12

The *tar* format is designed to capture all details of a UNIX-like filesystem,
which makes it very powerful.
Unfortunately, the features make it easy to create tar files that have
unintended -- and possibly malicious -- effects when extracted.
For example, extracting a tar file can overwrite arbitrary files in various
ways (e.g.  by using absolute paths, ``..`` path components, or symlinks that
affect later members).

In most cases, the full functionality is not needed.
Therefore, *tarfile* supports extraction filters: a mechanism to limit
functionality, and thus mitigate some of the security issues.

.. seealso::

   :pep:`706`
      Contains further motivation and rationale behind the design.

The *filter* argument to :meth:`TarFile.extract` or :meth:`~TarFile.extractall`
can be:

* the string ``'fully_trusted'``: Honor all metadata as specified in the
  archive.
  Should be used if the user trusts the archive completely, or implements
  their own complex verification.

* the string ``'tar'``: Honor most *tar*-specific features (i.e. features of
  UNIX-like filesystems), but block features that are very likely to be
  surprising or malicious. See :func:`tar_filter` for details.

* the string ``'data'``: Ignore or block most features specific to UNIX-like
  filesystems. Intended for extracting cross-platform data archives.
  See :func:`data_filter` for details.

* ``None`` (default): Use :attr:`TarFile.extraction_filter`.

  If that is also ``None`` (the default), raise a ``DeprecationWarning``,
  and fall back to the ``'fully_trusted'`` filter, whose dangerous behavior
  matches previous versions of Python.

  In Python 3.14, the ``'data'`` filter will become the default instead.
  It's possible to switch earlier; see :attr:`TarFile.extraction_filter`.

* A callable which will be called for each extracted member with a
  :ref:`TarInfo <tarinfo-objects>` describing the member and the destination
  path to where the archive is extracted (i.e. the same path is used for all
  members)::

      filter(member: TarInfo, path: str, /) -> TarInfo | None

  The callable is called just before each member is extracted, so it can
  take the current state of the disk into account.
  It can:

  - return a :class:`TarInfo` object which will be used instead of the metadata
    in the archive, or
  - return ``None``, in which case the member will be skipped, or
  - raise an exception to abort the operation or skip the member,
    depending on :attr:`~TarFile.errorlevel`.
    Note that when extraction is aborted, :meth:`~TarFile.extractall` may leave
    the archive partially extracted. It does not attempt to clean up.

Default named filters
~~~~~~~~~~~~~~~~~~~~~

The pre-defined, named filters are available as functions, so they can be
reused in custom filters:

.. function:: fully_trusted_filter(member, path)

   Return *member* unchanged.

   This implements the ``'fully_trusted'`` filter.

.. function:: tar_filter(member, path)

  Implements the ``'tar'`` filter.

  - Strip leading slashes (``/`` and :data:`os.sep`) from filenames.
  - :ref:`Refuse <tarfile-extraction-refuse>` to extract files with absolute
    paths (in case the name is absolute
    even after stripping slashes, e.g. ``C:/foo`` on Windows).
    This raises :class:`~tarfile.AbsolutePathError`.
  - :ref:`Refuse <tarfile-extraction-refuse>` to extract files whose absolute
    path (after following symlinks) would end up outside the destination.
    This raises :class:`~tarfile.OutsideDestinationError`.
  - Clear high mode bits (setuid, setgid, sticky) and group/other write bits
    (:const:`~stat.S_IWGRP` | :const:`~stat.S_IWOTH`).

  Return the modified ``TarInfo`` member.

.. function:: data_filter(member, path)

  Implements the ``'data'`` filter.
  In addition to what ``tar_filter`` does:

  - :ref:`Refuse <tarfile-extraction-refuse>` to extract links (hard or soft)
    that link to absolute paths, or ones that link outside the destination.

    This raises :class:`~tarfile.AbsoluteLinkError` or
    :class:`~tarfile.LinkOutsideDestinationError`.

    Note that such files are refused even on platforms that do not support
    symbolic links.

  - :ref:`Refuse <tarfile-extraction-refuse>` to extract device files
    (including pipes).
    This raises :class:`~tarfile.SpecialFileError`.

  - For regular files, including hard links:

    - Set the owner read and write permissions
      (:const:`~stat.S_IRUSR` | :const:`~stat.S_IWUSR`).
    - Remove the group & other executable permission
      (:const:`~stat.S_IXGRP` | :const:`~stat.S_IXOTH`)
      if the owner doesn’t have it (:const:`~stat.S_IXUSR`).

  - For other files (directories), set ``mode`` to ``None``, so
    that extraction methods skip applying permission bits.
  - Set user and group info (``uid``, ``gid``, ``uname``, ``gname``)
    to ``None``, so that extraction methods skip setting it.

  Return the modified ``TarInfo`` member.


.. _tarfile-extraction-refuse:

Filter errors
~~~~~~~~~~~~~

When a filter refuses to extract a file, it will raise an appropriate exception,
a subclass of :class:`~tarfile.FilterError`.
This will abort the extraction if :attr:`TarFile.errorlevel` is 1 or more.
With ``errorlevel=0`` the error will be logged and the member will be skipped,
but extraction will continue.


Hints for further verification
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Even with ``filter='data'``, *tarfile* is not suited for extracting untrusted
files without prior inspection.
Among other issues, the pre-defined filters do not prevent denial-of-service
attacks. Users should do additional checks.

Here is an incomplete list of things to consider:

* Extract to a :func:`new temporary directory <tempfile.mkdtemp>`
  to prevent e.g. exploiting pre-existing links, and to make it easier to
  clean up after a failed extraction.
* When working with untrusted data, use external (e.g. OS-level) limits on
  disk, memory and CPU usage.
* Check filenames against an allow-list of characters
  (to filter out control characters, confusables, foreign path separators,
  etc.).
* Check that filenames have expected extensions (discouraging files that
  execute when you “click on them”, or extension-less files like Windows special device names).
* Limit the number of extracted files, total size of extracted data,
  filename length (including symlink length), and size of individual files.
* Check for files that would be shadowed on case-insensitive filesystems.

Also note that:

* Tar files may contain multiple versions of the same file.
  Later ones are expected to overwrite any earlier ones.
  This feature is crucial to allow updating tape archives, but can be abused
  maliciously.
* *tarfile* does not protect against issues with “live” data,
  e.g. an attacker tinkering with the destination (or source) directory while
  extraction (or archiving) is in progress.


Supporting older Python versions
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Extraction filters were added to Python 3.12, but may be backported to older
versions as security updates.
To check whether the feature is available, use e.g.
``hasattr(tarfile, 'data_filter')`` rather than checking the Python version.

The following examples show how to support Python versions with and without
the feature.
Note that setting ``extraction_filter`` will affect any subsequent operations.

* Fully trusted archive::

    my_tarfile.extraction_filter = (lambda member, path: member)
    my_tarfile.extractall()

* Use the ``'data'`` filter if available, but revert to Python 3.11 behavior
  (``'fully_trusted'``) if this feature is not available::

    my_tarfile.extraction_filter = getattr(tarfile, 'data_filter',
                                           (lambda member, path: member))
    my_tarfile.extractall()

* Use the ``'data'`` filter; *fail* if it is not available::

    my_tarfile.extractall(filter=tarfile.data_filter)

  or::

    my_tarfile.extraction_filter = tarfile.data_filter
    my_tarfile.extractall()

* Use the ``'data'`` filter; *warn* if it is not available::

   if hasattr(tarfile, 'data_filter'):
       my_tarfile.extractall(filter='data')
   else:
       # remove this when no longer needed
       warn_the_user('Extracting may be unsafe; consider updating Python')
       my_tarfile.extractall()


Stateful extraction filter example
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

While *tarfile*'s extraction methods take a simple *filter* callable,
custom filters may be more complex objects with an internal state.
It may be useful to write these as context managers, to be used like this::

    with StatefulFilter() as filter_func:
        tar.extractall(path, filter=filter_func)

Such a filter can be written as, for example::

    class StatefulFilter:
        def __init__(self):
            self.file_count = 0

        def __enter__(self):
            return self

        def __call__(self, member, path):
            self.file_count += 1
            return member

        def __exit__(self, *exc_info):
            print(f'{self.file_count} files extracted')


.. _tarfile-commandline:
.. program:: tarfile


Command-Line Interface
----------------------

.. versionadded:: 3.4

The :mod:`tarfile` module provides a simple command-line interface to interact
with tar archives.

If you want to create a new tar archive, specify its name after the :option:`-c`
option and then list the filename(s) that should be included:

.. code-block:: shell-session

    $ python -m tarfile -c monty.tar  spam.txt eggs.txt

Passing a directory is also acceptable:

.. code-block:: shell-session

    $ python -m tarfile -c monty.tar life-of-brian_1979/

If you want to extract a tar archive into the current directory, use
the :option:`-e` option:

.. code-block:: shell-session

    $ python -m tarfile -e monty.tar

You can also extract a tar archive into a different directory by passing the
directory's name:

.. code-block:: shell-session

    $ python -m tarfile -e monty.tar  other-dir/

For a list of the files in a tar archive, use the :option:`-l` option:

.. code-block:: shell-session

    $ python -m tarfile -l monty.tar


Command-line options
~~~~~~~~~~~~~~~~~~~~

.. option:: -l <tarfile>
            --list <tarfile>

   List files in a tarfile.

.. option:: -c <tarfile> <source1> ... <sourceN>
            --create <tarfile> <source1> ... <sourceN>

   Create tarfile from source files.

.. option:: -e <tarfile> [<output_dir>]
            --extract <tarfile> [<output_dir>]

   Extract tarfile into the current directory if *output_dir* is not specified.

.. option:: -t <tarfile>
            --test <tarfile>

   Test whether the tarfile is valid or not.

.. option:: -v, --verbose

   Verbose output.

.. option:: --filter <filtername>

   Specifies the *filter* for ``--extract``.
   See :ref:`tarfile-extraction-filter` for details.
   Only string names are accepted (that is, ``fully_trusted``, ``tar``,
   and ``data``).

.. _tar-examples:

Examples
--------

How to extract an entire tar archive to the current working directory::

   import tarfile
   tar = tarfile.open("sample.tar.gz")
   tar.extractall(filter='data')
   tar.close()

How to extract a subset of a tar archive with :meth:`TarFile.extractall` using
a generator function instead of a list::

   import os
   import tarfile

   def py_files(members):
       for tarinfo in members:
           if os.path.splitext(tarinfo.name)[1] == ".py":
               yield tarinfo

   tar = tarfile.open("sample.tar.gz")
   tar.extractall(members=py_files(tar))
   tar.close()

How to create an uncompressed tar archive from a list of filenames::

   import tarfile
   tar = tarfile.open("sample.tar", "w")
   for name in ["foo", "bar", "quux"]:
       tar.add(name)
   tar.close()

The same example using the :keyword:`with` statement::

    import tarfile
    with tarfile.open("sample.tar", "w") as tar:
        for name in ["foo", "bar", "quux"]:
            tar.add(name)

How to read a gzip compressed tar archive and display some member information::

   import tarfile
   tar = tarfile.open("sample.tar.gz", "r:gz")
   for tarinfo in tar:
       print(tarinfo.name, "is", tarinfo.size, "bytes in size and is ", end="")
       if tarinfo.isreg():
           print("a regular file.")
       elif tarinfo.isdir():
           print("a directory.")
       else:
           print("something else.")
   tar.close()

How to create an archive and reset the user information using the *filter*
parameter in :meth:`TarFile.add`::

    import tarfile
    def reset(tarinfo):
        tarinfo.uid = tarinfo.gid = 0
        tarinfo.uname = tarinfo.gname = "root"
        return tarinfo
    tar = tarfile.open("sample.tar.gz", "w:gz")
    tar.add("foo", filter=reset)
    tar.close()


.. _tar-formats:

Supported tar formats
---------------------

There are three tar formats that can be created with the :mod:`tarfile` module:

* The POSIX.1-1988 ustar format (:const:`USTAR_FORMAT`). It supports filenames
  up to a length of at best 256 characters and linknames up to 100 characters.
  The maximum file size is 8 GiB. This is an old and limited but widely
  supported format.

* The GNU tar format (:const:`GNU_FORMAT`). It supports long filenames and
  linknames, files bigger than 8 GiB and sparse files. It is the de facto
  standard on GNU/Linux systems. :mod:`tarfile` fully supports the GNU tar
  extensions for long names, sparse file support is read-only.

* The POSIX.1-2001 pax format (:const:`PAX_FORMAT`). It is the most flexible
  format with virtually no limits. It supports long filenames and linknames, large
  files and stores pathnames in a portable way. Modern tar implementations,
  including GNU tar, bsdtar/libarchive and star, fully support extended *pax*
  features; some old or unmaintained libraries may not, but should treat
  *pax* archives as if they were in the universally supported *ustar* format.
  It is the current default format for new archives.

  It extends the existing *ustar* format with extra headers for information
  that cannot be stored otherwise. There are two flavours of pax headers:
  Extended headers only affect the subsequent file header, global
  headers are valid for the complete archive and affect all following files.
  All the data in a pax header is encoded in *UTF-8* for portability reasons.

There are some more variants of the tar format which can be read, but not
created:

* The ancient V7 format. This is the first tar format from Unix Seventh Edition,
  storing only regular files and directories. Names must not be longer than 100
  characters, there is no user/group name information. Some archives have
  miscalculated header checksums in case of fields with non-ASCII characters.

* The SunOS tar extended format. This format is a variant of the POSIX.1-2001
  pax format, but is not compatible.

.. _tar-unicode:

Unicode issues
--------------

The tar format was originally conceived to make backups on tape drives with the
main focus on preserving file system information. Nowadays tar archives are
commonly used for file distribution and exchanging archives over networks. One
problem of the original format (which is the basis of all other formats) is
that there is no concept of supporting different character encodings. For
example, an ordinary tar archive created on a *UTF-8* system cannot be read
correctly on a *Latin-1* system if it contains non-*ASCII* characters. Textual
metadata (like filenames, linknames, user/group names) will appear damaged.
Unfortunately, there is no way to autodetect the encoding of an archive. The
pax format was designed to solve this problem. It stores non-ASCII metadata
using the universal character encoding *UTF-8*.

The details of character conversion in :mod:`tarfile` are controlled by the
*encoding* and *errors* keyword arguments of the :class:`TarFile` class.

*encoding* defines the character encoding to use for the metadata in the
archive. The default value is :func:`sys.getfilesystemencoding` or ``'ascii'``
as a fallback. Depending on whether the archive is read or written, the
metadata must be either decoded or encoded. If *encoding* is not set
appropriately, this conversion may fail.

The *errors* argument defines how characters are treated that cannot be
converted. Possible values are listed in section :ref:`error-handlers`.
The default scheme is ``'surrogateescape'`` which Python also uses for its
file system calls, see :ref:`os-filenames`.

For :const:`PAX_FORMAT` archives (the default), *encoding* is generally not needed
because all the metadata is stored using *UTF-8*. *encoding* is only used in
the rare cases when binary pax headers are decoded or when strings with
surrogate characters are stored.
