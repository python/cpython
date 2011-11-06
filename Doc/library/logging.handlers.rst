:mod:`logging.handlers` --- Logging handlers
============================================

.. module:: logging.handlers
   :synopsis: Handlers for the logging module.


.. moduleauthor:: Vinay Sajip <vinay_sajip@red-dove.com>
.. sectionauthor:: Vinay Sajip <vinay_sajip@red-dove.com>

.. sidebar:: Important

   This page contains only reference information. For tutorials,
   please see

   * :ref:`Basic Tutorial <logging-basic-tutorial>`
   * :ref:`Advanced Tutorial <logging-advanced-tutorial>`
   * :ref:`Logging Cookbook <logging-cookbook>`

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
      is then written to the stream with a newline terminator. If exception
      information is present, it is formatted using
      :func:`traceback.print_exception` and appended to the stream.


   .. method:: flush()

      Flushes the stream by calling its :meth:`flush` method. Note that the
      :meth:`close` method is inherited from :class:`Handler` and so does
      no output, so an explicit :meth:`flush` call may be needed at times.

.. _file-handler:

FileHandler
^^^^^^^^^^^

The :class:`FileHandler` class, located in the core :mod:`logging` package,
sends logging output to a disk file.  It inherits the output functionality from
:class:`StreamHandler`.


.. class:: FileHandler(filename, mode='a', encoding=None, delay=False)

   Returns a new instance of the :class:`FileHandler` class. The specified file is
   opened and used as the stream for logging. If *mode* is not specified,
   :const:`'a'` is used.  If *encoding* is not *None*, it is used to open the file
   with that encoding.  If *delay* is true, then file opening is deferred until the
   first call to :meth:`emit`. By default, the file grows indefinitely.

   .. versionchanged:: 2.6
      *delay* was added.

   .. method:: close()

      Closes the file.


   .. method:: emit(record)

      Outputs the record to the file.


.. _null-handler:

NullHandler
^^^^^^^^^^^

.. versionadded:: 2.7

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

.. versionadded:: 2.6

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
*ST_INO* is not supported under Windows; :func:`stat` always returns zero for
this value.


.. class:: WatchedFileHandler(filename[,mode[, encoding[, delay]]])

   Returns a new instance of the :class:`WatchedFileHandler` class. The specified
   file is opened and used as the stream for logging. If *mode* is not specified,
   :const:`'a'` is used.  If *encoding* is not *None*, it is used to open the file
   with that encoding.  If *delay* is true, then file opening is deferred until the
   first call to :meth:`emit`.  By default, the file grows indefinitely.


   .. method:: emit(record)

      Outputs the record to the file, but first checks to see if the file has
      changed.  If it has, the existing stream is flushed and closed and the
      file opened again, before outputting the record to the file.

.. _rotating-file-handler:

RotatingFileHandler
^^^^^^^^^^^^^^^^^^^

The :class:`RotatingFileHandler` class, located in the :mod:`logging.handlers`
module, supports rotation of disk log files.


.. class:: RotatingFileHandler(filename, mode='a', maxBytes=0, backupCount=0, encoding=None, delay=0)

   Returns a new instance of the :class:`RotatingFileHandler` class. The specified
   file is opened and used as the stream for logging. If *mode* is not specified,
   ``'a'`` is used.  If *encoding* is not *None*, it is used to open the file
   with that encoding.  If *delay* is true, then file opening is deferred until the
   first call to :meth:`emit`.  By default, the file grows indefinitely.

   You can use the *maxBytes* and *backupCount* values to allow the file to
   :dfn:`rollover` at a predetermined size. When the size is about to be exceeded,
   the file is closed and a new file is silently opened for output. Rollover occurs
   whenever the current log file is nearly *maxBytes* in length; if *maxBytes* is
   zero, rollover never occurs.  If *backupCount* is non-zero, the system will save
   old log files by appending the extensions '.1', '.2' etc., to the filename. For
   example, with a *backupCount* of 5 and a base file name of :file:`app.log`, you
   would get :file:`app.log`, :file:`app.log.1`, :file:`app.log.2`, up to
   :file:`app.log.5`. The file being written to is always :file:`app.log`.  When
   this file is filled, it is closed and renamed to :file:`app.log.1`, and if files
   :file:`app.log.1`, :file:`app.log.2`, etc.  exist, then they are renamed to
   :file:`app.log.2`, :file:`app.log.3` etc.  respectively.

   .. versionchanged:: 2.6
      *delay* was added.


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


.. class:: TimedRotatingFileHandler(filename, when='h', interval=1, backupCount=0, encoding=None, delay=False, utc=False)

   Returns a new instance of the :class:`TimedRotatingFileHandler` class. The
   specified file is opened and used as the stream for logging. On rotating it also
   sets the filename suffix. Rotating happens based on the product of *when* and
   *interval*.

   You can use the *when* to specify the type of *interval*. The list of possible
   values is below.  Note that they are not case sensitive.

   +----------------+-----------------------+
   | Value          | Type of interval      |
   +================+=======================+
   | ``'S'``        | Seconds               |
   +----------------+-----------------------+
   | ``'M'``        | Minutes               |
   +----------------+-----------------------+
   | ``'H'``        | Hours                 |
   +----------------+-----------------------+
   | ``'D'``        | Days                  |
   +----------------+-----------------------+
   | ``'W'``        | Week day (0=Monday)   |
   +----------------+-----------------------+
   | ``'midnight'`` | Roll over at midnight |
   +----------------+-----------------------+

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

   .. versionchanged:: 2.6
      *delay* and *utc* were added.


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


   .. method:: close()

      Closes the socket.


   .. method:: emit()

      Pickles the record's attribute dictionary and writes it to the socket in
      binary format. If there is an error with the socket, silently drops the
      packet. If the connection was previously lost, re-establishes the
      connection. To unpickle the record at the receiving end into a
      :class:`LogRecord`, use the :func:`makeLogRecord` function.


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
      prefix, and returns it ready for transmission across the socket.

      Note that pickles aren't completely secure. If you are concerned about
      security, you may want to override this method to implement a more secure
      mechanism. For example, you can sign pickles using HMAC and then verify
      them on the receiving end, or alternatively you can disable unpickling of
      global objects on the receiving end.


   .. method:: send(packet)

      Send a pickled string *packet* to the socket. This function allows for
      partial sends which can happen when the network is busy.


   .. method:: createSocket()

      Tries to create a socket; on failure, uses an exponential back-off
      algorithm.  On intial failure, the handler will drop the message it was
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


   .. method:: emit()

      Pickles the record's attribute dictionary and writes it to the socket in
      binary format. If there is an error with the socket, silently drops the
      packet. To unpickle the record at the receiving end into a
      :class:`LogRecord`, use the :func:`makeLogRecord` function.


   .. method:: makeSocket()

      The factory method of :class:`SocketHandler` is here overridden to create
      a UDP socket (:const:`socket.SOCK_DGRAM`).


   .. method:: send(s)

      Send a pickled string to a socket.


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

   .. versionchanged:: 2.7
      *socktype* was added.


   .. method:: close()

      Closes the socket to the remote host.


   .. method:: emit(record)

      The record is formatted, and then sent to the syslog server. If exception
      information is present, it is *not* sent to the server.


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


.. class:: SMTPHandler(mailhost, fromaddr, toaddrs, subject, credentials=None, secure=None)

   Returns a new instance of the :class:`SMTPHandler` class. The instance is
   initialized with the from and to addresses and subject line of the email.
   The *toaddrs* should be a list of strings. To specify a non-standard SMTP
   port, use the (host, port) tuple format for the *mailhost* argument. If you
   use a string, the standard SMTP port is used. If your SMTP server requires
   authentication, you can specify a (username, password) tuple for the
   *credentials* argument.

   To specify the use of a secure protocol (TLS), pass in a tuple to the
   *secure* argument. This will only be used when authentication credentials are
   supplied. The tuple should be either an empty tuple, or a single-value tuple
   with the name of a keyfile, or a 2-value tuple with the names of the keyfile
   and certificate file. (This tuple is passed to the
   :meth:`smtplib.SMTP.starttls` method.)

   .. versionchanged:: 2.6
      *credentials* was added.

   .. versionchanged:: 2.7
      *secure* was added.


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
should, then :meth:`flush` is expected to do the needful.


.. class:: BufferingHandler(capacity)

   Initializes the handler with a buffer of the specified capacity.


   .. method:: emit(record)

      Appends the record to the buffer. If :meth:`shouldFlush` returns true,
      calls :meth:`flush` to process the buffer.


   .. method:: flush()

      You can override this to implement custom flushing behavior. This version
      just zaps the buffer to empty.


   .. method:: shouldFlush(record)

      Returns true if the buffer is up to capacity. This method can be
      overridden to implement custom flushing strategies.


.. class:: MemoryHandler(capacity, flushLevel=ERROR, target=None)

   Returns a new instance of the :class:`MemoryHandler` class. The instance is
   initialized with a buffer size of *capacity*. If *flushLevel* is not specified,
   :const:`ERROR` is used. If no *target* is specified, the target will need to be
   set using :meth:`setTarget` before this handler does anything useful.


   .. method:: close()

      Calls :meth:`flush`, sets the target to :const:`None` and clears the
      buffer.


   .. method:: flush()

      For a :class:`MemoryHandler`, flushing means just sending the buffered
      records to the target, if there is one. The buffer is also cleared when
      this happens. Override if you want different behavior.


   .. method:: setTarget(target)
   .. versionchanged:: 2.6
      *credentials* was added.


      Sets the target handler for this handler.


   .. method:: shouldFlush(record)

      Checks for buffer full or a record at the *flushLevel* or higher.


.. _http-handler:

HTTPHandler
^^^^^^^^^^^

The :class:`HTTPHandler` class, located in the :mod:`logging.handlers` module,
supports sending logging messages to a Web server, using either ``GET`` or
``POST`` semantics.


.. class:: HTTPHandler(host, url, method='GET')

   Returns a new instance of the :class:`HTTPHandler` class. The *host* can be
   of the form ``host:port``, should you need to use a specific port number.
   If no *method* is specified, ``GET`` is used.


   .. method:: emit(record)

      Sends the record to the Web server as a percent-encoded dictionary.


.. seealso::

   Module :mod:`logging`
      API reference for the logging module.

   Module :mod:`logging.config`
      Configuration API for the logging module.


