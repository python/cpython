:mod:`!io` --- Core tools for working with streams
==================================================

.. module:: io
   :synopsis: Core tools for working with streams.

.. moduleauthor:: Guido van Rossum <guido@python.org>
.. moduleauthor:: Mike Verdone <mike.verdone@gmail.com>
.. moduleauthor:: Mark Russell <mark.russell@zen.co.uk>
.. moduleauthor:: Antoine Pitrou <solipsis@pitrou.net>
.. moduleauthor:: Amaury Forgeot d'Arc <amauryfa@gmail.com>
.. moduleauthor:: Benjamin Peterson <benjamin@python.org>
.. sectionauthor:: Benjamin Peterson <benjamin@python.org>

**Source code:** :source:`Lib/io.py`

--------------

.. _io-overview:

Overview
--------

.. index::
   single: file object; io module

The :mod:`io` module provides Python's main facilities for dealing with various
types of I/O.  There are three main types of I/O: *text I/O*, *binary I/O*
and *raw I/O*.  These are generic categories, and various backing stores can
be used for each of them.  A concrete object belonging to any of these
categories is called a :term:`file object`.  Other common terms are *stream*
and *file-like object*.

Independent of its category, each concrete stream object will also have
various capabilities: it can be read-only, write-only, or read-write. It can
also allow arbitrary random access (seeking forwards or backwards to any
location), or only sequential access (for example in the case of a socket or
pipe).

All streams are careful about the type of data you give to them.  For example
giving a :class:`str` object to the :meth:`!write` method of a binary stream
will raise a :exc:`TypeError`.  So will giving a :class:`bytes` object to the
:meth:`!write` method of a text stream.

.. versionchanged:: 3.3
   Operations that used to raise :exc:`IOError` now raise :exc:`OSError`, since
   :exc:`IOError` is now an alias of :exc:`OSError`.


Text I/O
^^^^^^^^

Text I/O expects and produces :class:`str` objects.  This means that whenever
the backing store is natively made of bytes (such as in the case of a file),
encoding and decoding of data is made transparently as well as optional
translation of platform-specific newline characters.

The easiest way to create a text stream is with :meth:`open`, optionally
specifying an encoding::

   f = open("myfile.txt", "r", encoding="utf-8")

In-memory text streams are also available as :class:`StringIO` objects::

   f = io.StringIO("some initial text data")

.. note::

   When working with a non-blocking stream, be aware that read operations on text I/O objects
   might raise a :exc:`BlockingIOError` if the stream cannot perform the operation
   immediately.

The text stream API is described in detail in the documentation of
:class:`TextIOBase`.


Binary I/O
^^^^^^^^^^

Binary I/O (also called *buffered I/O*) expects
:term:`bytes-like objects <bytes-like object>` and produces :class:`bytes`
objects.  No encoding, decoding, or newline translation is performed.  This
category of streams can be used for all kinds of non-text data, and also when
manual control over the handling of text data is desired.

The easiest way to create a binary stream is with :meth:`open` with ``'b'`` in
the mode string::

   f = open("myfile.jpg", "rb")

In-memory binary streams are also available as :class:`BytesIO` objects::

   f = io.BytesIO(b"some initial binary data: \x00\x01")

The binary stream API is described in detail in the docs of
:class:`BufferedIOBase`.

Other library modules may provide additional ways to create text or binary
streams.  See :meth:`socket.socket.makefile` for example.


Raw I/O
^^^^^^^

Raw I/O (also called *unbuffered I/O*) is generally used as a low-level
building-block for binary and text streams; it is rarely useful to directly
manipulate a raw stream from user code.  Nevertheless, you can create a raw
stream by opening a file in binary mode with buffering disabled::

   f = open("myfile.jpg", "rb", buffering=0)

The raw stream API is described in detail in the docs of :class:`RawIOBase`.


.. _io-text-encoding:

Text Encoding
-------------

The default encoding of :class:`TextIOWrapper` and :func:`open` is
locale-specific (:func:`locale.getencoding`).

However, many developers forget to specify the encoding when opening text files
encoded in UTF-8 (e.g. JSON, TOML, Markdown, etc...) since most Unix
platforms use UTF-8 locale by default. This causes bugs because the locale
encoding is not UTF-8 for most Windows users. For example::

   # May not work on Windows when non-ASCII characters in the file.
   with open("README.md") as f:
       long_description = f.read()

Accordingly, it is highly recommended that you specify the encoding
explicitly when opening text files. If you want to use UTF-8, pass
``encoding="utf-8"``. To use the current locale encoding,
``encoding="locale"`` is supported since Python 3.10.

.. seealso::

   :ref:`utf8-mode`
      Python UTF-8 Mode can be used to change the default encoding to
      UTF-8 from locale-specific encoding.

   :pep:`686`
      Python 3.15 will make :ref:`utf8-mode` default.

.. _io-encoding-warning:

Opt-in EncodingWarning
^^^^^^^^^^^^^^^^^^^^^^

.. versionadded:: 3.10
   See :pep:`597` for more details.

To find where the default locale encoding is used, you can enable
the :option:`-X warn_default_encoding <-X>` command line option or set the
:envvar:`PYTHONWARNDEFAULTENCODING` environment variable, which will
emit an :exc:`EncodingWarning` when the default encoding is used.

If you are providing an API that uses :func:`open` or
:class:`TextIOWrapper` and passes ``encoding=None`` as a parameter, you
can use :func:`text_encoding` so that callers of the API will emit an
:exc:`EncodingWarning` if they don't pass an ``encoding``. However,
please consider using UTF-8 by default (i.e. ``encoding="utf-8"``) for
new APIs.


High-level Module Interface
---------------------------

.. data:: DEFAULT_BUFFER_SIZE

   An int containing the default buffer size used by the module's buffered I/O
   classes.  :func:`open` uses the file's blksize (as obtained by
   :func:`os.stat`) if possible.


.. function:: open(file, mode='r', buffering=-1, encoding=None, errors=None, newline=None, closefd=True, opener=None)

   This is an alias for the builtin :func:`open` function.

   .. audit-event:: open path,mode,flags io.open

      This function raises an :ref:`auditing event <auditing>` ``open`` with
      arguments *path*, *mode* and *flags*. The *mode* and *flags*
      arguments may have been modified or inferred from the original call.


.. function:: open_code(path)

   Opens the provided file with mode ``'rb'``. This function should be used
   when the intent is to treat the contents as executable code.

   *path* should be a :class:`str` and an absolute path.

   The behavior of this function may be overridden by an earlier call to the
   :c:func:`PyFile_SetOpenCodeHook`. However, assuming that *path* is a
   :class:`str` and an absolute path, ``open_code(path)`` should always behave
   the same as ``open(path, 'rb')``. Overriding the behavior is intended for
   additional validation or preprocessing of the file.

   .. versionadded:: 3.8


.. function:: text_encoding(encoding, stacklevel=2, /)

   This is a helper function for callables that use :func:`open` or
   :class:`TextIOWrapper` and have an ``encoding=None`` parameter.

   This function returns *encoding* if it is not ``None``.
   Otherwise, it returns ``"locale"`` or ``"utf-8"`` depending on
   :ref:`UTF-8 Mode <utf8-mode>`.

   This function emits an :class:`EncodingWarning` if
   :data:`sys.flags.warn_default_encoding <sys.flags>` is true and *encoding*
   is ``None``. *stacklevel* specifies where the warning is emitted.
   For example::

      def read_text(path, encoding=None):
          encoding = io.text_encoding(encoding)  # stacklevel=2
          with open(path, encoding) as f:
              return f.read()

   In this example, an :class:`EncodingWarning` is emitted for the caller of
   ``read_text()``.

   See :ref:`io-text-encoding` for more information.

   .. versionadded:: 3.10

   .. versionchanged:: 3.11
      :func:`text_encoding` returns "utf-8" when UTF-8 mode is enabled and
      *encoding* is ``None``.


.. exception:: BlockingIOError

   This is a compatibility alias for the builtin :exc:`BlockingIOError`
   exception.


.. exception:: UnsupportedOperation

   An exception inheriting :exc:`OSError` and :exc:`ValueError` that is raised
   when an unsupported operation is called on a stream.


.. seealso::

   :mod:`sys`
       contains the standard IO streams: :data:`sys.stdin`, :data:`sys.stdout`,
       and :data:`sys.stderr`.


Class hierarchy
---------------

The implementation of I/O streams is organized as a hierarchy of classes.  First
:term:`abstract base classes <abstract base class>` (ABCs), which are used to
specify the various categories of streams, then concrete classes providing the
standard stream implementations.

.. note::

   The abstract base classes also provide default implementations of some
   methods in order to help implementation of concrete stream classes.  For
   example, :class:`BufferedIOBase` provides unoptimized implementations of
   :meth:`!readinto` and :meth:`!readline`.

At the top of the I/O hierarchy is the abstract base class :class:`IOBase`.  It
defines the basic interface to a stream.  Note, however, that there is no
separation between reading and writing to streams; implementations are allowed
to raise :exc:`UnsupportedOperation` if they do not support a given operation.

The :class:`RawIOBase` ABC extends :class:`IOBase`.  It deals with the reading
and writing of bytes to a stream.  :class:`FileIO` subclasses :class:`RawIOBase`
to provide an interface to files in the machine's file system.

The :class:`BufferedIOBase` ABC extends :class:`IOBase`.  It deals with
buffering on a raw binary stream (:class:`RawIOBase`).  Its subclasses,
:class:`BufferedWriter`, :class:`BufferedReader`, and :class:`BufferedRWPair`
buffer raw binary streams that are writable, readable, and both readable and writable,
respectively. :class:`BufferedRandom` provides a buffered interface to seekable streams.
Another :class:`BufferedIOBase` subclass, :class:`BytesIO`, is a stream of
in-memory bytes.

The :class:`TextIOBase` ABC extends :class:`IOBase`.  It deals with
streams whose bytes represent text, and handles encoding and decoding to and
from strings.  :class:`TextIOWrapper`, which extends :class:`TextIOBase`, is a buffered text
interface to a buffered raw stream (:class:`BufferedIOBase`).  Finally,
:class:`StringIO` is an in-memory stream for text.

Argument names are not part of the specification, and only the arguments of
:func:`open` are intended to be used as keyword arguments.

The following table summarizes the ABCs provided by the :mod:`io` module:

.. tabularcolumns:: |l|l|L|L|

=========================  ==================  ========================  ==================================================
ABC                        Inherits            Stub Methods              Mixin Methods and Properties
=========================  ==================  ========================  ==================================================
:class:`IOBase`                                ``fileno``, ``seek``,     ``close``, ``closed``, ``__enter__``,
                                               and ``truncate``          ``__exit__``, ``flush``, ``isatty``, ``__iter__``,
                                                                         ``__next__``, ``readable``, ``readline``,
                                                                         ``readlines``, ``seekable``, ``tell``,
                                                                         ``writable``, and ``writelines``
:class:`RawIOBase`         :class:`IOBase`     ``readinto`` and          Inherited :class:`IOBase` methods, ``read``,
                                               ``write``                 and ``readall``
:class:`BufferedIOBase`    :class:`IOBase`     ``detach``, ``read``,     Inherited :class:`IOBase` methods, ``readinto``,
                                               ``read1``, and ``write``  and ``readinto1``
:class:`TextIOBase`        :class:`IOBase`     ``detach``, ``read``,     Inherited :class:`IOBase` methods, ``encoding``,
                                               ``readline``, and         ``errors``, and ``newlines``
                                               ``write``
=========================  ==================  ========================  ==================================================


I/O Base Classes
^^^^^^^^^^^^^^^^

.. class:: IOBase

   The abstract base class for all I/O classes.

   This class provides empty abstract implementations for many methods
   that derived classes can override selectively; the default
   implementations represent a file that cannot be read, written or
   seeked.

   Even though :class:`IOBase` does not declare :meth:`!read`
   or :meth:`!write` because their signatures will vary, implementations and
   clients should consider those methods part of the interface.  Also,
   implementations may raise a :exc:`ValueError` (or :exc:`UnsupportedOperation`)
   when operations they do not support are called.

   The basic type used for binary data read from or written to a file is
   :class:`bytes`.  Other :term:`bytes-like objects <bytes-like object>` are
   accepted as method arguments too.  Text I/O classes work with :class:`str` data.

   Note that calling any method (even inquiries) on a closed stream is
   undefined.  Implementations may raise :exc:`ValueError` in this case.

   :class:`IOBase` (and its subclasses) supports the iterator protocol, meaning
   that an :class:`IOBase` object can be iterated over yielding the lines in a
   stream.  Lines are defined slightly differently depending on whether the
   stream is a binary stream (yielding bytes), or a text stream (yielding
   character strings).  See :meth:`~IOBase.readline` below.

   :class:`IOBase` is also a context manager and therefore supports the
   :keyword:`with` statement.  In this example, *file* is closed after the
   :keyword:`!with` statement's suite is finished---even if an exception occurs::

      with open('spam.txt', 'w') as file:
          file.write('Spam and eggs!')

   :class:`IOBase` provides these data attributes and methods:

   .. method:: close()

      Flush and close this stream. This method has no effect if the file is
      already closed. Once the file is closed, any operation on the file
      (e.g. reading or writing) will raise a :exc:`ValueError`.

      As a convenience, it is allowed to call this method more than once;
      only the first call, however, will have an effect.

   .. attribute:: closed

      ``True`` if the stream is closed.

   .. method:: fileno()

      Return the underlying file descriptor (an integer) of the stream if it
      exists.  An :exc:`OSError` is raised if the IO object does not use a file
      descriptor.

   .. method:: flush()

      Flush the write buffers of the stream if applicable.  This does nothing
      for read-only and non-blocking streams.

   .. method:: isatty()

      Return ``True`` if the stream is interactive (i.e., connected to
      a terminal/tty device).

   .. method:: readable()

      Return ``True`` if the stream can be read from.
      If ``False``, :meth:`!read` will raise :exc:`OSError`.

   .. method:: readline(size=-1, /)

      Read and return one line from the stream.  If *size* is specified, at
      most *size* bytes will be read.

      The line terminator is always ``b'\n'`` for binary files; for text files,
      the *newline* argument to :func:`open` can be used to select the line
      terminator(s) recognized.

   .. method:: readlines(hint=-1, /)

      Read and return a list of lines from the stream.  *hint* can be specified
      to control the number of lines read: no more lines will be read if the
      total size (in bytes/characters) of all lines so far exceeds *hint*.

      *hint* values of ``0`` or less, as well as ``None``, are treated as no
      hint.

      Note that it's already possible to iterate on file objects using ``for
      line in file: ...`` without calling :meth:`!file.readlines`.

   .. method:: seek(offset, whence=os.SEEK_SET, /)

      Change the stream position to the given byte *offset*,
      interpreted relative to the position indicated by *whence*,
      and return the new absolute position.
      Values for *whence* are:

      * :data:`os.SEEK_SET` or ``0`` -- start of the stream (the default);
        *offset* should be zero or positive
      * :data:`os.SEEK_CUR` or ``1`` -- current stream position;
        *offset* may be negative
      * :data:`os.SEEK_END` or ``2`` -- end of the stream;
        *offset* is usually negative

      .. versionadded:: 3.1
         The :data:`!SEEK_*` constants.

      .. versionadded:: 3.3
         Some operating systems could support additional values, like
         :const:`os.SEEK_HOLE` or :const:`os.SEEK_DATA`. The valid values
         for a file could depend on it being open in text or binary mode.

   .. method:: seekable()

      Return ``True`` if the stream supports random access.  If ``False``,
      :meth:`seek`, :meth:`tell` and :meth:`truncate` will raise :exc:`OSError`.

   .. method:: tell()

      Return the current stream position.

   .. method:: truncate(size=None, /)

      Resize the stream to the given *size* in bytes (or the current position
      if *size* is not specified).  The current stream position isn't changed.
      This resizing can extend or reduce the current file size.  In case of
      extension, the contents of the new file area depend on the platform
      (on most systems, additional bytes are zero-filled).  The new file size
      is returned.

      .. versionchanged:: 3.5
         Windows will now zero-fill files when extending.

   .. method:: writable()

      Return ``True`` if the stream supports writing.  If ``False``,
      :meth:`!write` and :meth:`truncate` will raise :exc:`OSError`.

   .. method:: writelines(lines, /)

      Write a list of lines to the stream.  Line separators are not added, so it
      is usual for each of the lines provided to have a line separator at the
      end.

   .. method:: __del__()

      Prepare for object destruction. :class:`IOBase` provides a default
      implementation of this method that calls the instance's
      :meth:`~IOBase.close` method.


.. class:: RawIOBase

   Base class for raw binary streams.  It inherits from :class:`IOBase`.

   Raw binary streams typically provide low-level access to an underlying OS
   device or API, and do not try to encapsulate it in high-level primitives
   (this functionality is done at a higher-level in buffered binary streams and text streams, described later
   in this page).

   :class:`RawIOBase` provides these methods in addition to those from
   :class:`IOBase`:

   .. method:: read(size=-1, /)

      Read up to *size* bytes from the object and return them.  As a convenience,
      if *size* is unspecified or -1, all bytes until EOF are returned.
      Otherwise, only one system call is ever made.  Fewer than *size* bytes may
      be returned if the operating system call returns fewer than *size* bytes.

      If 0 bytes are returned, and *size* was not 0, this indicates end of file.
      If the object is in non-blocking mode and no bytes are available,
      ``None`` is returned.

      The default implementation defers to :meth:`readall` and
      :meth:`readinto`.

   .. method:: readall()

      Read and return all the bytes from the stream until EOF, using multiple
      calls to the stream if necessary.

   .. method:: readinto(b, /)

      Read bytes into a pre-allocated, writable
      :term:`bytes-like object` *b*, and return the
      number of bytes read.  For example, *b* might be a :class:`bytearray`.
      If the object is in non-blocking mode and no bytes
      are available, ``None`` is returned.

   .. method:: write(b, /)

      Write the given :term:`bytes-like object`, *b*, to the
      underlying raw stream, and return the number of
      bytes written.  This can be less than the length of *b* in
      bytes, depending on specifics of the underlying raw
      stream, and especially if it is in non-blocking mode.  ``None`` is
      returned if the raw stream is set not to block and no single byte could
      be readily written to it.  The caller may release or mutate *b* after
      this method returns, so the implementation should only access *b*
      during the method call.


.. class:: BufferedIOBase

   Base class for binary streams that support some kind of buffering.
   It inherits from :class:`IOBase`.

   The main difference with :class:`RawIOBase` is that methods :meth:`read`,
   :meth:`readinto` and :meth:`write` will try (respectively) to read
   as much input as requested or to emit all provided data.

   In addition, if the underlying raw stream is in non-blocking mode, when the
   system returns would block :meth:`write` will raise :exc:`BlockingIOError`
   with :attr:`BlockingIOError.characters_written` and :meth:`read` will return
   data read so far or ``None`` if no data is available.

   Besides, the :meth:`read` method does not have a default
   implementation that defers to :meth:`readinto`.

   A typical :class:`BufferedIOBase` implementation should not inherit from a
   :class:`RawIOBase` implementation, but wrap one, like
   :class:`BufferedWriter` and :class:`BufferedReader` do.

   :class:`BufferedIOBase` provides or overrides these data attributes and
   methods in addition to those from :class:`IOBase`:

   .. attribute:: raw

      The underlying raw stream (a :class:`RawIOBase` instance) that
      :class:`BufferedIOBase` deals with.  This is not part of the
      :class:`BufferedIOBase` API and may not exist on some implementations.

   .. method:: detach()

      Separate the underlying raw stream from the buffer and return it.

      After the raw stream has been detached, the buffer is in an unusable
      state.

      Some buffers, like :class:`BytesIO`, do not have the concept of a single
      raw stream to return from this method.  They raise
      :exc:`UnsupportedOperation`.

      .. versionadded:: 3.1

   .. method:: read(size=-1, /)

      Read and return up to *size* bytes. If the argument is omitted, ``None``,
      or negative read as much as possible.

      Fewer bytes may be returned than requested. An empty :class:`bytes` object
      is returned if the stream is already at EOF. More than one read may be
      made and calls may be retried if specific errors are encountered, see
      :meth:`os.read` and :pep:`475` for more details. Less than size bytes
      being returned does not imply that EOF is imminent.

      When reading as much as possible the default implementation will use
      ``raw.readall`` if available (which should implement
      :meth:`RawIOBase.readall`), otherwise will read in a loop until read
      returns ``None``, an empty :class:`bytes`, or a non-retryable error. For
      most streams this is to EOF, but for non-blocking streams more data may
      become available.

      .. note::

         When the underlying raw stream is non-blocking, implementations may
         either raise :exc:`BlockingIOError` or return ``None`` if no data is
         available. :mod:`io` implementations return ``None``.

   .. method:: read1(size=-1, /)

      Read and return up to *size* bytes, calling :meth:`~RawIOBase.readinto`
      which may retry if :py:const:`~errno.EINTR` is encountered per
      :pep:`475`. If *size* is ``-1`` or not provided, the implementation will
      choose an arbitrary value for *size*.

      .. note::

         When the underlying raw stream is non-blocking, implementations may
         either raise :exc:`BlockingIOError` or return ``None`` if no data is
         available. :mod:`io` implementations return ``None``.

   .. method:: readinto(b, /)

      Read bytes into a pre-allocated, writable
      :term:`bytes-like object` *b* and return the number of bytes read.
      For example, *b* might be a :class:`bytearray`.

      Like :meth:`read`, multiple reads may be issued to the underlying raw
      stream, unless the latter is interactive.

      A :exc:`BlockingIOError` is raised if the underlying raw stream is in non
      blocking-mode, and has no data available at the moment.

   .. method:: readinto1(b, /)

      Read bytes into a pre-allocated, writable
      :term:`bytes-like object` *b*, using at most one call to
      the underlying raw stream's :meth:`~RawIOBase.read` (or
      :meth:`~RawIOBase.readinto`) method. Return the number of bytes read.

      A :exc:`BlockingIOError` is raised if the underlying raw stream is in non
      blocking-mode, and has no data available at the moment.

      .. versionadded:: 3.5

   .. method:: write(b, /)

      Write the given :term:`bytes-like object`, *b*, and return the number
      of bytes written (always equal to the length of *b* in bytes, since if
      the write fails an :exc:`OSError` will be raised).  Depending on the
      actual implementation, these bytes may be readily written to the
      underlying stream, or held in a buffer for performance and latency
      reasons.

      When in non-blocking mode, a :exc:`BlockingIOError` is raised if the
      data needed to be written to the raw stream but it couldn't accept
      all the data without blocking.

      The caller may release or mutate *b* after this method returns,
      so the implementation should only access *b* during the method call.


Raw File I/O
^^^^^^^^^^^^

.. class:: FileIO(name, mode='r', closefd=True, opener=None)

   A raw binary stream representing an OS-level file containing bytes data.  It
   inherits from :class:`RawIOBase`.

   The *name* can be one of two things:

   * a character string or :class:`bytes` object representing the path to the
     file which will be opened. In this case closefd must be ``True`` (the default)
     otherwise an error will be raised.
   * an integer representing the number of an existing OS-level file descriptor
     to which the resulting :class:`FileIO` object will give access. When the
     FileIO object is closed this fd will be closed as well, unless *closefd*
     is set to ``False``.

   The *mode* can be ``'r'``, ``'w'``, ``'x'`` or ``'a'`` for reading
   (default), writing, exclusive creation or appending. The file will be
   created if it doesn't exist when opened for writing or appending; it will be
   truncated when opened for writing. :exc:`FileExistsError` will be raised if
   it already exists when opened for creating. Opening a file for creating
   implies writing, so this mode behaves in a similar way to ``'w'``. Add a
   ``'+'`` to the mode to allow simultaneous reading and writing.

   The :meth:`~RawIOBase.read` (when called with a positive argument),
   :meth:`~RawIOBase.readinto` and :meth:`~RawIOBase.write` methods on this
   class will only make one system call.

   A custom opener can be used by passing a callable as *opener*. The underlying
   file descriptor for the file object is then obtained by calling *opener* with
   (*name*, *flags*). *opener* must return an open file descriptor (passing
   :mod:`os.open` as *opener* results in functionality similar to passing
   ``None``).

   The newly created file is :ref:`non-inheritable <fd_inheritance>`.

   See the :func:`open` built-in function for examples on using the *opener*
   parameter.

   .. versionchanged:: 3.3
      The *opener* parameter was added.
      The ``'x'`` mode was added.

   .. versionchanged:: 3.4
      The file is now non-inheritable.

   :class:`FileIO` provides these data attributes in addition to those from
   :class:`RawIOBase` and :class:`IOBase`:

   .. attribute:: mode

      The mode as given in the constructor.

   .. attribute:: name

      The file name.  This is the file descriptor of the file when no name is
      given in the constructor.


Buffered Streams
^^^^^^^^^^^^^^^^

Buffered I/O streams provide a higher-level interface to an I/O device
than raw I/O does.

.. class:: BytesIO(initial_bytes=b'')

   A binary stream using an in-memory bytes buffer.  It inherits from
   :class:`BufferedIOBase`.  The buffer is discarded when the
   :meth:`~IOBase.close` method is called.

   The optional argument *initial_bytes* is a :term:`bytes-like object` that
   contains initial data.

   :class:`BytesIO` provides or overrides these methods in addition to those
   from :class:`BufferedIOBase` and :class:`IOBase`:

   .. method:: getbuffer()

      Return a readable and writable view over the contents of the buffer
      without copying them.  Also, mutating the view will transparently
      update the contents of the buffer::

         >>> b = io.BytesIO(b"abcdef")
         >>> view = b.getbuffer()
         >>> view[2:4] = b"56"
         >>> b.getvalue()
         b'ab56ef'

      .. note::
         As long as the view exists, the :class:`BytesIO` object cannot be
         resized or closed.

      .. versionadded:: 3.2

   .. method:: getvalue()

      Return :class:`bytes` containing the entire contents of the buffer.


   .. method:: read1(size=-1, /)

      In :class:`BytesIO`, this is the same as :meth:`~BufferedIOBase.read`.

      .. versionchanged:: 3.7
         The *size* argument is now optional.

   .. method:: readinto1(b, /)

      In :class:`BytesIO`, this is the same as :meth:`~BufferedIOBase.readinto`.

      .. versionadded:: 3.5

.. class:: BufferedReader(raw, buffer_size=DEFAULT_BUFFER_SIZE)

   A buffered binary stream providing higher-level access to a readable, non
   seekable :class:`RawIOBase` raw binary stream.  It inherits from
   :class:`BufferedIOBase`.

   When reading data from this object, a larger amount of data may be
   requested from the underlying raw stream, and kept in an internal buffer.
   The buffered data can then be returned directly on subsequent reads.

   The constructor creates a :class:`BufferedReader` for the given readable
   *raw* stream and *buffer_size*.  If *buffer_size* is omitted,
   :data:`DEFAULT_BUFFER_SIZE` is used.

   :class:`BufferedReader` provides or overrides these methods in addition to
   those from :class:`BufferedIOBase` and :class:`IOBase`:

   .. method:: peek(size=0, /)

      Return bytes from the stream without advancing the position. The number of
      bytes returned may be less or more than requested. If the underlying raw
      stream is non-blocking and the operation would block, returns empty bytes.

   .. method:: read(size=-1, /)

      In :class:`BufferedReader` this is the same as :meth:`io.BufferedIOBase.read`

   .. method:: read1(size=-1, /)

      In :class:`BufferedReader` this is the same as :meth:`io.BufferedIOBase.read1`

      .. versionchanged:: 3.7
         The *size* argument is now optional.

.. class:: BufferedWriter(raw, buffer_size=DEFAULT_BUFFER_SIZE)

   A buffered binary stream providing higher-level access to a writeable, non
   seekable :class:`RawIOBase` raw binary stream.  It inherits from
   :class:`BufferedIOBase`.

   When writing to this object, data is normally placed into an internal
   buffer.  The buffer will be written out to the underlying :class:`RawIOBase`
   object under various conditions, including:

   * when the buffer gets too small for all pending data;
   * when :meth:`flush` is called;
   * when a :meth:`~IOBase.seek` is requested (for :class:`BufferedRandom` objects);
   * when the :class:`BufferedWriter` object is closed or destroyed.

   The constructor creates a :class:`BufferedWriter` for the given writeable
   *raw* stream.  If the *buffer_size* is not given, it defaults to
   :data:`DEFAULT_BUFFER_SIZE`.

   :class:`BufferedWriter` provides or overrides these methods in addition to
   those from :class:`BufferedIOBase` and :class:`IOBase`:

   .. method:: flush()

      Force bytes held in the buffer into the raw stream.  A
      :exc:`BlockingIOError` should be raised if the raw stream blocks.

   .. method:: write(b, /)

      Write the :term:`bytes-like object`, *b*, and return the
      number of bytes written.  When in non-blocking mode, a
      :exc:`BlockingIOError` with :attr:`BlockingIOError.characters_written` set
      is raised if the buffer needs to be written out but the raw stream blocks.


.. class:: BufferedRandom(raw, buffer_size=DEFAULT_BUFFER_SIZE)

   A buffered binary stream providing higher-level access to a seekable
   :class:`RawIOBase` raw binary stream.  It inherits from :class:`BufferedReader`
   and :class:`BufferedWriter`.

   The constructor creates a reader and writer for a seekable raw stream, given
   in the first argument.  If the *buffer_size* is omitted it defaults to
   :data:`DEFAULT_BUFFER_SIZE`.

   :class:`BufferedRandom` is capable of anything :class:`BufferedReader` or
   :class:`BufferedWriter` can do.  In addition, :meth:`~IOBase.seek` and
   :meth:`~IOBase.tell` are guaranteed to be implemented.


.. class:: BufferedRWPair(reader, writer, buffer_size=DEFAULT_BUFFER_SIZE, /)

   A buffered binary stream providing higher-level access to two non seekable
   :class:`RawIOBase` raw binary streams---one readable, the other writeable.
   It inherits from :class:`BufferedIOBase`.

   *reader* and *writer* are :class:`RawIOBase` objects that are readable and
   writeable respectively.  If the *buffer_size* is omitted it defaults to
   :data:`DEFAULT_BUFFER_SIZE`.

   :class:`BufferedRWPair` implements all of :class:`BufferedIOBase`\'s methods
   except for :meth:`~BufferedIOBase.detach`, which raises
   :exc:`UnsupportedOperation`.

   .. warning::

      :class:`BufferedRWPair` does not attempt to synchronize accesses to
      its underlying raw streams.  You should not pass it the same object
      as reader and writer; use :class:`BufferedRandom` instead.


Text I/O
^^^^^^^^

.. class:: TextIOBase

   Base class for text streams.  This class provides a character and line based
   interface to stream I/O.  It inherits from :class:`IOBase`.

   :class:`TextIOBase` provides or overrides these data attributes and
   methods in addition to those from :class:`IOBase`:

   .. attribute:: encoding

      The name of the encoding used to decode the stream's bytes into
      strings, and to encode strings into bytes.

   .. attribute:: errors

      The error setting of the decoder or encoder.

   .. attribute:: newlines

      A string, a tuple of strings, or ``None``, indicating the newlines
      translated so far.  Depending on the implementation and the initial
      constructor flags, this may not be available.

   .. attribute:: buffer

      The underlying binary buffer (a :class:`BufferedIOBase`
      or :class:`RawIOBase` instance) that :class:`TextIOBase` deals with.
      This is not part of the :class:`TextIOBase` API and may not exist
      in some implementations.

   .. method:: detach()

      Separate the underlying binary buffer from the :class:`TextIOBase` and
      return it.

      After the underlying buffer has been detached, the :class:`TextIOBase` is
      in an unusable state.

      Some :class:`TextIOBase` implementations, like :class:`StringIO`, may not
      have the concept of an underlying buffer and calling this method will
      raise :exc:`UnsupportedOperation`.

      .. versionadded:: 3.1

   .. method:: read(size=-1, /)

      Read and return at most *size* characters from the stream as a single
      :class:`str`.  If *size* is negative or ``None``, reads until EOF.

   .. method:: readline(size=-1, /)

      Read until newline or EOF and return a single :class:`str`.  If the stream is
      already at EOF, an empty string is returned.

      If *size* is specified, at most *size* characters will be read.

   .. method:: seek(offset, whence=SEEK_SET, /)

      Change the stream position to the given *offset*.  Behaviour depends on
      the *whence* parameter.  The default value for *whence* is
      :data:`!SEEK_SET`.

      * :data:`!SEEK_SET` or ``0``: seek from the start of the stream
        (the default); *offset* must either be a number returned by
        :meth:`TextIOBase.tell`, or zero.  Any other *offset* value
        produces undefined behaviour.
      * :data:`!SEEK_CUR` or ``1``: "seek" to the current position;
        *offset* must be zero, which is a no-operation (all other values
        are unsupported).
      * :data:`!SEEK_END` or ``2``: seek to the end of the stream;
        *offset* must be zero (all other values are unsupported).

      Return the new absolute position as an opaque number.

      .. versionadded:: 3.1
         The :data:`!SEEK_*` constants.

   .. method:: tell()

      Return the current stream position as an opaque number.  The number
      does not usually represent a number of bytes in the underlying
      binary storage.

   .. method:: write(s, /)

      Write the string *s* to the stream and return the number of characters
      written.


.. class:: TextIOWrapper(buffer, encoding=None, errors=None, newline=None, \
                         line_buffering=False, write_through=False)

   A buffered text stream providing higher-level access to a
   :class:`BufferedIOBase` buffered binary stream.  It inherits from
   :class:`TextIOBase`.

   *encoding* gives the name of the encoding that the stream will be decoded or
   encoded with.  In :ref:`UTF-8 Mode <utf8-mode>`, this defaults to UTF-8.
   Otherwise, it defaults to :func:`locale.getencoding`.
   ``encoding="locale"`` can be used to specify the current locale's encoding
   explicitly. See :ref:`io-text-encoding` for more information.

   *errors* is an optional string that specifies how encoding and decoding
   errors are to be handled.  Pass ``'strict'`` to raise a :exc:`ValueError`
   exception if there is an encoding error (the default of ``None`` has the same
   effect), or pass ``'ignore'`` to ignore errors.  (Note that ignoring encoding
   errors can lead to data loss.)  ``'replace'`` causes a replacement marker
   (such as ``'?'``) to be inserted where there is malformed data.
   ``'backslashreplace'`` causes malformed data to be replaced by a
   backslashed escape sequence.  When writing, ``'xmlcharrefreplace'``
   (replace with the appropriate XML character reference)  or ``'namereplace'``
   (replace with ``\N{...}`` escape sequences) can be used.  Any other error
   handling name that has been registered with
   :func:`codecs.register_error` is also valid.

   .. index::
      single: universal newlines; io.TextIOWrapper class

   *newline* controls how line endings are handled.  It can be ``None``,
   ``''``, ``'\n'``, ``'\r'``, and ``'\r\n'``.  It works as follows:

   * When reading input from the stream, if *newline* is ``None``,
     :term:`universal newlines` mode is enabled.  Lines in the input can end in
     ``'\n'``, ``'\r'``, or ``'\r\n'``, and these are translated into ``'\n'``
     before being returned to the caller.  If *newline* is ``''``, universal
     newlines mode is enabled, but line endings are returned to the caller
     untranslated.  If *newline* has any of the other legal values, input lines
     are only terminated by the given string, and the line ending is returned to
     the caller untranslated.

   * When writing output to the stream, if *newline* is ``None``, any ``'\n'``
     characters written are translated to the system default line separator,
     :data:`os.linesep`.  If *newline* is ``''`` or ``'\n'``, no translation
     takes place.  If *newline* is any of the other legal values, any ``'\n'``
     characters written are translated to the given string.

   If *line_buffering* is ``True``, :meth:`~IOBase.flush` is implied when a call to
   write contains a newline character or a carriage return.

   If *write_through* is ``True``, calls to :meth:`~BufferedIOBase.write` are guaranteed
   not to be buffered: any data written on the :class:`TextIOWrapper`
   object is immediately handled to its underlying binary *buffer*.

   .. versionchanged:: 3.3
      The *write_through* argument has been added.

   .. versionchanged:: 3.3
      The default *encoding* is now ``locale.getpreferredencoding(False)``
      instead of ``locale.getpreferredencoding()``. Don't change temporary the
      locale encoding using :func:`locale.setlocale`, use the current locale
      encoding instead of the user preferred encoding.

   .. versionchanged:: 3.10
      The *encoding* argument now supports the ``"locale"`` dummy encoding name.

   .. note::

      When the underlying raw stream is non-blocking, a :exc:`BlockingIOError`
      may be raised if a read operation cannot be completed immediately.

   :class:`TextIOWrapper` provides these data attributes and methods in
   addition to those from :class:`TextIOBase` and :class:`IOBase`:

   .. attribute:: line_buffering

      Whether line buffering is enabled.

   .. attribute:: write_through

      Whether writes are passed immediately to the underlying binary
      buffer.

      .. versionadded:: 3.7

   .. method:: reconfigure(*, encoding=None, errors=None, newline=None, \
                           line_buffering=None, write_through=None)

      Reconfigure this text stream using new settings for *encoding*,
      *errors*, *newline*, *line_buffering* and *write_through*.

      Parameters not specified keep current settings, except
      ``errors='strict'`` is used when *encoding* is specified but
      *errors* is not specified.

      It is not possible to change the encoding or newline if some data
      has already been read from the stream. On the other hand, changing
      encoding after write is possible.

      This method does an implicit stream flush before setting the
      new parameters.

      .. versionadded:: 3.7

      .. versionchanged:: 3.11
         The method supports ``encoding="locale"`` option.

   .. method:: seek(cookie, whence=os.SEEK_SET, /)

      Set the stream position.
      Return the new stream position as an :class:`int`.

      Four operations are supported,
      given by the following argument combinations:

      * ``seek(0, SEEK_SET)``: Rewind to the start of the stream.
      * ``seek(cookie, SEEK_SET)``: Restore a previous position;
        *cookie* **must be** a number returned by :meth:`tell`.
      * ``seek(0, SEEK_END)``: Fast-forward to the end of the stream.
      * ``seek(0, SEEK_CUR)``: Leave the current stream position unchanged.

      Any other argument combinations are invalid,
      and may raise exceptions.

      .. seealso::

         :data:`os.SEEK_SET`, :data:`os.SEEK_CUR`, and :data:`os.SEEK_END`.

   .. method:: tell()

      Return the stream position as an opaque number.
      The return value of :meth:`!tell` can be given as input to :meth:`seek`,
      to restore a previous stream position.


.. class:: StringIO(initial_value='', newline='\n')

   A text stream using an in-memory text buffer.  It inherits from
   :class:`TextIOBase`.

   The text buffer is discarded when the :meth:`~IOBase.close` method is
   called.

   The initial value of the buffer can be set by providing *initial_value*.
   If newline translation is enabled, newlines will be encoded as if by
   :meth:`~TextIOBase.write`.  The stream is positioned at the start of the
   buffer which emulates opening an existing file in a ``w+`` mode, making it
   ready for an immediate write from the beginning or for a write that
   would overwrite the initial value.  To emulate opening a file in an ``a+``
   mode ready for appending, use ``f.seek(0, io.SEEK_END)`` to reposition the
   stream at the end of the buffer.

   The *newline* argument works like that of :class:`TextIOWrapper`,
   except that when writing output to the stream, if *newline* is ``None``,
   newlines are written as ``\n`` on all platforms.

   :class:`StringIO` provides this method in addition to those from
   :class:`TextIOBase` and :class:`IOBase`:

   .. method:: getvalue()

      Return a :class:`str` containing the entire contents of the buffer.
      Newlines are decoded as if by :meth:`~TextIOBase.read`, although
      the stream position is not changed.

   Example usage::

      import io

      output = io.StringIO()
      output.write('First line.\n')
      print('Second line.', file=output)

      # Retrieve file contents -- this will be
      # 'First line.\nSecond line.\n'
      contents = output.getvalue()

      # Close object and discard memory buffer --
      # .getvalue() will now raise an exception.
      output.close()


.. index::
   single: universal newlines; io.IncrementalNewlineDecoder class

.. class:: IncrementalNewlineDecoder

   A helper codec that decodes newlines for :term:`universal newlines` mode.
   It inherits from :class:`codecs.IncrementalDecoder`.


Static Typing
-------------

The following protocols can be used for annotating function and method
arguments for simple stream reading or writing operations. They are decorated
with :deco:`typing.runtime_checkable`.

.. class:: Reader[T]

   Generic protocol for reading from a file or other input stream. ``T`` will
   usually be :class:`str` or :class:`bytes`, but can be any type that is
   read from the stream.

   .. versionadded:: 3.14

   .. method:: read()
               read(size, /)

      Read data from the input stream and return it. If *size* is
      specified, it should be an integer, and at most *size* items
      (bytes/characters) will be read.

   For example::

     def read_it(reader: Reader[str]):
         data = reader.read(11)
         assert isinstance(data, str)

.. class:: Writer[T]

   Generic protocol for writing to a file or other output stream. ``T`` will
   usually be :class:`str` or :class:`bytes`, but can be any type that can be
   written to the stream.

   .. versionadded:: 3.14

   .. method:: write(data, /)

      Write *data* to the output stream and return the number of items
      (bytes/characters) written.

   For example::

     def write_binary(writer: Writer[bytes]):
         writer.write(b"Hello world!\n")

See :ref:`typing-io` for other I/O related protocols and classes that can be
used for static type checking.

Performance
-----------

This section discusses the performance of the provided concrete I/O
implementations.

Binary I/O
^^^^^^^^^^

By reading and writing only large chunks of data even when the user asks for a
single byte, buffered I/O hides any inefficiency in calling and executing the
operating system's unbuffered I/O routines.  The gain depends on the OS and the
kind of I/O which is performed.  For example, on some modern OSes such as Linux,
unbuffered disk I/O can be as fast as buffered I/O.  The bottom line, however,
is that buffered I/O offers predictable performance regardless of the platform
and the backing device.  Therefore, it is almost always preferable to use
buffered I/O rather than unbuffered I/O for binary data.

Text I/O
^^^^^^^^

Text I/O over a binary storage (such as a file) is significantly slower than
binary I/O over the same storage, because it requires conversions between
unicode and binary data using a character codec.  This can become noticeable
handling huge amounts of text data like large log files.  Also,
:meth:`~TextIOBase.tell` and :meth:`~TextIOBase.seek` are both quite slow
due to the reconstruction algorithm used.

:class:`StringIO`, however, is a native in-memory unicode container and will
exhibit similar speed to :class:`BytesIO`.

Multi-threading
^^^^^^^^^^^^^^^

:class:`FileIO` objects are thread-safe to the extent that the operating system
calls (such as :manpage:`read(2)` under Unix) they wrap are thread-safe too.

Binary buffered objects (instances of :class:`BufferedReader`,
:class:`BufferedWriter`, :class:`BufferedRandom` and :class:`BufferedRWPair`)
protect their internal structures using a lock; it is therefore safe to call
them from multiple threads at once.

:class:`TextIOWrapper` objects are not thread-safe.

Reentrancy
^^^^^^^^^^

Binary buffered objects (instances of :class:`BufferedReader`,
:class:`BufferedWriter`, :class:`BufferedRandom` and :class:`BufferedRWPair`)
are not reentrant.  While reentrant calls will not happen in normal situations,
they can arise from doing I/O in a :mod:`signal` handler.  If a thread tries to
re-enter a buffered object which it is already accessing, a :exc:`RuntimeError`
is raised.  Note this doesn't prohibit a different thread from entering the
buffered object.

The above implicitly extends to text files, since the :func:`open` function
will wrap a buffered object inside a :class:`TextIOWrapper`.  This includes
standard streams and therefore affects the built-in :func:`print` function as
well.
