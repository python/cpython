:mod:`zipfile` --- Work with ZIP archives
=========================================

.. module:: zipfile
   :synopsis: Read and write ZIP-format archive files.
.. moduleauthor:: James C. Ahlstrom <jim@interet.com>
.. sectionauthor:: James C. Ahlstrom <jim@interet.com>

**Source code:** :source:`Lib/zipfile.py`

--------------

The ZIP file format is a common archive and compression standard. This module
provides tools to create, read, write, append, and list a ZIP file.  Any
advanced use of this module will require an understanding of the format, as
defined in `PKZIP Application Note
<http://www.pkware.com/documents/casestudies/APPNOTE.TXT>`_.

This module does not currently handle multi-disk ZIP files.
It can handle ZIP files that use the ZIP64 extensions
(that is ZIP files that are more than 4 GiB in size).  It supports
decryption of encrypted files in ZIP archives, but it currently cannot
create an encrypted file.  Decryption is extremely slow as it is
implemented in native Python rather than C.

The module defines the following items:

.. exception:: BadZipFile

   The error raised for bad ZIP files.

   .. versionadded:: 3.2


.. exception:: BadZipfile

   Alias of :exc:`BadZipFile`, for compatibility with older Python versions.

   .. deprecated:: 3.2


.. exception:: LargeZipFile

   The error raised when a ZIP file would require ZIP64 functionality but that has
   not been enabled.


.. class:: ZipFile
   :noindex:

   The class for reading and writing ZIP files.  See section
   :ref:`zipfile-objects` for constructor details.


.. class:: PyZipFile
   :noindex:

   Class for creating ZIP archives containing Python libraries.


.. class:: ZipInfo(filename='NoName', date_time=(1980,1,1,0,0,0))

   Class used to represent information about a member of an archive. Instances
   of this class are returned by the :meth:`.getinfo` and :meth:`.infolist`
   methods of :class:`ZipFile` objects.  Most users of the :mod:`zipfile` module
   will not need to create these, but only use those created by this
   module. *filename* should be the full name of the archive member, and
   *date_time* should be a tuple containing six fields which describe the time
   of the last modification to the file; the fields are described in section
   :ref:`zipinfo-objects`.


.. function:: is_zipfile(filename)

   Returns ``True`` if *filename* is a valid ZIP file based on its magic number,
   otherwise returns ``False``.  *filename* may be a file or file-like object too.

   .. versionchanged:: 3.1
      Support for file and file-like objects.


.. data:: ZIP_STORED

   The numeric constant for an uncompressed archive member.


.. data:: ZIP_DEFLATED

   The numeric constant for the usual ZIP compression method.  This requires the
   :mod:`zlib` module.


.. data:: ZIP_BZIP2

   The numeric constant for the BZIP2 compression method.  This requires the
   :mod:`bz2` module.

   .. versionadded:: 3.3

.. data:: ZIP_LZMA

   The numeric constant for the LZMA compression method.  This requires the
   :mod:`lzma` module.

   .. versionadded:: 3.3

   .. note::

      The ZIP file format specification has included support for bzip2 compression
      since 2001, and for LZMA compression since 2006. However, some tools
      (including older Python releases) do not support these compression
      methods, and may either refuse to process the ZIP file altogether,
      or fail to extract individual files.


.. seealso::

   `PKZIP Application Note <http://www.pkware.com/documents/casestudies/APPNOTE.TXT>`_
      Documentation on the ZIP file format by Phil Katz, the creator of the format and
      algorithms used.

   `Info-ZIP Home Page <http://www.info-zip.org/>`_
      Information about the Info-ZIP project's ZIP archive programs and development
      libraries.


.. _zipfile-objects:

ZipFile Objects
---------------


.. class:: ZipFile(file, mode='r', compression=ZIP_STORED, allowZip64=True)

   Open a ZIP file, where *file* can be either a path to a file (a string) or a
   file-like object.  The *mode* parameter should be ``'r'`` to read an existing
   file, ``'w'`` to truncate and write a new file, or ``'a'`` to append to an
   existing file.  If *mode* is ``'a'`` and *file* refers to an existing ZIP
   file, then additional files are added to it.  If *file* does not refer to a
   ZIP file, then a new ZIP archive is appended to the file.  This is meant for
   adding a ZIP archive to another file (such as :file:`python.exe`).  If
   *mode* is ``a`` and the file does not exist at all, it is created.
   *compression* is the ZIP compression method to use when writing the archive,
   and should be :const:`ZIP_STORED`, :const:`ZIP_DEFLATED`,
   :const:`ZIP_BZIP2` or :const:`ZIP_LZMA`; unrecognized
   values will cause :exc:`RuntimeError` to be raised.  If :const:`ZIP_DEFLATED`,
   :const:`ZIP_BZIP2` or :const:`ZIP_LZMA` is specified but the corresponding module
   (:mod:`zlib`, :mod:`bz2` or :mod:`lzma`) is not available, :exc:`RuntimeError`
   is also raised. The default is :const:`ZIP_STORED`.  If *allowZip64* is
   ``True`` (the default) zipfile will create ZIP files that use the ZIP64
   extensions when the zipfile is larger than 2 GiB. If it is  false :mod:`zipfile`
   will raise an exception when the ZIP file would require ZIP64 extensions.

   If the file is created with mode ``'a'`` or ``'w'`` and then
   :meth:`closed <close>` without adding any files to the archive, the appropriate
   ZIP structures for an empty archive will be written to the file.

   ZipFile is also a context manager and therefore supports the
   :keyword:`with` statement.  In the example, *myzip* is closed after the
   :keyword:`with` statement's suite is finished---even if an exception occurs::

      with ZipFile('spam.zip', 'w') as myzip:
          myzip.write('eggs.txt')

   .. versionadded:: 3.2
      Added the ability to use :class:`ZipFile` as a context manager.

   .. versionchanged:: 3.3
      Added support for :mod:`bzip2 <bz2>` and :mod:`lzma` compression.

   .. versionchanged:: 3.4
      ZIP64 extensions are enabled by default.


.. method:: ZipFile.close()

   Close the archive file.  You must call :meth:`close` before exiting your program
   or essential records will not be written.


.. method:: ZipFile.getinfo(name)

   Return a :class:`ZipInfo` object with information about the archive member
   *name*.  Calling :meth:`getinfo` for a name not currently contained in the
   archive will raise a :exc:`KeyError`.


.. method:: ZipFile.infolist()

   Return a list containing a :class:`ZipInfo` object for each member of the
   archive.  The objects are in the same order as their entries in the actual ZIP
   file on disk if an existing archive was opened.


.. method:: ZipFile.namelist()

   Return a list of archive members by name.


.. index::
   single: universal newlines; zipfile.ZipFile.open method

.. method:: ZipFile.open(name, mode='r', pwd=None)

   Extract a member from the archive as a file-like object (ZipExtFile). *name*
   is the name of the file in the archive, or a :class:`ZipInfo` object. The
   *mode* parameter, if included, must be one of the following: ``'r'`` (the
   default), ``'U'``, or ``'rU'``. Choosing ``'U'`` or  ``'rU'`` will enable
   :term:`universal newlines` support in the read-only object.  *pwd* is the
   password used for encrypted files.  Calling  :meth:`.open` on a closed
   ZipFile will raise a  :exc:`RuntimeError`.

   .. note::

      The file-like object is read-only and provides the following methods:
      :meth:`~io.BufferedIOBase.read`, :meth:`~io.IOBase.readline`,
      :meth:`~io.IOBase.readlines`, :meth:`__iter__`,
      :meth:`~iterator.__next__`.

   .. note::

      If the ZipFile was created by passing in a file-like object as the  first
      argument to the constructor, then the object returned by :meth:`.open` shares the
      ZipFile's file pointer.  Under these  circumstances, the object returned by
      :meth:`.open` should not  be used after any additional operations are performed
      on the  ZipFile object.  If the ZipFile was created by passing in a string (the
      filename) as the first argument to the constructor, then  :meth:`.open` will
      create a new file object that will be held by the ZipExtFile, allowing it to
      operate independently of the  ZipFile.

   .. note::

      The :meth:`.open`, :meth:`read` and :meth:`extract` methods can take a filename
      or a :class:`ZipInfo` object.  You will appreciate this when trying to read a
      ZIP file that contains members with duplicate names.

   .. deprecated-removed:: 3.4 3.6
      The ``'U'`` or  ``'rU'`` mode.  Use :class:`io.TextIOWrapper` for reading
      compressed text files in :term:`universal newlines` mode.

.. method:: ZipFile.extract(member, path=None, pwd=None)

   Extract a member from the archive to the current working directory; *member*
   must be its full name or a :class:`ZipInfo` object).  Its file information is
   extracted as accurately as possible.  *path* specifies a different directory
   to extract to.  *member* can be a filename or a :class:`ZipInfo` object.
   *pwd* is the password used for encrypted files.

   .. note::

      If a member filename is an absolute path, a drive/UNC sharepoint and
      leading (back)slashes will be stripped, e.g.: ``///foo/bar`` becomes
      ``foo/bar`` on Unix, and ``C:\foo\bar`` becomes ``foo\bar`` on Windows.
      And all ``".."`` components in a member filename will be removed, e.g.:
      ``../../foo../../ba..r`` becomes ``foo../ba..r``.  On Windows illegal
      characters (``:``, ``<``, ``>``, ``|``, ``"``, ``?``, and ``*``)
      replaced by underscore (``_``).


.. method:: ZipFile.extractall(path=None, members=None, pwd=None)

   Extract all members from the archive to the current working directory.  *path*
   specifies a different directory to extract to.  *members* is optional and must
   be a subset of the list returned by :meth:`namelist`.  *pwd* is the password
   used for encrypted files.

   .. warning::

      Never extract archives from untrusted sources without prior inspection.
      It is possible that files are created outside of *path*, e.g. members
      that have absolute filenames starting with ``"/"`` or filenames with two
      dots ``".."``.  This module attempts to prevent that.
      See :meth:`extract` note.


.. method:: ZipFile.printdir()

   Print a table of contents for the archive to ``sys.stdout``.


.. method:: ZipFile.setpassword(pwd)

   Set *pwd* as default password to extract encrypted files.


.. method:: ZipFile.read(name, pwd=None)

   Return the bytes of the file *name* in the archive.  *name* is the name of the
   file in the archive, or a :class:`ZipInfo` object.  The archive must be open for
   read or append. *pwd* is the password used for encrypted  files and, if specified,
   it will override the default password set with :meth:`setpassword`.  Calling
   :meth:`read` on a closed ZipFile  will raise a :exc:`RuntimeError`.


.. method:: ZipFile.testzip()

   Read all the files in the archive and check their CRC's and file headers.
   Return the name of the first bad file, or else return ``None``. Calling
   :meth:`testzip` on a closed ZipFile will raise a :exc:`RuntimeError`.


.. method:: ZipFile.write(filename, arcname=None, compress_type=None)

   Write the file named *filename* to the archive, giving it the archive name
   *arcname* (by default, this will be the same as *filename*, but without a drive
   letter and with leading path separators removed).  If given, *compress_type*
   overrides the value given for the *compression* parameter to the constructor for
   the new entry.  The archive must be open with mode ``'w'`` or ``'a'`` -- calling
   :meth:`write` on a ZipFile created with mode ``'r'`` will raise a
   :exc:`RuntimeError`.  Calling  :meth:`write` on a closed ZipFile will raise a
   :exc:`RuntimeError`.

   .. note::

      There is no official file name encoding for ZIP files. If you have unicode file
      names, you must convert them to byte strings in your desired encoding before
      passing them to :meth:`write`. WinZip interprets all file names as encoded in
      CP437, also known as DOS Latin.

   .. note::

      Archive names should be relative to the archive root, that is, they should not
      start with a path separator.

   .. note::

      If ``arcname`` (or ``filename``, if ``arcname`` is  not given) contains a null
      byte, the name of the file in the archive will be truncated at the null byte.


.. method:: ZipFile.writestr(zinfo_or_arcname, bytes[, compress_type])

   Write the string *bytes* to the archive; *zinfo_or_arcname* is either the file
   name it will be given in the archive, or a :class:`ZipInfo` instance.  If it's
   an instance, at least the filename, date, and time must be given.  If it's a
   name, the date and time is set to the current date and time. The archive must be
   opened with mode ``'w'`` or ``'a'`` -- calling  :meth:`writestr` on a ZipFile
   created with mode ``'r'``  will raise a :exc:`RuntimeError`.  Calling
   :meth:`writestr` on a closed ZipFile will raise a :exc:`RuntimeError`.

   If given, *compress_type* overrides the value given for the *compression*
   parameter to the constructor for the new entry, or in the *zinfo_or_arcname*
   (if that is a :class:`ZipInfo` instance).

   .. note::

      When passing a :class:`ZipInfo` instance as the *zinfo_or_arcname* parameter,
      the compression method used will be that specified in the *compress_type*
      member of the given :class:`ZipInfo` instance.  By default, the
      :class:`ZipInfo` constructor sets this member to :const:`ZIP_STORED`.

   .. versionchanged:: 3.2
      The *compress_type* argument.

The following data attributes are also available:


.. attribute:: ZipFile.debug

   The level of debug output to use.  This may be set from ``0`` (the default, no
   output) to ``3`` (the most output).  Debugging information is written to
   ``sys.stdout``.

.. attribute:: ZipFile.comment

   The comment text associated with the ZIP file.  If assigning a comment to a
   :class:`ZipFile` instance created with mode 'a' or 'w', this should be a
   string no longer than 65535 bytes.  Comments longer than this will be
   truncated in the written archive when :meth:`close` is called.


.. _pyzipfile-objects:

PyZipFile Objects
-----------------

The :class:`PyZipFile` constructor takes the same parameters as the
:class:`ZipFile` constructor, and one additional parameter, *optimize*.

.. class:: PyZipFile(file, mode='r', compression=ZIP_STORED, allowZip64=True, \
                     optimize=-1)

   .. versionadded:: 3.2
      The *optimize* parameter.

   .. versionchanged:: 3.4
      ZIP64 extensions are enabled by default.

   Instances have one method in addition to those of :class:`ZipFile` objects:

   .. method:: PyZipFile.writepy(pathname, basename='', filterfunc=None)

      Search for files :file:`\*.py` and add the corresponding file to the
      archive.

      If the *optimize* parameter to :class:`PyZipFile` was not given or ``-1``,
      the corresponding file is a :file:`\*.pyo` file if available, else a
      :file:`\*.pyc` file, compiling if necessary.

      If the *optimize* parameter to :class:`PyZipFile` was ``0``, ``1`` or
      ``2``, only files with that optimization level (see :func:`compile`) are
      added to the archive, compiling if necessary.

      If *pathname* is a file, the filename must end with :file:`.py`, and
      just the (corresponding :file:`\*.py[co]`) file is added at the top level
      (no path information).  If *pathname* is a file that does not end with
      :file:`.py`, a :exc:`RuntimeError` will be raised.  If it is a directory,
      and the directory is not a package directory, then all the files
      :file:`\*.py[co]` are added at the top level.  If the directory is a
      package directory, then all :file:`\*.py[co]` are added under the package
      name as a file path, and if any subdirectories are package directories,
      all of these are added recursively.

      *basename* is intended for internal use only.

      *filterfunc*, if given, must be a function taking a single string
      argument.  It will be passed each path (including each individual full
      file path) before it is added to the archive.  If *filterfunc* returns a
      false value, the path will not be added, and if it is a directory its
      contents will be ignored.  For example, if our test files are all either
      in ``test`` directories or start with the string ``test_``, we can use a
      *filterfunc* to exclude them::

          >>> zf = PyZipFile('myprog.zip')
          >>> def notests(s):
          ...     fn = os.path.basename(s)
          ...     return (not (fn == 'test' or fn.startswith('test_')))
          >>> zf.writepy('myprog', filterfunc=notests)

      The :meth:`writepy` method makes archives with file names like
      this::

         string.pyc                   # Top level name
         test/__init__.pyc            # Package directory
         test/testall.pyc             # Module test.testall
         test/bogus/__init__.pyc      # Subpackage directory
         test/bogus/myfile.pyc        # Submodule test.bogus.myfile

      .. versionadded:: 3.4
         The *filterfunc* parameter.


.. _zipinfo-objects:

ZipInfo Objects
---------------

Instances of the :class:`ZipInfo` class are returned by the :meth:`.getinfo` and
:meth:`.infolist` methods of :class:`ZipFile` objects.  Each object stores
information about a single member of the ZIP archive.

Instances have the following attributes:


.. attribute:: ZipInfo.filename

   Name of the file in the archive.


.. attribute:: ZipInfo.date_time

   The time and date of the last modification to the archive member.  This is a
   tuple of six values:

   +-------+--------------------------+
   | Index | Value                    |
   +=======+==========================+
   | ``0`` | Year (>= 1980)           |
   +-------+--------------------------+
   | ``1`` | Month (one-based)        |
   +-------+--------------------------+
   | ``2`` | Day of month (one-based) |
   +-------+--------------------------+
   | ``3`` | Hours (zero-based)       |
   +-------+--------------------------+
   | ``4`` | Minutes (zero-based)     |
   +-------+--------------------------+
   | ``5`` | Seconds (zero-based)     |
   +-------+--------------------------+

   .. note::

      The ZIP file format does not support timestamps before 1980.


.. attribute:: ZipInfo.compress_type

   Type of compression for the archive member.


.. attribute:: ZipInfo.comment

   Comment for the individual archive member.


.. attribute:: ZipInfo.extra

   Expansion field data.  The `PKZIP Application Note
   <http://www.pkware.com/documents/casestudies/APPNOTE.TXT>`_ contains
   some comments on the internal structure of the data contained in this string.


.. attribute:: ZipInfo.create_system

   System which created ZIP archive.


.. attribute:: ZipInfo.create_version

   PKZIP version which created ZIP archive.


.. attribute:: ZipInfo.extract_version

   PKZIP version needed to extract archive.


.. attribute:: ZipInfo.reserved

   Must be zero.


.. attribute:: ZipInfo.flag_bits

   ZIP flag bits.


.. attribute:: ZipInfo.volume

   Volume number of file header.


.. attribute:: ZipInfo.internal_attr

   Internal attributes.


.. attribute:: ZipInfo.external_attr

   External file attributes.


.. attribute:: ZipInfo.header_offset

   Byte offset to the file header.


.. attribute:: ZipInfo.CRC

   CRC-32 of the uncompressed file.


.. attribute:: ZipInfo.compress_size

   Size of the compressed data.


.. attribute:: ZipInfo.file_size

   Size of the uncompressed file.

