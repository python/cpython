:mod:`logging.handlers` --- Logging handlers
============================================

.. module:: logging.handlers
   :synopsis: Handlers for the logging module.

.. moduleauthor:: Vinay Sajip <vinay_sajip@red-dove.com>
.. sectionauthor:: Vinay Sajip <vinay_sajip@red-dove.com>

**Source code:** :source:`Lib/logging/handlers.py`

.. sidebar:: Important

   This page contains only reference information. For tutorials,
   please see

   * :ref:`Basic Tutorial <logging-basic-tutorial>`
   * :ref:`Advanced Tutorial <logging-advanced-tutorial>`
   * :ref:`Logging Cookbook <logging-cookbook>`

--------------

.. currentmodule:: logging

The following useful handlers are provided in the package. Note that three of
the handlers (:class:`StreamHandler`, :class:`FileHandler` and
:class:`NullHandler`) are actually defined in the :mod:`logging` module itself,
but have been documented here along with the other handlers.

.. _stream-handler:

StreamHandler
^^^^^^^^^^^^^

The :class:`StreamHandler` class, located in the core :mod:`logging` package,
sends logging output to streams such as *sys.stdout*, *sys.stderr* or any
file-like object (or, more precisely, any object which supports :meth:`write`
and :meth:`flush` methods).


.. class:: StreamHandler(stream=None)

   Returns a new instance of the :class:`StreamHandler` class. If *stream* is
   specified, the instance will use it for logging output; otherwise, *sys.stderr*
   will be used.


   .. method:: emit(record)

      If a formatter is specified, it is used to format the record. The record
      is then written to the stream followed by :attr:`terminator`. If exception information
      is present, it is formatted using :func:`traceback.print_exception` and
      appended to the stream.


   .. method:: flush()

      Flushes the stream by calling its :meth:`flush` method. Note that the
      :meth:`close` method is inherited from :class:`~logging.Handler` and so
      does no output, so an explicit :meth:`flush` call may be needed at times.

   .. method:: setStream(stream)

      Sets the instance's stream to the specified value, if it is different.
      The old stream is flushed before the new stream is set.

      :param stream: The stream that the handler should use.

      :return: the old stream, if the stream was changed, or *None* if it wasn't.

      .. versionadded:: 3.7

   .. attribute:: terminator

      String used as the terminator when writing a formatted record to a stream.
      Default value is ``'\n'``.

      If you don't want a newline termination, you can set the handler instance's
      ``terminator`` attribute to the empty string.

      In earlier versions, the terminator was hardcoded as ``'\n'``.

      .. versionadded:: 3.2


.. _file-handler:

FileHandler
^^^^^^^^^^^

The :class:`FileHandler` class, located in the core :mod:`logging` package,
sends logging output to a disk file.  It inherits the output functionality from
:class:`StreamHandler`.


.. class:: FileHandler(filename, mode='a', encoding=None, delay=False, errors=None)

   Returns a new instance of the :class:`FileHandler` class. The specified file is
   opened and used as the stream for logging. If *mode* is not specified,
   :const:`'a'` is used.  If *encoding* is not ``None``, it is used to open the file
   with that encoding.  If *delay* is true, then file opening is deferred until the
   first call to :meth:`emit`. By default, the file grows indefinitely. If
   *errors* is specified, it's used to determine how encoding errors are handled.

   .. versionchanged:: 3.6
      As well as string values, :class:`~pathlib.Path` objects are also accepted
      for the *filename* argument.

   .. versionchanged:: 3.9
      The *errors* parameter was added.

   .. method:: close()

      Closes the file.

   .. method:: emit(record)

      Outputs the record to the file.


.. _null-handler:

NullHandler
^^^^^^^^^^^

.. versionadded:: 3.1

The :class:`NullHandler` class, located in the core :mod:`logging` package,
does not do any formatting or output. It is essentially a 'no-op' handler
for use by library developers.

.. class:: NullHandler()

   Returns a new instance of the :class:`NullHandler` class.

   .. method:: emit(record)

      This method does nothing.

   .. method:: handle(record)

      This method does nothing.

   .. method:: createLock()

      This method returns ``None`` for the lock, since there is no
      underlying I/O to which access needs to be serialized.


See :ref:`library-config` for more information on how to use
:class:`NullHandler`.

.. _watched-file-handler:

WatchedFileHandler
^^^^^^^^^^^^^^^^^^

.. currentmodule:: logging.handlers

The :class:`WatchedFileHandler` class, located in the :mod:`logging.handlers`
module, is a :class:`FileHandler` which watches the file it is logging to. If
the file changes, it is closed and reopened using the file name.

A file change can happen because of usage of programs such as *newsyslog* and
*logrotate* which perform log file rotation. This handler, intended for use
under Unix/Linux, watches the file to see if it has changed since the last emit.
(A file is deemed to have changed if its device or inode have changed.) If the
file has changed, the old file stream is closed, and the file opened to get a
new stream.

This handler is not appropriate for use under Windows, because under Windows
open log files cannot be moved or renamed - logging opens the files with
exclusive locks - and so there is no need for such a handler. Furthermore,
*ST_INO* is not supported under Windows; :func:`~os.stat` always returns zero
for this value.


.. class:: WatchedFileHandler(filename, mode='a', encoding=None, delay=False, errors=None)

   Returns a new instance of the :class:`WatchedFileHandler` class. The specified
   file is opened and used as the stream for logging. If *mode* is not specified,
   :const:`'a'` is used.  If *encoding* is not ``None``, it is used to open the file
   with that encoding.  If *delay* is true, then file opening is deferred until the
   first call to :meth:`emit`.  By default, the file grows indefinitely. If
   *errors* is provided, it determines how encoding errors are handled.

   .. versionchanged:: 3.6
      As well as string values, :class:`~pathlib.Path` objects are also accepted
      for the *filename* argument.

   .. versionchanged:: 3.9
      The *errors* parameter was added.

   .. method:: reopenIfNeeded()

      Checks to see if the file has changed.  If it has, the existing stream is
      flushed and closed and the file opened again, typically as a precursor to
      outputting the record to the file.

      .. versionadded:: 3.6


   .. method:: emit(record)

      Outputs the record to the file, but first calls :meth:`reopenIfNeeded` to
      reopen the file if it has changed.

.. _base-rotating-handler:

BaseRotatingHandler
^^^^^^^^^^^^^^^^^^^

The :class:`BaseRotatingHandler` class, located in the :mod:`logging.handlers`
module, is the base class for the rotating file handlers,
:class:`RotatingFileHandler` and :class:`TimedRotatingFileHandler`. You should
not need to instantiate this class, but it has attributes and methods you may
need to override.

.. class:: BaseRotatingHandler(filename, mode, encoding=None, delay=False, errors=None)

   The parameters are as for :class:`FileHandler`. The attributes are:

   .. attribute:: namer

      If this attribute is set to a callable, the :meth:`rotation_filename`
      method delegates to this callable. The parameters passed to the callable
      are those passed to :meth:`rotation_filename`.

      .. note:: The namer function is called quite a few times during rollover,
         so it should be as simple and as fast as possible. It should also
         return the same output every time for a given input, otherwise the
         rollover behaviour may not work as expected.

      .. versionadded:: 3.3


   .. attribute:: BaseRotatingHandler.rotator

      If this attribute is set to a callable, the :meth:`rotate` method
      delegates to this callable.  The parameters passed to the callable are
      those passed to :meth:`rotate`.

      .. versionadded:: 3.3

   .. method:: BaseRotatingHandler.rotation_filename(default_name)

      Modify the filename of a log file when rotating.

      This is provided so that a custom filename can be provided.

      The default implementation calls the 'namer' attribute of the handler,
      if it's callable, passing the default name to it. If the attribute isn't
      callable (the default is ``None``), the name is returned unchanged.

      :param default_name: The default name for the log file.

      .. versionadded:: 3.3


   .. method:: BaseRotatingHandler.rotate(source, dest)

      When rotating, rotate the current log.

      The default implementation calls the 'rotator' attribute of the handler,
      if it's callable, passing the source and dest arguments to it. If the
      attribute isn't callable (the default is ``None``), the source is simply
      renamed to the destination.

      :param source: The source filename. This is normally the base
                     filename, e.g. 'test.log'.
      :param dest:   The destination filename. This is normally
                     what the source is rotated to, e.g. 'test.log.1'.

      .. versionadded:: 3.3

The reason the attributes exist is to save you having to subclass - you can use
the same callables for instances of :class:`RotatingFileHandler` and
:class:`TimedRotatingFileHandler`. If either the namer or rotator callable
raises an exception, this will be handled in the same way as any other
exception during an :meth:`emit` call, i.e. via the :meth:`handleError` method
of the handler.

If you need to make more significant changes to rotation processing, you can
override the methods.

For an example, see :ref:`cookbook-rotator-namer`.


.. _rotating-file-handler:

RotatingFileHandler
^^^^^^^^^^^^^^^^^^^

The :class:`RotatingFileHandler` class, located in the :mod:`logging.handlers`
module, supports rotation of disk log files.


.. class:: RotatingFileHandler(filename, mode='a', maxBytes=0, backupCount=0, encoding=None, delay=False, errors=None)

   Returns a new instance of the :class:`RotatingFileHandler` class. The specified
   file is opened and used as the stream for logging. If *mode* is not specified,
   ``'a'`` is used.  If *encoding* is not ``None``, it is used to open the file
   with that encoding.  If *delay* is true, then file opening is deferred until the
   first call to :meth:`emit`.  By default, the file grows indefinitely. If
   *errors* is provided, it determines how encoding errors are handled.

   You can use the *maxBytes* and *backupCount* values to allow the file to
   :dfn:`rollover` at a predetermined size. When the size is about to be exceeded,
   the file is closed and a new file is silently opened for output. Rollover occurs
   whenever the current log file is nearly *maxBytes* in length; but if either of
   *maxBytes* or *backupCount* is zero, rollover never occurs, so you generally want
   to set *backupCount* to at least 1, and have a non-zero *maxBytes*.
   When *backupCount* is non-zero, the system will save old log files by appending
   the extensions '.1', '.2' etc., to the filename. For example, with a *backupCount*
   of 5 and a base file name of :file:`app.log`, you would get :file:`app.log`,
   :file:`app.log.1`, :file:`app.log.2`, up to :file:`app.log.5`. The file being
   written to is always :file:`app.log`.  When this file is filled, it is closed
   and renamed to :file:`app.log.1`, and if files :file:`app.log.1`,
   :file:`app.log.2`, etc. exist, then they are renamed to :file:`app.log.2`,
   :file:`app.log.3` etc. respectively.

   .. versionchanged:: 3.6
      As well as string values, :class:`~pathlib.Path` objects are also accepted
      for the *filename* argument.

   .. versionchanged:: 3.9
      The *errors* parameter was added.

   .. method:: doRollover()

      Does a rollover, as described above.


   .. method:: emit(record)

      Outputs the record to the file, catering for rollover as described
      previously.

.. _timed-rotating-file-handler:

TimedRotatingFileHandler
^^^^^^^^^^^^^^^^^^^^^^^^

The :class:`TimedRotatingFileHandler` class, located in the
:mod:`logging.handlers` module, supports rotation of disk log files at certain
timed intervals.


.. class:: TimedRotatingFileHandler(filename, when='h', interval=1, backupCount=0, encoding=None, delay=False, utc=False, atTime=None, errors=None)

   Returns a new instance of the :class:`TimedRotatingFileHandler` class. The
   specified file is opened and used as the stream for logging. On rotating it also
   sets the filename suffix. Rotating happens based on the product of *when* and
   *interval*.

   You can use the *when* to specify the type of *interval*. The list of possible
   values is below.  Note that they are not case sensitive.

   +----------------+----------------------------+-------------------------+
   | Value          | Type of interval           | If/how *atTime* is used |
   +================+============================+=========================+
   | ``'S'``        | Seconds                    | Ignored                 |
   +----------------+----------------------------+-------------------------+
   | ``'M'``        | Minutes                    | Ignored                 |
   +----------------+----------------------------+-------------------------+
   | ``'H'``        | Hours                      | Ignored                 |
   +----------------+----------------------------+-------------------------+
   | ``'D'``        | Days                       | Ignored                 |
   +----------------+----------------------------+-------------------------+
   | ``'W0'-'W6'``  | Weekday (0=Monday)         | Used to compute initial |
   |                |                            | rollover time           |
   +----------------+----------------------------+-------------------------+
   | ``'midnight'`` | Roll over at midnight, if  | Used to compute initial |
   |                | *atTime* not specified,    | rollover time           |
   |                | else at time *atTime*      |                         |
   +----------------+----------------------------+-------------------------+

   When using weekday-based rotation, specify 'W0' for Monday, 'W1' for
   Tuesday, and so on up to 'W6' for Sunday. In this case, the value passed for
   *interval* isn't used.

   The system will save old log files by appending extensions to the filename.
   The extensions are date-and-time based, using the strftime format
   ``%Y-%m-%d_%H-%M-%S`` or a leading portion thereof, depending on the
   rollover interval.

   When computing the next rollover time for the first time (when the handler
   is created), the last modification time of an existing log file, or else
   the current time, is used to compute when the next rotation will occur.

   If the *utc* argument is true, times in UTC will be used; otherwise
   local time is used.

   If *backupCount* is nonzero, at most *backupCount* files
   will be kept, and if more would be created when rollover occurs, the oldest
   one is deleted. The deletion logic uses the interval to determine which
   files to delete, so changing the interval may leave old files lying around.

   If *delay* is true, then file opening is deferred until the first call to
   :meth:`emit`.

   If *atTime* is not ``None``, it must be a ``datetime.time`` instance which
   specifies the time of day when rollover occurs, for the cases where rollover
   is set to happen "at midnight" or "on a particular weekday". Note that in
   these cases, the *atTime* value is effectively used to compute the *initial*
   rollover, and subsequent rollovers would be calculated via the normal
   interval calculation.

   If *errors* is specified, it's used to determine how encoding errors are
   handled.

   .. note:: Calculation of the initial rollover time is done when the handler
      is initialised. Calculation of subsequent rollover times is done only
      when rollover occurs, and rollover occurs only when emitting output. If
      this is not kept in mind, it might lead to some confusion. For example,
      if an interval of "every minute" is set, that does not mean you will
      always see log files with times (in the filename) separated by a minute;
      if, during application execution, logging output is generated more
      frequently than once a minute, *then* you can expect to see log files
      with times separated by a minute. If, on the other hand, logging messages
      are only output once every five minutes (say), then there will be gaps in
      the file times corresponding to the minutes where no output (and hence no
      rollover) occurred.

   .. versionchanged:: 3.4
      *atTime* parameter was added.

   .. versionchanged:: 3.6
      As well as string values, :class:`~pathlib.Path` objects are also accepted
      for the *filename* argument.

   .. versionchanged:: 3.9
      The *errors* parameter was added.

   .. method:: doRollover()

      Does a rollover, as described above.

   .. method:: emit(record)

      Outputs the record to the file, catering for rollover as described above.


.. _socket-handler:

SocketHandler
^^^^^^^^^^^^^

The :class:`SocketHandler` class, located in the :mod:`logging.handlers` module,
sends logging output to a network socket. The base class uses a TCP socket.


.. class:: SocketHandler(host, port)

   Returns a new instance of the :class:`SocketHandler` class intended to
   communicate with a remote machine whose address is given by *host* and *port*.

   .. versionchanged:: 3.4
      If ``port`` is specified as ``None``, a Unix domain socket is created
      using the value in ``host`` - otherwise, a TCP socket is created.

   .. method:: close()

      Closes the socket.


   .. method:: emit()

      Pickles the record's attribute dictionary and writes it to the socket in
      binary format. If there is an error with the socket, silently drops the
      packet. If the connection was previously lost, re-establishes the
      connection. To unpickle the record at the receiving end into a
      :class:`~logging.LogRecord`, use the :func:`~logging.makeLogRecord`
      function.


   .. method:: handleError()

      Handles an error which has occurred during :meth:`emit`. The most likely
      cause is a lost connection. Closes the socket so that we can retry on the
      next event.


   .. method:: makeSocket()

      This is a factory method which allows subclasses to define the precise
      type of socket they want. The default implementation creates a TCP socket
      (:const:`socket.SOCK_STREAM`).


   .. method:: makePickle(record)

      Pickles the record's attribute dictionary in binary format with a length
      prefix, and returns it ready for transmission across the socket. The
      details of this operation are equivalent to::

          data = pickle.dumps(record_attr_dict, 1)
          datalen = struct.pack('>L', len(data))
          return datalen + data

      Note that pickles aren't completely secure. If you are concerned about
      security, you may want to override this method to implement a more secure
      mechanism. For example, you can sign pickles using HMAC and then verify
      them on the receiving end, or alternatively you can disable unpickling of
      global objects on the receiving end.


   .. method:: send(packet)

      Send a pickled byte-string *packet* to the socket. The format of the sent
      byte-string is as described in the documentation for
      :meth:`~SocketHandler.makePickle`.

      This function allows for partial sends, which can happen when the network
      is busy.


   .. method:: createSocket()

      Tries to create a socket; on failure, uses an exponential back-off
      algorithm.  On initial failure, the handler will drop the message it was
      trying to send.  When subsequent messages are handled by the same
      instance, it will not try connecting until some time has passed.  The
      default parameters are such that the initial delay is one second, and if
      after that delay the connection still can't be made, the handler will
      double the delay each time up to a maximum of 30 seconds.

      This behaviour is controlled by the following handler attributes:

      * ``retryStart`` (initial delay, defaulting to 1.0 seconds).
      * ``retryFactor`` (multiplier, defaulting to 2.0).
      * ``retryMax`` (maximum delay, defaulting to 30.0 seconds).

      This means that if the remote listener starts up *after* the handler has
      been used, you could lose messages (since the handler won't even attempt
      a connection until the delay has elapsed, but just silently drop messages
      during the delay period).


.. _datagram-handler:

DatagramHandler
^^^^^^^^^^^^^^^

The :class:`DatagramHandler` class, located in the :mod:`logging.handlers`
module, inherits from :class:`SocketHandler` to support sending logging messages
over UDP sockets.


.. class:: DatagramHandler(host, port)

   Returns a new instance of the :class:`DatagramHandler` class intended to
   communicate with a remote machine whose address is given by *host* and *port*.

   .. versionchanged:: 3.4
      If ``port`` is specified as ``None``, a Unix domain socket is created
      using the value in ``host`` - otherwise, a UDP socket is created.

   .. method:: emit()

      Pickles the record's attribute dictionary and writes it to the socket in
      binary format. If there is an error with the socket, silently drops the
      packet. To unpickle the record at the receiving end into a
      :class:`~logging.LogRecord`, use the :func:`~logging.makeLogRecord`
      function.


   .. method:: makeSocket()

      The factory method of :class:`SocketHandler` is here overridden to create
      a UDP socket (:const:`socket.SOCK_DGRAM`).


   .. method:: send(s)

      Send a pickled byte-string to a socket. The format of the sent byte-string
      is as described in the documentation for :meth:`SocketHandler.makePickle`.


.. _syslog-handler:

SysLogHandler
^^^^^^^^^^^^^

The :class:`SysLogHandler` class, located in the :mod:`logging.handlers` module,
supports sending logging messages to a remote or local Unix syslog.


.. class:: SysLogHandler(address=('localhost', SYSLOG_UDP_PORT), facility=LOG_USER, socktype=socket.SOCK_DGRAM)

   Returns a new instance of the :class:`SysLogHandler` class intended to
   communicate with a remote Unix machine whose address is given by *address* in
   the form of a ``(host, port)`` tuple.  If *address* is not specified,
   ``('localhost', 514)`` is used.  The address is used to open a socket.  An
   alternative to providing a ``(host, port)`` tuple is providing an address as a
   string, for example '/dev/log'. In this case, a Unix domain socket is used to
   send the message to the syslog. If *facility* is not specified,
   :const:`LOG_USER` is used. The type of socket opened depends on the
   *socktype* argument, which defaults to :const:`socket.SOCK_DGRAM` and thus
   opens a UDP socket. To open a TCP socket (for use with the newer syslog
   daemons such as rsyslog), specify a value of :const:`socket.SOCK_STREAM`.

   Note that if your server is not listening on UDP port 514,
   :class:`SysLogHandler` may appear not to work. In that case, check what
   address you should be using for a domain socket - it's system dependent.
   For example, on Linux it's usually '/dev/log' but on OS/X it's
   '/var/run/syslog'. You'll need to check your platform and use the
   appropriate address (you may need to do this check at runtime if your
   application needs to run on several platforms). On Windows, you pretty
   much have to use the UDP option.

   .. versionchanged:: 3.2
      *socktype* was added.


   .. method:: close()

      Closes the socket to the remote host.


   .. method:: emit(record)

      The record is formatted, and then sent to the syslog server. If exception
      information is present, it is *not* sent to the server.

      .. versionchanged:: 3.2.1
         (See: :issue:`12168`.) In earlier versions, the message sent to the
         syslog daemons was always terminated with a NUL byte, because early
         versions of these daemons expected a NUL terminated message - even
         though it's not in the relevant specification (:rfc:`5424`). More recent
         versions of these daemons don't expect the NUL byte but strip it off
         if it's there, and even more recent daemons (which adhere more closely
         to RFC 5424) pass the NUL byte on as part of the message.

         To enable easier handling of syslog messages in the face of all these
         differing daemon behaviours, the appending of the NUL byte has been
         made configurable, through the use of a class-level attribute,
         ``append_nul``. This defaults to ``True`` (preserving the existing
         behaviour) but can be set to ``False`` on a ``SysLogHandler`` instance
         in order for that instance to *not* append the NUL terminator.

      .. versionchanged:: 3.3
         (See: :issue:`12419`.) In earlier versions, there was no facility for
         an "ident" or "tag" prefix to identify the source of the message. This
         can now be specified using a class-level attribute, defaulting to
         ``""`` to preserve existing behaviour, but which can be overridden on
         a ``SysLogHandler`` instance in order for that instance to prepend
         the ident to every message handled. Note that the provided ident must
         be text, not bytes, and is prepended to the message exactly as is.

   .. method:: encodePriority(facility, priority)

      Encodes the facility and priority into an integer. You can pass in strings
      or integers - if strings are passed, internal mapping dictionaries are
      used to convert them to integers.

      The symbolic ``LOG_`` values are defined in :class:`SysLogHandler` and
      mirror the values defined in the ``sys/syslog.h`` header file.

      **Priorities**

      +--------------------------+---------------+
      | Name (string)            | Symbolic value|
      +==========================+===============+
      | ``alert``                | LOG_ALERT     |
      +--------------------------+---------------+
      | ``crit`` or ``critical`` | LOG_CRIT      |
      +--------------------------+---------------+
      | ``debug``                | LOG_DEBUG     |
      +--------------------------+---------------+
      | ``emerg`` or ``panic``   | LOG_EMERG     |
      +--------------------------+---------------+
      | ``err`` or ``error``     | LOG_ERR       |
      +--------------------------+---------------+
      | ``info``                 | LOG_INFO      |
      +--------------------------+---------------+
      | ``notice``               | LOG_NOTICE    |
      +--------------------------+---------------+
      | ``warn`` or ``warning``  | LOG_WARNING   |
      +--------------------------+---------------+

      **Facilities**

      +---------------+---------------+
      | Name (string) | Symbolic value|
      +===============+===============+
      | ``auth``      | LOG_AUTH      |
      +---------------+---------------+
      | ``authpriv``  | LOG_AUTHPRIV  |
      +---------------+---------------+
      | ``cron``      | LOG_CRON      |
      +---------------+---------------+
      | ``daemon``    | LOG_DAEMON    |
      +---------------+---------------+
      | ``ftp``       | LOG_FTP       |
      +---------------+---------------+
      | ``kern``      | LOG_KERN      |
      +---------------+---------------+
      | ``lpr``       | LOG_LPR       |
      +---------------+---------------+
      | ``mail``      | LOG_MAIL      |
      +---------------+---------------+
      | ``news``      | LOG_NEWS      |
      +---------------+---------------+
      | ``syslog``    | LOG_SYSLOG    |
      +---------------+---------------+
      | ``user``      | LOG_USER      |
      +---------------+---------------+
      | ``uucp``      | LOG_UUCP      |
      +---------------+---------------+
      | ``local0``    | LOG_LOCAL0    |
      +---------------+---------------+
      | ``local1``    | LOG_LOCAL1    |
      +---------------+---------------+
      | ``local2``    | LOG_LOCAL2    |
      +---------------+---------------+
      | ``local3``    | LOG_LOCAL3    |
      +---------------+---------------+
      | ``local4``    | LOG_LOCAL4    |
      +---------------+---------------+
      | ``local5``    | LOG_LOCAL5    |
      +---------------+---------------+
      | ``local6``    | LOG_LOCAL6    |
      +---------------+---------------+
      | ``local7``    | LOG_LOCAL7    |
      +---------------+---------------+

   .. method:: mapPriority(levelname)

      Maps a logging level name to a syslog priority name.
      You may need to override this if you are using custom levels, or
      if the default algorithm is not suitable for your needs. The
      default algorithm maps ``DEBUG``, ``INFO``, ``WARNING``, ``ERROR`` and
      ``CRITICAL`` to the equivalent syslog names, and all other level
      names to 'warning'.

.. _nt-eventlog-handler:

NTEventLogHandler
^^^^^^^^^^^^^^^^^

The :class:`NTEventLogHandler` class, located in the :mod:`logging.handlers`
module, supports sending logging messages to a local Windows NT, Windows 2000 or
Windows XP event log. Before you can use it, you need Mark Hammond's Win32
extensions for Python installed.


.. class:: NTEventLogHandler(appname, dllname=None, logtype='Application')

   Returns a new instance of the :class:`NTEventLogHandler` class. The *appname* is
   used to define the application name as it appears in the event log. An
   appropriate registry entry is created using this name. The *dllname* should give
   the fully qualified pathname of a .dll or .exe which contains message
   definitions to hold in the log (if not specified, ``'win32service.pyd'`` is used
   - this is installed with the Win32 extensions and contains some basic
   placeholder message definitions. Note that use of these placeholders will make
   your event logs big, as the entire message source is held in the log. If you
   want slimmer logs, you have to pass in the name of your own .dll or .exe which
   contains the message definitions you want to use in the event log). The
   *logtype* is one of ``'Application'``, ``'System'`` or ``'Security'``, and
   defaults to ``'Application'``.


   .. method:: close()

      At this point, you can remove the application name from the registry as a
      source of event log entries. However, if you do this, you will not be able
      to see the events as you intended in the Event Log Viewer - it needs to be
      able to access the registry to get the .dll name. The current version does
      not do this.


   .. method:: emit(record)

      Determines the message ID, event category and event type, and then logs
      the message in the NT event log.


   .. method:: getEventCategory(record)

      Returns the event category for the record. Override this if you want to
      specify your own categories. This version returns 0.


   .. method:: getEventType(record)

      Returns the event type for the record. Override this if you want to
      specify your own types. This version does a mapping using the handler's
      typemap attribute, which is set up in :meth:`__init__` to a dictionary
      which contains mappings for :const:`DEBUG`, :const:`INFO`,
      :const:`WARNING`, :const:`ERROR` and :const:`CRITICAL`. If you are using
      your own levels, you will either need to override this method or place a
      suitable dictionary in the handler's *typemap* attribute.


   .. method:: getMessageID(record)

      Returns the message ID for the record. If you are using your own messages,
      you could do this by having the *msg* passed to the logger being an ID
      rather than a format string. Then, in here, you could use a dictionary
      lookup to get the message ID. This version returns 1, which is the base
      message ID in :file:`win32service.pyd`.

.. _smtp-handler:

SMTPHandler
^^^^^^^^^^^

The :class:`SMTPHandler` class, located in the :mod:`logging.handlers` module,
supports sending logging messages to an email address via SMTP.


.. class:: SMTPHandler(mailhost, fromaddr, toaddrs, subject, credentials=None, secure=None, timeout=1.0)

   Returns a new instance of the :class:`SMTPHandler` class. The instance is
   initialized with the from and to addresses and subject line of the email. The
   *toaddrs* should be a list of strings. To specify a non-standard SMTP port, use
   the (host, port) tuple format for the *mailhost* argument. If you use a string,
   the standard SMTP port is used. If your SMTP server requires authentication, you
   can specify a (username, password) tuple for the *credentials* argument.

   To specify the use of a secure protocol (TLS), pass in a tuple to the
   *secure* argument. This will only be used when authentication credentials are
   supplied. The tuple should be either an empty tuple, or a single-value tuple
   with the name of a keyfile, or a 2-value tuple with the names of the keyfile
   and certificate file. (This tuple is passed to the
   :meth:`smtplib.SMTP.starttls` method.)

   A timeout can be specified for communication with the SMTP server using the
   *timeout* argument.

   .. versionadded:: 3.3
      The *timeout* argument was added.

   .. method:: emit(record)

      Formats the record and sends it to the specified addressees.


   .. method:: getSubject(record)

      If you want to specify a subject line which is record-dependent, override
      this method.

.. _memory-handler:

MemoryHandler
^^^^^^^^^^^^^

The :class:`MemoryHandler` class, located in the :mod:`logging.handlers` module,
supports buffering of logging records in memory, periodically flushing them to a
:dfn:`target` handler. Flushing occurs whenever the buffer is full, or when an
event of a certain severity or greater is seen.

:class:`MemoryHandler` is a subclass of the more general
:class:`BufferingHandler`, which is an abstract class. This buffers logging
records in memory. Whenever each record is added to the buffer, a check is made
by calling :meth:`shouldFlush` to see if the buffer should be flushed.  If it
should, then :meth:`flush` is expected to do the flushing.


.. class:: BufferingHandler(capacity)

   Initializes the handler with a buffer of the specified capacity. Here,
   *capacity* means the number of logging records buffered.


   .. method:: emit(record)

      Append the record to the buffer. If :meth:`shouldFlush` returns true,
      call :meth:`flush` to process the buffer.


   .. method:: flush()

      You can override this to implement custom flushing behavior. This version
      just zaps the buffer to empty.


   .. method:: shouldFlush(record)

      Return ``True`` if the buffer is up to capacity. This method can be
      overridden to implement custom flushing strategies.


.. class:: MemoryHandler(capacity, flushLevel=ERROR, target=None, flushOnClose=True)

   Returns a new instance of the :class:`MemoryHandler` class. The instance is
   initialized with a buffer size of *capacity* (number of records buffered).
   If *flushLevel* is not specified, :const:`ERROR` is used. If no *target* is
   specified, the target will need to be set using :meth:`setTarget` before this
   handler does anything useful. If *flushOnClose* is specified as ``False``,
   then the buffer is *not* flushed when the handler is closed. If not specified
   or specified as ``True``, the previous behaviour of flushing the buffer will
   occur when the handler is closed.

   .. versionchanged:: 3.6
      The *flushOnClose* parameter was added.


   .. method:: close()

      Calls :meth:`flush`, sets the target to ``None`` and clears the
      buffer.


   .. method:: flush()

      For a :class:`MemoryHandler`, flushing means just sending the buffered
      records to the target, if there is one. The buffer is also cleared when
      this happens. Override if you want different behavior.


   .. method:: setTarget(target)

      Sets the target handler for this handler.


   .. method:: shouldFlush(record)

      Checks for buffer full or a record at the *flushLevel* or higher.


.. _http-handler:

HTTPHandler
^^^^^^^^^^^

The :class:`HTTPHandler` class, located in the :mod:`logging.handlers` module,
supports sending logging messages to a Web server, using either ``GET`` or
``POST`` semantics.


.. class:: HTTPHandler(host, url, method='GET', secure=False, credentials=None, context=None)

   Returns a new instance of the :class:`HTTPHandler` class. The *host* can be
   of the form ``host:port``, should you need to use a specific port number.  If
   no *method* is specified, ``GET`` is used. If *secure* is true, a HTTPS
   connection will be used. The *context* parameter may be set to a
   :class:`ssl.SSLContext` instance to configure the SSL settings used for the
   HTTPS connection. If *credentials* is specified, it should be a 2-tuple
   consisting of userid and password, which will be placed in a HTTP
   'Authorization' header using Basic authentication. If you specify
   credentials, you should also specify secure=True so that your userid and
   password are not passed in cleartext across the wire.

   .. versionchanged:: 3.5
      The *context* parameter was added.

   .. method:: mapLogRecord(record)

      Provides a dictionary, based on ``record``, which is to be URL-encoded
      and sent to the web server. The default implementation just returns
      ``record.__dict__``. This method can be overridden if e.g. only a
      subset of :class:`~logging.LogRecord` is to be sent to the web server, or
      if more specific customization of what's sent to the server is required.

   .. method:: emit(record)

      Sends the record to the Web server as a URL-encoded dictionary. The
      :meth:`mapLogRecord` method is used to convert the record to the
      dictionary to be sent.

   .. note:: Since preparing a record for sending it to a Web server is not
      the same as a generic formatting operation, using
      :meth:`~logging.Handler.setFormatter` to specify a
      :class:`~logging.Formatter` for a :class:`HTTPHandler` has no effect.
      Instead of calling :meth:`~logging.Handler.format`, this handler calls
      :meth:`mapLogRecord` and then :func:`urllib.parse.urlencode` to encode the
      dictionary in a form suitable for sending to a Web server.


.. _queue-handler:


QueueHandler
^^^^^^^^^^^^

.. versionadded:: 3.2

The :class:`QueueHandler` class, located in the :mod:`logging.handlers` module,
supports sending logging messages to a queue, such as those implemented in the
:mod:`queue` or :mod:`multiprocessing` modules.

Along with the :class:`QueueListener` class, :class:`QueueHandler` can be used
to let handlers do their work on a separate thread from the one which does the
logging. This is important in Web applications and also other service
applications where threads servicing clients need to respond as quickly as
possible, while any potentially slow operations (such as sending an email via
:class:`SMTPHandler`) are done on a separate thread.

.. class:: QueueHandler(queue)

   Returns a new instance of the :class:`QueueHandler` class. The instance is
   initialized with the queue to send messages to. The *queue* can be any
   queue-like object; it's used as-is by the :meth:`enqueue` method, which
   needs to know how to send messages to it. The queue is not *required* to
   have the task tracking API, which means that you can use
   :class:`~queue.SimpleQueue` instances for *queue*.


   .. method:: emit(record)

      Enqueues the result of preparing the LogRecord. Should an exception
      occur (e.g. because a bounded queue has filled up), the
      :meth:`~logging.Handler.handleError` method is called to handle the
      error. This can result in the record silently being dropped (if
      :attr:`logging.raiseExceptions` is ``False``) or a message printed to
      ``sys.stderr`` (if :attr:`logging.raiseExceptions` is ``True``).

   .. method:: prepare(record)

      Prepares a record for queuing. The object returned by this
      method is enqueued.

      The base implementation formats the record to merge the message,
      arguments, and exception information, if present.  It also
      removes unpickleable items from the record in-place.

      You might want to override this method if you want to convert
      the record to a dict or JSON string, or send a modified copy
      of the record while leaving the original intact.

   .. method:: enqueue(record)

      Enqueues the record on the queue using ``put_nowait()``; you may
      want to override this if you want to use blocking behaviour, or a
      timeout, or a customized queue implementation.



.. _queue-listener:

QueueListener
^^^^^^^^^^^^^

.. versionadded:: 3.2

The :class:`QueueListener` class, located in the :mod:`logging.handlers`
module, supports receiving logging messages from a queue, such as those
implemented in the :mod:`queue` or :mod:`multiprocessing` modules. The
messages are received from a queue in an internal thread and passed, on
the same thread, to one or more handlers for processing. While
:class:`QueueListener` is not itself a handler, it is documented here
because it works hand-in-hand with :class:`QueueHandler`.

Along with the :class:`QueueHandler` class, :class:`QueueListener` can be used
to let handlers do their work on a separate thread from the one which does the
logging. This is important in Web applications and also other service
applications where threads servicing clients need to respond as quickly as
possible, while any potentially slow operations (such as sending an email via
:class:`SMTPHandler`) are done on a separate thread.

.. class:: QueueListener(queue, *handlers, respect_handler_level=False)

   Returns a new instance of the :class:`QueueListener` class. The instance is
   initialized with the queue to send messages to and a list of handlers which
   will handle entries placed on the queue. The queue can be any queue-like
   object; it's passed as-is to the :meth:`dequeue` method, which needs
   to know how to get messages from it. The queue is not *required* to have the
   task tracking API (though it's used if available), which means that you can
   use :class:`~queue.SimpleQueue` instances for *queue*.

   If ``respect_handler_level`` is ``True``, a handler's level is respected
   (compared with the level for the message) when deciding whether to pass
   messages to that handler; otherwise, the behaviour is as in previous Python
   versions - to always pass each message to each handler.

   .. versionchanged:: 3.5
      The ``respect_handler_level`` argument was added.

   .. method:: dequeue(block)

      Dequeues a record and return it, optionally blocking.

      The base implementation uses ``get()``. You may want to override this
      method if you want to use timeouts or work with custom queue
      implementations.

   .. method:: prepare(record)

      Prepare a record for handling.

      This implementation just returns the passed-in record. You may want to
      override this method if you need to do any custom marshalling or
      manipulation of the record before passing it to the handlers.

   .. method:: handle(record)

      Handle a record.

      This just loops through the handlers offering them the record
      to handle. The actual object passed to the handlers is that which
      is returned from :meth:`prepare`.

   .. method:: start()

      Starts the listener.

      This starts up a background thread to monitor the queue for
      LogRecords to process.

   .. method:: stop()

      Stops the listener.

      This asks the thread to terminate, and then waits for it to do so.
      Note that if you don't call this before your application exits, there
      may be some records still left on the queue, which won't be processed.

   .. method:: enqueue_sentinel()

      Writes a sentinel to the queue to tell the listener to quit. This
      implementation uses ``put_nowait()``.  You may want to override this
      method if you want to use timeouts or work with custom queue
      implementations.

      .. versionadded:: 3.3


.. seealso::

   Module :mod:`logging`
      API reference for the logging module.

   Module :mod:`logging.config`
      Configuration API for the logging module.


