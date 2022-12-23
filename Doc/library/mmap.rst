:mod:`mmap` --- Memory-mapped file support
==========================================

.. module:: mmap
   :synopsis: Interface to memory-mapped files for Unix and Windows.

--------------

.. include:: ../includes/wasm-notavail.rst

Memory-mapped file objects behave like both :class:`bytearray` and like
:term:`file objects <file object>`.  You can use mmap objects in most places
where :class:`bytearray` are expected; for example, you can use the :mod:`re`
module to search through a memory-mapped file.  You can also change a single
byte by doing ``obj[index] = 97``, or change a subsequence by assigning to a
slice: ``obj[i1:i2] = b'...'``.  You can also read and write data starting at
the current file position, and :meth:`seek` through the file to different positions.

A memory-mapped file is created by the :class:`~mmap.mmap` constructor, which is
different on Unix and on Windows.  In either case you must provide a file
descriptor for a file opened for update. If you wish to map an existing Python
file object, use its :meth:`fileno` method to obtain the correct value for the
*fileno* parameter.  Otherwise, you can open the file using the
:func:`os.open` function, which returns a file descriptor directly (the file
still needs to be closed when done).

.. note::
   If you want to create a memory-mapping for a writable, buffered file, you
   should :func:`~io.IOBase.flush` the file first.  This is necessary to ensure
   that local modifications to the buffers are actually available to the
   mapping.

For both the Unix and Windows versions of the constructor, *access* may be
specified as an optional keyword parameter. *access* accepts one of four
values: :const:`ACCESS_READ`, :const:`ACCESS_WRITE`, or :const:`ACCESS_COPY` to
specify read-only, write-through or copy-on-write memory respectively, or
:const:`ACCESS_DEFAULT` to defer to *prot*.  *access* can be used on both Unix
and Windows.  If *access* is not specified, Windows mmap returns a
write-through mapping.  The initial memory values for all three access types
are taken from the specified file.  Assignment to an :const:`ACCESS_READ`
memory map raises a :exc:`TypeError` exception.  Assignment to an
:const:`ACCESS_WRITE` memory map affects both memory and the underlying file.
Assignment to an :const:`ACCESS_COPY` memory map affects memory but does not
update the underlying file.

.. versionchanged:: 3.7
   Added :const:`ACCESS_DEFAULT` constant.

To map anonymous memory, -1 should be passed as the fileno along with the length.

.. class:: mmap(fileno, length, tagname=None, access=ACCESS_DEFAULT[, offset])

   **(Windows version)** Maps *length* bytes from the file specified by the
   file handle *fileno*, and creates a mmap object.  If *length* is larger
   than the current size of the file, the file is extended to contain *length*
   bytes.  If *length* is ``0``, the maximum length of the map is the current
   size of the file, except that if the file is empty Windows raises an
   exception (you cannot create an empty mapping on Windows).

   *tagname*, if specified and not ``None``, is a string giving a tag name for
   the mapping.  Windows allows you to have many different mappings against
   the same file.  If you specify the name of an existing tag, that tag is
   opened, otherwise a new tag of this name is created.  If this parameter is
   omitted or ``None``, the mapping is created without a name.  Avoiding the
   use of the tag parameter will assist in keeping your code portable between
   Unix and Windows.

   *offset* may be specified as a non-negative integer offset. mmap references
   will be relative to the offset from the beginning of the file. *offset*
   defaults to 0.  *offset* must be a multiple of the :const:`ALLOCATIONGRANULARITY`.

   .. audit-event:: mmap.__new__ fileno,length,access,offset mmap.mmap

.. class:: mmap(fileno, length, flags=MAP_SHARED, prot=PROT_WRITE|PROT_READ, access=ACCESS_DEFAULT[, offset])
   :noindex:

   **(Unix version)** Maps *length* bytes from the file specified by the file
   descriptor *fileno*, and returns a mmap object.  If *length* is ``0``, the
   maximum length of the map will be the current size of the file when
   :class:`~mmap.mmap` is called.

   *flags* specifies the nature of the mapping. :const:`MAP_PRIVATE` creates a
   private copy-on-write mapping, so changes to the contents of the mmap
   object will be private to this process, and :const:`MAP_SHARED` creates a
   mapping that's shared with all other processes mapping the same areas of
   the file.  The default value is :const:`MAP_SHARED`. Some systems have
   additional possible flags with the full list specified in
   :ref:`MAP_* constants <map-constants>`.

   *prot*, if specified, gives the desired memory protection; the two most
   useful values are :const:`PROT_READ` and :const:`PROT_WRITE`, to specify
   that the pages may be read or written.  *prot* defaults to
   :const:`PROT_READ \| PROT_WRITE`.

   *access* may be specified in lieu of *flags* and *prot* as an optional
   keyword parameter.  It is an error to specify both *flags*, *prot* and
   *access*.  See the description of *access* above for information on how to
   use this parameter.

   *offset* may be specified as a non-negative integer offset. mmap references
   will be relative to the offset from the beginning of the file. *offset*
   defaults to 0. *offset* must be a multiple of :const:`ALLOCATIONGRANULARITY`
   which is equal to :const:`PAGESIZE` on Unix systems.

   To ensure validity of the created memory mapping the file specified
   by the descriptor *fileno* is internally automatically synchronized
   with the physical backing store on macOS.

   This example shows a simple way of using :class:`~mmap.mmap`::

      import mmap

      # write a simple example file
      with open("hello.txt", "wb") as f:
          f.write(b"Hello Python!\n")

      with open("hello.txt", "r+b") as f:
          # memory-map the file, size 0 means whole file
          mm = mmap.mmap(f.fileno(), 0)
          # read content via standard file methods
          print(mm.readline())  # prints b"Hello Python!\n"
          # read content via slice notation
          print(mm[:5])  # prints b"Hello"
          # update content using slice notation;
          # note that new content must have same size
          mm[6:] = b" world!\n"
          # ... and read again using standard file methods
          mm.seek(0)
          print(mm.readline())  # prints b"Hello  world!\n"
          # close the map
          mm.close()


   :class:`~mmap.mmap` can also be used as a context manager in a :keyword:`with`
   statement::

      import mmap

      with mmap.mmap(-1, 13) as mm:
          mm.write(b"Hello world!")

   .. versionadded:: 3.2
      Context manager support.


   The next example demonstrates how to create an anonymous map and exchange
   data between the parent and child processes::

      import mmap
      import os

      mm = mmap.mmap(-1, 13)
      mm.write(b"Hello world!")

      pid = os.fork()

      if pid == 0:  # In a child process
          mm.seek(0)
          print(mm.readline())

          mm.close()

   .. audit-event:: mmap.__new__ fileno,length,access,offset mmap.mmap

   Memory-mapped file objects support the following methods:

   .. method:: close()

      Closes the mmap. Subsequent calls to other methods of the object will
      result in a ValueError exception being raised. This will not close
      the open file.


   .. attribute:: closed

      ``True`` if the file is closed.

      .. versionadded:: 3.2


   .. method:: find(sub[, start[, end]])

      Returns the lowest index in the object where the subsequence *sub* is
      found, such that *sub* is contained in the range [*start*, *end*].
      Optional arguments *start* and *end* are interpreted as in slice notation.
      Returns ``-1`` on failure.

      .. versionchanged:: 3.5
         Writable :term:`bytes-like object` is now accepted.


   .. method:: flush([offset[, size]])

      Flushes changes made to the in-memory copy of a file back to disk. Without
      use of this call there is no guarantee that changes are written back before
      the object is destroyed.  If *offset* and *size* are specified, only
      changes to the given range of bytes will be flushed to disk; otherwise, the
      whole extent of the mapping is flushed.  *offset* must be a multiple of the
      :const:`PAGESIZE` or :const:`ALLOCATIONGRANULARITY`.

      ``None`` is returned to indicate success.  An exception is raised when the
      call failed.

      .. versionchanged:: 3.8
         Previously, a nonzero value was returned on success; zero was returned
         on error under Windows.  A zero value was returned on success; an
         exception was raised on error under Unix.


   .. method:: madvise(option[, start[, length]])

      Send advice *option* to the kernel about the memory region beginning at
      *start* and extending *length* bytes.  *option* must be one of the
      :ref:`MADV_* constants <madvise-constants>` available on the system.  If
      *start* and *length* are omitted, the entire mapping is spanned.  On
      some systems (including Linux), *start* must be a multiple of the
      :const:`PAGESIZE`.

      Availability: Systems with the ``madvise()`` system call.

      .. versionadded:: 3.8


   .. method:: move(dest, src, count)

      Copy the *count* bytes starting at offset *src* to the destination index
      *dest*.  If the mmap was created with :const:`ACCESS_READ`, then calls to
      move will raise a :exc:`TypeError` exception.


   .. method:: read([n])

      Return a :class:`bytes` containing up to *n* bytes starting from the
      current file position. If the argument is omitted, ``None`` or negative,
      return all bytes from the current file position to the end of the
      mapping. The file position is updated to point after the bytes that were
      returned.

      .. versionchanged:: 3.3
         Argument can be omitted or ``None``.

   .. method:: read_byte()

      Returns a byte at the current file position as an integer, and advances
      the file position by 1.


   .. method:: readline()

      Returns a single line, starting at the current file position and up to the
      next newline. The file position is updated to point after the bytes that were
      returned.


   .. method:: resize(newsize)

      Resizes the map and the underlying file, if any. If the mmap was created
      with :const:`ACCESS_READ` or :const:`ACCESS_COPY`, resizing the map will
      raise a :exc:`TypeError` exception.

      **On Windows**: Resizing the map will raise an :exc:`OSError` if there are other
      maps against the same named file. Resizing an anonymous map (ie against the
      pagefile) will silently create a new map with the original data copied over
      up to the length of the new size.

      .. versionchanged:: 3.11
         Correctly fails if attempting to resize when another map is held
         Allows resize against an anonymous map on Windows

   .. method:: rfind(sub[, start[, end]])

      Returns the highest index in the object where the subsequence *sub* is
      found, such that *sub* is contained in the range [*start*, *end*].
      Optional arguments *start* and *end* are interpreted as in slice notation.
      Returns ``-1`` on failure.

      .. versionchanged:: 3.5
         Writable :term:`bytes-like object` is now accepted.


   .. method:: seek(pos[, whence])

      Set the file's current position.  *whence* argument is optional and
      defaults to ``os.SEEK_SET`` or ``0`` (absolute file positioning); other
      values are ``os.SEEK_CUR`` or ``1`` (seek relative to the current
      position) and ``os.SEEK_END`` or ``2`` (seek relative to the file's end).


   .. method:: size()

      Return the length of the file, which can be larger than the size of the
      memory-mapped area.


   .. method:: tell()

      Returns the current position of the file pointer.


   .. method:: write(bytes)

      Write the bytes in *bytes* into memory at the current position of the
      file pointer and return the number of bytes written (never less than
      ``len(bytes)``, since if the write fails, a :exc:`ValueError` will be
      raised).  The file position is updated to point after the bytes that
      were written.  If the mmap was created with :const:`ACCESS_READ`, then
      writing to it will raise a :exc:`TypeError` exception.

      .. versionchanged:: 3.5
         Writable :term:`bytes-like object` is now accepted.

      .. versionchanged:: 3.6
         The number of bytes written is now returned.


   .. method:: write_byte(byte)

      Write the integer *byte* into memory at the current
      position of the file pointer; the file position is advanced by ``1``. If
      the mmap was created with :const:`ACCESS_READ`, then writing to it will
      raise a :exc:`TypeError` exception.

.. _madvise-constants:

MADV_* Constants
++++++++++++++++

.. data:: MADV_NORMAL
          MADV_RANDOM
          MADV_SEQUENTIAL
          MADV_WILLNEED
          MADV_DONTNEED
          MADV_REMOVE
          MADV_DONTFORK
          MADV_DOFORK
          MADV_HWPOISON
          MADV_MERGEABLE
          MADV_UNMERGEABLE
          MADV_SOFT_OFFLINE
          MADV_HUGEPAGE
          MADV_NOHUGEPAGE
          MADV_DONTDUMP
          MADV_DODUMP
          MADV_FREE
          MADV_NOSYNC
          MADV_AUTOSYNC
          MADV_NOCORE
          MADV_CORE
          MADV_PROTECT
          MADV_FREE_REUSABLE
          MADV_FREE_REUSE

   These options can be passed to :meth:`mmap.madvise`.  Not every option will
   be present on every system.

   Availability: Systems with the madvise() system call.

   .. versionadded:: 3.8

.. _map-constants:

MAP_* Constants
+++++++++++++++

.. data:: MAP_SHARED
          MAP_PRIVATE
          MAP_DENYWRITE
          MAP_EXECUTABLE
          MAP_ANON
          MAP_ANONYMOUS
          MAP_POPULATE
          MAP_STACK

    These are the various flags that can be passed to :meth:`mmap.mmap`. Note that some options might not be present on some systems.

    .. versionchanged:: 3.10
       Added MAP_POPULATE constant.

    .. versionadded:: 3.11
       Added MAP_STACK constant.
