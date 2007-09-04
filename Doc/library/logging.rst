:mod:`logging` --- Logging facility for Python
==============================================

.. module:: logging
   :synopsis: Flexible error logging system for applications.


.. moduleauthor:: Vinay Sajip <vinay_sajip@red-dove.com>
.. sectionauthor:: Vinay Sajip <vinay_sajip@red-dove.com>


.. % These apply to all modules, and may be given more than once:



.. index:: pair: Errors; logging

This module defines functions and classes which implement a flexible error
logging system for applications.

Logging is performed by calling methods on instances of the :class:`Logger`
class (hereafter called :dfn:`loggers`). Each instance has a name, and they are
conceptually arranged in a name space hierarchy using dots (periods) as
separators. For example, a logger named "scan" is the parent of loggers
"scan.text", "scan.html" and "scan.pdf". Logger names can be anything you want,
and indicate the area of an application in which a logged message originates.

Logged messages also have levels of importance associated with them. The default
levels provided are :const:`DEBUG`, :const:`INFO`, :const:`WARNING`,
:const:`ERROR` and :const:`CRITICAL`. As a convenience, you indicate the
importance of a logged message by calling an appropriate method of
:class:`Logger`. The methods are :meth:`debug`, :meth:`info`, :meth:`warning`,
:meth:`error` and :meth:`critical`, which mirror the default levels. You are not
constrained to use these levels: you can specify your own and use a more general
:class:`Logger` method, :meth:`log`, which takes an explicit level argument.

The numeric values of logging levels are given in the following table. These are
primarily of interest if you want to define your own levels, and need them to
have specific values relative to the predefined levels. If you define a level
with the same numeric value, it overwrites the predefined value; the predefined
name is lost.

+--------------+---------------+
| Level        | Numeric value |
+==============+===============+
| ``CRITICAL`` | 50            |
+--------------+---------------+
| ``ERROR``    | 40            |
+--------------+---------------+
| ``WARNING``  | 30            |
+--------------+---------------+
| ``INFO``     | 20            |
+--------------+---------------+
| ``DEBUG``    | 10            |
+--------------+---------------+
| ``NOTSET``   | 0             |
+--------------+---------------+

Levels can also be associated with loggers, being set either by the developer or
through loading a saved logging configuration. When a logging method is called
on a logger, the logger compares its own level with the level associated with
the method call. If the logger's level is higher than the method call's, no
logging message is actually generated. This is the basic mechanism controlling
the verbosity of logging output.

Logging messages are encoded as instances of the :class:`LogRecord` class. When
a logger decides to actually log an event, a :class:`LogRecord` instance is
created from the logging message.

Logging messages are subjected to a dispatch mechanism through the use of
:dfn:`handlers`, which are instances of subclasses of the :class:`Handler`
class. Handlers are responsible for ensuring that a logged message (in the form
of a :class:`LogRecord`) ends up in a particular location (or set of locations)
which is useful for the target audience for that message (such as end users,
support desk staff, system administrators, developers). Handlers are passed
:class:`LogRecord` instances intended for particular destinations. Each logger
can have zero, one or more handlers associated with it (via the
:meth:`addHandler` method of :class:`Logger`). In addition to any handlers
directly associated with a logger, *all handlers associated with all ancestors
of the logger* are called to dispatch the message.

Just as for loggers, handlers can have levels associated with them. A handler's
level acts as a filter in the same way as a logger's level does. If a handler
decides to actually dispatch an event, the :meth:`emit` method is used to send
the message to its destination. Most user-defined subclasses of :class:`Handler`
will need to override this :meth:`emit`.

In addition to the base :class:`Handler` class, many useful subclasses are
provided:

#. :class:`StreamHandler` instances send error messages to streams (file-like
   objects).

#. :class:`FileHandler` instances send error messages to disk files.

#. :class:`BaseRotatingHandler` is the base class for handlers that rotate log
   files at a certain point. It is not meant to be  instantiated directly. Instead,
   use :class:`RotatingFileHandler` or :class:`TimedRotatingFileHandler`.

#. :class:`RotatingFileHandler` instances send error messages to disk files,
   with support for maximum log file sizes and log file rotation.

#. :class:`TimedRotatingFileHandler` instances send error messages to disk files
   rotating the log file at certain timed intervals.

#. :class:`SocketHandler` instances send error messages to TCP/IP sockets.

#. :class:`DatagramHandler` instances send error messages to UDP sockets.

#. :class:`SMTPHandler` instances send error messages to a designated email
   address.

#. :class:`SysLogHandler` instances send error messages to a Unix syslog daemon,
   possibly on a remote machine.

#. :class:`NTEventLogHandler` instances send error messages to a Windows
   NT/2000/XP event log.

#. :class:`MemoryHandler` instances send error messages to a buffer in memory,
   which is flushed whenever specific criteria are met.

#. :class:`HTTPHandler` instances send error messages to an HTTP server using
   either ``GET`` or ``POST`` semantics.

The :class:`StreamHandler` and :class:`FileHandler` classes are defined in the
core logging package. The other handlers are defined in a sub- module,
:mod:`logging.handlers`. (There is also another sub-module,
:mod:`logging.config`, for configuration functionality.)

Logged messages are formatted for presentation through instances of the
:class:`Formatter` class. They are initialized with a format string suitable for
use with the % operator and a dictionary.

For formatting multiple messages in a batch, instances of
:class:`BufferingFormatter` can be used. In addition to the format string (which
is applied to each message in the batch), there is provision for header and
trailer format strings.

When filtering based on logger level and/or handler level is not enough,
instances of :class:`Filter` can be added to both :class:`Logger` and
:class:`Handler` instances (through their :meth:`addFilter` method). Before
deciding to process a message further, both loggers and handlers consult all
their filters for permission. If any filter returns a false value, the message
is not processed further.

The basic :class:`Filter` functionality allows filtering by specific logger
name. If this feature is used, messages sent to the named logger and its
children are allowed through the filter, and all others dropped.

In addition to the classes described above, there are a number of module- level
functions.


.. function:: getLogger([name])

   Return a logger with the specified name or, if no name is specified, return a
   logger which is the root logger of the hierarchy. If specified, the name is
   typically a dot-separated hierarchical name like *"a"*, *"a.b"* or *"a.b.c.d"*.
   Choice of these names is entirely up to the developer who is using logging.

   All calls to this function with a given name return the same logger instance.
   This means that logger instances never need to be passed between different parts
   of an application.


.. function:: getLoggerClass()

   Return either the standard :class:`Logger` class, or the last class passed to
   :func:`setLoggerClass`. This function may be called from within a new class
   definition, to ensure that installing a customised :class:`Logger` class will
   not undo customisations already applied by other code. For example::

      class MyLogger(logging.getLoggerClass()):
          # ... override behaviour here


.. function:: debug(msg[, *args[, **kwargs]])

   Logs a message with level :const:`DEBUG` on the root logger. The *msg* is the
   message format string, and the *args* are the arguments which are merged into
   *msg* using the string formatting operator. (Note that this means that you can
   use keywords in the format string, together with a single dictionary argument.)

   There are two keyword arguments in *kwargs* which are inspected: *exc_info*
   which, if it does not evaluate as false, causes exception information to be
   added to the logging message. If an exception tuple (in the format returned by
   :func:`sys.exc_info`) is provided, it is used; otherwise, :func:`sys.exc_info`
   is called to get the exception information.

   The other optional keyword argument is *extra* which can be used to pass a
   dictionary which is used to populate the __dict__ of the LogRecord created for
   the logging event with user-defined attributes. These custom attributes can then
   be used as you like. For example, they could be incorporated into logged
   messages. For example::

      FORMAT = "%(asctime)-15s %(clientip)s %(user)-8s %(message)s"
      logging.basicConfig(format=FORMAT)
      d = {'clientip': '192.168.0.1', 'user': 'fbloggs'}
      logging.warning("Protocol problem: %s", "connection reset", extra=d)

   would print something like  ::

      2006-02-08 22:20:02,165 192.168.0.1 fbloggs  Protocol problem: connection reset

   The keys in the dictionary passed in *extra* should not clash with the keys used
   by the logging system. (See the :class:`Formatter` documentation for more
   information on which keys are used by the logging system.)

   If you choose to use these attributes in logged messages, you need to exercise
   some care. In the above example, for instance, the :class:`Formatter` has been
   set up with a format string which expects 'clientip' and 'user' in the attribute
   dictionary of the LogRecord. If these are missing, the message will not be
   logged because a string formatting exception will occur. So in this case, you
   always need to pass the *extra* dictionary with these keys.

   While this might be annoying, this feature is intended for use in specialized
   circumstances, such as multi-threaded servers where the same code executes in
   many contexts, and interesting conditions which arise are dependent on this
   context (such as remote client IP address and authenticated user name, in the
   above example). In such circumstances, it is likely that specialized
   :class:`Formatter`\ s would be used with particular :class:`Handler`\ s.


.. function:: info(msg[, *args[, **kwargs]])

   Logs a message with level :const:`INFO` on the root logger. The arguments are
   interpreted as for :func:`debug`.


.. function:: warning(msg[, *args[, **kwargs]])

   Logs a message with level :const:`WARNING` on the root logger. The arguments are
   interpreted as for :func:`debug`.


.. function:: error(msg[, *args[, **kwargs]])

   Logs a message with level :const:`ERROR` on the root logger. The arguments are
   interpreted as for :func:`debug`.


.. function:: critical(msg[, *args[, **kwargs]])

   Logs a message with level :const:`CRITICAL` on the root logger. The arguments
   are interpreted as for :func:`debug`.


.. function:: exception(msg[, *args])

   Logs a message with level :const:`ERROR` on the root logger. The arguments are
   interpreted as for :func:`debug`. Exception info is added to the logging
   message. This function should only be called from an exception handler.


.. function:: log(level, msg[, *args[, **kwargs]])

   Logs a message with level *level* on the root logger. The other arguments are
   interpreted as for :func:`debug`.


.. function:: disable(lvl)

   Provides an overriding level *lvl* for all loggers which takes precedence over
   the logger's own level. When the need arises to temporarily throttle logging
   output down across the whole application, this function can be useful.


.. function:: addLevelName(lvl, levelName)

   Associates level *lvl* with text *levelName* in an internal dictionary, which is
   used to map numeric levels to a textual representation, for example when a
   :class:`Formatter` formats a message. This function can also be used to define
   your own levels. The only constraints are that all levels used must be
   registered using this function, levels should be positive integers and they
   should increase in increasing order of severity.


.. function:: getLevelName(lvl)

   Returns the textual representation of logging level *lvl*. If the level is one
   of the predefined levels :const:`CRITICAL`, :const:`ERROR`, :const:`WARNING`,
   :const:`INFO` or :const:`DEBUG` then you get the corresponding string. If you
   have associated levels with names using :func:`addLevelName` then the name you
   have associated with *lvl* is returned. If a numeric value corresponding to one
   of the defined levels is passed in, the corresponding string representation is
   returned. Otherwise, the string "Level %s" % lvl is returned.


.. function:: makeLogRecord(attrdict)

   Creates and returns a new :class:`LogRecord` instance whose attributes are
   defined by *attrdict*. This function is useful for taking a pickled
   :class:`LogRecord` attribute dictionary, sent over a socket, and reconstituting
   it as a :class:`LogRecord` instance at the receiving end.


.. function:: basicConfig([**kwargs])

   Does basic configuration for the logging system by creating a
   :class:`StreamHandler` with a default :class:`Formatter` and adding it to the
   root logger. The functions :func:`debug`, :func:`info`, :func:`warning`,
   :func:`error` and :func:`critical` will call :func:`basicConfig` automatically
   if no handlers are defined for the root logger.

   The following keyword arguments are supported.

   +--------------+---------------------------------------------+
   | Format       | Description                                 |
   +==============+=============================================+
   | ``filename`` | Specifies that a FileHandler be created,    |
   |              | using the specified filename, rather than a |
   |              | StreamHandler.                              |
   +--------------+---------------------------------------------+
   | ``filemode`` | Specifies the mode to open the file, if     |
   |              | filename is specified (if filemode is       |
   |              | unspecified, it defaults to 'a').           |
   +--------------+---------------------------------------------+
   | ``format``   | Use the specified format string for the     |
   |              | handler.                                    |
   +--------------+---------------------------------------------+
   | ``datefmt``  | Use the specified date/time format.         |
   +--------------+---------------------------------------------+
   | ``level``    | Set the root logger level to the specified  |
   |              | level.                                      |
   +--------------+---------------------------------------------+
   | ``stream``   | Use the specified stream to initialize the  |
   |              | StreamHandler. Note that this argument is   |
   |              | incompatible with 'filename' - if both are  |
   |              | present, 'stream' is ignored.               |
   +--------------+---------------------------------------------+


.. function:: shutdown()

   Informs the logging system to perform an orderly shutdown by flushing and
   closing all handlers.


.. function:: setLoggerClass(klass)

   Tells the logging system to use the class *klass* when instantiating a logger.
   The class should define :meth:`__init__` such that only a name argument is
   required, and the :meth:`__init__` should call :meth:`Logger.__init__`. This
   function is typically called before any loggers are instantiated by applications
   which need to use custom logger behavior.


.. seealso::

   :pep:`282` - A Logging System
      The proposal which described this feature for inclusion in the Python standard
      library.

   `Original Python :mod:`logging` package <http://www.red-dove.com/python_logging.html>`_
      This is the original source for the :mod:`logging` package.  The version of the
      package available from this site is suitable for use with Python 1.5.2, 2.1.x
      and 2.2.x, which do not include the :mod:`logging` package in the standard
      library.


Logger Objects
--------------

Loggers have the following attributes and methods. Note that Loggers are never
instantiated directly, but always through the module-level function
``logging.getLogger(name)``.


.. attribute:: Logger.propagate

   If this evaluates to false, logging messages are not passed by this logger or by
   child loggers to higher level (ancestor) loggers. The constructor sets this
   attribute to 1.


.. method:: Logger.setLevel(lvl)

   Sets the threshold for this logger to *lvl*. Logging messages which are less
   severe than *lvl* will be ignored. When a logger is created, the level is set to
   :const:`NOTSET` (which causes all messages to be processed when the logger is
   the root logger, or delegation to the parent when the logger is a non-root
   logger). Note that the root logger is created with level :const:`WARNING`.

   The term "delegation to the parent" means that if a logger has a level of
   NOTSET, its chain of ancestor loggers is traversed until either an ancestor with
   a level other than NOTSET is found, or the root is reached.

   If an ancestor is found with a level other than NOTSET, then that ancestor's
   level is treated as the effective level of the logger where the ancestor search
   began, and is used to determine how a logging event is handled.

   If the root is reached, and it has a level of NOTSET, then all messages will be
   processed. Otherwise, the root's level will be used as the effective level.


.. method:: Logger.isEnabledFor(lvl)

   Indicates if a message of severity *lvl* would be processed by this logger.
   This method checks first the module-level level set by
   ``logging.disable(lvl)`` and then the logger's effective level as determined
   by :meth:`getEffectiveLevel`.


.. method:: Logger.getEffectiveLevel()

   Indicates the effective level for this logger. If a value other than
   :const:`NOTSET` has been set using :meth:`setLevel`, it is returned. Otherwise,
   the hierarchy is traversed towards the root until a value other than
   :const:`NOTSET` is found, and that value is returned.


.. method:: Logger.debug(msg[, *args[, **kwargs]])

   Logs a message with level :const:`DEBUG` on this logger. The *msg* is the
   message format string, and the *args* are the arguments which are merged into
   *msg* using the string formatting operator. (Note that this means that you can
   use keywords in the format string, together with a single dictionary argument.)

   There are two keyword arguments in *kwargs* which are inspected: *exc_info*
   which, if it does not evaluate as false, causes exception information to be
   added to the logging message. If an exception tuple (in the format returned by
   :func:`sys.exc_info`) is provided, it is used; otherwise, :func:`sys.exc_info`
   is called to get the exception information.

   The other optional keyword argument is *extra* which can be used to pass a
   dictionary which is used to populate the __dict__ of the LogRecord created for
   the logging event with user-defined attributes. These custom attributes can then
   be used as you like. For example, they could be incorporated into logged
   messages. For example::

      FORMAT = "%(asctime)-15s %(clientip)s %(user)-8s %(message)s"
      logging.basicConfig(format=FORMAT)
      dict = { 'clientip' : '192.168.0.1', 'user' : 'fbloggs' }
      logger = logging.getLogger("tcpserver")
      logger.warning("Protocol problem: %s", "connection reset", extra=d)

   would print something like  ::

      2006-02-08 22:20:02,165 192.168.0.1 fbloggs  Protocol problem: connection reset

   The keys in the dictionary passed in *extra* should not clash with the keys used
   by the logging system. (See the :class:`Formatter` documentation for more
   information on which keys are used by the logging system.)

   If you choose to use these attributes in logged messages, you need to exercise
   some care. In the above example, for instance, the :class:`Formatter` has been
   set up with a format string which expects 'clientip' and 'user' in the attribute
   dictionary of the LogRecord. If these are missing, the message will not be
   logged because a string formatting exception will occur. So in this case, you
   always need to pass the *extra* dictionary with these keys.

   While this might be annoying, this feature is intended for use in specialized
   circumstances, such as multi-threaded servers where the same code executes in
   many contexts, and interesting conditions which arise are dependent on this
   context (such as remote client IP address and authenticated user name, in the
   above example). In such circumstances, it is likely that specialized
   :class:`Formatter`\ s would be used with particular :class:`Handler`\ s.


.. method:: Logger.info(msg[, *args[, **kwargs]])

   Logs a message with level :const:`INFO` on this logger. The arguments are
   interpreted as for :meth:`debug`.


.. method:: Logger.warning(msg[, *args[, **kwargs]])

   Logs a message with level :const:`WARNING` on this logger. The arguments are
   interpreted as for :meth:`debug`.


.. method:: Logger.error(msg[, *args[, **kwargs]])

   Logs a message with level :const:`ERROR` on this logger. The arguments are
   interpreted as for :meth:`debug`.


.. method:: Logger.critical(msg[, *args[, **kwargs]])

   Logs a message with level :const:`CRITICAL` on this logger. The arguments are
   interpreted as for :meth:`debug`.


.. method:: Logger.log(lvl, msg[, *args[, **kwargs]])

   Logs a message with integer level *lvl* on this logger. The other arguments are
   interpreted as for :meth:`debug`.


.. method:: Logger.exception(msg[, *args])

   Logs a message with level :const:`ERROR` on this logger. The arguments are
   interpreted as for :meth:`debug`. Exception info is added to the logging
   message. This method should only be called from an exception handler.


.. method:: Logger.addFilter(filt)

   Adds the specified filter *filt* to this logger.


.. method:: Logger.removeFilter(filt)

   Removes the specified filter *filt* from this logger.


.. method:: Logger.filter(record)

   Applies this logger's filters to the record and returns a true value if the
   record is to be processed.


.. method:: Logger.addHandler(hdlr)

   Adds the specified handler *hdlr* to this logger.


.. method:: Logger.removeHandler(hdlr)

   Removes the specified handler *hdlr* from this logger.


.. method:: Logger.findCaller()

   Finds the caller's source filename and line number. Returns the filename, line
   number and function name as a 3-element tuple.


.. method:: Logger.handle(record)

   Handles a record by passing it to all handlers associated with this logger and
   its ancestors (until a false value of *propagate* is found). This method is used
   for unpickled records received from a socket, as well as those created locally.
   Logger-level filtering is applied using :meth:`filter`.


.. method:: Logger.makeRecord(name, lvl, fn, lno, msg, args, exc_info [, func, extra])

   This is a factory method which can be overridden in subclasses to create
   specialized :class:`LogRecord` instances.


.. _minimal-example:

Basic example
-------------

The :mod:`logging` package provides a lot of flexibility, and its configuration
can appear daunting.  This section demonstrates that simple use of the logging
package is possible.

The simplest example shows logging to the console::

   import logging

   logging.debug('A debug message')
   logging.info('Some information')
   logging.warning('A shot across the bows')

If you run the above script, you'll see this::

   WARNING:root:A shot across the bows

Because no particular logger was specified, the system used the root logger. The
debug and info messages didn't appear because by default, the root logger is
configured to only handle messages with a severity of WARNING or above. The
message format is also a configuration default, as is the output destination of
the messages - ``sys.stderr``. The severity level, the message format and
destination can be easily changed, as shown in the example below::

   import logging

   logging.basicConfig(level=logging.DEBUG,
                       format='%(asctime)s %(levelname)s %(message)s',
                       filename='/tmp/myapp.log',
                       filemode='w')
   logging.debug('A debug message')
   logging.info('Some information')
   logging.warning('A shot across the bows')

The :meth:`basicConfig` method is used to change the configuration defaults,
which results in output (written to ``/tmp/myapp.log``) which should look
something like the following::

   2004-07-02 13:00:08,743 DEBUG A debug message
   2004-07-02 13:00:08,743 INFO Some information
   2004-07-02 13:00:08,743 WARNING A shot across the bows

This time, all messages with a severity of DEBUG or above were handled, and the
format of the messages was also changed, and output went to the specified file
rather than the console.

.. XXX logging should probably be updated for new string formatting!

Formatting uses the old Python string formatting - see section
:ref:`old-string-formatting`. The format string takes the following common
specifiers. For a complete list of specifiers, consult the :class:`Formatter`
documentation.

+-------------------+-----------------------------------------------+
| Format            | Description                                   |
+===================+===============================================+
| ``%(name)s``      | Name of the logger (logging channel).         |
+-------------------+-----------------------------------------------+
| ``%(levelname)s`` | Text logging level for the message            |
|                   | (``'DEBUG'``, ``'INFO'``, ``'WARNING'``,      |
|                   | ``'ERROR'``, ``'CRITICAL'``).                 |
+-------------------+-----------------------------------------------+
| ``%(asctime)s``   | Human-readable time when the                  |
|                   | :class:`LogRecord` was created.  By default   |
|                   | this is of the form "2003-07-08 16:49:45,896" |
|                   | (the numbers after the comma are millisecond  |
|                   | portion of the time).                         |
+-------------------+-----------------------------------------------+
| ``%(message)s``   | The logged message.                           |
+-------------------+-----------------------------------------------+

To change the date/time format, you can pass an additional keyword parameter,
*datefmt*, as in the following::

   import logging

   logging.basicConfig(level=logging.DEBUG,
                       format='%(asctime)s %(levelname)-8s %(message)s',
                       datefmt='%a, %d %b %Y %H:%M:%S',
                       filename='/temp/myapp.log',
                       filemode='w')
   logging.debug('A debug message')
   logging.info('Some information')
   logging.warning('A shot across the bows')

which would result in output like ::

   Fri, 02 Jul 2004 13:06:18 DEBUG    A debug message
   Fri, 02 Jul 2004 13:06:18 INFO     Some information
   Fri, 02 Jul 2004 13:06:18 WARNING  A shot across the bows

The date format string follows the requirements of :func:`strftime` - see the
documentation for the :mod:`time` module.

If, instead of sending logging output to the console or a file, you'd rather use
a file-like object which you have created separately, you can pass it to
:func:`basicConfig` using the *stream* keyword argument. Note that if both
*stream* and *filename* keyword arguments are passed, the *stream* argument is
ignored.

Of course, you can put variable information in your output. To do this, simply
have the message be a format string and pass in additional arguments containing
the variable information, as in the following example::

   import logging

   logging.basicConfig(level=logging.DEBUG,
                       format='%(asctime)s %(levelname)-8s %(message)s',
                       datefmt='%a, %d %b %Y %H:%M:%S',
                       filename='/temp/myapp.log',
                       filemode='w')
   logging.error('Pack my box with %d dozen %s', 5, 'liquor jugs')

which would result in ::

   Wed, 21 Jul 2004 15:35:16 ERROR    Pack my box with 5 dozen liquor jugs


.. _multiple-destinations:

Logging to multiple destinations
--------------------------------

Let's say you want to log to console and file with different message formats and
in differing circumstances. Say you want to log messages with levels of DEBUG
and higher to file, and those messages at level INFO and higher to the console.
Let's also assume that the file should contain timestamps, but the console
messages should not. Here's how you can achieve this::

   import logging

   # set up logging to file - see previous section for more details
   logging.basicConfig(level=logging.DEBUG,
                       format='%(asctime)s %(name)-12s %(levelname)-8s %(message)s',
                       datefmt='%m-%d %H:%M',
                       filename='/temp/myapp.log',
                       filemode='w')
   # define a Handler which writes INFO messages or higher to the sys.stderr
   console = logging.StreamHandler()
   console.setLevel(logging.INFO)
   # set a format which is simpler for console use
   formatter = logging.Formatter('%(name)-12s: %(levelname)-8s %(message)s')
   # tell the handler to use this format
   console.setFormatter(formatter)
   # add the handler to the root logger
   logging.getLogger('').addHandler(console)

   # Now, we can log to the root logger, or any other logger. First the root...
   logging.info('Jackdaws love my big sphinx of quartz.')

   # Now, define a couple of other loggers which might represent areas in your
   # application:

   logger1 = logging.getLogger('myapp.area1')
   logger2 = logging.getLogger('myapp.area2')

   logger1.debug('Quick zephyrs blow, vexing daft Jim.')
   logger1.info('How quickly daft jumping zebras vex.')
   logger2.warning('Jail zesty vixen who grabbed pay from quack.')
   logger2.error('The five boxing wizards jump quickly.')

When you run this, on the console you will see ::

   root        : INFO     Jackdaws love my big sphinx of quartz.
   myapp.area1 : INFO     How quickly daft jumping zebras vex.
   myapp.area2 : WARNING  Jail zesty vixen who grabbed pay from quack.
   myapp.area2 : ERROR    The five boxing wizards jump quickly.

and in the file you will see something like ::

   10-22 22:19 root         INFO     Jackdaws love my big sphinx of quartz.
   10-22 22:19 myapp.area1  DEBUG    Quick zephyrs blow, vexing daft Jim.
   10-22 22:19 myapp.area1  INFO     How quickly daft jumping zebras vex.
   10-22 22:19 myapp.area2  WARNING  Jail zesty vixen who grabbed pay from quack.
   10-22 22:19 myapp.area2  ERROR    The five boxing wizards jump quickly.

As you can see, the DEBUG message only shows up in the file. The other messages
are sent to both destinations.

This example uses console and file handlers, but you can use any number and
combination of handlers you choose.


.. _network-logging:

Sending and receiving logging events across a network
-----------------------------------------------------

Let's say you want to send logging events across a network, and handle them at
the receiving end. A simple way of doing this is attaching a
:class:`SocketHandler` instance to the root logger at the sending end::

   import logging, logging.handlers

   rootLogger = logging.getLogger('')
   rootLogger.setLevel(logging.DEBUG)
   socketHandler = logging.handlers.SocketHandler('localhost',
                       logging.handlers.DEFAULT_TCP_LOGGING_PORT)
   # don't bother with a formatter, since a socket handler sends the event as
   # an unformatted pickle
   rootLogger.addHandler(socketHandler)

   # Now, we can log to the root logger, or any other logger. First the root...
   logging.info('Jackdaws love my big sphinx of quartz.')

   # Now, define a couple of other loggers which might represent areas in your
   # application:

   logger1 = logging.getLogger('myapp.area1')
   logger2 = logging.getLogger('myapp.area2')

   logger1.debug('Quick zephyrs blow, vexing daft Jim.')
   logger1.info('How quickly daft jumping zebras vex.')
   logger2.warning('Jail zesty vixen who grabbed pay from quack.')
   logger2.error('The five boxing wizards jump quickly.')

At the receiving end, you can set up a receiver using the :mod:`SocketServer`
module. Here is a basic working example::

   import cPickle
   import logging
   import logging.handlers
   import SocketServer
   import struct


   class LogRecordStreamHandler(SocketServer.StreamRequestHandler):
       """Handler for a streaming logging request.

       This basically logs the record using whatever logging policy is
       configured locally.
       """

       def handle(self):
           """
           Handle multiple requests - each expected to be a 4-byte length,
           followed by the LogRecord in pickle format. Logs the record
           according to whatever policy is configured locally.
           """
           while 1:
               chunk = self.connection.recv(4)
               if len(chunk) < 4:
                   break
               slen = struct.unpack(">L", chunk)[0]
               chunk = self.connection.recv(slen)
               while len(chunk) < slen:
                   chunk = chunk + self.connection.recv(slen - len(chunk))
               obj = self.unPickle(chunk)
               record = logging.makeLogRecord(obj)
               self.handleLogRecord(record)

       def unPickle(self, data):
           return cPickle.loads(data)

       def handleLogRecord(self, record):
           # if a name is specified, we use the named logger rather than the one
           # implied by the record.
           if self.server.logname is not None:
               name = self.server.logname
           else:
               name = record.name
           logger = logging.getLogger(name)
           # N.B. EVERY record gets logged. This is because Logger.handle
           # is normally called AFTER logger-level filtering. If you want
           # to do filtering, do it at the client end to save wasting
           # cycles and network bandwidth!
           logger.handle(record)

   class LogRecordSocketReceiver(SocketServer.ThreadingTCPServer):
       """simple TCP socket-based logging receiver suitable for testing.
       """

       allow_reuse_address = 1

       def __init__(self, host='localhost',
                    port=logging.handlers.DEFAULT_TCP_LOGGING_PORT,
                    handler=LogRecordStreamHandler):
           SocketServer.ThreadingTCPServer.__init__(self, (host, port), handler)
           self.abort = 0
           self.timeout = 1
           self.logname = None

       def serve_until_stopped(self):
           import select
           abort = 0
           while not abort:
               rd, wr, ex = select.select([self.socket.fileno()],
                                          [], [],
                                          self.timeout)
               if rd:
                   self.handle_request()
               abort = self.abort

   def main():
       logging.basicConfig(
           format="%(relativeCreated)5d %(name)-15s %(levelname)-8s %(message)s")
       tcpserver = LogRecordSocketReceiver()
       print("About to start TCP server...")
       tcpserver.serve_until_stopped()

   if __name__ == "__main__":
       main()

First run the server, and then the client. On the client side, nothing is
printed on the console; on the server side, you should see something like::

   About to start TCP server...
      59 root            INFO     Jackdaws love my big sphinx of quartz.
      59 myapp.area1     DEBUG    Quick zephyrs blow, vexing daft Jim.
      69 myapp.area1     INFO     How quickly daft jumping zebras vex.
      69 myapp.area2     WARNING  Jail zesty vixen who grabbed pay from quack.
      69 myapp.area2     ERROR    The five boxing wizards jump quickly.


Handler Objects
---------------

Handlers have the following attributes and methods. Note that :class:`Handler`
is never instantiated directly; this class acts as a base for more useful
subclasses. However, the :meth:`__init__` method in subclasses needs to call
:meth:`Handler.__init__`.


.. method:: Handler.__init__(level=NOTSET)

   Initializes the :class:`Handler` instance by setting its level, setting the list
   of filters to the empty list and creating a lock (using :meth:`createLock`) for
   serializing access to an I/O mechanism.


.. method:: Handler.createLock()

   Initializes a thread lock which can be used to serialize access to underlying
   I/O functionality which may not be threadsafe.


.. method:: Handler.acquire()

   Acquires the thread lock created with :meth:`createLock`.


.. method:: Handler.release()

   Releases the thread lock acquired with :meth:`acquire`.


.. method:: Handler.setLevel(lvl)

   Sets the threshold for this handler to *lvl*. Logging messages which are less
   severe than *lvl* will be ignored. When a handler is created, the level is set
   to :const:`NOTSET` (which causes all messages to be processed).


.. method:: Handler.setFormatter(form)

   Sets the :class:`Formatter` for this handler to *form*.


.. method:: Handler.addFilter(filt)

   Adds the specified filter *filt* to this handler.


.. method:: Handler.removeFilter(filt)

   Removes the specified filter *filt* from this handler.


.. method:: Handler.filter(record)

   Applies this handler's filters to the record and returns a true value if the
   record is to be processed.


.. method:: Handler.flush()

   Ensure all logging output has been flushed. This version does nothing and is
   intended to be implemented by subclasses.


.. method:: Handler.close()

   Tidy up any resources used by the handler. This version does nothing and is
   intended to be implemented by subclasses.


.. method:: Handler.handle(record)

   Conditionally emits the specified logging record, depending on filters which may
   have been added to the handler. Wraps the actual emission of the record with
   acquisition/release of the I/O thread lock.


.. method:: Handler.handleError(record)

   This method should be called from handlers when an exception is encountered
   during an :meth:`emit` call. By default it does nothing, which means that
   exceptions get silently ignored. This is what is mostly wanted for a logging
   system - most users will not care about errors in the logging system, they are
   more interested in application errors. You could, however, replace this with a
   custom handler if you wish. The specified record is the one which was being
   processed when the exception occurred.


.. method:: Handler.format(record)

   Do formatting for a record - if a formatter is set, use it. Otherwise, use the
   default formatter for the module.


.. method:: Handler.emit(record)

   Do whatever it takes to actually log the specified logging record. This version
   is intended to be implemented by subclasses and so raises a
   :exc:`NotImplementedError`.


StreamHandler
^^^^^^^^^^^^^

The :class:`StreamHandler` class, located in the core :mod:`logging` package,
sends logging output to streams such as *sys.stdout*, *sys.stderr* or any
file-like object (or, more precisely, any object which supports :meth:`write`
and :meth:`flush` methods).


.. class:: StreamHandler([strm])

   Returns a new instance of the :class:`StreamHandler` class. If *strm* is
   specified, the instance will use it for logging output; otherwise, *sys.stderr*
   will be used.


.. method:: StreamHandler.emit(record)

   If a formatter is specified, it is used to format the record. The record is then
   written to the stream with a trailing newline. If exception information is
   present, it is formatted using :func:`traceback.print_exception` and appended to
   the stream.


.. method:: StreamHandler.flush()

   Flushes the stream by calling its :meth:`flush` method. Note that the
   :meth:`close` method is inherited from :class:`Handler` and so does nothing, so
   an explicit :meth:`flush` call may be needed at times.


FileHandler
^^^^^^^^^^^

The :class:`FileHandler` class, located in the core :mod:`logging` package,
sends logging output to a disk file.  It inherits the output functionality from
:class:`StreamHandler`.


.. class:: FileHandler(filename[, mode[, encoding]])

   Returns a new instance of the :class:`FileHandler` class. The specified file is
   opened and used as the stream for logging. If *mode* is not specified,
   :const:`'a'` is used.  If *encoding* is not *None*, it is used to open the file
   with that encoding.  By default, the file grows indefinitely.


.. method:: FileHandler.close()

   Closes the file.


.. method:: FileHandler.emit(record)

   Outputs the record to the file.


WatchedFileHandler
^^^^^^^^^^^^^^^^^^

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


.. class:: WatchedFileHandler(filename[,mode[, encoding]])

   Returns a new instance of the :class:`WatchedFileHandler` class. The specified
   file is opened and used as the stream for logging. If *mode* is not specified,
   :const:`'a'` is used.  If *encoding* is not *None*, it is used to open the file
   with that encoding.  By default, the file grows indefinitely.


.. method:: WatchedFileHandler.emit(record)

   Outputs the record to the file, but first checks to see if the file has changed.
   If it has, the existing stream is flushed and closed and the file opened again,
   before outputting the record to the file.


RotatingFileHandler
^^^^^^^^^^^^^^^^^^^

The :class:`RotatingFileHandler` class, located in the :mod:`logging.handlers`
module, supports rotation of disk log files.


.. class:: RotatingFileHandler(filename[, mode[, maxBytes[, backupCount]]])

   Returns a new instance of the :class:`RotatingFileHandler` class. The specified
   file is opened and used as the stream for logging. If *mode* is not specified,
   ``'a'`` is used. By default, the file grows indefinitely.

   You can use the *maxBytes* and *backupCount* values to allow the file to
   :dfn:`rollover` at a predetermined size. When the size is about to be exceeded,
   the file is closed and a new file is silently opened for output. Rollover occurs
   whenever the current log file is nearly *maxBytes* in length; if *maxBytes* is
   zero, rollover never occurs.  If *backupCount* is non-zero, the system will save
   old log files by appending the extensions ".1", ".2" etc., to the filename. For
   example, with a *backupCount* of 5 and a base file name of :file:`app.log`, you
   would get :file:`app.log`, :file:`app.log.1`, :file:`app.log.2`, up to
   :file:`app.log.5`. The file being written to is always :file:`app.log`.  When
   this file is filled, it is closed and renamed to :file:`app.log.1`, and if files
   :file:`app.log.1`, :file:`app.log.2`, etc.  exist, then they are renamed to
   :file:`app.log.2`, :file:`app.log.3` etc.  respectively.


.. method:: RotatingFileHandler.doRollover()

   Does a rollover, as described above.


.. method:: RotatingFileHandler.emit(record)

   Outputs the record to the file, catering for rollover as described previously.


TimedRotatingFileHandler
^^^^^^^^^^^^^^^^^^^^^^^^

The :class:`TimedRotatingFileHandler` class, located in the
:mod:`logging.handlers` module, supports rotation of disk log files at certain
timed intervals.


.. class:: TimedRotatingFileHandler(filename [,when [,interval [,backupCount]]])

   Returns a new instance of the :class:`TimedRotatingFileHandler` class. The
   specified file is opened and used as the stream for logging. On rotating it also
   sets the filename suffix. Rotating happens based on the product of *when* and
   *interval*.

   You can use the *when* to specify the type of *interval*. The list of possible
   values is, note that they are not case sensitive:

   +----------+-----------------------+
   | Value    | Type of interval      |
   +==========+=======================+
   | S        | Seconds               |
   +----------+-----------------------+
   | M        | Minutes               |
   +----------+-----------------------+
   | H        | Hours                 |
   +----------+-----------------------+
   | D        | Days                  |
   +----------+-----------------------+
   | W        | Week day (0=Monday)   |
   +----------+-----------------------+
   | midnight | Roll over at midnight |
   +----------+-----------------------+

   If *backupCount* is non-zero, the system will save old log files by appending
   extensions to the filename. The extensions are date-and-time based, using the
   strftime format ``%Y-%m-%d_%H-%M-%S`` or a leading portion thereof, depending on
   the rollover interval. At most *backupCount* files will be kept, and if more
   would be created when rollover occurs, the oldest one is deleted.


.. method:: TimedRotatingFileHandler.doRollover()

   Does a rollover, as described above.


.. method:: TimedRotatingFileHandler.emit(record)

   Outputs the record to the file, catering for rollover as described above.


SocketHandler
^^^^^^^^^^^^^

The :class:`SocketHandler` class, located in the :mod:`logging.handlers` module,
sends logging output to a network socket. The base class uses a TCP socket.


.. class:: SocketHandler(host, port)

   Returns a new instance of the :class:`SocketHandler` class intended to
   communicate with a remote machine whose address is given by *host* and *port*.


.. method:: SocketHandler.close()

   Closes the socket.


.. method:: SocketHandler.emit()

   Pickles the record's attribute dictionary and writes it to the socket in binary
   format. If there is an error with the socket, silently drops the packet. If the
   connection was previously lost, re-establishes the connection. To unpickle the
   record at the receiving end into a :class:`LogRecord`, use the
   :func:`makeLogRecord` function.


.. method:: SocketHandler.handleError()

   Handles an error which has occurred during :meth:`emit`. The most likely cause
   is a lost connection. Closes the socket so that we can retry on the next event.


.. method:: SocketHandler.makeSocket()

   This is a factory method which allows subclasses to define the precise type of
   socket they want. The default implementation creates a TCP socket
   (:const:`socket.SOCK_STREAM`).


.. method:: SocketHandler.makePickle(record)

   Pickles the record's attribute dictionary in binary format with a length prefix,
   and returns it ready for transmission across the socket.


.. method:: SocketHandler.send(packet)

   Send a pickled string *packet* to the socket. This function allows for partial
   sends which can happen when the network is busy.


DatagramHandler
^^^^^^^^^^^^^^^

The :class:`DatagramHandler` class, located in the :mod:`logging.handlers`
module, inherits from :class:`SocketHandler` to support sending logging messages
over UDP sockets.


.. class:: DatagramHandler(host, port)

   Returns a new instance of the :class:`DatagramHandler` class intended to
   communicate with a remote machine whose address is given by *host* and *port*.


.. method:: DatagramHandler.emit()

   Pickles the record's attribute dictionary and writes it to the socket in binary
   format. If there is an error with the socket, silently drops the packet. To
   unpickle the record at the receiving end into a :class:`LogRecord`, use the
   :func:`makeLogRecord` function.


.. method:: DatagramHandler.makeSocket()

   The factory method of :class:`SocketHandler` is here overridden to create a UDP
   socket (:const:`socket.SOCK_DGRAM`).


.. method:: DatagramHandler.send(s)

   Send a pickled string to a socket.


SysLogHandler
^^^^^^^^^^^^^

The :class:`SysLogHandler` class, located in the :mod:`logging.handlers` module,
supports sending logging messages to a remote or local Unix syslog.


.. class:: SysLogHandler([address[, facility]])

   Returns a new instance of the :class:`SysLogHandler` class intended to
   communicate with a remote Unix machine whose address is given by *address* in
   the form of a ``(host, port)`` tuple.  If *address* is not specified,
   ``('localhost', 514)`` is used.  The address is used to open a UDP socket.  An
   alternative to providing a ``(host, port)`` tuple is providing an address as a
   string, for example "/dev/log". In this case, a Unix domain socket is used to
   send the message to the syslog. If *facility* is not specified,
   :const:`LOG_USER` is used.


.. method:: SysLogHandler.close()

   Closes the socket to the remote host.


.. method:: SysLogHandler.emit(record)

   The record is formatted, and then sent to the syslog server. If exception
   information is present, it is *not* sent to the server.


.. method:: SysLogHandler.encodePriority(facility, priority)

   Encodes the facility and priority into an integer. You can pass in strings or
   integers - if strings are passed, internal mapping dictionaries are used to
   convert them to integers.


NTEventLogHandler
^^^^^^^^^^^^^^^^^

The :class:`NTEventLogHandler` class, located in the :mod:`logging.handlers`
module, supports sending logging messages to a local Windows NT, Windows 2000 or
Windows XP event log. Before you can use it, you need Mark Hammond's Win32
extensions for Python installed.


.. class:: NTEventLogHandler(appname[, dllname[, logtype]])

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


.. method:: NTEventLogHandler.close()

   At this point, you can remove the application name from the registry as a source
   of event log entries. However, if you do this, you will not be able to see the
   events as you intended in the Event Log Viewer - it needs to be able to access
   the registry to get the .dll name. The current version does not do this (in fact
   it doesn't do anything).


.. method:: NTEventLogHandler.emit(record)

   Determines the message ID, event category and event type, and then logs the
   message in the NT event log.


.. method:: NTEventLogHandler.getEventCategory(record)

   Returns the event category for the record. Override this if you want to specify
   your own categories. This version returns 0.


.. method:: NTEventLogHandler.getEventType(record)

   Returns the event type for the record. Override this if you want to specify your
   own types. This version does a mapping using the handler's typemap attribute,
   which is set up in :meth:`__init__` to a dictionary which contains mappings for
   :const:`DEBUG`, :const:`INFO`, :const:`WARNING`, :const:`ERROR` and
   :const:`CRITICAL`. If you are using your own levels, you will either need to
   override this method or place a suitable dictionary in the handler's *typemap*
   attribute.


.. method:: NTEventLogHandler.getMessageID(record)

   Returns the message ID for the record. If you are using your own messages, you
   could do this by having the *msg* passed to the logger being an ID rather than a
   format string. Then, in here, you could use a dictionary lookup to get the
   message ID. This version returns 1, which is the base message ID in
   :file:`win32service.pyd`.


SMTPHandler
^^^^^^^^^^^

The :class:`SMTPHandler` class, located in the :mod:`logging.handlers` module,
supports sending logging messages to an email address via SMTP.


.. class:: SMTPHandler(mailhost, fromaddr, toaddrs, subject[, credentials])

   Returns a new instance of the :class:`SMTPHandler` class. The instance is
   initialized with the from and to addresses and subject line of the email. The
   *toaddrs* should be a list of strings. To specify a non-standard SMTP port, use
   the (host, port) tuple format for the *mailhost* argument. If you use a string,
   the standard SMTP port is used. If your SMTP server requires authentication, you
   can specify a (username, password) tuple for the *credentials* argument.


.. method:: SMTPHandler.emit(record)

   Formats the record and sends it to the specified addressees.


.. method:: SMTPHandler.getSubject(record)

   If you want to specify a subject line which is record-dependent, override this
   method.


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


.. method:: BufferingHandler.emit(record)

   Appends the record to the buffer. If :meth:`shouldFlush` returns true, calls
   :meth:`flush` to process the buffer.


.. method:: BufferingHandler.flush()

   You can override this to implement custom flushing behavior. This version just
   zaps the buffer to empty.


.. method:: BufferingHandler.shouldFlush(record)

   Returns true if the buffer is up to capacity. This method can be overridden to
   implement custom flushing strategies.


.. class:: MemoryHandler(capacity[, flushLevel [, target]])

   Returns a new instance of the :class:`MemoryHandler` class. The instance is
   initialized with a buffer size of *capacity*. If *flushLevel* is not specified,
   :const:`ERROR` is used. If no *target* is specified, the target will need to be
   set using :meth:`setTarget` before this handler does anything useful.


.. method:: MemoryHandler.close()

   Calls :meth:`flush`, sets the target to :const:`None` and clears the buffer.


.. method:: MemoryHandler.flush()

   For a :class:`MemoryHandler`, flushing means just sending the buffered records
   to the target, if there is one. Override if you want different behavior.


.. method:: MemoryHandler.setTarget(target)

   Sets the target handler for this handler.


.. method:: MemoryHandler.shouldFlush(record)

   Checks for buffer full or a record at the *flushLevel* or higher.


HTTPHandler
^^^^^^^^^^^

The :class:`HTTPHandler` class, located in the :mod:`logging.handlers` module,
supports sending logging messages to a Web server, using either ``GET`` or
``POST`` semantics.


.. class:: HTTPHandler(host, url[, method])

   Returns a new instance of the :class:`HTTPHandler` class. The instance is
   initialized with a host address, url and HTTP method. The *host* can be of the
   form ``host:port``, should you need to use a specific port number. If no
   *method* is specified, ``GET`` is used.


.. method:: HTTPHandler.emit(record)

   Sends the record to the Web server as an URL-encoded dictionary.


Formatter Objects
-----------------

:class:`Formatter`\ s have the following attributes and methods. They are
responsible for converting a :class:`LogRecord` to (usually) a string which can
be interpreted by either a human or an external system. The base
:class:`Formatter` allows a formatting string to be specified. If none is
supplied, the default value of ``'%(message)s'`` is used.

A Formatter can be initialized with a format string which makes use of knowledge
of the :class:`LogRecord` attributes - such as the default value mentioned above
making use of the fact that the user's message and arguments are pre-formatted
into a :class:`LogRecord`'s *message* attribute.  This format string contains
standard python %-style mapping keys. See section :ref:`old-string-formatting`
for more information on string formatting.

Currently, the useful mapping keys in a :class:`LogRecord` are:

+-------------------------+-----------------------------------------------+
| Format                  | Description                                   |
+=========================+===============================================+
| ``%(name)s``            | Name of the logger (logging channel).         |
+-------------------------+-----------------------------------------------+
| ``%(levelno)s``         | Numeric logging level for the message         |
|                         | (:const:`DEBUG`, :const:`INFO`,               |
|                         | :const:`WARNING`, :const:`ERROR`,             |
|                         | :const:`CRITICAL`).                           |
+-------------------------+-----------------------------------------------+
| ``%(levelname)s``       | Text logging level for the message            |
|                         | (``'DEBUG'``, ``'INFO'``, ``'WARNING'``,      |
|                         | ``'ERROR'``, ``'CRITICAL'``).                 |
+-------------------------+-----------------------------------------------+
| ``%(pathname)s``        | Full pathname of the source file where the    |
|                         | logging call was issued (if available).       |
+-------------------------+-----------------------------------------------+
| ``%(filename)s``        | Filename portion of pathname.                 |
+-------------------------+-----------------------------------------------+
| ``%(module)s``          | Module (name portion of filename).            |
+-------------------------+-----------------------------------------------+
| ``%(funcName)s``        | Name of function containing the logging call. |
+-------------------------+-----------------------------------------------+
| ``%(lineno)d``          | Source line number where the logging call was |
|                         | issued (if available).                        |
+-------------------------+-----------------------------------------------+
| ``%(created)f``         | Time when the :class:`LogRecord` was created  |
|                         | (as returned by :func:`time.time`).           |
+-------------------------+-----------------------------------------------+
| ``%(relativeCreated)d`` | Time in milliseconds when the LogRecord was   |
|                         | created, relative to the time the logging     |
|                         | module was loaded.                            |
+-------------------------+-----------------------------------------------+
| ``%(asctime)s``         | Human-readable time when the                  |
|                         | :class:`LogRecord` was created.  By default   |
|                         | this is of the form "2003-07-08 16:49:45,896" |
|                         | (the numbers after the comma are millisecond  |
|                         | portion of the time).                         |
+-------------------------+-----------------------------------------------+
| ``%(msecs)d``           | Millisecond portion of the time when the      |
|                         | :class:`LogRecord` was created.               |
+-------------------------+-----------------------------------------------+
| ``%(thread)d``          | Thread ID (if available).                     |
+-------------------------+-----------------------------------------------+
| ``%(threadName)s``      | Thread name (if available).                   |
+-------------------------+-----------------------------------------------+
| ``%(process)d``         | Process ID (if available).                    |
+-------------------------+-----------------------------------------------+
| ``%(message)s``         | The logged message, computed as ``msg %       |
|                         | args``.                                       |
+-------------------------+-----------------------------------------------+


.. class:: Formatter([fmt[, datefmt]])

   Returns a new instance of the :class:`Formatter` class. The instance is
   initialized with a format string for the message as a whole, as well as a format
   string for the date/time portion of a message. If no *fmt* is specified,
   ``'%(message)s'`` is used. If no *datefmt* is specified, the ISO8601 date format
   is used.


.. method:: Formatter.format(record)

   The record's attribute dictionary is used as the operand to a string formatting
   operation. Returns the resulting string. Before formatting the dictionary, a
   couple of preparatory steps are carried out. The *message* attribute of the
   record is computed using *msg* % *args*. If the formatting string contains
   ``'(asctime)'``, :meth:`formatTime` is called to format the event time. If there
   is exception information, it is formatted using :meth:`formatException` and
   appended to the message.


.. method:: Formatter.formatTime(record[, datefmt])

   This method should be called from :meth:`format` by a formatter which wants to
   make use of a formatted time. This method can be overridden in formatters to
   provide for any specific requirement, but the basic behavior is as follows: if
   *datefmt* (a string) is specified, it is used with :func:`time.strftime` to
   format the creation time of the record. Otherwise, the ISO8601 format is used.
   The resulting string is returned.


.. method:: Formatter.formatException(exc_info)

   Formats the specified exception information (a standard exception tuple as
   returned by :func:`sys.exc_info`) as a string. This default implementation just
   uses :func:`traceback.print_exception`. The resulting string is returned.


Filter Objects
--------------

:class:`Filter`\ s can be used by :class:`Handler`\ s and :class:`Logger`\ s for
more sophisticated filtering than is provided by levels. The base filter class
only allows events which are below a certain point in the logger hierarchy. For
example, a filter initialized with "A.B" will allow events logged by loggers
"A.B", "A.B.C", "A.B.C.D", "A.B.D" etc. but not "A.BB", "B.A.B" etc. If
initialized with the empty string, all events are passed.


.. class:: Filter([name])

   Returns an instance of the :class:`Filter` class. If *name* is specified, it
   names a logger which, together with its children, will have its events allowed
   through the filter. If no name is specified, allows every event.


.. method:: Filter.filter(record)

   Is the specified record to be logged? Returns zero for no, nonzero for yes. If
   deemed appropriate, the record may be modified in-place by this method.


LogRecord Objects
-----------------

:class:`LogRecord` instances are created every time something is logged. They
contain all the information pertinent to the event being logged. The main
information passed in is in msg and args, which are combined using msg % args to
create the message field of the record. The record also includes information
such as when the record was created, the source line where the logging call was
made, and any exception information to be logged.


.. class:: LogRecord(name, lvl, pathname, lineno, msg, args, exc_info [, func])

   Returns an instance of :class:`LogRecord` initialized with interesting
   information. The *name* is the logger name; *lvl* is the numeric level;
   *pathname* is the absolute pathname of the source file in which the logging
   call was made; *lineno* is the line number in that file where the logging
   call is found; *msg* is the user-supplied message (a format string); *args*
   is the tuple which, together with *msg*, makes up the user message; and
   *exc_info* is the exception tuple obtained by calling :func:`sys.exc_info`
   (or :const:`None`, if no exception information is available). The *func* is
   the name of the function from which the logging call was made. If not
   specified, it defaults to ``None``.


.. method:: LogRecord.getMessage()

   Returns the message for this :class:`LogRecord` instance after merging any
   user-supplied arguments with the message.


Thread Safety
-------------

The logging module is intended to be thread-safe without any special work
needing to be done by its clients. It achieves this though using threading
locks; there is one lock to serialize access to the module's shared data, and
each handler also creates a lock to serialize access to its underlying I/O.


Configuration
-------------


.. _logging-config-api:

Configuration functions
^^^^^^^^^^^^^^^^^^^^^^^

.. % 

The following functions configure the logging module. They are located in the
:mod:`logging.config` module.  Their use is optional --- you can configure the
logging module using these functions or by making calls to the main API (defined
in :mod:`logging` itself) and defining handlers which are declared either in
:mod:`logging` or :mod:`logging.handlers`.


.. function:: fileConfig(fname[, defaults])

   Reads the logging configuration from a ConfigParser-format file named *fname*.
   This function can be called several times from an application, allowing an end
   user the ability to select from various pre-canned configurations (if the
   developer provides a mechanism to present the choices and load the chosen
   configuration). Defaults to be passed to ConfigParser can be specified in the
   *defaults* argument.


.. function:: listen([port])

   Starts up a socket server on the specified port, and listens for new
   configurations. If no port is specified, the module's default
   :const:`DEFAULT_LOGGING_CONFIG_PORT` is used. Logging configurations will be
   sent as a file suitable for processing by :func:`fileConfig`. Returns a
   :class:`Thread` instance on which you can call :meth:`start` to start the
   server, and which you can :meth:`join` when appropriate. To stop the server,
   call :func:`stopListening`. To send a configuration to the socket, read in the
   configuration file and send it to the socket as a string of bytes preceded by a
   four-byte length packed in binary using struct.\ ``pack('>L', n)``.


.. function:: stopListening()

   Stops the listening server which was created with a call to :func:`listen`. This
   is typically called before calling :meth:`join` on the return value from
   :func:`listen`.


.. _logging-config-fileformat:

Configuration file format
^^^^^^^^^^^^^^^^^^^^^^^^^

.. % 

The configuration file format understood by :func:`fileConfig` is based on
ConfigParser functionality. The file must contain sections called ``[loggers]``,
``[handlers]`` and ``[formatters]`` which identify by name the entities of each
type which are defined in the file. For each such entity, there is a separate
section which identified how that entity is configured. Thus, for a logger named
``log01`` in the ``[loggers]`` section, the relevant configuration details are
held in a section ``[logger_log01]``. Similarly, a handler called ``hand01`` in
the ``[handlers]`` section will have its configuration held in a section called
``[handler_hand01]``, while a formatter called ``form01`` in the
``[formatters]`` section will have its configuration specified in a section
called ``[formatter_form01]``. The root logger configuration must be specified
in a section called ``[logger_root]``.

Examples of these sections in the file are given below. ::

   [loggers]
   keys=root,log02,log03,log04,log05,log06,log07

   [handlers]
   keys=hand01,hand02,hand03,hand04,hand05,hand06,hand07,hand08,hand09

   [formatters]
   keys=form01,form02,form03,form04,form05,form06,form07,form08,form09

The root logger must specify a level and a list of handlers. An example of a
root logger section is given below. ::

   [logger_root]
   level=NOTSET
   handlers=hand01

The ``level`` entry can be one of ``DEBUG, INFO, WARNING, ERROR, CRITICAL`` or
``NOTSET``. For the root logger only, ``NOTSET`` means that all messages will be
logged. Level values are :func:`eval`\ uated in the context of the ``logging``
package's namespace.

The ``handlers`` entry is a comma-separated list of handler names, which must
appear in the ``[handlers]`` section. These names must appear in the
``[handlers]`` section and have corresponding sections in the configuration
file.

For loggers other than the root logger, some additional information is required.
This is illustrated by the following example. ::

   [logger_parser]
   level=DEBUG
   handlers=hand01
   propagate=1
   qualname=compiler.parser

The ``level`` and ``handlers`` entries are interpreted as for the root logger,
except that if a non-root logger's level is specified as ``NOTSET``, the system
consults loggers higher up the hierarchy to determine the effective level of the
logger. The ``propagate`` entry is set to 1 to indicate that messages must
propagate to handlers higher up the logger hierarchy from this logger, or 0 to
indicate that messages are **not** propagated to handlers up the hierarchy. The
``qualname`` entry is the hierarchical channel name of the logger, that is to
say the name used by the application to get the logger.

Sections which specify handler configuration are exemplified by the following.
::

   [handler_hand01]
   class=StreamHandler
   level=NOTSET
   formatter=form01
   args=(sys.stdout,)

The ``class`` entry indicates the handler's class (as determined by :func:`eval`
in the ``logging`` package's namespace). The ``level`` is interpreted as for
loggers, and ``NOTSET`` is taken to mean "log everything".

The ``formatter`` entry indicates the key name of the formatter for this
handler. If blank, a default formatter (``logging._defaultFormatter``) is used.
If a name is specified, it must appear in the ``[formatters]`` section and have
a corresponding section in the configuration file.

The ``args`` entry, when :func:`eval`\ uated in the context of the ``logging``
package's namespace, is the list of arguments to the constructor for the handler
class. Refer to the constructors for the relevant handlers, or to the examples
below, to see how typical entries are constructed. ::

   [handler_hand02]
   class=FileHandler
   level=DEBUG
   formatter=form02
   args=('python.log', 'w')

   [handler_hand03]
   class=handlers.SocketHandler
   level=INFO
   formatter=form03
   args=('localhost', handlers.DEFAULT_TCP_LOGGING_PORT)

   [handler_hand04]
   class=handlers.DatagramHandler
   level=WARN
   formatter=form04
   args=('localhost', handlers.DEFAULT_UDP_LOGGING_PORT)

   [handler_hand05]
   class=handlers.SysLogHandler
   level=ERROR
   formatter=form05
   args=(('localhost', handlers.SYSLOG_UDP_PORT), handlers.SysLogHandler.LOG_USER)

   [handler_hand06]
   class=handlers.NTEventLogHandler
   level=CRITICAL
   formatter=form06
   args=('Python Application', '', 'Application')

   [handler_hand07]
   class=handlers.SMTPHandler
   level=WARN
   formatter=form07
   args=('localhost', 'from@abc', ['user1@abc', 'user2@xyz'], 'Logger Subject')

   [handler_hand08]
   class=handlers.MemoryHandler
   level=NOTSET
   formatter=form08
   target=
   args=(10, ERROR)

   [handler_hand09]
   class=handlers.HTTPHandler
   level=NOTSET
   formatter=form09
   args=('localhost:9022', '/log', 'GET')

Sections which specify formatter configuration are typified by the following. ::

   [formatter_form01]
   format=F1 %(asctime)s %(levelname)s %(message)s
   datefmt=
   class=logging.Formatter

The ``format`` entry is the overall format string, and the ``datefmt`` entry is
the :func:`strftime`\ -compatible date/time format string. If empty, the package
substitutes ISO8601 format date/times, which is almost equivalent to specifying
the date format string "The ISO8601 format also specifies milliseconds, which
are appended to the result of using the above format string, with a comma
separator. An example time in ISO8601 format is ``2003-01-23 00:29:50,411``.

.. % Y-%m-%d %H:%M:%S".

The ``class`` entry is optional.  It indicates the name of the formatter's class
(as a dotted module and class name.)  This option is useful for instantiating a
:class:`Formatter` subclass.  Subclasses of :class:`Formatter` can present
exception tracebacks in an expanded or condensed format.

