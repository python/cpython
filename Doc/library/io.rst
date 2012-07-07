:mod:`io` --- Core tools for working with streams
=================================================

.. module:: io
   :synopsis: Core tools for working with streams.
.. moduleauthor:: Guido van Rossum <guido@python.org>
.. moduleauthor:: Mike Verdone <mike.verdone@gmail.com>
.. moduleauthor:: Mark Russell <mark.russell@zen.co.uk>
.. moduleauthor:: Antoine Pitrou <solipsis@pitrou.net>
.. moduleauthor:: Amaury Forgeot d'Arc <amauryfa@gmail.com>
.. moduleauthor:: Benjamin Peterson <benjamin@python.org>
.. sectionauthor:: Benjamin Peterson <benjamin@python.org>

.. _io-overview:

Overview
--------

The :mod:`io` module provides Python's main facilities for dealing for various
types of I/O.  There are three main types of I/O: *text I/O*, *binary I/O*, *raw
I/O*.  These are generic categories, and various backing stores can be used for
each of them.  Concrete objects belonging to any of these categories will often
be called *streams*; another common term is *file-like objects*.

Independently of its category, each concrete stream object will also have
various capabilities: it can be read-only, write-only, or read-write. It can
also allow arbitrary random access (seeking forwards or backwards to any
location), or only sequential access (for example in the case of a socket or
pipe).

All streams are careful about the type of data you give to them.  For example
giving a :class:`str` object to the ``write()`` method of a binary stream
will raise a ``TypeError``.  So will giving a :class:`bytes` object to the
``write()`` method of a text stream.

.. versionchanged:: 3.3
   Operations defined in this module used to raise :exc:`IOError`, which is
   now an alias of :exc:`OSError`.


Text I/O
^^^^^^^^

Text I/O expects and produces :class:`str` objects.  This means that whenever
the backing store is natively made of bytes (such as in the case of a file),
encoding and decoding of data is made transparently as well as optional
translation of platform-specific newline characters.

The easiest way to create a text stream is with :meth:`open()`, optionally
specifying an encoding::

   f = open("myfile.txt", "r", encoding="utf-8")

In-memory text streams are also available as :class:`StringIO` objects::

   f = io.StringIO("some initial text data")

The text stream API is described in detail in the documentation for the
:class:`TextIOBase`.


Binary I/O
^^^^^^^^^^

Binary I/O (also called *buffered I/O*) expects and produces :class:`bytes`
objects.  No encoding, decoding, or newline translation is performed.  This
category of streams can be used for all kinds of non-text data, and also when
manual control over the handling of text data is desired.

The easiest way to create a binary stream is with :meth:`open()` with ``'b'`` in
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


High-level Module Interface
---------------------------

.. data:: DEFAULT_BUFFER_SIZE

   An int containing the default buffer size used by the module's buffered I/O
   classes.  :func:`open` uses the file's blksize (as obtained by
   :func:`os.stat`) if possible.


.. function:: open(file, mode='r', buffering=-1, encoding=None, errors=None, newline=None, closefd=True)

   This is an alias for the builtin :func:`open` function.


.. exception:: BlockingIOError

   This is a compatibility alias for the builtin :exc:`BlockingIOError`
   exception.


.. exception:: UnsupportedOperation

   An exception inheriting :exc:`OSError` and :exc:`ValueError` that is raised
   when an unsupported operation is called on a stream.


In-memory streams
^^^^^^^^^^^^^^^^^

It is also possible to use a :class:`str` or :class:`bytes`-like object as a
file for both reading and writing.  For strings :class:`StringIO` can be used
like a file opened in text mode.  :class:`BytesIO` can be used like a file
opened in binary mode.  Both provide full read-write capabilities with random
access.


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
      ``readinto()`` and ``readline()``.

At the top of the I/O hierarchy is the abstract base class :class:`IOBase`.  It
defines the basic interface to a stream.  Note, however, that there is no
separation between reading and writing to streams; implementations are allowed
to raise :exc:`UnsupportedOperation` if they do not support a given operation.

The :class:`RawIOBase` ABC extends :class:`IOBase`.  It deals with the reading
and writing of bytes to a stream.  :class:`FileIO` subclasses :class:`RawIOBase`
to provide an interface to files in the machine's file system.

The :class:`BufferedIOBase` ABC deals with buffering on a raw byte stream
(:class:`RawIOBase`).  Its subclasses, :class:`BufferedWriter`,
:class:`BufferedReader`, and :class:`BufferedRWPair` buffer streams that are
readable, writable, and both readable and writable.  :class:`BufferedRandom`
provides a buffered interface to random access streams.  Another
:class:`BufferedIOBase` subclass, :class:`BytesIO`, is a stream of in-memory
bytes.

The :class:`TextIOBase` ABC, another subclass of :class:`IOBase`, deals with
streams whose bytes represent text, and handles encoding and decoding to and
from strings. :class:`TextIOWrapper`, which extends it, is a buffered text
interface to a buffered raw stream (:class:`BufferedIOBase`). Finally,
:class:`StringIO` is an in-memory stream for text.

Argument names are not part of the specification, and only the arguments of
:func:`open` are intended to be used as keyword arguments.


I/O Base Classes
^^^^^^^^^^^^^^^^

.. class:: IOBase

   The abstract base class for all I/O classes, acting on streams of bytes.
   There is no public constructor.

   This class provides empty abstract implementations for many methods
   that derived classes can override selectively; the default
   implementations represent a file that cannot be read, written or
   seeked.

   Even though :class:`IOBase` does not declare :meth:`read`, :meth:`readinto`,
   or :meth:`write` because their signatures will vary, implementations and
   clients should consider those methods part of the interface.  Also,
   implementations may raise a :exc:`ValueError` (or :exc:`UnsupportedOperation`)
   when operations they do not support are called.

   The basic type used for binary data read from or written to a file is
   :class:`bytes`.  :class:`bytearray`\s are accepted too, and in some cases
   (such as :class:`readinto`) required.  Text I/O classes work with
   :class:`str` data.

   Note that calling any method (even inquiries) on a closed stream is
   undefined.  Implementations may raise :exc:`ValueError` in this case.

   IOBase (and its subclasses) support the iterator protocol, meaning that an
   :class:`IOBase` object can be iterated over yielding the lines in a stream.
   Lines are defined slightly differently depending on whether the stream is
   a binary stream (yielding bytes), or a text stream (yielding character
   strings).  See :meth:`~IOBase.readline` below.

   IOBase is also a context manager and therefore supports the
   :keyword:`with` statement.  In this example, *file* is closed after the
   :keyword:`with` statement's suite is finished---even if an exception occurs::

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

      True if the stream is closed.

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

      Return ``True`` if the stream can be read from.  If False, :meth:`read`
      will raise :exc:`OSError`.

   .. method:: readline(limit=-1)

      Read and return one line from the stream.  If *limit* is specified, at
      most *limit* bytes will be read.

      The line terminator is always ``b'\n'`` for binary files; for text files,
      the *newlines* argument to :func:`open` can be used to select the line
      terminator(s) recognized.

   .. method:: readlines(hint=-1)

      Read and return a list of lines from the stream.  *hint* can be specified
      to control the number of lines read: no more lines will be read if the
      total size (in bytes/characters) of all lines so far exceeds *hint*.

   .. method:: seek(offset, whence=SEEK_SET)

      Change the stream position to the given byte *offset*.  *offset* is
      interpreted relative to the position indicated by *whence*.  Values for
      *whence* are:

      * :data:`SEEK_SET` or ``0`` -- start of the stream (the default);
        *offset* should be zero or positive
      * :data:`SEEK_CUR` or ``1`` -- current stream position; *offset* may
        be negative
      * :data:`SEEK_END` or ``2`` -- end of the stream; *offset* is usually
        negative

      Return the new absolute position.

      .. versionadded:: 3.1
         The ``SEEK_*`` constants.

      .. versionadded:: 3.3
         Some operating systems could support additional values, like
         :data:`os.SEEK_HOLE` or :data:`os.SEEK_DATA`. The valid values
         for a file could depend on it being open in text or binary mode.

   .. method:: seekable()

      Return ``True`` if the stream supports random access.  If ``False``,
      :meth:`seek`, :meth:`tell` and :meth:`truncate` will raise :exc:`OSError`.

   .. method:: tell()

      Return the current stream position.

   .. method:: truncate(size=None)

      Resize the stream to the given *size* in bytes (or the current position
      if *size* is not specified).  The current stream position isn't changed.
      This resizing can extend or reduce the current file size.  In case of
      extension, the contents of the new file area depend on the platform
      (on most systems, additional bytes are zero-filled, on Windows they're
      undetermined).  The new file size is returned.

   .. method:: writable()

      Return ``True`` if the stream supports writing.  If ``False``,
      :meth:`write` and :meth:`truncate` will raise :exc:`OSError`.

   .. method:: writelines(lines)

      Write a list of lines to the stream.  Line separators are not added, so it
      is usual for each of the lines provided to have a line separator at the
      end.


.. class:: RawIOBase

   Base class for raw binary I/O.  It inherits :class:`IOBase`.  There is no
   public constructor.

   Raw binary I/O typically provides low-level access to an underlying OS
   device or API, and does not try to encapsulate it in high-level primitives
   (this is left to Buffered I/O and Text I/O, described later in this page).

   In addition to the attributes and methods from :class:`IOBase`,
   RawIOBase provides the following methods:

   .. method:: read(n=-1)

      Read up to *n* bytes from the object and return them.  As a convenience,
      if *n* is unspecified or -1, :meth:`readall` is called.  Otherwise,
      only one system call is ever made.  Fewer than *n* bytes may be
      returned if the operating system call returns fewer than *n* bytes.

      If 0 bytes are returned, and *n* was not 0, this indicates end of file.
      If the object is in non-blocking mode and no bytes are available,
      ``None`` is returned.

   .. method:: readall()

      Read and return all the bytes from the stream until EOF, using multiple
      calls to the stream if necessary.

   .. method:: readinto(b)

      Read up to len(b) bytes into bytearray *b* and return the number
      of bytes read.  If the object is in non-blocking mode and no
      bytes are available, ``None`` is returned.

   .. method:: write(b)

      Write the given bytes or bytearray object, *b*, to the underlying raw
      stream and return the number of bytes written.  This can be less than
      ``len(b)``, depending on specifics of the underlying raw stream, and
      especially if it is in non-blocking mode.  ``None`` is returned if the
      raw stream is set not to block and no single byte could be readily
      written to it.


.. class:: BufferedIOBase

   Base class for binary streams that support some kind of buffering.
   It inherits :class:`IOBase`. There is no public constructor.

   The main difference with :class:`RawIOBase` is that methods :meth:`read`,
   :meth:`readinto` and :meth:`write` will try (respectively) to read as much
   input as requested or to consume all given output, at the expense of
   making perhaps more than one system call.

   In addition, those methods can raise :exc:`BlockingIOError` if the
   underlying raw stream is in non-blocking mode and cannot take or give
   enough data; unlike their :class:`RawIOBase` counterparts, they will
   never return ``None``.

   Besides, the :meth:`read` method does not have a default
   implementation that defers to :meth:`readinto`.

   A typical :class:`BufferedIOBase` implementation should not inherit from a
   :class:`RawIOBase` implementation, but wrap one, like
   :class:`BufferedWriter` and :class:`BufferedReader` do.

   :class:`BufferedIOBase` provides or overrides these methods and attribute in
   addition to those from :class:`IOBase`:

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

   .. method:: read(n=-1)

      Read and return up to *n* bytes.  If the argument is omitted, ``None``, or
      negative, data is read and returned until EOF is reached.  An empty bytes
      object is returned if the stream is already at EOF.

      If the argument is positive, and the underlying raw stream is not
      interactive, multiple raw reads may be issued to satisfy the byte count
      (unless EOF is reached first).  But for interactive raw streams, at most
      one raw read will be issued, and a short result does not imply that EOF is
      imminent.

      A :exc:`BlockingIOError` is raised if the underlying raw stream is in
      non blocking-mode, and has no data available at the moment.

   .. method:: read1(n=-1)

      Read and return up to *n* bytes, with at most one call to the underlying
      raw stream's :meth:`~RawIOBase.read` method.  This can be useful if you
      are implementing your own buffering on top of a :class:`BufferedIOBase`
      object.

   .. method:: readinto(b)

      Read up to len(b) bytes into bytearray *b* and return the number of bytes
      read.

      Like :meth:`read`, multiple reads may be issued to the underlying raw
      stream, unless the latter is 'interactive'.

      A :exc:`BlockingIOError` is raised if the underlying raw stream is in
      non blocking-mode, and has no data available at the moment.

   .. method:: write(b)

      Write the given bytes or bytearray object, *b* and return the number
      of bytes written (never less than ``len(b)``, since if the write fails
      an :exc:`OSError` will be raised).  Depending on the actual
      implementation, these bytes may be readily written to the underlying
      stream, or held in a buffer for performance and latency reasons.

      When in non-blocking mode, a :exc:`BlockingIOError` is raised if the
      data needed to be written to the raw stream but it couldn't accept
      all the data without blocking.


Raw File I/O
^^^^^^^^^^^^

.. class:: FileIO(name, mode='r', closefd=True, opener=None)

   :class:`FileIO` represents an OS-level file containing bytes data.
   It implements the :class:`RawIOBase` interface (and therefore the
   :class:`IOBase` interface, too).

   The *name* can be one of two things:

   * a character string or bytes object representing the path to the file
     which will be opened;
   * an integer representing the number of an existing OS-level file descriptor
     to which the resulting :class:`FileIO` object will give access.

   The *mode* can be ``'r'``, ``'w'``, ``'x'`` or ``'a'`` for reading
   (default), writing, exclusive creation or appending. The file will be
   created if it doesn't exist when opened for writing or appending; it will be
   truncated when opened for writing. :exc:`FileExistsError` will be raised if
   it already exists when opened for creating. Opening a file for creating
   implies writing, so this mode behaves in a similar way to ``'w'``. Add a
   ``'+'`` to the mode to allow simultaneous reading and writing.

   The :meth:`read` (when called with a positive argument), :meth:`readinto`
   and :meth:`write` methods on this class will only make one system call.

   A custom opener can be used by passing a callable as *opener*. The underlying
   file descriptor for the file object is then obtained by calling *opener* with
   (*name*, *flags*). *opener* must return an open file descriptor (passing
   :mod:`os.open` as *opener* results in functionality similar to passing
   ``None``).

   .. versionchanged:: 3.3
      The *opener* parameter was added.
      The ``'x'`` mode was added.

   In addition to the attributes and methods from :class:`IOBase` and
   :class:`RawIOBase`, :class:`FileIO` provides the following data
   attributes and methods:

   .. attribute:: mode

      The mode as given in the constructor.

   .. attribute:: name

      The file name.  This is the file descriptor of the file when no name is
      given in the constructor.


Buffered Streams
^^^^^^^^^^^^^^^^

Buffered I/O streams provide a higher-level interface to an I/O device
than raw I/O does.

.. class:: BytesIO([initial_bytes])

   A stream implementation using an in-memory bytes buffer.  It inherits
   :class:`BufferedIOBase`.

   The argument *initial_bytes* contains optional initial :class:`bytes` data.

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
         resized.

      .. versionadded:: 3.2

   .. method:: getvalue()

      Return ``bytes`` containing the entire contents of the buffer.

   .. method:: read1()

      In :class:`BytesIO`, this is the same as :meth:`read`.


.. class:: BufferedReader(raw, buffer_size=DEFAULT_BUFFER_SIZE)

   A buffer providing higher-level access to a readable, sequential
   :class:`RawIOBase` object.  It inherits :class:`BufferedIOBase`.
   When reading data from this object, a larger amount of data may be
   requested from the underlying raw stream, and kept in an internal buffer.
   The buffered data can then be returned directly on subsequent reads.

   The constructor creates a :class:`BufferedReader` for the given readable
   *raw* stream and *buffer_size*.  If *buffer_size* is omitted,
   :data:`DEFAULT_BUFFER_SIZE` is used.

   :class:`BufferedReader` provides or overrides these methods in addition to
   those from :class:`BufferedIOBase` and :class:`IOBase`:

   .. method:: peek([n])

      Return bytes from the stream without advancing the position.  At most one
      single read on the raw stream is done to satisfy the call. The number of
      bytes returned may be less or more than requested.

   .. method:: read([n])

      Read and return *n* bytes, or if *n* is not given or negative, until EOF
      or if the read call would block in non-blocking mode.

   .. method:: read1(n)

      Read and return up to *n* bytes with only one call on the raw stream.  If
      at least one byte is buffered, only buffered bytes are returned.
      Otherwise, one raw stream read call is made.


.. class:: BufferedWriter(raw, buffer_size=DEFAULT_BUFFER_SIZE)

   A buffer providing higher-level access to a writeable, sequential
   :class:`RawIOBase` object.  It inherits :class:`BufferedIOBase`.
   When writing to this object, data is normally held into an internal
   buffer.  The buffer will be written out to the underlying :class:`RawIOBase`
   object under various conditions, including:

   * when the buffer gets too small for all pending data;
   * when :meth:`flush()` is called;
   * when a :meth:`seek()` is requested (for :class:`BufferedRandom` objects);
   * when the :class:`BufferedWriter` object is closed or destroyed.

   The constructor creates a :class:`BufferedWriter` for the given writeable
   *raw* stream.  If the *buffer_size* is not given, it defaults to
   :data:`DEFAULT_BUFFER_SIZE`.

   :class:`BufferedWriter` provides or overrides these methods in addition to
   those from :class:`BufferedIOBase` and :class:`IOBase`:

   .. method:: flush()

      Force bytes held in the buffer into the raw stream.  A
      :exc:`BlockingIOError` should be raised if the raw stream blocks.

   .. method:: write(b)

      Write the bytes or bytearray object, *b* and return the number of bytes
      written.  When in non-blocking mode, a :exc:`BlockingIOError` is raised
      if the buffer needs to be written out but the raw stream blocks.


.. class:: BufferedRandom(raw, buffer_size=DEFAULT_BUFFER_SIZE)

   A buffered interface to random access streams.  It inherits
   :class:`BufferedReader` and :class:`BufferedWriter`, and further supports
   :meth:`seek` and :meth:`tell` functionality.

   The constructor creates a reader and writer for a seekable raw stream, given
   in the first argument.  If the *buffer_size* is omitted it defaults to
   :data:`DEFAULT_BUFFER_SIZE`.

   :class:`BufferedRandom` is capable of anything :class:`BufferedReader` or
   :class:`BufferedWriter` can do.


.. class:: BufferedRWPair(reader, writer, buffer_size=DEFAULT_BUFFER_SIZE)

   A buffered I/O object combining two unidirectional :class:`RawIOBase`
   objects -- one readable, the other writeable -- into a single bidirectional
   endpoint.  It inherits :class:`BufferedIOBase`.

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
   interface to stream I/O.  There is no :meth:`readinto` method because
   Python's character strings are immutable.  It inherits :class:`IOBase`.
   There is no public constructor.

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

      The underlying binary buffer (a :class:`BufferedIOBase` instance) that
      :class:`TextIOBase` deals with.  This is not part of the
      :class:`TextIOBase` API and may not exist on some implementations.

   .. method:: detach()

      Separate the underlying binary buffer from the :class:`TextIOBase` and
      return it.

      After the underlying buffer has been detached, the :class:`TextIOBase` is
      in an unusable state.

      Some :class:`TextIOBase` implementations, like :class:`StringIO`, may not
      have the concept of an underlying buffer and calling this method will
      raise :exc:`UnsupportedOperation`.

      .. versionadded:: 3.1

   .. method:: read(n)

      Read and return at most *n* characters from the stream as a single
      :class:`str`.  If *n* is negative or ``None``, reads until EOF.

   .. method:: readline()

      Read until newline or EOF and return a single ``str``.  If the stream is
      already at EOF, an empty string is returned.

   .. method:: seek(offset, whence=SEEK_SET)

      Change the stream position to the given *offset*.  Behaviour depends
      on the *whence* parameter:

      * :data:`SEEK_SET` or ``0``: seek from the start of the stream
        (the default); *offset* must either be a number returned by
        :meth:`TextIOBase.tell`, or zero.  Any other *offset* value
        produces undefined behaviour.
      * :data:`SEEK_CUR` or ``1``: "seek" to the current position;
        *offset* must be zero, which is a no-operation (all other values
        are unsupported).
      * :data:`SEEK_END` or ``2``: seek to the end of the stream;
        *offset* must be zero (all other values are unsupported).

      Return the new absolute position as an opaque number.

      .. versionadded:: 3.1
         The ``SEEK_*`` constants.

   .. method:: tell()

      Return the current stream position as an opaque number.  The number
      does not usually represent a number of bytes in the underlying
      binary storage.

   .. method:: write(s)

      Write the string *s* to the stream and return the number of characters
      written.


.. class:: TextIOWrapper(buffer, encoding=None, errors=None, newline=None, \
                         line_buffering=False, write_through=False)

   A buffered text stream over a :class:`BufferedIOBase` binary stream.
   It inherits :class:`TextIOBase`.

   *encoding* gives the name of the encoding that the stream will be decoded or
   encoded with.  It defaults to ``locale.getpreferredencoding(False)``.

   *errors* is an optional string that specifies how encoding and decoding
   errors are to be handled.  Pass ``'strict'`` to raise a :exc:`ValueError`
   exception if there is an encoding error (the default of ``None`` has the same
   effect), or pass ``'ignore'`` to ignore errors.  (Note that ignoring encoding
   errors can lead to data loss.)  ``'replace'`` causes a replacement marker
   (such as ``'?'``) to be inserted where there is malformed data.  When
   writing, ``'xmlcharrefreplace'`` (replace with the appropriate XML character
   reference) or ``'backslashreplace'`` (replace with backslashed escape
   sequences) can be used.  Any other error handling name that has been
   registered with :func:`codecs.register_error` is also valid.

   *newline* can be ``None``, ``''``, ``'\n'``, ``'\r'``, or ``'\r\n'``.  It
   controls the handling of line endings.  If it is ``None``, universal newlines
   is enabled.  With this enabled, on input, the lines endings ``'\n'``,
   ``'\r'``, or ``'\r\n'`` are translated to ``'\n'`` before being returned to
   the caller.  Conversely, on output, ``'\n'`` is translated to the system
   default line separator, :data:`os.linesep`.  If *newline* is any other of its
   legal values, that newline becomes the newline when the file is read and it
   is returned untranslated.  On output, ``'\n'`` is converted to the *newline*.

   If *line_buffering* is ``True``, :meth:`flush` is implied when a call to
   write contains a newline character.

   If *write_through* is ``True``, calls to :meth:`write` are guaranteed
   not to be buffered: any data written on the :class:`TextIOWrapper`
   object is immediately handled to its underlying binary *buffer*.

   .. versionchanged:: 3.3
      The *write_through* argument has been added.

   .. versionchanged:: 3.3
      The default *encoding* is now ``locale.getpreferredencoding(False)``
      instead of ``locale.getpreferredencoding()``. Don't change temporary the
      locale encoding using :func:`locale.setlocale`, use the current locale
      encoding instead of the user preferred encoding.

   :class:`TextIOWrapper` provides one attribute in addition to those of
   :class:`TextIOBase` and its parents:

   .. attribute:: line_buffering

      Whether line buffering is enabled.


.. class:: StringIO(initial_value='', newline=None)

   An in-memory stream for text I/O.

   The initial value of the buffer (an empty string by default) can be set by
   providing *initial_value*.  The *newline* argument works like that of
   :class:`TextIOWrapper`.  The default is to do no newline translation.

   :class:`StringIO` provides this method in addition to those from
   :class:`TextIOBase` and its parents:

   .. method:: getvalue()

      Return a ``str`` containing the entire contents of the buffer at any
      time before the :class:`StringIO` object's :meth:`close` method is
      called.

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


.. class:: IncrementalNewlineDecoder

   A helper codec that decodes newlines for universal newlines mode.  It
   inherits :class:`codecs.IncrementalDecoder`.


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
and the backing device.  Therefore, it is most always preferable to use buffered
I/O rather than unbuffered I/O for binary datal

Text I/O
^^^^^^^^

Text I/O over a binary storage (such as a file) is significantly slower than
binary I/O over the same storage, because it requires conversions between
unicode and binary data using a character codec.  This can become noticeable
handling huge amounts of text data like large log files.  Also,
:meth:`TextIOWrapper.tell` and :meth:`TextIOWrapper.seek` are both quite slow
due to the reconstruction algorithm used.

:class:`StringIO`, however, is a native in-memory unicode container and will
exhibit similar speed to :class:`BytesIO`.

Multi-threading
^^^^^^^^^^^^^^^

:class:`FileIO` objects are thread-safe to the extent that the operating system
calls (such as ``read(2)`` under Unix) they wrap are thread-safe too.

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
renter a buffered object which it is already accessing, a :exc:`RuntimeError` is
raised.  Note this doesn't prohibit a different thread from entering the
buffered object.

The above implicitly extends to text files, since the :func:`open()` function
will wrap a buffered object inside a :class:`TextIOWrapper`.  This includes
standard streams and therefore affects the built-in function :func:`print()` as
well.

